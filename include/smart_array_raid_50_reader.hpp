#pragma once

#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"
#include "smart_array_raid_0_reader.hpp"
#include "smart_array_raid_5_reader.hpp"
#include "types.hpp"

namespace sg
{

struct SmartArrayRaid50ReaderOptions
{
    /// @brief stripe size in kilobytes.
    u32 stripeSize;
    u16 parityDelay;
    u16 parityGroups;

    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u64 size = 0;
    u64 offset = 0;
};

class SmartArrayRaid50Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid50Reader(const SmartArrayRaid50ReaderOptions& options);
    int read(void *buf, u32 len, u64 offset) override;
    u64 driveSize() override;
private:
    std::unique_ptr<SmartArrayRaid0Reader> raid0Reader;
};

} // end namespace sg