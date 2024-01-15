#pragma once

#include "types.hpp"
#include <string>
#include <fstream>

namespace sg
{

class DriveReader
{
public:
    virtual int read(void* buf, u32 len, u64 offset) = 0;
    virtual u64 driveSize() = 0;
    virtual inline ~DriveReader() {};
    virtual std::string name();

protected:
    std::string driveName;
};

class BlockDeviceReader : public DriveReader
{
public:
    BlockDeviceReader(std::string path);
    int read(void* buf, u32 len, u64 offset) override;
    u64 driveSize() override;

private:
    std::string path;
    std::ifstream devstream;
    u64 size;
    u64 readDriveSize(std::string path);
};

} // end namespace sg