#include "BUSE/buse.h"
#include <argp.h>
#include <string>
#include <memory.h>
#include <memory>
#include <iostream>
#include <exception>
#include <limits.h>
#include "smart_array_raid_0_reader.hpp"
#include "smart_array_raid_5_reader.hpp"
#include "smart_array_raid_1_reader.hpp"

std::unique_ptr<DriveReader> reader;

struct ProgramOptions
{
    /// @brief Stripe size in kilobytes
    u_int32_t stripeSize;
    u_int16_t parityDelay;
    u_int16_t raidLevel;
    std::vector<std::string> drives;
    std::string outputDevice;
};

int read(void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
    return reader->read(buf, len, offset);
}

// Argument parsing
static argp_option options[] = {
    { "stripe-size", 's', "256", 0, "Stripe size in KiB. Default: 256", 0 },
    { "parity-delay", 'p', "16", 0, "Parity delay, P420 Controllers use parity delay for their RAID 5, read more: https://www.freeraidrecovery.com/library/delayed-parity.aspx. Default: 16", 0 },
    { "delay", 'p', 0, OPTION_ALIAS, 0, 0 },
    { "raid", 'r', "<level>", 0, "Raid level, required!", 0},
    { "output", 'o', "/dev/nbd0", 0, "Output device, it has to be /dev/nbdx. Default: /dev/nbd0", 0 },
    {0}
};

u_int16_t argToU16(std::string arg, std::string argName)
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

u_int32_t argToU32(std::string arg, std::string argName)
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
    SmartArrayRaid0ReaderOptions readerOpts = {
        .stripeSize = opts.stripeSize
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);
    
    reader = std::make_unique<SmartArrayRaid0Reader>(readerOpts);
}


void initForRaid1(ProgramOptions &opts)
{
    SmartArrayRaid1ReaderOptions readerOpts;
    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);
    reader = std::make_unique<SmartArrayRaid1Reader>(readerOpts);
}

void initForRaid5(ProgramOptions &opts)
{
    SmartArrayRaid5ReaderOptions readerOpts = {
        .stripeSize = opts.stripeSize,
        .parityDelay = opts.parityDelay
    };

    drivesPathVectorToDeviceReaderVector(opts.drives, readerOpts.driveReaders);

    reader = std::make_unique<SmartArrayRaid5Reader>(readerOpts);
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
            std::cerr << "RAID 6 is not implemented yet." << std::endl;
            return -1;
        case 10:
            std::cerr << "RAID 10 is not implemented yet and probably won't be. Just use RAID 0 with 1 drive from each RAID 1 group." << std::endl;
            return -1;
        case 50:
            std::cerr << "RAID 50 is not implemented yet." << std::endl;
            return -1;
        case 60:
            std::cerr << "RAID 60 is not implemented yet." << std::endl;
            return -1;
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
