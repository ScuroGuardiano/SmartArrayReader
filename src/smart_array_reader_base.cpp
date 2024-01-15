#include "smart_array_reader_base.hpp"

namespace sg
{

void SmartArrayReaderBase::setSize(u64 size, u64 maximumSize)
{
    if (size > maximumSize && maximumSize != 0)
    {
        throw std::invalid_argument(
            "Provided logical drive size " + std::to_string(size) +
            " is bigger than maximum possible size of: " + std::to_string(maximumSize));
    }
    this->size = size;
}

u64 SmartArrayReaderBase::driveSize()
{
    return this->size;
}

void SmartArrayReaderBase::setPhysicalDriveOffset(u64 offset)
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

u64 SmartArrayReaderBase::getPhysicalDriveOffset()
{
    return this->physicalDriveOffset;
}

} // end namespace sg