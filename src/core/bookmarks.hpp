#pragma once

#include <cstddef>
#include <list>
#include <string>

#include "utils/hash_map.hpp"

namespace core
{

struct Bookmarks
{
    struct Bookmark
    {
        size_t      lineNumber;
        std::string name;
    };

    void add(size_t lineNumber, std::string name);
    Bookmark* find(size_t lineNumber) const;

    auto size() const { return mData.size(); }

    auto begin() const { return mData.begin(); }
    auto end() const { return mData.end(); }

private:
    std::list<Bookmark>               mData;
    utils::HashMap<size_t, Bookmark*> mMap;
};

}  // namespace core
