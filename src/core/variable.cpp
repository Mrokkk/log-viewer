#include "variable.hpp"

#include <ios>
#include <map>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <utility>

#include "core/context.hpp"
#include "core/type.hpp"

namespace core
{

VariablesMap& Variables::map()
{
    static VariablesMap map;
    return map;
}

Variable* Variables::find(const std::string& name)
{
    auto& variables = map();
    const auto variableIt = variables.find(name);

    return variableIt != variables.end()
        ? &variableIt->second
        : nullptr;
}

static void valueToStream(std::ostream& os, const Variable::Value& value)
{
    switch (value.type)
    {
        case Type::boolean:
            os << std::boolalpha << value.boolean << std::noboolalpha;
            break;

        case Type::integer:
            os << value.integer;
            break;

        case Type::string:
            if (not value.ptr)
            {
                os << "<none>";
                return;
            }
            os << *value.string;
            break;

        default:
            break;
    }
}

std::string getValueString(const Variable::Value& value)
{
    std::stringstream ss;
    valueToStream(ss, value);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const VariableWithContext& wrapped)
{
    auto value = wrapped.variable.reader(wrapped.context);

    valueToStream(os, value);

    return os;
}

template <typename T>
void Variables::addUserDefined(const std::string& name, Type type, const T& value)
{
    static std::map<std::string, T> userDefinedVariables;
    auto& variables = core::Variables::map();
    const auto emplaceResult = userDefinedVariables.emplace(std::make_pair(name, value));
    const auto& userVariable = *emplaceResult.first;
    const auto nameCstr = userVariable.first.c_str();
    variables.emplace(std::make_pair(
        name,
        Variable{
            .name = nameCstr,
            .help = "User defined",
            .type = type,
            .access = Variable::Access::readWrite,
            .reader =
                [name = nameCstr](Context&) -> Variable::Value
                {
                    if constexpr (std::is_scalar_v<T>)
                    {
                        return userDefinedVariables[name];
                    }
                    else
                    {
                        return &userDefinedVariables[name];
                    }
                },
            .writer =
                [name = nameCstr](Variable::Value value, Context&) -> bool
                {
                    if constexpr (std::is_same_v<T, bool>)
                    {
                        userDefinedVariables[name] = value.boolean;
                    }
                    else if constexpr (std::is_same_v<T, long>)
                    {
                        userDefinedVariables[name] = value.integer;
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        userDefinedVariables[name] = *value.string;
                    }
                    return true;
                }
        }));
}

template void Variables::addUserDefined(const std::string& name, Type type, const std::string& value);
template void Variables::addUserDefined(const std::string& name, Type type, const bool& value);
template void Variables::addUserDefined(const std::string& name, Type type, const long& value);

}  // namespace core
