#pragma once

#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"
#include "types.hpp"

namespace sg
{

struct SmartArrayRaid1ReaderOptions
{
    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u64 size = 0;
    u64 offset = 0;
};

class SmartArrayRaid1Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid1Reader(const SmartArrayRaid1ReaderOptions& options);
    int read(void *buf, u32 len, u64 offset) override;

private:
    std::vector<std::shared_ptr<DriveReader>> drives;
};

} // end namespace sg
