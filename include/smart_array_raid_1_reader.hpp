#pragma once

#include <sys/types.h>
#include <vector>
#include <memory>
#include "smart_array_reader_base.hpp"

struct SmartArrayRaid1ReaderOptions
{
    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
    u_int64_t size = 0;
    u_int64_t offset = 0;
};

class SmartArrayRaid1Reader : public SmartArrayReaderBase
{
public:
    SmartArrayRaid1Reader(const SmartArrayRaid1ReaderOptions& options);
    int read(void *buf, u_int32_t len, u_int64_t offset) override;

private:
    std::vector<std::shared_ptr<DriveReader>> drives;
};
