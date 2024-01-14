#pragma once

#include <sys/types.h>
#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"
#include "smart_array_raid_0_reader.hpp"
#include "smart_array_raid_1_reader.hpp"

struct SmartArrayRaid10ReaderOptions
{
    /// @brief stripe size in kilobytes.
    u_int32_t stripeSize;

    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u_int64_t size = 0;
    u_int64_t offset = 0;
};

class SmartArrayRaid10Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid10Reader(SmartArrayRaid10ReaderOptions& options);
    int read(void *buf, u_int32_t len, u_int64_t offset) override;
    u_int64_t driveSize() override;
private:
    std::unique_ptr<SmartArrayRaid0Reader> raid0Reader;
};
