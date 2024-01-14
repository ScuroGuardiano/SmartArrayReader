#include "smart_array_raid_60_reader.hpp"

SmartArrayRaid60Reader::SmartArrayRaid60Reader(const SmartArrayRaid60ReaderOptions &options)
{
    if (options.driveReaders.size() < 6)
    {
        throw std::invalid_argument("For RAID 60 at least 8 drives must be provided (two for each parity group can be missing but still they have to be in the driveReaders list represented with nullptr)");
    }

    u_int16_t drivesPerParityGroup = options.driveReaders.size() / options.parityGroups;
    
    if (drivesPerParityGroup < 4)
    {
        throw std::invalid_argument("For RAID 60 there must be at least 4 drives per parity group.");
    }

    if (drivesPerParityGroup * options.parityGroups != options.driveReaders.size())
    {
        throw std::invalid_argument("For RAID 60 number of provided drives must be divisible by parity groups.");
    }

    std::vector<std::shared_ptr<DriveReader>> parityGroupsReaders;
    SmartArrayRaid6ReaderOptions parityOptions;
    int missingDrives = 0;

    for (auto drive : options.driveReaders)
    {
        parityOptions.readerName += drive->name() + " ";

        if (!drive)
        {
            missingDrives++;
            if (missingDrives > 2)
            {
                throw std::invalid_argument("For RAID 60 only 2 missing drives per parity group are allowed.");
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
                std::make_shared<SmartArrayRaid6Reader>(parityOptions)
            );
            parityOptions = SmartArrayRaid6ReaderOptions();
            missingDrives = 0;
        }
    }

    SmartArrayRaid0ReaderOptions reader0Options {
        .stripeSize = options.stripeSize * (drivesPerParityGroup - 2),
        .driveReaders = parityGroupsReaders,
        .readerName = "RAID 60 - Raid 0 reader",
        .size = options.size,
        // Offset is zero here, because in raid 60 raid 0 is not
        // "touching" physical drives direcly but rather thru
        // raid 6 readers
        .offset = 0,
        .nometadata = true
    };

    this->raid0Reader = std::make_unique<SmartArrayRaid0Reader>(reader0Options);
}

int SmartArrayRaid60Reader::read(void *buf, u_int32_t len, u_int64_t offset)
{
    return this->raid0Reader->read(buf, len, offset);
}

u_int64_t SmartArrayRaid60Reader::driveSize()
{
    return this->raid0Reader->driveSize();
}
