#include "smart_array_raid_10_reader.hpp"
#include <vector>

SmartArrayRaid10Reader::SmartArrayRaid10Reader(const SmartArrayRaid10ReaderOptions &options)
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

    std::vector<std::shared_ptr<DriveReader>> mirrorReaders;

    /*
        Little explanation to RAID 10 on HP Smart Array Controller
        It's not
        A1 A1 A2 A2 A3 A3
        But
        A1 A2 A3 A1 A2 A3
        So in case of 3 drives we must pair them like that: 1 -> 4, 2 -> 5, 3 -> 6 
    */
    for (int i = 0; i < options.driveReaders.size() / 2; i++)
    {
        auto drive1 = options.driveReaders[i];
        auto drive2 = options.driveReaders[i + options.driveReaders.size() / 2];

        if (!drive1 && !drive2)
        {
            throw std::invalid_argument("For RAID 10 only 1 missing drive per mirror is allowed.");
        }

        std::string drive1Name = drive1 ? drive1->name() : "X";
        std::string drive2Name = drive2 ? drive2->name() : "X";

        mirrorReaders.push_back(std::make_shared<SmartArrayRaid1Reader>(SmartArrayRaid1ReaderOptions {
            .driveReaders = { drive1, drive2 },
            .readerName = "RAID 10 Mirror: " + drive1Name + " " + drive2Name,
            .size = options.size / (options.driveReaders.size() / 2),
            .offset = options.offset
        }));
    }

    SmartArrayRaid0ReaderOptions reader0Options {
        .stripeSize = options.stripeSize,
        .driveReaders = mirrorReaders,
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
