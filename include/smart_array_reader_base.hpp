#pragma once

#include "drive_reader.hpp"
#include "types.hpp"

namespace sg
{

class SmartArrayReaderBase : public DriveReader
{
public:
    virtual u64 driveSize() override;

protected:
    // Smallest drive in the array
    u64 singleDriveSize;

    /// @brief Sets size of logical drive, throws std::invalid_argument
    /// if provided size is bigger than maximum.
    /// @param size size in bytes
    /// @param maximumSize maximum size in bytes, if set to 0 then it's ignored
    void setSize(u64 size, u64 maximumSize);

    /// @brief Sets physical drive offset, use after setting "singleDriveSize".
    /// If offset is bigger than single drive sie then std::invalid_argument is thrown
    /// @param offset 
    void setPhysicalDriveOffset(u64 offset);
    u64 getPhysicalDriveOffset();

private:
    u64 size;
    u64 physicalDriveOffset;    
};

} // end namespace sg
