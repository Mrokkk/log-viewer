#include "variable.hpp"

#include <map>
#include <type_traits>
#include <utility>

#include "core/context.hpp"
#include "core/type.hpp"
#include "utils/buffer.hpp"

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

static void valueToStream(utils::Buffer& buf, const Variable::Value& value)
{
    switch (value.type)
    {
        case Type::null:
            buf << "<none>";
            break;

        case Type::boolean:
            buf << value.boolean;
            break;

        case Type::integer:
            buf << value.integer;
            break;

        case Type::string:
            if (not value.ptr)
            {
                buf << "<none>";
                return;
            }
            buf << *value.string;
            break;

        default:
            break;
    }
}

std::string getValueString(const Variable::Value& value)
{
    utils::Buffer buf;
    valueToStream(buf, value);
    return buf.str();
}

utils::Buffer& operator<<(utils::Buffer& buf, const VariableWithContext& wrapped)
{
    auto value = wrapped.variable.reader(wrapped.context);

    valueToStream(buf, value);

    return buf;
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
