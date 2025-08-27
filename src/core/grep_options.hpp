#pragma once

namespace core
{

struct GrepOptions
{
    bool regex           = false;
    bool inverted        = false;
    bool caseInsensitive = false;
};

}  // namespace core
