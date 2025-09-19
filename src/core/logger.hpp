#pragma once

#include <string_view>

#include "core/log_entry.hpp"
#include "core/severity.hpp"
#include "utils/function_ref.hpp"
#include "utils/immobile.hpp"
#include "utils/buffer.hpp"
#include "utils/source_location.hpp"

#ifndef LOG_HEADER
#define LOG_HEADER nullptr
#endif

#ifndef LOG_FLAGS
#define LOG_FLAGS
#endif

namespace core
{

struct Logger final
{
    struct Flusher final : utils::Immobile
    {
        Flusher(
            Severity severity,
            LogEntryFlags flags,
            const char* header,
            utils::SourceLocation loc,
            utils::Buffer& buffer);

        constexpr ~Flusher()
        {
            Logger::registerLogEntry(mSeverity, mFlags, mHeader, mLocation);
        }

        constexpr utils::Buffer& buffer()
        {
            return mBuffer;
        }

        template <typename T>
        constexpr utils::Buffer& operator<<(T&& value)
        {
            return mBuffer << std::forward<T>(value);
        }

    private:
        const Severity              mSeverity;
        const LogEntryFlags         mFlags;
        const char* const           mHeader;
        const utils::SourceLocation mLocation;
        utils::Buffer&              mBuffer;
    };

    constexpr static Flusher debug(
        LogEntryFlags flags = LogEntryFlags(LOG_FLAGS),
        const char* header = LOG_HEADER,
        utils::SourceLocation loc = utils::SourceLocation::current())
    {
        return log(Severity::debug, flags, header, loc);
    }

    constexpr static Flusher info(
        LogEntryFlags flags = LogEntryFlags(LOG_FLAGS),
        const char* header = LOG_HEADER,
        utils::SourceLocation loc = utils::SourceLocation::current())
    {
        return log(Severity::info, flags, header, loc);
    }

    constexpr static Flusher warning(
        LogEntryFlags flags = LogEntryFlags(LOG_FLAGS),
        const char* header = LOG_HEADER,
        utils::SourceLocation loc = utils::SourceLocation::current())
    {
        return log(Severity::warning, flags, header, loc);
    }

    constexpr static Flusher error(
        LogEntryFlags flags = LogEntryFlags(LOG_FLAGS),
        const char* header = LOG_HEADER,
        utils::SourceLocation loc = utils::SourceLocation::current())
    {
        return log(Severity::error, flags, header, loc);
    }

    static void flushToStderr();
    static void setLogFile(std::string_view path);
    static void forEachLogEntry(utils::FunctionRef<void(const LogEntry&)> visitor);

private:
    static Flusher log(
        Severity severity,
        LogEntryFlags flags,
        const char* header,
        utils::SourceLocation loc);

    static void registerLogEntry(
        Severity severity,
        LogEntryFlags flags,
        const char* header,
        utils::SourceLocation loc);
};

}  // namespace core

constexpr static core::Logger logger;
