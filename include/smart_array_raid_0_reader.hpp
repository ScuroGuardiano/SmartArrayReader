#pragma once

#include <sys/types.h>
#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"

struct SmartArrayRaid0ReaderOptions
{
    /// @brief stripe size in kilobytes.
    u_int32_t stripeSize;

    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u_int64_t size = 0;
    u_int64_t offset = 0;
    bool nometadata = false;
};

class SmartArrayRaid0Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid0Reader(const SmartArrayRaid0ReaderOptions& options);
    int read(void *buf, u_int32_t len, u_int64_t offset) override;

private:
    u_int32_t stripeSizeInBytes;

    std::vector<std::shared_ptr<DriveReader>> drives;

    // Stripes will be index from 0.
    u_int64_t stripeNumber(u_int64_t offset);
    u_int32_t stripeRelativeOffset(u_int64_t stripenum, u_int64_t offset);
    u_int16_t stripeDriveNumber(u_int64_t stripenum);
    u_int64_t stripeDriveOffset(u_int64_t stripenum, u_int32_t stripeRelativeOffset);
    u_int32_t lastRowStripeSize();
    bool isLastRow(u_int64_t rownum);
    u_int32_t readFromStripe(void* buf, u_int64_t stripenum, u_int32_t stripeRelativeOffset, u_int32_t len);
};
