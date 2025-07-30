#include "alias.hpp"

namespace core
{

AliasesMap& Aliases::map()
{
    static AliasesMap map;
    return map;
}

}  // namespace core
