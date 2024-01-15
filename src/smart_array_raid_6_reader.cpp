#include "smart_array_raid_6_reader.hpp"
#include <math.h>
#include <memory.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <memory>

namespace sg
{

SmartArrayRaid6Reader::SmartArrayRaid6Reader(const SmartArrayRaid6ReaderOptions &options)
{
    this->driveName = options.readerName;
    this->parityDelay = options.parityDelay;
    this->stripeSizeInBytes = options.stripeSize * 1024;

    if (options.driveReaders.size() < 4)
    {
        throw std::invalid_argument("For RAID 6 at least 4 drives must be provided (two can be missing but still they have to be in the driveReaders list represented with nullptr)");
    }

    int missingDrives = 0;
    std::vector<u64> drivesSizes;


    for (auto& drive : options.driveReaders)
    {
        if (!drive)
        {
            if (missingDrives > 0)
            {
                throw std::invalid_argument("Sowwi but for now only 1 missing drive is supported with RAID 6. Reed Solomon's kinda hard :(");
            }
            if (missingDrives > 1)
            {
                throw std::invalid_argument("For RAID 6 only 2 missing drives are allowed.");
            }
            missingDrives++;
            this->drives.push_back(drive);
            continue;
        }

        drivesSizes.push_back(drive->driveSize());
        this->drives.push_back(drive);
    }

    this->singleDriveSize = *std::min_element(drivesSizes.begin(), drivesSizes.end());

    // 32MiB from the end of drive are stored controller metadata.
    this->singleDriveSize -= 1024 * 1024 * 32;
    this->setPhysicalDriveOffset(options.offset);

    u64 wholeStripesOnDrive = (this->singleDriveSize - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    u64 defaultSize =  wholeStripesOnDrive * this->stripeSizeInBytes * (drives.size() - 2);
    u64 maximumSize = (this->singleDriveSize - this->getPhysicalDriveOffset()) * (drives.size() - 2);

    this->setSize(defaultSize, 0);

    if (options.size > 0)
    {
        this->setSize(options.size, maximumSize);
    }
}

int SmartArrayRaid6Reader::read(void *buf, u32 len, u64 offset)
{
    if (offset >= this->driveSize())
    {
        std::cerr << this->name() << ": Tried to read from offset exceeding array size. Skipping." << std::endl;
        std::cerr << "Offset: " << offset << std::endl;
        std::cerr << "Drive Size: " << this->driveSize() << std::endl;
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

u64 SmartArrayRaid6Reader::stripeNumber(u64 offset)
{
    u64 stripenum = offset / this->stripeSizeInBytes;
    if (this->isLastRow(stripenum / (drives.size() - 2)))
    {
        u64 o = offset - (stripenum * this->stripeSizeInBytes);
        u64 lastStripeNum = o / this->lastRowStripeSize();
        stripenum += lastStripeNum;
    }

    return stripenum;
}

u32 SmartArrayRaid6Reader::stripeRelativeOffset(u64 stripenum, u64 offset)
{
    u32 o = offset - stripenum * this->stripeSizeInBytes;;
    if (this->isLastRow(stripenum / (drives.size() - 2)))
    {
        o %= this->lastRowStripeSize();
    }
    return o;
}

u16 SmartArrayRaid6Reader::stripeDriveNumber(u64 stripenum)
{
    u64 currentStripeRow = stripenum / (drives.size() - 2);
    u32 parityCycle = (this->drives.size() * this->parityDelay);
    u32 reedSolomonCycleRow = currentStripeRow % parityCycle;
    u32 parityCycleRow = (currentStripeRow + this->parityDelay) % parityCycle;
    u16 reedSolomonDrive = drives.size() - (reedSolomonCycleRow / this->parityDelay) - 1;
    u16 parityDrive = drives.size() - (parityCycleRow / this->parityDelay) - 1;
    u16 stripeDrive = stripenum % (drives.size() - 2);
    
    // ORDER OF THOSE IFS MATTERS!
    if ((stripeDrive - parityDrive) >= 0)
    {
        stripeDrive++;
    }
    if ((stripeDrive - reedSolomonDrive) >= 0)
    {
        stripeDrive++;
    }

    return stripeDrive;
}

u64 SmartArrayRaid6Reader::stripeDriveOffset(u64 stripenum, u32 stripeRelativeOffset)
{
    u64 currentStripeRow = stripenum / (drives.size() - 2);
    return (currentStripeRow * this->stripeSizeInBytes) + stripeRelativeOffset + this->getPhysicalDriveOffset();
}

u32 SmartArrayRaid6Reader::lastRowStripeSize()
{
    u32 fullStripeSize = this->stripeSizeInBytes * (this->drives.size() - 2);
    u64 wholeStripesOnDrive = this->driveSize() / fullStripeSize;
    u32 lastFullStripeSize = this->driveSize() - wholeStripesOnDrive * fullStripeSize;
    return lastFullStripeSize / (this->drives.size() - 2);
}

bool SmartArrayRaid6Reader::isLastRow(u64 rownum)
{
    u32 fullStripeSize = this->stripeSizeInBytes * (this->drives.size() - 2);
    u64 wholeStripesOnDrive = this->driveSize() / fullStripeSize;
    return rownum == wholeStripesOnDrive;
}

u32 SmartArrayRaid6Reader::readFromStripe(void *buf, u64 stripenum, u32 stripeRelativeOffset, u32 len)
{
    auto drivenum = stripeDriveNumber(stripenum);
    auto driveOffset = stripeDriveOffset(stripenum, stripeRelativeOffset);
    auto stripeSize = this->stripeSizeInBytes;

    if (this->isLastRow(stripenum / (drives.size() - 2)))
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

    if (!drivePtr) 
    {
        return this->recoverForDrive(buf, drivenum, driveOffset, len);
    }

    drivePtr->read(buf, len, driveOffset);

    return len;
}

bool SmartArrayRaid6Reader::isReedSolomonDrive(u16 drivenum, u64 driveOffset)
{
    u64 currentStripeRow = (driveOffset - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    u32 parityCycle = (this->drives.size() * this->parityDelay);
    u32 reedSolomonCycleRow = currentStripeRow % parityCycle;
    u16 reedSolomonDrive = drives.size() - (reedSolomonCycleRow / this->parityDelay) - 1;

    return drivenum == reedSolomonDrive;
}

bool SmartArrayRaid6Reader::isParityDrive(u16 drivenum, u64 driveOffset)
{
    u64 currentStripeRow = (driveOffset - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    u32 parityCycle = (this->drives.size() * this->parityDelay);
    u32 parityCycleRow = (currentStripeRow + this->parityDelay) % parityCycle;
    u16 parityDrive = drives.size() - (parityCycleRow / this->parityDelay) - 1;

    return drivenum == parityDrive;
}

u32 SmartArrayRaid6Reader::recoverForDrive(void *buf, u16 drivenum, u64 driveOffset, u32 len)
{
    std::vector<std::shared_ptr<DriveReader>> otherDrives;
    for (int i = 0; i < this->drives.size(); i++)
    {
        if (i != drivenum && !this->isReedSolomonDrive(i, driveOffset))
        {
            auto drive = this->drives[i];
            if (!drive)
            {
                // We have another failed data drive, we need to use recovery for 2 missing drives
                return this->recoverForTwoDrives(buf, drivenum, i, driveOffset, len);
            }
            otherDrives.push_back(this->drives[i]);
        }
    }

    std::unique_ptr<char[]> _uniq_out(new char[len]);
    std::unique_ptr<char[]> _uniq_temp(new char[len]);

    // I am getting raw pointers coz for array operations
    // compiler will use SSE for them with -O3
    // with unique pointer that is not the case
    // https://godbolt.org/z/aTYrhqPGb
    // I am using unique here only to delete it automatically ^^
    char* out = _uniq_out.get();
    char* temp = _uniq_temp.get();

    memset(out, 0, len);
    
    // Parallel reading is not necessary here
    // On 12 drives RAID 5 with 1 missing drive I have 341 MB/s of sequential read.
    for (int i = 0; i < otherDrives.size(); i++)
    {
        auto drive = otherDrives[i];
        drive->read(temp, len, driveOffset);

        for (int i = 0; i < len; i++)
        {
            // Compiler with flag -O3 should optimize it using sse ^^
            out[i] ^= temp[i];
        }
    }

    memcpy(buf, out, len);
    return len;
}

u32 SmartArrayRaid6Reader::recoverForTwoDrives(void *buf, u16 drive1num, u16 drive2num, u64 driveOffset, u32 len)
{
    throw std::runtime_error("Not implemented yet :c");
    return u32();
}

} // end namespace sg
