#include "smart_array_raid_6_reader.hpp"
#include <math.h>
#include <memory.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <memory>

SmartArrayRaid6Reader::SmartArrayRaid6Reader(SmartArrayRaid6ReaderOptions &options)
{
    this->driveName = options.readerName;
    this->parityDelay = options.parityDelay;
    this->stripeSizeInBytes = options.stripeSize * 1024;

    if (options.driveReaders.size() < 4)
    {
        throw std::invalid_argument("For RAID 6 at least 4 drives must be provided (two can be missing but still they have to be in the driveReaders list represented with nullptr)");
    }

    int missingDrives = 0;
    std::vector<u_int64_t> drivesSizes;


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

    u_int64_t wholeStripesOnDrive = (this->singleDriveSize - this->getPhysicalDriveOffset()) / this->stripeSizeInBytes;
    u_int64_t defaultSize =  wholeStripesOnDrive * this->stripeSizeInBytes * (drives.size() - 2);
    u_int64_t maximumSize = (this->singleDriveSize - this->getPhysicalDriveOffset()) * (drives.size() - 2);

    this->setSize(defaultSize, 0);

    if (options.size > 0)
    {
        this->setSize(options.size, maximumSize);
    }
}

int SmartArrayRaid6Reader::read(void *buf, u_int32_t len, u_int64_t offset)
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

u_int64_t SmartArrayRaid6Reader::stripeNumber(u_int64_t offset)
{
    return offset / this->stripeSizeInBytes;
}

u_int32_t SmartArrayRaid6Reader::stripeRelativeOffset(u_int64_t stripenum, u_int64_t offset)
{
    return offset - stripenum * this->stripeSizeInBytes;
}

u_int16_t SmartArrayRaid6Reader::stripeDriveNumber(u_int64_t stripenum)
{
    u_int64_t currentStripeRow = stripenum / (drives.size() - 2);
    u_int32_t parityCycle = (this->drives.size() * this->parityDelay);
    u_int32_t reedSolomonCycleRow = currentStripeRow % parityCycle;
    u_int32_t parityCycleRow = (currentStripeRow + this->parityDelay) % parityCycle;
    u_int16_t reedSolomonDrive = drives.size() - (reedSolomonCycleRow / this->parityDelay) - 1;
    u_int16_t parityDrive = drives.size() - (parityCycleRow / this->parityDelay) - 1;
    u_int16_t stripeDrive = stripenum % (drives.size() - 2);
    
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

u_int64_t SmartArrayRaid6Reader::stripeDriveOffset(u_int64_t stripenum, u_int32_t stripeRelativeOffset)
{
    u_int64_t currentStripeRow = stripenum / (drives.size() - 2);

    return (currentStripeRow * this->stripeSizeInBytes) + stripeRelativeOffset + this->getPhysicalDriveOffset();
}

u_int32_t SmartArrayRaid6Reader::readFromStripe(void *buf, u_int64_t stripenum, u_int32_t stripeRelativeOffset, u_int32_t len)
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

    if (!drivePtr) 
    {
        return this->recoverForDrive(buf, drivenum, driveOffset, len);
    }

    drivePtr->read(buf, len, driveOffset);

    return len;
}

bool SmartArrayRaid6Reader::isReedSolomonDrive(u_int16_t drivenum, u_int64_t driveOffset)
{
    u_int64_t currentStripeRow = driveOffset / this->stripeSizeInBytes;
    u_int32_t parityCycle = (this->drives.size() * this->parityDelay);
    u_int32_t reedSolomonCycleRow = currentStripeRow % parityCycle;
    u_int16_t reedSolomonDrive = drives.size() - (reedSolomonCycleRow / this->parityDelay) - 1;

    return drivenum == reedSolomonDrive;
}

bool SmartArrayRaid6Reader::isParityDrive(u_int16_t drivenum, u_int64_t driveOffset)
{
    u_int64_t currentStripeRow = driveOffset / this->stripeSizeInBytes;
    u_int32_t parityCycle = (this->drives.size() * this->parityDelay);
    u_int32_t parityCycleRow = (currentStripeRow + this->parityDelay) % parityCycle;
    u_int16_t parityDrive = drives.size() - (parityCycleRow / this->parityDelay) - 1;

    return drivenum == parityDrive;
}

u_int32_t SmartArrayRaid6Reader::recoverForDrive(void *buf, u_int16_t drivenum, u_int64_t driveOffset, u_int32_t len)
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
    
    // TODO: For god sake, make it multithreaded please.
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

u_int32_t SmartArrayRaid6Reader::recoverForTwoDrives(void *buf, u_int16_t drive1num, u_int16_t drive2num, u_int64_t driveOffset, u_int32_t len)
{
    throw std::runtime_error("Not implemented yet :c");
    return u_int32_t();
}
