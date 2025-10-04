#include "bookmarks.hpp"

namespace core
{

void Bookmarks::add(size_t lineNumber, std::string name, std::string line)
{
    auto it = mData.begin();
    for (; it != mData.end(); ++it)
    {
        if (it->lineNumber > lineNumber)
        {
            break;
        }
    }
    auto bookmark = mData.emplace(it, Bookmark{lineNumber, std::move(name), std::move(line)});
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

void Bookmarks::remove()
{
    int i = 0;
    for (auto it = mData.begin(); it != mData.end(); ++it, ++i)
    {
        if (i == currentIndex)
        {
            mMap.erase(it->lineNumber);
            mData.erase(it);
            return;
        }
    }
}

const Bookmarks::Bookmark& Bookmarks::operator[](size_t index) const
{
    size_t i = 0;
    for (const auto& bookmark : mData)
    {
        if (i == index)
        {
            return bookmark;
        }
        ++i;
    }
    std::abort();
}

}  // namespace core
