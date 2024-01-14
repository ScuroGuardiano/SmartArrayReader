#include "smart_array_raid_10_reader.hpp"
#include <vector>

SmartArrayRaid10Reader::SmartArrayRaid10Reader(SmartArrayRaid10ReaderOptions &options)
{
    // From what I've observed my raid controller only supports 2 drives for RAID 1
    // So I will assume each mirror group contains 2 drives.

    if (options.driveReaders.size() < 4)
    {
        throw std::invalid_argument("For RAID 10 at least 4 drives must be provided (one for each mirror group can be missing but still they have to be in the driveReaders list represented with nullptr)");
    }

    if (options.driveReaders.size() % 2 != 0)
    {
        throw std::invalid_argument("For RAID 10 you must provide even amount of drives.");
    }

    std::vector<std::shared_ptr<DriveReader>> mirrorGroupsReaders;
    SmartArrayRaid1ReaderOptions mirrorOptions;
    int missingDrives = 0;

    for (auto drive : options.driveReaders)
    {
        mirrorOptions.readerName += drive->name() + " ";

        if (!drive)
        {
            missingDrives++;
            if (missingDrives > 1)
            {
                throw std::invalid_argument("For RAID 10 only 1 missing drive per mirror group is allowed.");
            }
        }

        mirrorOptions.driveReaders.push_back(drive);

        if (mirrorOptions.driveReaders.size() == 2)
        {
            // Remove leading space
            mirrorOptions.readerName.erase(mirrorOptions.readerName.end() - 1);
            mirrorOptions.size = options.size / (options.driveReaders.size() / 2);
            mirrorOptions.offset = options.offset;

            mirrorGroupsReaders.push_back(
                std::make_shared<SmartArrayRaid1Reader>(mirrorOptions)
            );
            mirrorOptions = SmartArrayRaid1ReaderOptions();
            missingDrives = 0;
        }
    }

    SmartArrayRaid0ReaderOptions reader0Options {
        .stripeSize = options.stripeSize,
        .driveReaders = mirrorGroupsReaders,
        .readerName = "RAID 10 - Raid 0 reader",
        .size = options.size,
        // Offset is zero here, because in raid 10 raid 0 is not
        // "touching" physical drives direcly but rather thru
        // raid 1 readers
        .offset = 0,
        .nometadata = true
    };

    this->raid0Reader = std::make_unique<SmartArrayRaid0Reader>(reader0Options);
}

int SmartArrayRaid10Reader::read(void *buf, u_int32_t len, u_int64_t offset)
{
    return this->raid0Reader->read(buf, len, offset);
}

u_int64_t SmartArrayRaid10Reader::driveSize()
{
    return this->raid0Reader->driveSize();
}
