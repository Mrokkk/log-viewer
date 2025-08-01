#include "system.hpp"

#include <algorithm>
#include <backtrace.h>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <ios>
#include <iostream>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/logger.hpp"
#include "core/mapping.hpp"
#include "utils/side_effect.hpp"
#include "utils/string.hpp"

#define COLOR_BLUE    "\e[34m"
#define COLOR_YELLOW  "\e[33m"
#define COLOR_GREEN   "\e[32m"
#define COLOR_RESET   "\e[0m"

namespace sys
{

static backtrace_state* backtraceState;

struct ProcessMapping
{
    std::string name;
    uintptr_t   start;
    uintptr_t   end;
};

using ProcessMappings = std::vector<ProcessMapping>;

struct BacktraceContext
{
    unsigned int    index;
    ProcessMappings maps;
};

template <typename T>
std::string toHex(T value)
{
    std::ostringstream ss;
    ss << std::hex << std::showbase << value;
    return ss.str();
}

static std::string mappingNameFind(const ProcessMappings& mappings, uintptr_t address)
{
    auto mappingIt = std::ranges::find_if(
        mappings,
        [address](const auto& mapping)
        {
            return address >= mapping.start and address < mapping.end;
        });

    return mappingIt != mappings.end()
        ? mappingIt->name + "+" + toHex(address - mappingIt->start)
        : "??";
}

using BacktraceCreateStateFn = decltype(&backtrace_create_state);
using BacktraceFullFn = decltype(&backtrace_full);

BacktraceCreateStateFn backtraceCreateState;
BacktraceFullFn backtraceFull;

static int backtraceCallback(void* data, uintptr_t pc, const char* pathname, int lineNumber, const char* function)
{
    auto ctx = static_cast<BacktraceContext*>(data);

    if (pathname != NULL || function != NULL)
    {
        int status;
        std::string mappingName;

        if (function)
        {
            auto demangled = abi::__cxa_demangle(function, NULL, NULL, &status);
            if (!status)
            {
                function = demangled;
            }
        }
        else
        {
            auto mappingName = mappingNameFind(ctx->maps, pc);
            function = mappingName.c_str();
        }

        logger << info << '#' << ctx->index << " " COLOR_BLUE << (void*)pc << COLOR_RESET " in " COLOR_YELLOW << function
                       << COLOR_RESET " at " COLOR_GREEN << pathname << COLOR_RESET ":" << lineNumber;
    }
    else
    {
        auto mappingName = mappingNameFind(ctx->maps, pc);
        logger << info << '#' << ctx->index << " " COLOR_BLUE << (void*)pc << COLOR_RESET " in " COLOR_YELLOW << mappingName << COLOR_RESET;
    }

    ctx->index++;

    return 0;
}

static void backtraceErrorCallback(void*, const char* message, int error)
{
    if (error == -1)
    {
        printf("Debug info missing\n");
        return;
    }

    printf("Backtrace error %d: %s\n", error, message);
}

template <typename T>
static T hexTo(const std::string& string)
{
    T value;
    std::stringstream ss;
    ss << std::hex << string;
    ss >> value;
    return value;
}

void logLineOfWords(const utils::Strings& words)
{
    std::stringstream ss;

    for (const auto& w : words)
    {
        ss << w << ' ';
    }

    logger << info << ss.rdbuf();
}

static ProcessMappings mappingRead()
{
    // Example of 2 mappings from /proc/self/maps
    // 58868b471000-58868b473000 r--p 00000000 fe:00 12861860                   /usr/bin/cat
    // 702259d69000-702259dae000 rw-p 00000000 00:00 0
    ProcessMappings mappings;

    for (const auto& line : "/proc/self/maps" | utils::readText | utils::splitBy("\n"))
    {
        const auto words = line | utils::splitBy(" ") | utils::sideEffect(logLineOfWords);

        if (words.size() == 0)
        {
            continue;
        }

        const auto addresses = words[0] | utils::splitBy("-");

        mappings.emplace_back(
            ProcessMapping{
                .name = words.size() > 5 ? words[5] : "[anon]",
                .start = hexTo<uintptr_t>(addresses[0]),
                .end = hexTo<uintptr_t>(addresses[1])
            });
    }

    return mappings;
}

void stacktraceLog(void)
{
    if (not backtraceFull)
    {
        logger << warning << "libbacktrace unavailable; cannot collect stacktrace";
        return;
    }

    auto context = BacktraceContext{
        .index = 0,
        .maps = mappingRead()
    };

    logger << info << "Stacktrace:";

    backtraceFull(backtraceState, 1, backtraceCallback, backtraceErrorCallback, static_cast<void*>(&context));
}

void initialize()
{
    auto libbacktrace = dlopen("libbacktrace.so", RTLD_LAZY);

    if (not libbacktrace)
    {
        std::cerr << "cannot load libbacktrace.so\n";
        return;
    }

    backtraceCreateState = reinterpret_cast<BacktraceCreateStateFn>(dlsym(libbacktrace, "backtrace_create_state"));
    backtraceFull = reinterpret_cast<BacktraceFullFn>(dlsym(libbacktrace, "backtrace_full"));

    if (not backtraceFull or not backtraceCreateState)
    {
        std::cerr << "cannot find backtrace_full or backtrace_create_state\n";
        backtraceFull = nullptr;
        backtraceCreateState = nullptr;
        return;
    }

    backtraceState = backtraceCreateState(NULL, 0, backtraceErrorCallback, NULL);
}

void finalize()
{
    // FIXME: https://github.com/ArthurSonzogni/FTXUI/issues/904
    printf("\033[?12l\033[?25h");
    fflush(stdout);
}

void crashHandle(const int signal)
{
    logger << error << "Received SIG" << sigabbrev_np(signal);
    sys::stacktraceLog();
    sys::finalize();
    logger.flushToStderr();
}

core::File fileOpen(std::string path)
{
    int fd = open(path.c_str(), O_RDONLY);

    if (fd == -1) [[unlikely]]
    {
        return {};
    }

    struct stat stat;

    if (fstat(fd, &stat) == -1) [[unlikely]]
    {
        return {};
    }

    return core::File{
        .path = std::move(path),
        .size = static_cast<size_t>(stat.st_size),
        .fd = fd,
    };
}

void fileClose(core::File& file)
{
    close(file.fd);
}

int remap(const core::File& file, core::Mapping& mapping, size_t newOffset, size_t newLen)
{
    static unsigned int pageMask = ~(static_cast<uintptr_t>(getpagesize()) - 1);

    if (mapping)
    {
        munmap(mapping.ptr, mapping.len);
    }

    size_t pageStart = newOffset & pageMask;
    newLen += newOffset - pageStart;

    const auto newPtr = mmap(nullptr, newLen, PROT_READ, MAP_PRIVATE, file.fd, pageStart);

    if (newPtr == MAP_FAILED) [[unlikely]]
    {
        return -1;
    }

    mapping = core::Mapping{
        .ptr = newPtr,
        .offset = pageStart,
        .len = newLen
    };

    return 0;
}

void unmap(core::Mapping& mapping)
{
    if (mapping)
    {
        munmap(mapping.ptr, mapping.len);
        mapping.ptr = nullptr;
        mapping.len = 0;
    }
}

Paths getConfigFiles()
{
    const auto home = std::getenv("HOME");

    if (not home)
    {
        return {};
    }

    return {std::filesystem::path(home) / ".config/log-viewer/config"};
}

}  // namespace sys
