#include "BUSE/buse.h"
#include <string>
#include <memory.h>
#include <memory>
#include <iostream>
#include "smart_array_raid_5_reader.hpp"

std::unique_ptr<DriveReader> reader;

int read(void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
    return reader->read(buf, len, offset);
}

int main(int argc, char** argv)
{
    SmartArrayRaid5ReaderOptions readerOpts = {
        .stripeSize = 256,
        .parityDelay = 16,
        .driveReaders = {
            std::make_shared<BlockDeviceReader>("/dev/sde"),
            std::make_shared<BlockDeviceReader>("/dev/sdb"),
            std::make_shared<BlockDeviceReader>("/dev/sdc"),
            nullptr
        }
    };

    reader = std::make_unique<SmartArrayRaid5Reader>(readerOpts);

    buse_operations ops = {
        .read = read,
        .size = reader->driveSize(),
        .blksize = 512
    };

    buse_main("/dev/nbd1", &ops, NULL);
}
