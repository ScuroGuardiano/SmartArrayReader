#include "types.hpp"
#include <string>
#include <vector>

namespace sg
{

struct PhysicalDrive
{
    std::string serialNumber;
    u8 boxNumber;
    u8 bayNumber;
};

struct LogicalDrive
{
    std::string label;
    u8 physicalDriveCount;

    // RAID 0 = physicalDriveCount
    // RAID 1 = 1
    // RAID 5 = physicalDriveCount - 1
    // RAID 6 = physicalDriveCount - 2
    // With RAID 10 it's weird, idk really, for 4 drives it's 2.
    // With RAID 50 and RAID 60 it's drives needed to run for each parity group
    u8 physicalDrivesNeededToRun;
    // Decoded, so: RAID 0 -> 0, RAID 1 -> 1, RAID 10 -> 10 etc.
    u8 raidLevel;
    // Only for RAID 50 and 60, otherwise it will be 0
    u8 parityGroups;
    std::vector<u8> physicalDrives;
    u32 stripeSizeInBytes;
    u64 logicalDriveSizeInBytes;
    u64 offsetOnEachPhysicalDriveInBytes;
    u64 spaceTakenOnEachPhysicalDriveInBytes;
};

struct P420Metadata
{
    u8 driveNumber;
    std::string controllerSerialNumber;
    std::vector<PhysicalDrive> physicalDrives;
    std::vector<LogicalDrive> logicalDrives;
};


/// @brief Parses metadata from drives. Metadata starts 31MiB from the end
/// of physical drive and is 163840 bytes long.
/// @param metadata must be at least 163840 bytes long
/// @param output struct where parsed data will be placed
void parseMetadata(const u8* metadataBin, P420Metadata* output);

} // end namespace sg