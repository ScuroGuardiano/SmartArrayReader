#include "drive_reader.hpp"
#include <sstream>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

std::string DriveReader::name()
{
    return this->driveName;
}

BlockDeviceReader::BlockDeviceReader(std::string path)
{
    this->size = this->readDriveSize(path);
    this->driveName = path;

    // Yeah, I use linux syscalls to read drive sizes and here
    // I use ifstream, I could use just syscalls
    // But they're scary and I am too lazy to read how to process errors
    // from them. So I'll stick with ifstream, at least for now.
    this->devstream = std::ifstream(path, std::ios::binary);

    if (!this->devstream.is_open())
    {
        std::stringstream errMsgStream;
        errMsgStream << "Could not open drive "
                     << path << " Reason: "
                     << strerror(errno);

        throw std::runtime_error(errMsgStream.str());
    }
}

int BlockDeviceReader::read(void *buf, u_int32_t len, u_int64_t offset)
{
    this->devstream.seekg(offset);
    this->devstream.read((char*)buf, len);

    if (this->devstream.fail())
    {
        std::stringstream errMsg;
        errMsg << "Reading from drive " << this->name() << " has failed. ";
        if (this->devstream.eof()) {
            errMsg << "Reason: end of file.";
        }
        else {
            errMsg << "Reason: unknown, errno: " << strerror(errno);
        }

        throw std::runtime_error(errMsg.str());
    }

    return 0;
}

u_int64_t BlockDeviceReader::driveSize()
{
    return this->size;
}

u_int64_t BlockDeviceReader::readDriveSize(std::string path)
{
    int fd = open(path.c_str(), O_RDONLY);

    if (fd == -1)
    {
        std::stringstream errMsgStream;
        errMsgStream << "Could not read size of drive "
            << path << "; Reason: "
            << strerror(errno);

        throw std::runtime_error(errMsgStream.str());
    }

    size_t driveSize = 0;
    int rc = ioctl(fd, BLKGETSIZE64, &driveSize);
    close(fd);

    if (rc != 0)
    {
        std::stringstream errMsgStream;
        errMsgStream << "Could not read size of drive "
            << path << "; Reason: "
            << strerror(errno);

        throw std::runtime_error(errMsgStream.str());
    }

    return driveSize;
}
