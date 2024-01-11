#include "smart_array_raid_6_reader.hpp"
#include <math.h>
#include <memory.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <memory>

static const u_int8_t gflog[] = {
    0x00, 0x00, 0x01, 0x19, 0x02, 0x32, 0x1a, 0xc6, 0x03, 0xdf, 0x33, 0xee, 0x1b, 0x68, 0xc7, 0x4b,
    0x04, 0x64, 0xe0, 0x0e, 0x34, 0x8d, 0xef, 0x81, 0x1c, 0xc1, 0x69, 0xf8, 0xc8, 0x08, 0x4c, 0x71,
    0x05, 0x8a, 0x65, 0x2f, 0xe1, 0x24, 0x0f, 0x21, 0x35, 0x93, 0x8e, 0xda, 0xf0, 0x12, 0x82, 0x45,
    0x1d, 0xb5, 0xc2, 0x7d, 0x6a, 0x27, 0xf9, 0xb9, 0xc9, 0x9a, 0x09, 0x78, 0x4d, 0xe4, 0x72, 0xa6,
    0x06, 0xbf, 0x8b, 0x62, 0x66, 0xdd, 0x30, 0xfd, 0xe2, 0x98, 0x25, 0xb3, 0x10, 0x91, 0x22, 0x88,
    0x36, 0xd0, 0x94, 0xce, 0x8f, 0x96, 0xdb, 0xbd, 0xf1, 0xd2, 0x13, 0x5c, 0x83, 0x38, 0x46, 0x40,
    0x1e, 0x42, 0xb6, 0xa3, 0xc3, 0x48, 0x7e, 0x6e, 0x6b, 0x3a, 0x28, 0x54, 0xfa, 0x85, 0xba, 0x3d,
    0xca, 0x5e, 0x9b, 0x9f, 0x0a, 0x15, 0x79, 0x2b, 0x4e, 0xd4, 0xe5, 0xac, 0x73, 0xf3, 0xa7, 0x57,
    0x07, 0x70, 0xc0, 0xf7, 0x8c, 0x80, 0x63, 0x0d, 0x67, 0x4a, 0xde, 0xed, 0x31, 0xc5, 0xfe, 0x18,
    0xe3, 0xa5, 0x99, 0x77, 0x26, 0xb8, 0xb4, 0x7c, 0x11, 0x44, 0x92, 0xd9, 0x23, 0x20, 0x89, 0x2e,
    0x37, 0x3f, 0xd1, 0x5b, 0x95, 0xbc, 0xcf, 0xcd, 0x90, 0x87, 0x97, 0xb2, 0xdc, 0xfc, 0xbe, 0x61,
    0xf2, 0x56, 0xd3, 0xab, 0x14, 0x2a, 0x5d, 0x9e, 0x84, 0x3c, 0x39, 0x53, 0x47, 0x6d, 0x41, 0xa2,
    0x1f, 0x2d, 0x43, 0xd8, 0xb7, 0x7b, 0xa4, 0x76, 0xc4, 0x17, 0x49, 0xec, 0x7f, 0x0c, 0x6f, 0xf6,
    0x6c, 0xa1, 0x3b, 0x52, 0x29, 0x9d, 0x55, 0xaa, 0xfb, 0x60, 0x86, 0xb1, 0xbb, 0xcc, 0x3e, 0x5a,
    0xcb, 0x59, 0x5f, 0xb0, 0x9c, 0xa9, 0xa0, 0x51, 0x0b, 0xf5, 0x16, 0xeb, 0x7a, 0x75, 0x2c, 0xd7,
    0x4f, 0xae, 0xd5, 0xe9, 0xe6, 0xe7, 0xad, 0xe8, 0x74, 0xd6, 0xf4, 0xea, 0xa8, 0x50, 0x58, 0xaf
};

static const u_int8_t gfilog[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1d, 0x3a, 0x74, 0xe8, 0xcd, 0x87, 0x13, 0x26,
    0x4c, 0x98, 0x2d, 0x5a, 0xb4, 0x75, 0xea, 0xc9, 0x8f, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0,
    0x9d, 0x27, 0x4e, 0x9c, 0x25, 0x4a, 0x94, 0x35, 0x6a, 0xd4, 0xb5, 0x77, 0xee, 0xc1, 0x9f, 0x23,
    0x46, 0x8c, 0x05, 0x0a, 0x14, 0x28, 0x50, 0xa0, 0x5d, 0xba, 0x69, 0xd2, 0xb9, 0x6f, 0xde, 0xa1,
    0x5f, 0xbe, 0x61, 0xc2, 0x99, 0x2f, 0x5e, 0xbc, 0x65, 0xca, 0x89, 0x0f, 0x1e, 0x3c, 0x78, 0xf0,
    0xfd, 0xe7, 0xd3, 0xbb, 0x6b, 0xd6, 0xb1, 0x7f, 0xfe, 0xe1, 0xdf, 0xa3, 0x5b, 0xb6, 0x71, 0xe2,
    0xd9, 0xaf, 0x43, 0x86, 0x11, 0x22, 0x44, 0x88, 0x0d, 0x1a, 0x34, 0x68, 0xd0, 0xbd, 0x67, 0xce,
    0x81, 0x1f, 0x3e, 0x7c, 0xf8, 0xed, 0xc7, 0x93, 0x3b, 0x76, 0xec, 0xc5, 0x97, 0x33, 0x66, 0xcc,
    0x85, 0x17, 0x2e, 0x5c, 0xb8, 0x6d, 0xda, 0xa9, 0x4f, 0x9e, 0x21, 0x42, 0x84, 0x15, 0x2a, 0x54,
    0xa8, 0x4d, 0x9a, 0x29, 0x52, 0xa4, 0x55, 0xaa, 0x49, 0x92, 0x39, 0x72, 0xe4, 0xd5, 0xb7, 0x73,
    0xe6, 0xd1, 0xbf, 0x63, 0xc6, 0x91, 0x3f, 0x7e, 0xfc, 0xe5, 0xd7, 0xb3, 0x7b, 0xf6, 0xf1, 0xff,
    0xe3, 0xdb, 0xab, 0x4b, 0x96, 0x31, 0x62, 0xc4, 0x95, 0x37, 0x6e, 0xdc, 0xa5, 0x57, 0xae, 0x41,
    0x82, 0x19, 0x32, 0x64, 0xc8, 0x8d, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xdd, 0xa7, 0x53, 0xa6,
    0x51, 0xa2, 0x59, 0xb2, 0x79, 0xf2, 0xf9, 0xef, 0xc3, 0x9b, 0x2b, 0x56, 0xac, 0x45, 0x8a, 0x09,
    0x12, 0x24, 0x48, 0x90, 0x3d, 0x7a, 0xf4, 0xf5, 0xf7, 0xf3, 0xfb, 0xeb, 0xcb, 0x8b, 0x0b, 0x16,
    0x2c, 0x58, 0xb0, 0x7d, 0xfa, 0xe9, 0xcf, 0x83, 0x1b, 0x36, 0x6c, 0xd8, 0xad, 0x47, 0x8e, 0x01
};

u_int8_t gfDrive(u_int8_t index)
{
    return gfilog[index];
}

u_int8_t gfMul(u_int8_t a, u_int8_t b)
{
    if (a == 0 || b == 0)
    {
        return 0;
    }
    return gfilog[(gflog[a] + gflog[b]) % 255];
}

u_int8_t subGf8(u_int8_t a, u_int8_t b)
{
    return a > b ? a - b : 255 - (b - a);
}

u_int8_t gfDiv(u_int8_t a, u_int8_t b)
{
    return gfilog[subGf8(gflog[a], gflog[b])];
}

SmartArrayRaid6Reader::SmartArrayRaid6Reader(SmartArrayRaid6ReaderOptions &options)
{
    this->driveName = options.readerName;
    this->parityDelay = options.parityDelay;
    this->stripeSizeInBytes = options.stripeSize * 1024;

    if (options.driveReaders.size() < 4)
    {
        throw std::invalid_argument("For RAID 6 at least 4 drives must be provider (two can be missing but still they have to be in the driveReaders list represented with nullptr)");
    }

    if (options.driveReaders.size() > 256)
    {
        throw std::invalid_argument("You can provide no more than 256 drives for RAID 6.");
    }

    int missingDrives = 0;
    std::vector<u_int64_t> drivesSizes;


    for (auto& drive : options.driveReaders)
    {
        if (!drive)
        {
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

    // 32MiB + something from the end of drive are stored controller metadata.
    // Data are stored until that metadata + *something* in RAID 6
    // Although I don't think if we need to get this correctly.
    // TODO: Find out how to calculate this "something"
    this->singleDriveSize -= 1024 * 1024 * 32;
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

u_int64_t SmartArrayRaid6Reader::driveSize()
{
    // I am skipping last stripe on drive if it's not whole
    // I don't know how Smart Array hanle that, does it use it or skip it?
    // For simplicity sake I will skip it for now
    // TODO: Check if last, not whole stripe is used. If it used, add code to read from it.
    u_int64_t wholeStripesOnDrive = this->singleDriveSize / this->stripeSizeInBytes;
    return wholeStripesOnDrive * this->stripeSizeInBytes * (drives.size() - 2);
}

u_int64_t SmartArrayRaid6Reader::stripeNumber(u_int64_t offset)
{
    return offset / this->stripeSizeInBytes;
}

u_int32_t SmartArrayRaid6Reader::stripeRelativeOffset(u_int64_t stripenum, u_int64_t offset)
{
    return offset - stripenum * this->stripeSizeInBytes;
}

u_int8_t SmartArrayRaid6Reader::stripeDriveNumber(u_int64_t stripenum)
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

    return (currentStripeRow * this->stripeSizeInBytes) + stripeRelativeOffset;
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

bool SmartArrayRaid6Reader::isReedSolomonDrive(u_int8_t drivenum, u_int64_t driveOffset)
{
    u_int64_t currentStripeRow = driveOffset / this->stripeSizeInBytes;
    u_int32_t parityCycle = (this->drives.size() * this->parityDelay);
    u_int32_t reedSolomonCycleRow = currentStripeRow % parityCycle;
    u_int16_t reedSolomonDrive = drives.size() - (reedSolomonCycleRow / this->parityDelay) - 1;

    return drivenum == reedSolomonDrive;
}

bool SmartArrayRaid6Reader::isParityDrive(u_int8_t drivenum, u_int64_t driveOffset)
{
    u_int64_t currentStripeRow = driveOffset / this->stripeSizeInBytes;
    u_int32_t parityCycle = (this->drives.size() * this->parityDelay);
    u_int32_t parityCycleRow = (currentStripeRow + this->parityDelay) % parityCycle;
    u_int16_t parityDrive = drives.size() - (parityCycleRow / this->parityDelay) - 1;

    return drivenum == parityDrive;
}

u_int32_t SmartArrayRaid6Reader::recoverForDrive(void *buf, u_int8_t drivenum, u_int64_t driveOffset, u_int32_t len)
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

    std::unique_ptr<u_int8_t[]> _uniq_out(new u_int8_t[len]);
    std::unique_ptr<u_int8_t[]> _uniq_temp(new u_int8_t[len]);

    // I am getting raw pointers coz for array operations
    // compiler will use SSE for them with -O3
    // with unique pointer that is not the case
    // https://godbolt.org/z/aTYrhqPGb
    // I am using unique here only to delete it automatically ^^
    u_int8_t* out = _uniq_out.get();
    u_int8_t* temp = _uniq_temp.get();

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

u_int32_t SmartArrayRaid6Reader::recoverForTwoDrives(void *buf, u_int8_t drive1num, u_int8_t drive2num, u_int64_t driveOffset, u_int32_t len)
{
    if  (this->isParityDrive(drive1num, driveOffset))
    {
        return this->recoverForOneMissingDataDrive(buf, drive2num, driveOffset, len);
    }

    if  (this->isParityDrive(drive2num, driveOffset))
    {
        return this->recoverForOneMissingDataDrive(buf, drive1num, driveOffset, len);
    }

    return this->recoverForTwoMissingDataDrives(buf, drive1num, drive2num, driveOffset, len);
}

u_int32_t SmartArrayRaid6Reader::recoverForOneMissingDataDrive(void *buf, u_int8_t drivenum, u_int64_t driveOffset, u_int32_t len)
{
    std::shared_ptr<DriveReader> reedSolomonDrive;
    std::vector<std::shared_ptr<DriveReader>> dataDrives;
    u_int8_t lostDataDriveNum ; // Only drive number. We have drives, eg: 1, 2, 3, 4 and data drives: 1, 2. I need number relative to data drives. PAIN

    for (int i = 0; i < this->drives.size(); i++)
    {
        if (this->isReedSolomonDrive(i, driveOffset))
        {
            reedSolomonDrive = this->drives[i];
            continue;
        }

        if (!this->isParityDrive(i, driveOffset))
        {
            if (i == drivenum)
            {
                lostDataDriveNum = dataDrives.size();
            }
            dataDrives.push_back(this->drives[i]);
        }
    }

    // I am using unique_ptr only to delete allocated data automatically
    // I prefer working with raw pointers in this scenario
    // BTW compilers better handle raw pointers -> https://godbolt.org/z/aTYrhqPGb
    // But in this particular case I don't think compiler will use SSE XD RAID 6 is tough man!
    // Yeah I know the consequences of getting raw pointer of unique_ptr
    // I triple checked, I am using this raw pointer only in this scope
    std::vector<std::unique_ptr<u_int8_t[]>> uniq_data;
    std::vector<u_int8_t*> raw_data;

    // Bruh, RAID 6 is tough. I could read that data on demand but loading
    // all of it into memory seems like trading memory for performance
    // With 256 drives and max stripe size of 1024K theoretically it could eat
    // up to 256M of memory, which is okey for me
    for (auto& drive : dataDrives)
    {
        uniq_data.push_back(std::unique_ptr<u_int8_t[]>(new u_int8_t[len]));
        u_int8_t* dataBuf = uniq_data[uniq_data.size() - 1].get();
        raw_data.push_back(dataBuf);

        if (drive)
        {
            drive->read(dataBuf, len, driveOffset);

            // Calculate some reed solomon shit for the data, will need that later
            for (int i = 0; i < len; i++)
            {
                dataBuf[i] = gfMul(gfDrive(i), dataBuf[i]);
            }

            continue;
        }
        reedSolomonDrive->read(dataBuf, len, driveOffset);
    }

    for (int i = 0; i < len; i++)
    {
        u_int8_t partialRS = 0;

        for (int j = 0; j < raw_data.size(); j++)
        {
            partialRS ^= raw_data[j][i];
        }

        u_int8_t divRes = gfDiv(1, gfDrive(lostDataDriveNum));
        static_cast<u_int8_t*>(buf)[i] = gfMul(divRes, partialRS);
    }

    return len;
}

u_int32_t SmartArrayRaid6Reader::recoverForTwoMissingDataDrives(void *buf, u_int8_t drive1num, u_int8_t drive2num, u_int64_t driveOffset, u_int32_t len)
{
    std::shared_ptr<DriveReader> reedSolomonDrive;
    std::shared_ptr<DriveReader> parityDrive;
    std::vector<std::shared_ptr<DriveReader>> dataDrives;
    u_int8_t lostDataDrive1Num; // Only drive number. We have drives, eg: 1, 2, 3, 4 and data drives: 1, 2. I need number relative to data drives. PAIN
    u_int8_t lostDataDrive2Num;

    for (int i = 0; i < this->drives.size(); i++)
    {
        if (this->isReedSolomonDrive(i, driveOffset))
        {
            reedSolomonDrive = this->drives[i];
            continue;
        }

        if (this->isParityDrive(i, driveOffset))
        {
            parityDrive = this->drives[i];
            continue;
        }

        if (i == drive1num)
        {
            lostDataDrive1Num = dataDrives.size();
        }

        if (i == drive1num)
        {
            lostDataDrive2Num = dataDrives.size();
        }

        dataDrives.push_back(this->drives[i]);
    }

    // I am using unique_ptr only to delete allocated data automatically
    // I prefer working with raw pointers in this scenario
    // BTW compilers better handle raw pointers -> https://godbolt.org/z/aTYrhqPGb
    // But in this particular case I don't think compiler will use SSE XD RAID 6 is tough man!
    // Yeah I know the consequences of getting raw pointer of unique_ptr
    // I triple checked, I am using those raw pointers only in this scope
    std::vector<std::unique_ptr<u_int8_t[]>> uniq_data;
    std::unique_ptr<u_int8_t[]> uniq_rs(new u_int8_t[len]);
    std::unique_ptr<u_int8_t[]> uniq_parity(new u_int8_t[len]);

    std::vector<std::pair<u_int8_t, u_int8_t*>> raw_data;
    u_int8_t* rsData = uniq_rs.get();
    u_int8_t* parityData = uniq_parity.get();

    reedSolomonDrive->read(rsData, len, driveOffset);
    parityDrive->read(parityData, len, driveOffset);

    // Bruh, RAID 6 is tough. I could read that data on demand but loading
    // all of it into memory seems like trading memory for performance
    // With 256 drives and max stripe size of 1024K theoretically it could eat
    // up to 256M of memory, which is okey for me
    for (int i = 0; i < dataDrives.size(); i++)
    {
        if (dataDrives[i])
        {
            uniq_data.push_back(std::unique_ptr<u_int8_t[]>(new u_int8_t[len]));
            u_int8_t* dataBuf = uniq_data[uniq_data.size() - 1].get();
            raw_data.push_back({ i, dataBuf });
            dataDrives[i]->read(dataBuf, len, driveOffset);
            // Now I can't calculate reed solomon shit here
            // Coz I will need raw data for parity, gosh
            // But I still need drive index for later calculations, so I use std::pair here.
            // I feel pain
            continue;
        }
    }

    for (int i = 0; i < len; i++)
    {
        u_int8_t partialParity = 0;
        u_int8_t partialRS = 0;

        for (int j = 0; j < raw_data.size(); j++)
        {
            partialParity ^= raw_data[j].second[i];
            partialRS ^= gfMul(gfDrive(raw_data[j].first), raw_data[j].second[i]);
        }

        u_int8_t g = gfDiv(1, lostDataDrive1Num ^ lostDataDrive2Num);
        u_int8_t xoredParity = partialParity ^ parityData[i];
        u_int8_t xoredRS = partialRS ^ rsData[i];
        u_int8_t mid = gfMul(gfDrive(lostDataDrive2Num), xoredParity) ^ xoredRS;

        // Yes, we have to do all of this calculations to get ONE BYTE, ugh
        u_int8_t dataByte = gfMul(mid, g);
        static_cast<u_int8_t*>(buf)[i] = dataByte;

        // We could also recover for second drive: u_int8_t secondDriveDataByte = dataByte ^ xoredPD
        // But I don't care about seconds, this code is not meant to rebuild the damaged array but to read the damaged array.
    }

    return len;
}
