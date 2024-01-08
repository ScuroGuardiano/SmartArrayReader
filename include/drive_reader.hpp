#include <sys/types.h>
#include <string>
#include <fstream>

class DriveReader
{
public:
    virtual int read(void* buf, u_int32_t len, u_int64_t offset) = 0;
    virtual u_int64_t driveSize() = 0;
    virtual inline ~DriveReader() {};
};

class BlockDeviceReader : public DriveReader
{
public:
    BlockDeviceReader(std::string path);
    int read(void* buf, u_int32_t len, u_int64_t offset) override;
    u_int64_t driveSize() override;

private:
    std::string path;
    std::ifstream devstream;
    u_int64_t size;
    u_int64_t readDriveSize(std::string path);
};
