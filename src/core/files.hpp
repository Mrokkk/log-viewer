#pragma once

#include <list>
#include <string>

#include "core/file.hpp"

namespace core
{

struct Files
{
    File& emplaceBack(std::string path);

private:
    std::list<File> files_;
};

}  // namespace core
