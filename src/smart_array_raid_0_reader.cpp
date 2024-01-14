#include "smart_array_raid_0_reader.hpp"
#include <math.h>
#include <memory.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <memory>

SmartArrayRaid0Reader::SmartArrayRaid0Reader(const SmartArrayRaid0ReaderOptions &options)
{
    this->driveName = options.readerName;
    this->stripeSizeInBytes = options.stripeSize * 1024;

    std::vector<u_int64_t> drivesSizes;

    for (auto& drive : options.driveReaders)
    {
        if (!drive)
        {
            throw std::invalid_argument("For RAID 0 no missings drive are allowed.");
        }

        drivesSizes.push_back(drive->driveSize());
        this->drives.push_back(drive);
    }

    this->singleDriveSize = *std::min_element(drivesSizes.begin(), drivesSizes.end());

    if (!options.nometadata)
    {
        // 32MiB from the end of drive are stored controller metadata.
        this->singleDriveSize -= 1024 * 1024 * 32;
        this->setPhysicalDriveOffset(options.offset);
    }

    // I am skipping last stripe on drive if it's not whole
    // I don't know how Smart Array hanle that but you have drive size in metadata
    // So use `packard-tell` and copy command from there ^^
    u_int64_t wholeStripesOnDrive = (this->singleDriveSize - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    u_int64_t defaultSize = wholeStripesOnDrive * this->stripeSizeInBytes * drives.size();
    u_int64_t maximumSize = (this->singleDriveSize - this->getPhysicalDriveOffset()) * drives.size();

    this->setSize(defaultSize, 0);

    if (options.size > 0)
    {
        std::cout << "0 SET SIZE" << std::endl;
        this->setSize(options.size, maximumSize);
    }
}

int SmartArrayRaid0Reader::read(void* buf, u_int32_t len, u_int64_t offset)
{
    if (offset >= this->driveSize())
    {
        std::cerr << "Tried to read from offset exceeding array size. Skipping." << std::endl;
        return -1;
    }

    u_int64_t stripenum = this->stripeNumber(offset);
    u_int32_t stripeRelativeOffset = this->stripeRelativeOffset(stripenum, offset);

    while (len != 0)
    {
        u_int32_t read = this->readFromStripe(buf, stripenum, stripeRelativeOffset, len);
        len -= read;
        buf = static_cast<char*>(buf) + read;

        if (len > 0)
        {
            stripenum++;
            stripeRelativeOffset = 0;
        }
    }

    return 0;
}

u_int64_t SmartArrayRaid0Reader::stripeNumber(u_int64_t offset)
{
    return offset / this->stripeSizeInBytes;
}

u_int32_t SmartArrayRaid0Reader::stripeRelativeOffset(u_int64_t stripenumber, u_int64_t offset)
{
    return offset - stripenumber * this->stripeSizeInBytes;
}

u_int16_t SmartArrayRaid0Reader::stripeDriveNumber(u_int64_t stripenum)
{
    return stripenum % drives.size();
}

u_int64_t SmartArrayRaid0Reader::stripeDriveOffset(u_int64_t stripenum, u_int32_t stripeRelativeOffset)
{
    u_int64_t currentStripeRow = stripenum / drives.size();

    if (this->isLastRow(currentStripeRow))
    {
        u_int64_t x = (currentStripeRow - 1) * this->stripeSizeInBytes;
        return x + stripeRelativeOffset + this->getPhysicalDriveOffset();
    }

    return (currentStripeRow * this->stripeSizeInBytes) + stripeRelativeOffset + this->getPhysicalDriveOffset();
}

u_int32_t SmartArrayRaid0Reader::lastRowStripeSize()
{
    u_int64_t wholeStripesOnDrive = (this->singleDriveSize - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    return (this->singleDriveSize - this->getPhysicalDriveOffset()) - wholeStripesOnDrive * this->stripeSizeInBytes;
}

bool SmartArrayRaid0Reader::isLastRow(u_int64_t rownum)
{
    u_int64_t wholeStripesOnDrive = (this->singleDriveSize - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    return rownum == wholeStripesOnDrive;
}

u_int32_t SmartArrayRaid0Reader::readFromStripe(void *buf, u_int64_t stripenum, u_int32_t stripeRelativeOffset, u_int32_t len)
{
    auto drivenum = stripeDriveNumber(stripenum);
    auto driveOffset = stripeDriveOffset(stripenum, stripeRelativeOffset);
    
    if ((len + stripeRelativeOffset) > this->stripeSizeInBytes)
    {
        // We will to the end of the stripe if
        // stripe area is exceed. We will return len
        // from this method so called will know that
        // some data must be read from another stripe
        len = this->stripeSizeInBytes - stripeRelativeOffset;
    }

    auto& drivePtr = this->drives[drivenum];
    drivePtr->read(buf, len, driveOffset);

    return len;
}
