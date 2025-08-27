#pragma once

#include <expected>
#include <string>

#include "sys/file.hpp"
#include "sys/mapping.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct File final : utils::Immobile
{
    File();
    ~File();

    std::expected<bool, std::string> open(std::string path);
    std::expected<bool, std::string> remap(size_t offset, size_t len);
    void clone(const File& other);
    const std::string& path() const;
    size_t size() const;

    bool isAreaMapped(size_t start, size_t len) const
    {
        return start >= mapping_.offset and len + start < mapping_.len + mapping_.offset;
    }

    const char* at(size_t offset)
    {
        return mapping_.ptrAt<const char*>(offset);
    }

private:
    sys::MaybeFile     file_;
    sys::Mapping       mapping_;
    int*               refCount_;
};

}  // namespace core
