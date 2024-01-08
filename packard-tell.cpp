#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "drive_reader.hpp"

// #define DEBUG

struct DriveNumPair
{
    std::string path;
    int num;
};

const int DRIVE_NUMBER_NEGATIVE_OFFSET = 31 * 1024 * 1024;

int readDriveNumber(std::string path)
{
    BlockDeviceReader reader(path);
    char ret;
    reader.read(&ret, 1, reader.driveSize() - DRIVE_NUMBER_NEGATIVE_OFFSET);
    return ret;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Scuro Guardiano's Smart Array Order Teller v1.3.3.7" << std::endl;
        std::cout << "Have you forgotten drives order in your raid array? Don't worry!" << std::endl;
        std::cout << "Just give me the drives and I will tell you their order in the array!" << std::endl;
        std::cout << "Provide at least 2 drives, logical, right?" << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\tpackard-tell /drive1 /drive2 .../driveN" << std::endl;
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

    #ifdef DEBUG
    for (auto& drive : drives)
    {
        std::cout << drive.path << " : " << drive.num << std::endl;
    }
    #endif

    std::cout << "The correct order is:" << std::endl;
    
    for (auto& drive : drives)
    {
        std::cout << drive.path << " ";
    }
    std::cout << std::endl;
}
