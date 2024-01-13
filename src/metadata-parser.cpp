#include <memory.h>
#include <bit>

#include "types.hpp"
#include "metadata-parser.hpp"

#pragma pack(1)

namespace sg
{
// 28 bytes per drive, starting from 0x4100
struct RawPhysicalDrive
{
    u8 serialNumber[20];                            // 0x00 - 0x13
    u8 dunno1;                                      // 0x14
    u8 dunno2MaybeBoxNumber;                        // 0x15
    u8 bayNumber;                                   // 0x16
    u8 dunno3;                                      // 0x17
    u32 useless;                                    // 0x18 - 0x1b
};

// Each contains 2080 bytes
struct RawLogicalDrive
{
    u8 label[64];                                   // 0x00 - 0x39
    u8 uselessShit[64];                             // 0x40 - 0x79
    u32 dunno;                                      // 0x80 - 0x83
    u32 dunno2;                                     // 0x84 - 0x87
    u8 dunno3;                                      // 0x88
    u8 physicalDriveCount;                          // 0x89

    // RAID 0 = physicalDriveCount
    // RAID 1 = 1
    // RAID 5 = physicalDriveCount - 1
    // RAID 6 = physicalDriveCount - 2
    // With RAID 10 it's weird, idk really, for 4 drives it's 2.
    // With RAID 50 and RAID 60 it's drives needed to run for each parity group
    u8 physicalDriveNeededForLogicalDriveToRun;     // 0x8a

    // 0x00 -> RAID 0
    // 0x02 -> RAID 1 or RAID 10
    // 0x03 -> RAID 5 or RAID 50
    // 0x05 -> RAID 6 or RAID 60
    // To figure out whether it's raid X or X0 you need to two previous values
    // Read comment above ^^
    // Btw using 2 previous value you can figure out parity groups:
    // Let's say physicalDrivesCount is n, and physicalDriveNeededForLogicalDriveToRun is m
    // Here's how to calculate parity groups for RAID 50 and 60:
    // RAID 50: pg = n / (m + 1)
    // RAID 60: pg = n / (m + 2)
    u8 raidLevel;                                   // 0x8b

    u32 dunno4;                                     // 0x8c - 0x8f
    // I don't know how many drives can be in a single array
    // Anyways it's terminated with 00, so if you encounter 00
    // then that's the end of drives array and you should stop
    // reading any further to avoid any trash data.
    u8 physicalDrivesNumbers[160];                  // 0x90 - 0x12f
    u8 uselessShit2[292];                           // 0x130 - 0x253
    u16 stripeSizeIn512byteSectorsBigEndian;        // 0x254 - 0x255
    u8 dunno5[10];                                  // 0x256 - 0x25f
    u64 logicalDriveSizeIn512byteSectorsBigEndian;  // 0x260 - 0x267
    u64 dunno6;                                     // 0x268 - 0x26f
    u64 offsetOnEachPhysicalDriveIn512byteSectorsBigEndian; // 0x270 - 0x277
    u64 spaceTakenOnEachPhysicalDrivesIn512byteSectorsBigEndian; // 0x278 - 0x27f
    u8 uselessShit3[2080 - 0x280];
};

// Metadata starts from 31MiB from the end of the drive
// On 0x28000 it's repeated for some reason, so for the sake of correctness
// I made this to have size of 0x28000 which is precisely 160KB
// Also important note: all numbers are BIG ENDIAN
struct RawMetadata
{
    u8 physicalDriveNumber; // 0x00
    u8 useless[0x105f];
    u8 controllerSerialNumber[20]; // 0x1060 - 0x1073
    u8 useless2[0x40ff - 0x1073];
    
    // Next data starts at 0x5e78, over 255*28 bytes 0x4100
    // So I'll assume there can be as much as 255 drives
    // Although my controller supports up to 60 drives
    // Anyways after first physical serial number stats with 0x00
    // you should tread is as the end of list I guess
    // Each has 28 bytes
    RawPhysicalDrive physicalDrives[255]; // 0x4100 - 0x5ce3
    u8 useless3[0x602b - 0x5ce3];
    u8 logicalDrivesCount; // 0x602c
    u8 useless4[0x7007 - 0x602c];
    // From what I've read in controller docs it supports up to 64 logical drives
    // Read as many as `logicalDrivesCount` or until first label starts with 0x00
    // Each has 2080 bytes
    RawLogicalDrive logicalDrives[64]; // 0x7008 - 0x27807
    u8 padding[0x27fff - 0x27807];
};

#pragma pack(0)

template <typename T>
T decodeNumber(T num)
{
    if constexpr (std::endian::native == std::endian::big)
    {
        return num;
    }

    return std::byteswap(num);
}

void parseMetadata(const u8 *metadataBin, P420Metadata *output)
{
    RawMetadata rawMetadata;
    memcpy(&rawMetadata, metadataBin, 0x28000);

    output->driveNumber = rawMetadata.physicalDriveNumber;
    output->controllerSerialNumber = std::string(
        (char*)rawMetadata.controllerSerialNumber,
        20
    );

    for (int i = 0; i < 255; i++)
    {
        RawPhysicalDrive* d = &rawMetadata.physicalDrives[i];
        if (d->serialNumber[0] == 0)
        {
            break;
        }
        output->physicalDrives.push_back(PhysicalDrive {
            .serialNumber = std::string((char*)d->serialNumber, sizeof(d->serialNumber)),
            .boxNumber = d->dunno2MaybeBoxNumber,
            .bayNumber = d->bayNumber 
        });
    }
    
    for (int i = 0; i < rawMetadata.logicalDrivesCount && i < 64; i++)
    {
        RawLogicalDrive* ld = &rawMetadata.logicalDrives[i];

        if (ld->label[0] == 0)
        {
            break;
        }

        LogicalDrive parsedLd = LogicalDrive {
            .label = std::string((char*)ld->label, sizeof(ld->label)),
            .physicalDriveCount = ld->physicalDriveCount,
            .physicalDrivesNeededToRun = ld->physicalDriveNeededForLogicalDriveToRun,
            .stripeSizeInBytes = decodeNumber(ld->stripeSizeIn512byteSectorsBigEndian) * 512u,
            .logicalDriveSizeInBytes = decodeNumber(ld->logicalDriveSizeIn512byteSectorsBigEndian) * 512u,
            .offsetOnEachPhysicalDriveInBytes = decodeNumber(ld->offsetOnEachPhysicalDriveIn512byteSectorsBigEndian) * 512u,
            .spaceTakenOnEachPhysicalDriveInBytes = decodeNumber(ld->spaceTakenOnEachPhysicalDrivesIn512byteSectorsBigEndian) * 512u,
        };

        // Physical Drives
        for (int j = 0; j < ld->physicalDriveCount && j < 160; j++)
        {
            parsedLd.physicalDrives.push_back(ld->physicalDrivesNumbers[j]);
        }

        // RAID Level and Parity Groups
        u8 n = ld->physicalDriveCount;
        u8 m = ld->physicalDriveNeededForLogicalDriveToRun;
        switch (ld->raidLevel)
        {
            case 0:
                parsedLd.raidLevel = 0;
                break;
            case 2:
                parsedLd.raidLevel = m == 1 ? 1 : 10;
                break;
            case 3:
                if (n - m == 1)
                {
                    parsedLd.raidLevel = 5;
                }
                else
                {
                    parsedLd.raidLevel = 50;
                    parsedLd.parityGroups = n / (m + 1);
                }
                break;
            case 5:
                if (n - m == 2)
                {
                    parsedLd.raidLevel = 6;
                }
                else
                {
                    parsedLd.raidLevel = 60;
                    parsedLd.parityGroups = n / (m + 2);
                }
                break;
            default:
                parsedLd.raidLevel = 255;
        }

        output->logicalDrives.push_back(std::move(parsedLd));
    }
}

} // end namespace sg
