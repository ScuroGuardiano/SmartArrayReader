#include "smart_array_raid_50_reader.hpp"
#include <iostream>

SmartArrayRaid50Reader::SmartArrayRaid50Reader(const SmartArrayRaid50ReaderOptions &options)
{
    if (options.driveReaders.size() < 6)
    {
        throw std::invalid_argument("For RAID 50 at least 6 drives must be provided (one for each parity group can be missing but still they have to be in the driveReaders list represented with nullptr)");
    }

    u_int16_t drivesPerParityGroup = options.driveReaders.size() / options.parityGroups;
    
    if (drivesPerParityGroup < 3)
    {
        throw std::invalid_argument("For RAID 50 there must be at least 3 drives per parity group.");
    }

    if (drivesPerParityGroup * options.parityGroups != options.driveReaders.size())
    {
        throw std::invalid_argument("For RAID 50 number of provided drives must be divisible by parity groups.");
    }

    std::vector<std::shared_ptr<DriveReader>> parityGroupsReaders;
    SmartArrayRaid5ReaderOptions parityOptions;
    int missingDrives = 0;

    for (auto drive : options.driveReaders)
    {
        parityOptions.readerName += (drive ? drive->name() : "X") + " ";

        if (!drive)
        {
            missingDrives++;
            if (missingDrives > 1)
            {
                throw std::invalid_argument("For RAID 50 only 1 missing drive per parity group is allowed.");
            }
        }

        parityOptions.driveReaders.push_back(drive);

        if (parityOptions.driveReaders.size() == drivesPerParityGroup)
        {
            // Remove leading space
            parityOptions.readerName.erase(parityOptions.readerName.end() - 1);
            parityOptions.stripeSize = options.stripeSize;
            parityOptions.parityDelay = options.parityDelay;
            parityOptions.size = options.size / options.parityGroups;
            parityOptions.offset = options.offset;

            parityGroupsReaders.push_back(
                std::make_shared<SmartArrayRaid5Reader>(parityOptions)
            );
            parityOptions = SmartArrayRaid5ReaderOptions();
            missingDrives = 0;
        }
    }

    SmartArrayRaid0ReaderOptions reader0Options {
        .stripeSize = options.stripeSize * (drivesPerParityGroup - 1),
        .driveReaders = parityGroupsReaders,
        .readerName = "RAID 50 - Raid 0 reader",
        .size = options.size,
        // Offset is zero here, because in raid 50 raid 0 is not
        // "touching" physical drives direcly but rather thru
        // raid 5 readers
        .offset = 0,
        .nometadata = true
    };

    this->raid0Reader = std::make_unique<SmartArrayRaid0Reader>(reader0Options);
}

int SmartArrayRaid50Reader::read(void *buf, u_int32_t len, u_int64_t offset)
{
    return this->raid0Reader->read(buf, len, offset);
}

u_int64_t SmartArrayRaid50Reader::driveSize()
{
    return this->raid0Reader->driveSize();
}
