#include "regex.hpp"

#include <re2/re2.h>

#include "utils/inline.hpp"
#include "utils/memory.hpp"

namespace core
{

ALWAYS_INLINE constexpr static RE2* impl(char* data)
{
    return reinterpret_cast<RE2*>(data);
}

Regex::Regex(std::string regex, bool caseInsensitive)
{
    static_assert(sizeof(mData) >= sizeof(RE2), "Too small size of opaque mData");

    RE2::Options reOptions;
    reOptions.set_log_errors(false);
    reOptions.set_case_sensitive(not caseInsensitive);

    utils::constructAt(impl(mData), std::move(regex), reOptions);
}

Regex::~Regex()
{
    utils::destroyAt(impl(mData));
}

bool Regex::ok()
{
    return impl(mData)->ok();
}

std::string Regex::error()
{
    return impl(mData)->error();
}

bool Regex::partialMatch(const std::string_view& sv)
{
    return RE2::PartialMatch(sv, *impl(mData));
}

}  // namespace core
