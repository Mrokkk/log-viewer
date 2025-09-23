#include "config.hpp"

#include <climits>

#include "core/assert.hpp"
#include "core/context.hpp"
#include "core/interpreter/symbol.hpp"
#include "core/interpreter/value.hpp"
#include "core/main_view.hpp"
#include "core/palette.hpp"
#include "core/thread.hpp"
#include "utils/buffer.hpp"
#include "utils/units.hpp"

namespace core
{

using namespace interpreter;

namespace detail
{

template <typename T>
requires Integral<T>
constexpr static bool validateRange(long value, const Limits<T>& limits)
{
    if constexpr (sizeof(T) >= sizeof(long) * (std::is_unsigned_v<T> + 1))
    {
        if (static_cast<T>(value) < limits.min or
            static_cast<T>(value) > limits.max)
        {
            return false;
        }
    }
    else
    {
        if (static_cast<long long>(value) < static_cast<long long>(limits.min) or
            static_cast<unsigned long>(value) > static_cast<unsigned long>(limits.max))
        {
            return false;
        }
    }

    return true;
}

template <typename T>
ConfigVariable<T>& ConfigVariable<T>::setFlag(ConfigFlags flags)
{
    mFlags |= flags;
    return *this;
}

template <typename T>
OpResult ConfigVariable<T>::assign(Value newValue, Context& context)
{
    if (mAccess != Symbol::Access::readWrite) [[unlikely]]
    {
        return std::unexpected("Not writable");
    }

    if constexpr (Integral<T>)
    {
        if (mValue.type() != Value::Type::integer) [[unlikely]]
        {
            return std::unexpected("Invalid type");
        }
        if (not validateRange(*newValue.integer(), mLimits)) [[unlikely]]
        {
            utils::Buffer buf;
            buf << "Value outside of possible range: [" << mLimits.min << '-' << mLimits.max << ']';
            return OpResult::error(buf.view());
        }
    }

    auto result = mValue.assign(newValue);

    if (not result) [[unlikely]]
    {
        return result;
    }

    onChange(context);

    return OpResult::success;
}

template <typename T>
void ConfigVariable<T>::onChange(Context& context)
{
    if (mFlags[ConfigFlags::reloadAllWindows])
    {
        context.mainView.reloadAll(context);
    }
}

template struct ConfigVariable<uint8_t>;
template struct ConfigVariable<size_t>;
template struct ConfigVariable<bool>;
template struct ConfigVariable<std::string_view>;

}  // namesapce detail

Config::Config()
    : maxThreads{hardwareThreadCount(), 0, hardwareThreadCount()}
    , linesPerThread{5000000, 0, LONG_MAX}
    , bytesPerThread{1_GiB, 0, LONG_MAX}
    , showLineNumbers{false}
    , absoluteLineNumbers{false}
    , highlightSearch{true}
    , scrollJump{5, 0, 16}
    , scrollOff{3, 0, 8}
    , fastMoveLen{16, 0, UCHAR_MAX}
    , tabWidth{4, 0, 8}
    , highlightColor(Palette::yellow, 0, 0xffffff)
    , lineNumberSeparator{" "}
    , tabChar{"›"}
{
    static bool initialized = false;

    assert(not initialized, "Config was already instantiated");

    initialized = true;

    Symbols::add("maxThreads", maxThreads.setHelp("Number of threads used for parallel grep"));
    Symbols::add("linesPerThread", linesPerThread.setHelp("Number of lines processed per thread in parallel grep"));
    Symbols::add("bytesPerThread", bytesPerThread.setHelp("Number of bytes processed per thread in parallel file loading"));
    Symbols::add("showLineNumbers", showLineNumbers.setFlag(ConfigFlags::reloadAllWindows).setHelp("Show line numbers on the left"));
    Symbols::add("absoluteLineNumbers", absoluteLineNumbers.setHelp("Print file absolute line numbers"));
    Symbols::add("highlightSearch", highlightSearch.setFlag(ConfigFlags::reloadAllWindows).setHelp("Highlight searched text"));
    Symbols::add("scrollJump", scrollJump.setHelp("Minimal number of lines to scroll when the cursor gets off the screen"));
    Symbols::add("scrollOff", scrollOff.setHelp("Minimal number of screen lines to keep above and below the cursor"));
    Symbols::add("fastMoveLen", fastMoveLen.setHelp("Amount of characters to jump in fast forward/backward movement"));
    Symbols::add("tabWidth", tabWidth.setFlag(ConfigFlags::reloadAllWindows).setHelp("Tab width"));
    Symbols::add("highlightColor", highlightColor.setFlag(ConfigFlags::reloadAllWindows).setHelp("Color of highlight"));
    Symbols::add("lineNumberSeparator", lineNumberSeparator.setHelp("Line number and view separator"));
    Symbols::add("tabChar", tabChar.setFlag(ConfigFlags::reloadAllWindows).setHelp("Tab character"));
}

}  // namespace core
