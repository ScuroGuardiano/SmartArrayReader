#include "smart_array_reader_base.hpp"

void SmartArrayReaderBase::setSize(u_int64_t size, u_int64_t maximumSize)
{
    if (size > maximumSize && maximumSize != 0)
    {
        throw std::invalid_argument(
            "Provided logical drive size " + std::to_string(size) +
            " is bigger than maximum possible size of: " + std::to_string(maximumSize));
    }
    this->size = size;
}

u_int64_t SmartArrayReaderBase::driveSize()
{
    return this->size;
}

void SmartArrayReaderBase::setPhysicalDriveOffset(u_int64_t offset)
{
    if (offset >= this->singleDriveSize)
    {
        throw std::invalid_argument(
            "Offset " + std::to_string(offset) +
            " is bigger or equal to single drive size of: " + std::to_string(this->size)
        );
    }
    this->physicalDriveOffset = offset;
}

u_int64_t SmartArrayReaderBase::getPhysicalDriveOffset()
{
    return this->physicalDriveOffset;
}
