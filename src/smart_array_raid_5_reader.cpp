#include "smart_array_raid_5_reader.hpp"
#include <math.h>

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
