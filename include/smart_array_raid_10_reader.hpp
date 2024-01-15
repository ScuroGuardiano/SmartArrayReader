#pragma once

#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"
#include "smart_array_raid_0_reader.hpp"
#include "smart_array_raid_1_reader.hpp"
#include "types.hpp"

namespace sg
{

struct SmartArrayRaid10ReaderOptions
{
    /// @brief stripe size in kilobytes.
    u32 stripeSize;

    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u64 size = 0;
    u64 offset = 0;
};

class SmartArrayRaid10Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid10Reader(const SmartArrayRaid10ReaderOptions& options);
    int read(void *buf, u32 len, u64 offset) override;
    u64 driveSize() override;
private:
    std::unique_ptr<SmartArrayRaid0Reader> raid0Reader;
};

} // end namespace sg
