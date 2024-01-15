#include "smart_array_raid_1_reader.hpp"
#include <memory.h>
#include <algorithm>
#include <iostream>
#include <memory>

namespace sg
{

SmartArrayRaid1Reader::SmartArrayRaid1Reader(const SmartArrayRaid1ReaderOptions& options)
{
    this->driveName = options.readerName;
    std::vector<u64> drivesSizes;
    int missingDrives = 0;

    for (auto& drive : options.driveReaders)
    {
        if (!drive)
        {
            if (missingDrives >= options.driveReaders.size())
            {
                throw std::invalid_argument("For RAID 1 only at least 1 drive must exists :)");
            }
            missingDrives++;
            continue;
        }

        drivesSizes.push_back(drive->driveSize());
        this->drives.push_back(drive);
    }

    this->singleDriveSize = *std::min_element(drivesSizes.begin(), drivesSizes.end());

    // 32MiB from the end of drive are stored controller's metadata.
    this->singleDriveSize -= 1024 * 1024 * 32;
    this->setPhysicalDriveOffset(options.offset);

    u64 maximumSize = this->singleDriveSize - this->getPhysicalDriveOffset();
    this->setSize(maximumSize, 0);

    if (options.size > 0)
    {
        this->setSize(options.size, this->singleDriveSize);
    }
}

int SmartArrayRaid1Reader::read(void *buf, u32 len, u64 offset)
{
    if (offset >= this->driveSize())
    {
        std::cerr << this->name() << ": Tried to read from offset exceeding array size. Skipping." << std::endl;
        std::cerr << "Offset: " << offset << std::endl;
        std::cerr << "Drive Size: " << this->driveSize() << std::endl;
        return -1;
    }

    for (int i = 0; i < this->drives.size(); i++)
    {
        try
        {
            return this->drives[i]->read(buf, len, offset + this->getPhysicalDriveOffset());
        }
        catch (std::exception& ex)
        {
            if (( i + 1 ) >= this->drives.size())
            {
                throw;
            }
            std::cerr << "Read from drive "
                << this->drives[i]->name() << " has failed."
                << " Reason: " << ex.what() << std::endl;
            std::cout << "Reader will try to read from another drive in the RAID 1 array.";
        }
    }

    return -1;
}

} // end namespace sg