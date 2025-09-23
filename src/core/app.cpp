#define LOG_HEADER "core::App"
#include "app.hpp"

#include <cstdio>

#include "core/argparse.hpp"
#include "core/commands/open.hpp"
#include "core/commands/source.hpp"
#include "core/context.hpp"
#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/main_loop.hpp"
#include "sys/system.hpp"
#include "utils/time.hpp"

namespace core
{

static const CommandLineOption file(
    {
        .type = Type::string,
        .variant = Option::positional,
        .longName = "file",
        .help = "file to open",
    });

static const CommandLineOption logFile(
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

    if (auto result = parseArgs(argc, argv); not result) [[unlikely]]
    {
        fprintf(stderr, "%s\n", result.error().c_str());
        return -1;
    }

    if (logFile)
    {
        logger.setLogFile(logFile->string);
    }

    initializeInput(context);

    for (const auto& configFile : sys::getConfigFiles())
    {
        auto t = utils::startTimeMeasurement();
        commands::source(configFile, context);
        logger.info() << "sourced " << configFile << "; took: " << 1000 * t.elapsed() << " ms";
    }

    if (file)
    {
        commands::open(std::string(file->string), context);
    }

    context.mainLoop->run(context);

    sys::finalize();
    logger.flushToStderr();

    return 0;
}

}  // namespace core
