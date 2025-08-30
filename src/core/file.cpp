#include "file.hpp"

#include <expected>
#include <sstream>

#include "core/assert.hpp"
#include "sys/system.hpp"

namespace core
{

File::File()
    : mFile(std::unexpected(0))
    , mMapping{.ptr = nullptr, .offset = 0, .len = 0}
    , mRefCount(nullptr)
{
}

File::~File()
{
    free();
}

File& File::operator=(const File& other)
{
    free();
    mFile = other.mFile;
    assert(other.mRefCount);
    mRefCount = other.mRefCount;
    ++*mRefCount;
    return *this;
}

static std::string getErrorMessage(const std::string& path, sys::Error error)
{
    std::stringstream ss;
    ss << path << ": cannot open: " << sys::errorDescribe(error);
    return ss.str();
}

static std::string getErrorMessage(const sys::File& file, size_t blockSize, size_t offset, sys::Error error)
{
    std::stringstream ss;
    ss << file.path << ": cannot map block of size " << blockSize << " at offset " << offset << ": " << sys::errorDescribe(error);
    return ss.str();
}

std::expected<bool, std::string> File::open(std::string path)
{
    mFile = sys::fileOpen(path);

    if (not mFile) [[unlikely]]
    {
        return std::unexpected(getErrorMessage(path, mFile.error()));
    }

    mRefCount = new int(1);

    return true;
}

std::expected<bool, std::string> File::remap(size_t offset, size_t len)
{
    if (const auto error = sys::remap(mFile.value(), mMapping, offset, len)) [[unlikely]]
    {
        auto errorMessage = getErrorMessage(mFile.value(), len, offset, error);
        return std::unexpected(std::move(errorMessage));
    }

    return true;
}

const std::string& File::path() const
{
    return mFile->path;
}

size_t File::size() const
{
    return mFile->size;
}

void File::free()
{
    sys::unmap(mMapping);
    if (mFile and --*mRefCount == 0)
    {
        sys::fileClose(mFile.value());
        delete mRefCount;
    }
}

}  // namespace core
