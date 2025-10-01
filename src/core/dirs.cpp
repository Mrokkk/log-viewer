#include "dirs.hpp"

#include <exception>
#include <filesystem>

#include "core/logger.hpp"

namespace core
{

utils::Strings readCurrentDirectory()
{
    using namespace std::filesystem;
    utils::Strings result;
    const auto& current = current_path();
    try
    {
        for (const auto& entry : directory_iterator(current))
        {
            if (entry.is_regular_file())
            {
                result.push_back(relative(entry.path(), current));
            }
        }
    }
    catch (const std::exception& e)
    {
        logger.error() << e.what();
    }
    return result;
}

utils::Strings readCurrentDirectoryRecursive()
{
    using namespace std::filesystem;
    utils::Strings result;
    const auto& current = current_path();
    try
    {
        for (const auto& entry : recursive_directory_iterator(current))
        {
            if (entry.is_regular_file())
            {
                result.push_back(relative(entry.path(), current));
            }
        }
    }
    catch (const std::exception& e)
    {
        logger.error() << e.what();
    }
    return result;
}

}  // namespace core
