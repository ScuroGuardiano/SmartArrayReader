#include <sys/types.h>

#pragma pack(1)

// 28 bytes per drive, starting from 0x4100
// Next data starts at 0x5e78, over 255*28 bytes 0x4100
// So I'll assume there can be as much as 255 drives
// Although my controller supports up to 60 drives
struct PhysicalDrive
{
    u_int8_t serialNumber[20];
    u_int8_t dunno1;
    u_int8_t dunno2MaybeBoxNumber;
    u_int8_t bayNumber;
    u_int8_t dunno3;
};

// Each contains 2080 bytes
struct LogicalDrive
{
    u_int8_t label[64];
    u_int8_t uselessShit[64];
    u_int32_t dunno;
    u_int32_t dunno2;
    u_int8_t dunno3;
    u_int8_t physicalDriveCount;

    // RAID 0 = physicalDriveCount
    // RAID 1 = 1
    // RAID 5 = physicalDriveCount - 1
    // RAID 6 = physicalDriveCount - 2
    // With RAID 10 it's weird, idk really, for 4 drives it's 2.
    // With RAID 50 and RAID 60 it's drives needed to run for each parity group
    u_int8_t physicalDriveNeededForLogicalDriveToRun;

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
    u_int8_t raidLevel;

    u_int32_t dunno4;
    // I don't know how many drives can be in a single array
    // Anyways it's terminated with 00, so if you encounter 00
    // then that's the end of drives array and you should stop
    // reading any further to avoid any trash data.
    u_int8_t physicalDrivesNumbers[160];
    u_int8_t uselessShit2[292];
    u_int16_t stripeSizeIn512byteSectorsBigEndian;
    u_int8_t dunno5[10];
    u_int64_t logicalDriveSizeIn512byteSectorsBigEndian;
    u_int64_t dunno6;
    u_int64_t offsetOnEachPhysicalDriveIn512byteSectorsBigEndian;
    u_int64_t spaceTakenOnEachPhysicalDrivesIn512byteSectorsBigEndian;
    u_int8_t uselessShit3[2080 - 640];
};

#pragma pack(0)