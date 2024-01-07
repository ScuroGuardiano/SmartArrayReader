#include "smart_array_raid_5_reader.hpp"
#include <math.h>
#include <iostream>
#include <stdexcept>
#include <sstream>

SmartArrayRaid5Reader::SmartArrayRaid5Reader(SmartArrayRaid5ReaderOptions &options)
{
    this->parityDelay = parityDelay;
    this->stripeSizeInBytes = options.stripeSize * 1024;
    
    int missingDrives = 0;
    for (auto drive : options.drives)
    {
        if (drive.empty())
        {
            throw std::runtime_error("read with missing drive is currently not implemeted.");
            if (missingDrives > 0)
            {
                throw std::invalid_argument("For RAID 5 only 1 missing drive is allowed.");
            }
            missingDrives++;
            this->drives.push_back(std::nullopt);
        }

        std::ifstream driveHandle(drive, std::ios::binary);

        if (!driveHandle.is_open())
        {
            std::stringstream errMsgStream;
            errMsgStream << "Could not open drive "
                << drive << " Reason: "
                << errno;

            throw std::runtime_error(errMsgStream.str());
        }

        this->drives.push_back(std::optional(std::move(driveHandle)));
    }
}

int SmartArrayRaid5Reader::read(void *buf, u_int32_t len, u_int64_t offset)
{
    return 0;
}

u_int64_t SmartArrayRaid5Reader::totalArraySize()
{
    return this->singleDriveSize * (drives.size() - 1);
}

u_int64_t SmartArrayRaid5Reader::stripeNumber(u_int64_t offset)
{
    return offset / (double)this->stripeSizeInBytes;
}

u_int64_t SmartArrayRaid5Reader::stripeEndOffset(u_int64_t stripenum)
{
    return stripenum * this->stripeSizeInBytes - 1;
}

u_int32_t SmartArrayRaid5Reader::stripeRelativeOffset(u_int64_t stripenumber, u_int64_t offset)
{
    return offset - stripenumber * this->stripeSizeInBytes;
}

u_int16_t SmartArrayRaid5Reader::stripeDriveNumber(u_int64_t stripenum)
{
    u_int64_t currentStripeRow = stripenum / (drives.size() - 1);
    u_int32_t delayRowOffset = currentStripeRow % (this->drives.size() * this->parityDelay);
    u_int16_t parityDrive = drives.size() - (delayRowOffset / this->parityDelay) - 1;
    u_int16_t stripeDrive = stripenum % (drives.size() - 1);

    if (stripeDrive == parityDelay)
    {
        stripeDrive++;
    }

    return stripeDrive;
}

u_int64_t SmartArrayRaid5Reader::stripeDriveOffset(u_int64_t stripenum, u_int32_t stripeRelativeOffset)
{
    u_int64_t currentStripeRow = stripenum / (drives.size() - 1);

    return (currentStripeRow * this->stripeSizeInBytes) + stripeRelativeOffset;
}

u_int32_t SmartArrayRaid5Reader::readFromStripe(void *buf, u_int64_t stripenum, u_int32_t stripeRelativeOffset, u_int32_t len)
{
    auto drivenum = stripeDriveNumber(stripenum);
    auto driveOffset = stripeDriveOffset(stripenum, stripeRelativeOffset);
    
    if ((len + stripeRelativeOffset) > this->stripeSizeInBytes)
    {
        std::stringstream errMsg;
        errMsg << "Exceeded stripe data area, "
            << "stripeRelativeOffset = " << stripeRelativeOffset << ";"
            << "len = " << len;
        throw std::out_of_range(errMsg.str());
    }

    auto& driveHandleOpt = this->drives[drivenum];

    if (!driveHandleOpt.has_value()) 
    {
        return this->recoverForDrive(buf, drivenum, driveOffset, len);
    }

    auto& driveHandle = driveHandleOpt.value();
    driveHandle.read((char*)buf, len);
    
    if (driveHandle.fail())
    {
        std::stringstream errMsg;
        errMsg << "Reading from drive has failed. ";
        if (driveHandle.eof()) {
            errMsg << "Reason: end of file.";
        }
        else {
            errMsg << "Reason: unknown, errno: " << errno;
        }

        throw std::out_of_range(errMsg.str());
    }

    return 0;
}

u_int32_t SmartArrayRaid5Reader::recoverForDrive(void *buf, u_int16_t drivenum, u_int64_t driveOffset, u_int32_t len)
{
    throw std::runtime_error("Recovery from parity is not implemented yet");
    return u_int32_t();
}
