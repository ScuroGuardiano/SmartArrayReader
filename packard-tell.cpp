#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "drive_reader.hpp"
#include "metadata_parser.hpp"
#include "types.hpp"

using namespace sg;

// #define DEBUG

struct DriveNumPair
{
    std::string path;
    u8 num;
};

const int DRIVE_METADATA_NEGATIVE_OFFSET = 31 * 1024 * 1024;

void readMetadata(std::string path, u8* out, u32 bytesToLoad)
{
    BlockDeviceReader reader(path);
    reader.read(out, bytesToLoad, reader.driveSize() - DRIVE_METADATA_NEGATIVE_OFFSET);
}

u8 readDriveNumber(std::string path)
{
    BlockDeviceReader reader(path);
    u8 ret;
    reader.read(&ret, 1, reader.driveSize() - DRIVE_METADATA_NEGATIVE_OFFSET);
    return ret;
}

std::string findDrivePathByNum(std::vector<DriveNumPair>& drives, u8 num)
{
    auto res = std::find_if(
        drives.begin(),
        drives.end(),
        [num](DriveNumPair& x) {
            return x.num == num;
        }
    );

    return res == drives.end() ? "" : res->path;
}

std::string findDriveSerialByBayNumber(P420Metadata& metadata, u8 baynum)
{
    auto res = std::find_if(
        metadata.physicalDrives.begin(),
        metadata.physicalDrives.end(),
        [baynum](PhysicalDrive& x) {
            return x.bayNumber == baynum;
        }
    );

    return res == metadata.physicalDrives.end() ? "" : res->serialNumber;
}

std::string sizeToHuman(u64 size)
{
    static const std::string suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB" };
    double s = size;
    int i = 0;

    for (; i < 6 && s >= 1024.0; i++)
    {
        s = s / 1024;
    }

    std::stringstream ss;
    if (s == static_cast<u32>(s))
    {
        ss << static_cast<u32>(s);
    }
    else
    {
        ss << std::setprecision(2) << std::fixed << s;
    }

    ss << suffixes[i];
    return ss.str();
}

std::string formatSize(u64 size)
{
    if (size > 1024)
    {
        return std::to_string(size) + "B (" + sizeToHuman(size) + ")";
    }
    return std::to_string(size) + "B";
}

std::string join(std::vector<std::string> vec, std::string delimiter)
{
    return vec.empty() ? "" : std::accumulate(
        std::next(vec.begin()),
        vec.end(),
        vec[0],
        [delimiter](std::string a, std::string b) {
            return a + delimiter + b;
        }
    );
}

std::string generateCommandToMount(LogicalDrive& ld, std::vector<DriveNumPair>& drives)
{
    std::vector<std::string> arguments;
    arguments.push_back("./hewlett-read");
    u8 missingDrives = 0;

    for (auto& pd : ld.physicalDrives)
    {
        std::string path = findDrivePathByNum(drives, pd);
        if (path.empty())
        {
            missingDrives++;
            path = "X";
        }
        arguments.push_back(path);
    }

    switch (ld.raidLevel)
    {
        case 0:
        case 1:
        case 10:
        case 5:
        case 6:
            if (ld.physicalDriveCount - missingDrives < ld.physicalDrivesNeededToRun)
            {
                return "Error: Too many missing drives";
            }
            break;
        
        case 50:
        case 60:
            arguments.push_back("--parity-groups=" + std::to_string(ld.parityGroups));
            // I won't calculate here missing drives because I would have to
            // tell it for every parity group. hewlett-read will fail anyway
            // if certain drives will be missing.
            break;

        default:
            return "Error: Uknown RAID level";
    }

    arguments.push_back("--raid=" + std::to_string(ld.raidLevel));
    
    if (ld.raidLevel != 1)
    {
        arguments.push_back("--stripe-size=" + std::to_string(ld.stripeSizeInBytes / 1024));
    }
    arguments.push_back("--size=" + std::to_string(ld.logicalDriveSizeInBytes));
    
    if (ld.offsetOnEachPhysicalDriveInBytes > 0)
    {
        arguments.push_back("--offset=" + std::to_string(ld.offsetOnEachPhysicalDriveInBytes));
    }

    return join(arguments, " ");
}

void printLogicalDrive(LogicalDrive& ld, std::vector<DriveNumPair>& drives, P420Metadata& metadata, std::string indent)
{
    std::string raidLevel = ld.raidLevel != 255 ? std::to_string(ld.raidLevel) : "UNKNOWN";

    std::cout << "Label: " << ld.label << ":" << std::endl;
    std::cout << indent << "Raid level:                    " << raidLevel << std::endl;
    if (ld.raidLevel == 50 || ld.raidLevel == 60)
    {
        std::cout << indent << "Parity groups:                 " << ld.parityGroups << std::endl;
    }

    std::cout << indent << "Size:                          " << formatSize(ld.logicalDriveSizeInBytes) << std::endl;
    std::cout << indent << "Stripe size:                   " << formatSize(ld.stripeSizeInBytes) << std::endl;
    std::cout << indent << "Offset on physical drive:      " << formatSize(ld.offsetOnEachPhysicalDriveInBytes) << std::endl;
    std::cout << indent << "Space taken on physical drive: " << formatSize(ld.spaceTakenOnEachPhysicalDriveInBytes) << std::endl;
    std::cout << indent << "Physical drives:" << std::endl;
    
    u8 drivesFound = 0;
    for (auto pd : ld.physicalDrives)
    {
        std::string path = findDrivePathByNum(drives, pd);
        path = path.empty() ? "MISSING" : path;

        // From my observation drive number in logical drive is bayNumber + 7
        // But I am not sure, so serial number is experimental feature
        std::string serial = findDriveSerialByBayNumber(metadata, pd - 7);
        std::cout << indent << indent << std::setw(3) << pd << ": " << path << "\tSerial?: " << serial << std::endl;
    }
    std::cout << indent << "Command: " << generateCommandToMount(ld, drives) << std::endl;
}

std::string trimRight(std::string s)
{
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

int main(int argc, char** argv)
{
    const std::string INDENT = "    ";

    if (argc < 2)
    {
        std::cout << "Scuro Guardiano's Smart Array Order Teller v1.3.3.7" << std::endl;
        std::cout << "Have you forgotten drives order in your raid array? Don't worry!" << std::endl;
        std::cout << "Just give me the drives and I will tell everything I know about logical drives on them!" << std::endl;
        std::cout << "Provide at least 1 drive, logical, right?" << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\tpackard-tell drive1 drive2 ...driveN" << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "\tpackard-tell /dev/sda /dev/sdb /dev/sdc /dev/sdd" << std::endl << std::endl;
        return 0;
    }

    std::vector<DriveNumPair> drives;

    for (int i = 1; i < argc; i++)
    {
        std::string drivePath = argv[i];
        drives.push_back({
            .path = drivePath,
            .num = readDriveNumber(drivePath)
        });
    }

    std::sort(
        drives.begin(),
        drives.end(),
        [](auto& p1, auto& p2) {
            return p1.num < p2.num;
        }
    );

    std::cout << "Correct drives order: ";
    
    for (auto& drive : drives)
    {
        std::cout << drive.path << " ";
    }

    std::cout << std::endl << std::endl;

    P420Metadata metadata;
    u8 metadataBin[0x28000];
    readMetadata(drives[0].path, metadataBin, 0x28000);
    parseMetadata(metadataBin, &metadata);

    std::cout << "======== PARSED CONTROLLER METADATA ========" << std::endl;
    std::cout << "Controller Serial Number: " << metadata.controllerSerialNumber << std::endl;
    std::cout << "Logical drives count:     " << metadata.logicalDrives.size() << std::endl;
    std::cout << "Physical drives count:    " << metadata.physicalDrives.size() << std::endl << std::endl;

    std::cout << "Physical drives:" << std::endl;

    for (auto& pd : metadata.physicalDrives)
    {
        // From my observation drive number in logical drive is bayNumber + 7
        // But I am not sure, so path is experimental feature.
        // I am also not sure about Box number.
        std::string path = findDrivePathByNum(drives, pd.bayNumber + 7);

        std::cout << INDENT
            << "Box?: " << std::setw(3) << pd.boxNumber
            << INDENT << "Bay: " << std::setw(3) << pd.bayNumber
            << INDENT << "Serial: " << trimRight(pd.serialNumber)
            << INDENT << "Path?: " << (path.empty() ? "MISSING" : path)
            << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Logical drives:" << std::endl;
    for (auto& ld : metadata.logicalDrives)
    {
        printLogicalDrive(ld, drives, metadata, INDENT);
        std::cout << std::endl;
    }
}
