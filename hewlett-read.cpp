#include "BUSE/buse.h"
#include <argp.h>
#include <string>
#include <memory.h>
#include <memory>
#include <iostream>
#include <exception>
#include <limits.h>
#include "smart_array_raid_0_reader.hpp"
#include "smart_array_raid_1_reader.hpp"
#include "smart_array_raid_5_reader.hpp"
#include "smart_array_raid_6_reader.hpp"
#include "smart_array_raid_10_reader.hpp"
#include "smart_array_raid_50_reader.hpp"
#include "smart_array_raid_60_reader.hpp"

using namespace sg;

std::unique_ptr<DriveReader> reader;

struct ProgramOptions
{
    /// @brief Stripe size in kilobytes
    u32 stripeSize;
    u16 parityDelay;
    u16 raidLevel;
    u16 parityGroups;
    u64 size;
    u64 offset;
    std::vector<std::string> drives;
    std::string outputDevice;
};

int read(void *buf, u32 len, u64 offset, void *userdata)
{
    try 
    {
        return reader->read(buf, len, offset);
    }
    catch (std::runtime_error& ex)
    {
        std::cerr << "Runtime error occured while reading data: " << ex.what() << std::endl;
        return -1;
    }
}

// Argument parsing
static argp_option options[] = {
    { "stripe-size", 's', "256", 0, "Stripe size in KiB. Default: 256", 0 },
    { "parity-delay", 'p', "16", 0, "Parity delay, P420 Controllers use parity delay for their RAID 5, read more: https://www.freeraidrecovery.com/library/delayed-parity.aspx. Default: 16", 0 },
    { "delay", 'p', 0, OPTION_ALIAS, 0, 0 },
    { "raid", 'r', "<level>", 0, "Raid level, required!", 0 },
    { "output", 'o', "/dev/nbd0", 0, "Output device, it has to be /dev/nbdx. Default: /dev/nbd0", 0 },
    { "parity-groups", 'g', "2", 0, "Parity groups in RAID 50 and 60", 0 },
    { "size", 'S', "0", 0, "Size of logical drives, 0 is maximum possible. Default: 0" },
    { "offset", 'O', "0", 0, "Offset on each physical drive, Default: 0" },
    {0}
};

u16 argToU16(std::string arg, std::string argName)
{
    try 
    {
        unsigned num = std::stoul(arg);
        if (num > USHRT_MAX)
        {
            throw std::out_of_range("Argument exceeds range");
        }
        return num;
    }
    catch(std::invalid_argument const& ex)
    {
        throw std::invalid_argument("Argument " + argName + " (value:" + arg + ") is invalid. It has to be unsigned 16 bit integer.");
    }
    catch(std::out_of_range const& ex)
    {
        throw std::out_of_range("Argument " + argName + " (value:" + arg + ") is too large!. It has to be unsigned 16 bit integer.");
    }
}

u32 argToU32(std::string arg, std::string argName)
{
    try 
    {
        return std::stoul(arg);
    }
    catch(std::invalid_argument const& ex)
    {
        throw std::invalid_argument("Argument " + argName + " (value:" + arg + ") is invalid. It has to be unsigned 32 bit integer.");
    }
    catch(std::out_of_range const& ex)
    {
        throw std::out_of_range("Argument " + argName + " (value:" + arg + ") is too large!. It has to be unsigned 32 bit integer.");
    }
}

u64 argToU64(std::string arg, std::string argName)
{
    try 
    {
        return std::stoull(arg);
    }
    catch(std::invalid_argument const& ex)
    {
        throw std::invalid_argument("Argument " + argName + " (value:" + arg + ") is invalid. It has to be unsigned 64 bit integer.");
    }
    catch(std::out_of_range const& ex)
    {
        throw std::out_of_range("Argument " + argName + " (value:" + arg + ") is too large!. It has to be unsigned 64 bit integer.");
    }
}

error_t parseOpt(int key, char *arg, argp_state *state)
{
    ProgramOptions* options = reinterpret_cast<ProgramOptions*>(state->input);
    std::string argStr;

    switch (key)
    {
    case 's':
        options->stripeSize = argToU32(arg, "stripe-size");
        break;
    case 'p':
        options->parityDelay = argToU16(arg, "parity-delay");
        break;
    case 'r':
        options->raidLevel = argToU16(arg, "raid");
        break;
    case 'o':
        options->outputDevice = arg;
        break;
    case 'g':
        options->parityGroups = argToU16(arg, "parity-groups");
        break;
    case 'S':
        options->size = argToU64(arg, "size");
        break;
    case 'O':
        options->offset = argToU64(arg, "offset");
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 256)
        {
            throw std::invalid_argument("Too many arguments.");
        }
        argStr = arg;
        if (argStr == "X")
        {
            argStr.clear();
        }

        options->drives.push_back(argStr);
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 1)
        {
            std::cerr << "Not enough arguments." << std::endl;
            argp_usage(state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

void drivesPathVectorToDeviceReaderVector(
    std::vector<std::string>& paths,
    std::vector<std::shared_ptr<DriveReader>>& out)
{
    for (auto& path : paths)
    {
        if (!path.empty())
        {
            out.push_back(std::make_shared<BlockDeviceReader>(path));
        }
        else
        {
            // Reader should handle this
            out.push_back(nullptr);
        }
    }
}

void initForRaid0(ProgramOptions &opts)
{
    SmartArrayRaid0ReaderOptions readerOpts {
        .stripeSize = opts.stripeSize,
        .size = opts.size,
        .offset = opts.offset
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);
    
    reader = std::make_unique<SmartArrayRaid0Reader>(readerOpts);
}


void initForRaid1(ProgramOptions &opts)
{
    SmartArrayRaid1ReaderOptions readerOpts {
        .size = opts.size,
        .offset = opts.offset
    };
    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);
    reader = std::make_unique<SmartArrayRaid1Reader>(readerOpts);
}

void initForRaid5(ProgramOptions &opts)
{
    SmartArrayRaid5ReaderOptions readerOpts {
        .stripeSize = opts.stripeSize,
        .parityDelay = opts.parityDelay,
        .size = opts.size,
        .offset = opts.offset
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);

    reader = std::make_unique<SmartArrayRaid5Reader>(readerOpts);
}

void initForRaid6(ProgramOptions &opts)
{
    SmartArrayRaid6ReaderOptions readerOpts {
        .stripeSize = opts.stripeSize,
        .parityDelay = opts.parityDelay,
        .size = opts.size,
        .offset = opts.offset
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);

    reader = std::make_unique<SmartArrayRaid6Reader>(readerOpts);
}

void initForRaid10(ProgramOptions &opts)
{
    SmartArrayRaid10ReaderOptions readerOpts {
        .stripeSize = opts.stripeSize,
        .size = opts.size,
        .offset = opts.offset
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);

    reader = std::make_unique<SmartArrayRaid10Reader>(readerOpts);
}

void initForRaid50(ProgramOptions &opts)
{
    SmartArrayRaid50ReaderOptions readerOpts {
        .stripeSize = opts.stripeSize,
        .parityDelay = opts.parityDelay,
        .parityGroups = opts.parityGroups,
        .size = opts.size,
        .offset = opts.offset
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);

    reader = std::make_unique<SmartArrayRaid50Reader>(readerOpts);
}

void initForRaid60(ProgramOptions &opts)
{
    SmartArrayRaid60ReaderOptions readerOpts {
        .stripeSize = opts.stripeSize,
        .parityDelay = opts.parityDelay,
        .parityGroups = opts.parityGroups,
        .size = opts.size,
        .offset = opts.offset
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);

    reader = std::make_unique<SmartArrayRaid60Reader>(readerOpts);
}

int main(int argc, char** argv)
{
    ProgramOptions opts = {
        .stripeSize = 256,
        .parityDelay = 16,
        .raidLevel = 2137,
        .outputDevice = "/dev/nbd0"
    };

    static argp argp = {
        .options = options,
        .parser = parseOpt,
        .args_doc = "DRIVE1 DRIVE2 ...DRIVEN",
        .doc = "HP Smart Array Raid Reader.\n\n"
               "Drives must be provided in valid order. If you forgot order of drives in the array then use "
               "a program packard-tell included with hewlett-read:\n"
               "\tpackard-tell /dev/sda /dev/sdb /dev/sdc /dev/sdd\n"
               "For missing drives type X instead of their path.\n"
               "--raid is required option.\n\n"
               "Examples:\n"
               "\thewlett-read --raid 5 /dev/sda /dev/sdb /dev/sdc /dev/sdd\n"
               "\thewlett-read --raid 5 /dev/sda X /dev/sdc /dev/sdd\n"
               "\thewlett-read --raid 6 /dev/sda X /dev/sdc X"
    };

    int parseRet = argp_parse(&argp, argc, argv, 0, 0, &opts);
    if (parseRet != 0)
    {
        std::cerr << "Arguments parse error, see above." << std::endl;
        return -1;
    }

    switch (opts.raidLevel)
    {
        case 0:
            initForRaid0(opts);
            break;
        case 1:
            initForRaid1(opts);
            break;
        case 5:
            initForRaid5(opts);
            break;
        case 6:
            initForRaid6(opts);
            break;
        case 10:
            initForRaid10(opts);
            break;
        case 50:
            initForRaid50(opts);
            break;
        case 60:
            initForRaid60(opts);
            break;
        case 2137:
            std::cerr << "Error: No RAID level was provided." << std::endl;
            return -1;
        default:
            std::cerr << "RAID " << opts.raidLevel << " is not supported." << std::endl;
            return -1;
    }

    buse_operations ops = {
        .read = read,
        .size = reader->driveSize(),
        .blksize = 512
    };

    std::cout << "Attaching to " << opts.outputDevice << "..." << std::endl;
    buse_main(opts.outputDevice.c_str(), &ops, NULL);
}
