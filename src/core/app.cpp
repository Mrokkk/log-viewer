#include "app.hpp"

#include <iostream>

#include "core/argparse.hpp"
#include "core/commands/open.hpp"
#include "core/commands/source.hpp"
#include "core/context.hpp"
#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/severity.hpp"
#include "core/user_interface.hpp"
#include "sys/system.hpp"

namespace core
{

static CommandLineOption file(
    {
        .type = Type::string,
        .variant = Option::positional,
        .longName = "file",
        .help = "file to open",
    });

static CommandLineOption logFile(
    {
        .type = Type::string,
        .variant = Option::additional,
        .longName = "log-file",
        .shortName = 'l',
        .help = "file to which logs are saved",
    });

int run(int argc, char* const* argv, Context& context)
{
    sys::initialize();

    auto result = parseArgs(argc, argv);

    if (not result)
    {
        std::cerr << result.error() << '\n';
        return -1;
    }

    if (logFile)
    {
        logger.setLogFile(logFile->string);
    }

    initializeInput(context);

    for (const auto& configFile : sys::getConfigFiles())
    {
        if (std::filesystem::exists(configFile))
        {
            logger << info << "sourcing " << configFile;
            commands::source(configFile.string(), context);
            break;
        }
    }

    if (file)
    {
        commands::open(std::string(file->string), context);
    }

    context.ui->run(context);

    sys::finalize();
    logger.flushToStderr();

    return 0;
}

}  // namespace core
