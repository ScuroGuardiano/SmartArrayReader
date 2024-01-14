#pragma once

#include "drive_reader.hpp"

class SmartArrayReaderBase : public DriveReader
{
protected:
    // Smallest drive in the array
    u_int64_t singleDriveSize;

    /// @brief Sets size of logical drive, throws std::invalid_argument
    /// if provided size is bigger than maximum.
    /// @param size size in bytes
    /// @param maximumSize maximum size in bytes, if set to 0 then it's ignored
    void setSize(u_int64_t size, u_int64_t maximumSize);
    u_int64_t driveSize() override;


    /// @brief Sets physical drive offset, use after setting "singleDriveSize".
    /// If offset is bigger than single drive sie then std::invalid_argument is thrown
    /// @param offset 
    void setPhysicalDriveOffset(u_int64_t offset);
    u_int64_t getPhysicalDriveOffset();

private:
    u_int64_t size;
    u_int64_t physicalDriveOffset;    
};
