#pragma once

#include <expected>
#include <string>

#include "sys/file.hpp"
#include "sys/mapping.hpp"

namespace core
{

struct File final
{
    File();
    ~File();

    File(const File& other);
    File& operator=(const File& other);

    std::expected<bool, std::string> open(std::string path);
    std::expected<bool, std::string> remap(size_t offset, size_t len);
    const std::string& path() const;
    size_t size() const;

    constexpr bool isAreaMapped(size_t start, size_t len) const
    {
        return start >= mMapping.offset and len + start < mMapping.len + mMapping.offset;
    }

    constexpr const char* at(size_t offset)
    {
        return mMapping.ptrAt<const char*>(offset);
    }

private:
    void free();

    sys::MaybeFile mFile;
    sys::Mapping   mMapping;
    int*           mRefCount;
};

}  // namespace core
