#pragma once

#include <sys/types.h>
#include <vector>
#include <memory>
#include "drive_reader.hpp"

struct SmartArrayRaid1ReaderOptions
{
    /// @brief drives path
    std::vector<std::shared_ptr<DriveReader>> driveReaders;
    std::string readerName;
};

class SmartArrayRaid1Reader : public DriveReader
{
public:
    SmartArrayRaid1Reader(SmartArrayRaid1ReaderOptions& options);
    int read(void *buf, u_int32_t len, u_int64_t offset) override;
    u_int64_t driveSize() override;

private:
    // Smallest drive in the array
    u_int64_t singleDriveSize;
    std::vector<std::shared_ptr<DriveReader>> drives;
};
