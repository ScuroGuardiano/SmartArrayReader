#pragma once

#include <sys/types.h>
#include <vector>
#include <memory>
#include "drive_reader.hpp"

struct SmartArrayRaid5ReaderOptions
{

    /// @brief stripe size in kilobytes.
    u_int32_t stripeSize;
    u_int16_t parityDelay;

    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
};

class SmartArrayRaid5Reader : public DriveReader
{
public:
    SmartArrayRaid5Reader(SmartArrayRaid5ReaderOptions& options);
    int read(void *buf, u_int32_t len, u_int64_t offset);
    u_int64_t driveSize();

private:
    u_int32_t stripeSizeInBytes;
    u_int16_t parityDelay;

    // Smallest drive in the array
    u_int64_t singleDriveSize;
    std::vector<std::shared_ptr<DriveReader>> drives;

    // Stripes will be index from 0.
    u_int64_t stripeNumber(u_int64_t offset);
    u_int64_t stripeEndOffset(u_int64_t stripenum);
    u_int32_t stripeRelativeOffset(u_int64_t stripenumber, u_int64_t offset);
    u_int16_t stripeDriveNumber(u_int64_t stripenumber);
    u_int64_t stripeDriveOffset(u_int64_t stripenum, u_int32_t stripeRelativeOffset);
    u_int32_t readFromStripe(void* buf, u_int64_t stripenum, u_int32_t stripeRelativeOffset, u_int32_t len);
    u_int32_t recoverForDrive(void* buf, u_int16_t drivenum, u_int64_t driveOffset, u_int32_t len);
};