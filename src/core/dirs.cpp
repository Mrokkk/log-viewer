#include "dirs.hpp"

#include <filesystem>

namespace core
{

utils::Strings readCurrentDirectory()
{
    using namespace std::filesystem;
    utils::Strings result;
    const auto& current = current_path();
    for (const auto& entry : directory_iterator(current))
    {
        if (entry.is_regular_file())
        {
            result.push_back(relative(entry.path(), current));
        }
    }
    return result;
}

utils::Strings readCurrentDirectoryRecursive()
{
    using namespace std::filesystem;
    utils::Strings result;
    const auto& current = current_path();
    for (const auto& entry : recursive_directory_iterator(current))
    {
        if (entry.is_regular_file())
        {
            result.push_back(relative(entry.path(), current));
        }
    }
    return result;
}

}  // namespace core
