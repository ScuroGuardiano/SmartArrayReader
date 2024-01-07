#pragma once

#include <sys/types.h>
#include <fstream>
#include <vector>

class SmartArrayRaid5Reader
{
public:
    int read(void *buf, u_int32_t len, u_int64_t offset);

private:
    u_int32_t stripeSizeInBytes;
    u_int16_t parityDelay;

    // Smallest drive in the array
    u_int64_t singleDriveSize;
    std::vector<std::ifstream> drives;

    u_int64_t totalArraySize();
    // Stripes will be index from 0.
    u_int64_t stripeNumber(u_int64_t offset);
    u_int64_t stripeEndOffset(u_int64_t stripenum);
    u_int32_t stripeRelativeOffset(u_int64_t stripenumber, u_int64_t offset);
    u_int16_t stripeDriveNumber(u_int64_t stripenumber);
    u_int64_t stripeDriveOffset(u_int64_t stripenum, u_int32_t stripeRelativeOffset);
};