#include "input_mapping.hpp"

#include <expected>
#include <flat_map>
#include <string_view>

#include "core/context.hpp"

namespace core
{

InputMappingMap& InputMappings::map()
{
    static InputMappingMap map;
    return map;
}

InputMappingSet& InputMappings::set()
{
    static InputMappingSet set;
    return set;
}

#define MAPPING(KEYS, COMMAND) \
    {KEYS, InputMapping{.keys = KEYS, .command = COMMAND}}

void initializeDefaultInputMapping(Context&)
{
    auto& inputMappings = InputMappings::map();
    auto& inputMappingSet = InputMappings::set();

    inputMappings = {
        MAPPING("gg", "0"),
        MAPPING("G", "-1"),
        MAPPING("b", "toggle showLineNumbers"),
    };

    for (const auto& [keys, _] : inputMappings)
    {
        for (size_t i = 0; i <= keys.size(); ++i)
        {
            inputMappingSet.insert(keys.substr(0, i));
        }
    }
}

static std::expected<std::string, std::string> convertKeys(std::string_view input)
{
    static std::flat_map<std::string, char> mapping = {
        {"space", ' '},
        {"leader", ','},
    };

    std::string keys;

    while (input.begin() != input.end())
    {
        if (input[0] == '<')
        {
            auto end = input.find('>');
            if (end == input.npos)
            {
                return std::unexpected("Missing closing > in key");
            }
            auto key = input.substr(1, end - 1);
            input.remove_prefix(end + 1);

            if (key[1] == '-')
            {
                switch (key[0])
                {
                    case 'C':
                        keys += key[2] - 0x40;
                        break;
                    case 'c':
                        keys += key[2] - 0x60;
                        break;
                    default:
                        return std::unexpected("Unknown key");
                }
            }
            else if (auto it = mapping.find(std::string(key)); it != mapping.end())
            {
                keys += it->second;
            }
            else
            {
                return std::unexpected("Unknown key");
            }
        }
        else
        {
            keys += input[0];
            input.remove_prefix(1);
        }
    }

    return keys;
}

bool addInputMapping(std::string keys, const std::string& command, bool force, Context& context)
{
    const auto converted = convertKeys(keys);

    if (not converted)
    {
        *context.ui << error << converted.error();
        return false;
    }

    const auto& keySequence = converted.value();
    auto& inputMappings = InputMappings::map();
    auto& inputMappingSet = InputMappings::set();

    if (force)
    {
        inputMappings.erase(keySequence);
    }

    auto result = inputMappings.emplace(
        std::make_pair(
            keySequence,
            InputMapping{
                .keys = keySequence,
                .command = command
            }));

    if (not result.second)
    {
        *context.ui << error << "Mapping already exists (use ! to overwrite)";
        return false;
    }

    for (size_t i = 0; i <= keySequence.size(); ++i)
    {
        inputMappingSet.insert(keySequence.substr(0, i));
    }

    return true;
}

}  // namespace core
