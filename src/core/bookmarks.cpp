#include "bookmarks.hpp"

namespace core
{

void Bookmarks::add(size_t lineNumber, std::string name)
{
    auto it = mData.begin();
    for (; it != mData.end(); ++it)
    {
        if (it->lineNumber > lineNumber)
        {
            break;
        }
    }
    auto bookmark = mData.emplace(it, Bookmark{lineNumber, std::move(name)});
    mMap.insert(lineNumber, &*bookmark);
}

Bookmarks::Bookmark* Bookmarks::find(size_t lineNumber) const
{
    auto node = mMap.find(lineNumber);
    if (not node)
    {
        return nullptr;
    }
    return node->second;
}

}  // namespace core
