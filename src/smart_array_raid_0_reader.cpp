#include "smart_array_raid_0_reader.hpp"
#include <math.h>
#include <memory.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <memory>

namespace sg
{

SmartArrayRaid0Reader::SmartArrayRaid0Reader(const SmartArrayRaid0ReaderOptions &options)
{
    this->driveName = options.readerName;
    this->stripeSizeInBytes = options.stripeSize * 1024;

    std::vector<u64> drivesSizes;

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
    u64 wholeStripesOnDrive = (this->singleDriveSize - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    u64 defaultSize = wholeStripesOnDrive * this->stripeSizeInBytes * drives.size();
    u64 maximumSize = (this->singleDriveSize - this->getPhysicalDriveOffset()) * drives.size();

    this->setSize(defaultSize, 0);

    if (options.size > 0)
    {
        this->setSize(options.size, maximumSize);
    }
}

int SmartArrayRaid0Reader::read(void* buf, u32 len, u64 offset)
{
    if (offset >= this->driveSize())
    {
        std::cerr << "Tried to read from offset exceeding array size. Skipping." << std::endl;
        return -1;
    }

    u64 stripenum = this->stripeNumber(offset);
    u32 stripeRelativeOffset = this->stripeRelativeOffset(stripenum, offset);  

    while (len != 0)
    {
        u32 read = this->readFromStripe(buf, stripenum, stripeRelativeOffset, len);
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

u64 SmartArrayRaid0Reader::stripeNumber(u64 offset)
{
    u64 stripenum = offset / this->stripeSizeInBytes;
    if (this->isLastRow(stripenum / drives.size()))
    {
        u64 o = offset - (stripenum * this->stripeSizeInBytes);
        u64 lastStripeNum = o / this->lastRowStripeSize();
        stripenum += lastStripeNum;
    }

    return stripenum;
}

u32 SmartArrayRaid0Reader::stripeRelativeOffset(u64 stripenum, u64 offset)
{
    u32 o = offset - stripenum * this->stripeSizeInBytes;
    if (this->isLastRow(stripenum / drives.size()))
    {
        o %= this->lastRowStripeSize();
    }
    return o;
}

u16 SmartArrayRaid0Reader::stripeDriveNumber(u64 stripenum)
{
    return stripenum % drives.size();
}

u64 SmartArrayRaid0Reader::stripeDriveOffset(u64 stripenum, u32 stripeRelativeOffset)
{
    u64 currentStripeRow = stripenum / drives.size();
    return (currentStripeRow * this->stripeSizeInBytes) + stripeRelativeOffset + this->getPhysicalDriveOffset();
}

u32 SmartArrayRaid0Reader::lastRowStripeSize()
{
    u32 fullStripeSize = this->stripeSizeInBytes * this->drives.size();
    u64 wholeStripesOnDrive = this->driveSize() / fullStripeSize;
    u32 lastFullStripeSize = this->driveSize() - wholeStripesOnDrive * fullStripeSize;
    return lastFullStripeSize / this->drives.size();
}

bool SmartArrayRaid0Reader::isLastRow(u64 rownum)
{
    u32 fullStripeSize = this->stripeSizeInBytes * this->drives.size();
    u64 wholeStripesOnDrive = this->driveSize() / fullStripeSize;
    return rownum == wholeStripesOnDrive;
}

u32 SmartArrayRaid0Reader::readFromStripe(void *buf, u64 stripenum, u32 stripeRelativeOffset, u32 len)
{
    auto drivenum = stripeDriveNumber(stripenum);
    auto driveOffset = stripeDriveOffset(stripenum, stripeRelativeOffset);
    auto stripeSize = this->stripeSizeInBytes;

    if (this->isLastRow(stripenum / drives.size()))
    {
        stripeSize = this->lastRowStripeSize();
    }

    if ((len + stripeRelativeOffset) > stripeSize)
    {
        // We will to the end of the stripe if
        // stripe area is exceed. We will return len
        // from this method so called will know that
        // some data must be read from another stripe
        len = stripeSize - stripeRelativeOffset;
    }

    auto& drivePtr = this->drives[drivenum];
    drivePtr->read(buf, len, driveOffset);

    return len;
}

} // end namespace sg
