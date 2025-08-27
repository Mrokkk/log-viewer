#include "file.hpp"

#include <expected>
#include <sstream>

#include "core/assert.hpp"
#include "sys/system.hpp"

namespace core
{

File::File()
    : file_(std::unexpected(0))
    , mapping_{.ptr = nullptr, .offset = 0, .len = 0}
    , refCount_(nullptr)
{
}

File::~File()
{
    sys::unmap(mapping_);
    if (file_ and --(*refCount_) == 0)
    {
        sys::fileClose(file_.value());
    }
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
    file_ = sys::fileOpen(path);

    if (not file_) [[unlikely]]
    {
        return std::unexpected(getErrorMessage(path, file_.error()));
    }

    refCount_ = new int(1);

    return true;
}

std::expected<bool, std::string> File::remap(size_t offset, size_t len)
{
    if (const auto error = sys::remap(file_.value(), mapping_, offset, len)) [[unlikely]]
    {
        auto errorMessage = getErrorMessage(file_.value(), len, offset, error);
        return std::unexpected(std::move(errorMessage));
    }

    return true;
}

void File::clone(const File& other)
{
    file_ = other.file_;
    sys::unmap(mapping_);
    assert(other.refCount_);
    refCount_ = other.refCount_;
    ++*refCount_;
}

const std::string& File::path() const
{
    return file_->path;
}

size_t File::size() const
{
    return file_->size;
}

}  // namespace core
