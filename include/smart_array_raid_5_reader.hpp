#pragma once

#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"
#include "types.hpp"

namespace sg
{

struct SmartArrayRaid5ReaderOptions
{

    /// @brief stripe size in kilobytes.
    u32 stripeSize;
    u16 parityDelay;

    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u64 size = 0;
    u64 offset = 0;
};

class SmartArrayRaid5Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid5Reader(const SmartArrayRaid5ReaderOptions& options);
    int read(void *buf, u32 len, u64 offset) override;

private:
    u32 stripeSizeInBytes;
    u16 parityDelay;

    std::vector<std::shared_ptr<DriveReader>> drives;

    // Stripes will be index from 0.
    u64 stripeNumber(u64 offset);
    u32 stripeRelativeOffset(u64 stripenum, u64 offset);
    u16 stripeDriveNumber(u64 stripenum);
    u64 stripeDriveOffset(u64 stripenum, u32 stripeRelativeOffset);
    u32 lastRowStripeSize();
    bool isLastRow(u64 rownum);
    u32 readFromStripe(void* buf, u64 stripenum, u32 stripeRelativeOffset, u32 len);
    u32 recoverForDrive(void* buf, u16 drivenum, u64 driveOffset, u32 len);
};

} // end namespace sg
