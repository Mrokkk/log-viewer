#ifdef __unix__
#define LOG_FLAGS LogEntryFlags::noSourceLocation
#include "system.hpp"

#include <algorithm>
#include <backtrace.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <dlfcn.h>
#include <expected>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/log_entry.hpp"
#include "core/logger.hpp"
#include "sys/mapping.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"

#define COLOR_BLUE    "\e[34m"
#define COLOR_YELLOW  "\e[33m"
#define COLOR_GREEN   "\e[32m"
#define COLOR_RED     "\e[31m"
#define COLOR_RESET   "\e[m"

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

static std::string mappingNameFind(const ProcessMappings& mappings, uintptr_t address)
{
    auto mappingIt = std::ranges::find_if(
        mappings,
        [address](const auto& mapping)
        {
            return address >= mapping.start and address < mapping.end;
        });

    utils::Buffer buf;

    if (mappingIt != mappings.end())
    {
        buf << mappingIt->name << '+' << ((address - mappingIt->start) | utils::hex(utils::showbase));
    }
    else
    {
        buf << "??";
    }

    return buf.str();
}

using BacktraceCreateStateFn = decltype(&backtrace_create_state);
using BacktraceFullFn = decltype(&backtrace_full);

BacktraceCreateStateFn backtraceCreateState;
BacktraceFullFn backtraceFull;

static int backtraceCallback(void* data, uintptr_t pc, const char* pathname, int lineNumber, const char* function)
{
    auto ctx = static_cast<BacktraceContext*>(data);

    if (pathname != NULL or function != NULL)
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

        logger.info() << '#' << ctx->index << " " COLOR_BLUE << (void*)pc << COLOR_RESET " in " COLOR_YELLOW << function
                      << COLOR_RESET " at " COLOR_GREEN << pathname << COLOR_RESET ":" << lineNumber;
    }
    else
    {
        auto mappingName = mappingNameFind(ctx->maps, pc);
        logger.info() << '#' << ctx->index << " " COLOR_BLUE << (void*)pc << COLOR_RESET " in " COLOR_YELLOW << mappingName << COLOR_RESET;
    }

    ctx->index++;

    return 0;
}

static void backtraceErrorCallback(void*, const char* message, int error)
{
    logger.error() << "backtrace error[" <<  error << "]: " << message;
}

template <typename T>
static T hexTo(const std::string& string)
{
    return strtol(string.c_str(), nullptr, 16);
}

static void logLineOfWords(const utils::Strings& words)
{
    utils::Buffer buf;

    for (const auto& w : words)
    {
        buf << w << ' ';
    }

    logger.info().buffer() = std::move(buf);
}

static ProcessMappings mappingRead(bool logMappings)
{
    // Example of 2 mappings from /proc/self/maps
    // 58868b471000-58868b473000 r--p 00000000 fe:00 12861860                   /usr/bin/cat
    // 702259d69000-702259dae000 rw-p 00000000 00:00 0
    ProcessMappings mappings;

    for (const auto& line : "/proc/self/maps" | utils::readText | utils::splitBy("\n"))
    {
        const auto words = line | utils::splitBy(" ");

        if (logMappings)
        {
            logLineOfWords(words);
        }

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

static void stacktraceLogInternal(bool logMappings)
{
    if (not backtraceFull)
    {
        logger.warning() << "libbacktrace unavailable; cannot collect stacktrace";
        return;
    }

    auto context = BacktraceContext{
        .index = 0,
        .maps = mappingRead(logMappings)
    };

    logger.info() << "Stacktrace:";

    backtraceFull(backtraceState, 1, backtraceCallback, backtraceErrorCallback, static_cast<void*>(&context));
}

void stacktraceLog(void)
{
    stacktraceLogInternal(false);
}

const char* errorDescribe(Error error)
{
    return strerror(error);
}

void initialize()
{
    auto libbacktrace = dlopen("libbacktrace.so", RTLD_LAZY);

    if (not libbacktrace)
    {
        fprintf(stderr, "cannot load libbacktrace.so\n");
        return;
    }

    backtraceCreateState = reinterpret_cast<BacktraceCreateStateFn>(dlsym(libbacktrace, "backtrace_create_state"));
    backtraceFull = reinterpret_cast<BacktraceFullFn>(dlsym(libbacktrace, "backtrace_full"));

    if (not backtraceFull or not backtraceCreateState)
    {
        fprintf(stderr, "cannot find backtrace_full or backtrace_create_state\n");
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
    logger.flushToStderr();
    logger.error() << "Received SIG" << sigabbrev_np(signal);
    sys::stacktraceLogInternal(true);
    sys::finalize();
}

MaybeFile fileOpen(std::string path)
{
    int fd = open(path.c_str(), O_RDONLY);

    if (fd == -1) [[unlikely]]
    {
        return std::unexpected(errno);
    }

    struct stat stat;

    if (fstat(fd, &stat) == -1) [[unlikely]]
    {
        return std::unexpected(errno);
    }

    return File{
        .path = std::move(path),
        .size = static_cast<size_t>(stat.st_size),
        .fd = fd,
    };
}

Error fileClose(File& file)
{
    if (close(file.fd) == -1) [[unlikely]]
    {
        return errno;
    }
    return 0;
}

Error remap(const File& file, Mapping& mapping, size_t newOffset, size_t newLen)
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
        return errno;
    }

    mapping = Mapping{
        .ptr = newPtr,
        .offset = pageStart,
        .len = newLen
    };

    return 0;
}

Error unmap(Mapping& mapping)
{
    if (mapping)
    {
        if (munmap(mapping.ptr, mapping.len)) [[unlikely]]
        {
            return errno;
        }
        mapping.ptr = nullptr;
        mapping.len = 0;
    }

    return 0;
}

Paths getConfigFiles()
{
    const auto home = std::getenv("HOME");

    if (not home)
    {
        return {};
    }

    utils::Buffer buf;

    buf << home << "/.config/log-viewer/config";

    auto path = buf.str();

    struct stat s;

    if (stat(path.c_str(), &s) == -1) [[unlikely]]
    {
        return {};
    }

    return {path};
}

int copyToClipboard(std::string string)
{
    utils::Buffer buf;
    buf << "echo \'" << std::move(string) << "\' | xclip -sel clip";
    system(buf.data());
    return 0;
}

void printLogEntry(const LogEntry& entry, FILE* file)
{
    char timeBuf[32];

    struct tm tmInfo;
    localtime_r(&entry.time, &tmInfo);

    strftime(timeBuf, sizeof(timeBuf), "%F %T", &tmInfo);

#define COLORED(STRING, COLOR) \
    COLOR STRING COLOR_RESET

    fprintf(file, COLORED("[%s]", COLOR_GREEN) " ", timeBuf);

    if (entry.header)
    {
        fprintf(file, COLORED("[%s]", COLOR_BLUE) " ", entry.header);
    }

    if (not entry.flags[LogEntryFlags::noSourceLocation])
    {
        fprintf(file, "%s: ", entry.location.func);
    }

    switch (entry.severity)
    {
        case Severity::debug:
            fprintf(file, COLORED("%s\n", "\e[38;5;245m"), entry.message.c_str());
            break;
        case Severity::warning:
            fprintf(file, COLORED("%s\n", COLOR_YELLOW), entry.message.c_str());
            break;
        case Severity::error:
            fprintf(file, COLORED("%s\n", COLOR_RED), entry.message.c_str());
            break;
        default:
            fprintf(file, "%s\n", entry.message.c_str());
            break;
    }
}

}  // namespace sys
#endif  // __unix__
