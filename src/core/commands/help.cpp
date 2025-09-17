#include "core/interpreter/command.hpp"
#include "core/interpreter/symbol.hpp"
#include "core/message_line.hpp"

namespace core
{

DEFINE_COMMAND(help)
{
    HELP() = "print help about command/symbol";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            {Type::string, "name"}
        };
    };

    EXECUTOR()
    {
        auto name = *args[0].string();

        const auto command = interpreter::Commands::find(name);

        if (command)
        {
            context.messageLine.info() << name << ": " << command->help;
            return true;
        }

        const auto symbol = interpreter::Symbols::find(name);

        if (not symbol)
        {
            context.messageLine.error() << "No help entry for: " << name;
            return false;
        }

        context.messageLine.info() << name << ": " << symbol->help();
        return true;
    }
}

}  // namespace core
