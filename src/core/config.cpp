#include "config.hpp"

#include <functional>

#include "core/context.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/thread.hpp"
#include "core/variable.hpp"

namespace core
{

template <typename T>
constexpr static bool setIntegerVariable(T& variable, const Variable::Value& value, Context& context, std::function<void(Context& context)> onSuccess = {})
{
    if (not variable.set(value.integer)) [[unlikely]]
    {
        context.messageLine.error() << "Value not in range: [" << variable.min() << ", " << variable.max() << ']';
        return false;
    }
    if (onSuccess)
    {
        onSuccess(context);
    }
    return true;
}

DEFINE_READWRITE_VARIABLE(maxThreads, integer, "Number of threads used for parallel grep")
{
    READER()
    {
        return long(context.config.maxThreads);
    }

    WRITER()
    {
        auto hardwareMaxThreads = hardwareThreadCount();
        if (value.integer > hardwareMaxThreads)
        {
            context.messageLine.info() << "Value clamped to max number of hardware threads: " << hardwareMaxThreads;
            // FIXME: I really need to fix the Variable::Value (probably introduce
            // a ref-counted Object which will handle both pointers and actual values)
            return setIntegerVariable(
                context.config.maxThreads,
                core::Variable::Value(long(hardwareMaxThreads)),
                context,
                [](Context& context)
                {
                    context.mainView.reloadAll(context);
                });
        }
        return setIntegerVariable(
            context.config.maxThreads,
            value,
            context,
            [](Context& context)
            {
                context.mainView.reloadAll(context);
            });
    }
}

DEFINE_READWRITE_VARIABLE(linesPerThread, integer, "Number of lines processed per thread in parallel grep")
{
    READER()
    {
        return long(context.config.linesPerThread);
    }

    WRITER()
    {
        return setIntegerVariable(
            context.config.linesPerThread,
            value,
            context);
    }
}

DEFINE_READWRITE_VARIABLE(bytesPerThread, integer, "Number of bytes processed per thread in parallel file loading")
{
    READER()
    {
        return long(context.config.bytesPerThread);
    }

    WRITER()
    {
        return setIntegerVariable(
            context.config.bytesPerThread,
            value,
            context);
    }
}

DEFINE_READWRITE_VARIABLE(showLineNumbers, boolean, "Show line numbers on the left")
{
    READER()
    {
        return context.config.showLineNumbers.get();
    }

    WRITER()
    {
        context.config.showLineNumbers.set(value.boolean);
        context.mainView.reloadAll(context);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(absoluteLineNumbers, boolean, "Print file absolute line numbers")
{
    READER()
    {
        return context.config.absoluteLineNumbers.get();
    }

    WRITER()
    {
        context.config.absoluteLineNumbers.set(value.boolean);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(scrollJump, integer, "Minimal number of lines to scroll when the cursor gets off the screen")
{
    READER()
    {
        return long(context.config.scrollJump);
    }

    WRITER()
    {
        return setIntegerVariable(
            context.config.scrollJump,
            value,
            context);
    }
}

DEFINE_READWRITE_VARIABLE(scrollOff, integer, "Minimal number of screen lines to keep above and below the cursor")
{
    READER()
    {
        return long(context.config.scrollOff);
    }

    WRITER()
    {
        return setIntegerVariable(
            context.config.scrollOff,
            value,
            context);
    }
}

DEFINE_READWRITE_VARIABLE(fastMoveLen, integer, "Amount of characters to jump in fast forward/backward movement")
{
    READER()
    {
        return long(context.config.fastMoveLen);
    }

    WRITER()
    {
        return setIntegerVariable(
            context.config.fastMoveLen,
            value,
            context);
    }
}

DEFINE_READWRITE_VARIABLE(tabWidth, integer, "Tab width")
{
    READER()
    {
        return long(context.config.tabWidth);
    }

    WRITER()
    {
        return setIntegerVariable(
            context.config.tabWidth,
            value,
            context,
            [](Context& context)
            {
                context.mainView.reloadAll(context);
            });
    }
}

DEFINE_READWRITE_VARIABLE(lineNumberSeparator, string, "Line number and view separator")
{
    READER()
    {
        return &context.config.lineNumberSeparator.get();
    }

    WRITER()
    {
        context.config.lineNumberSeparator.set(*value.string);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(tabChar, string, "Tab character")
{
    READER()
    {
        return &context.config.tabChar.get();
    }

    WRITER()
    {
        context.config.tabChar.set(*value.string);
        context.mainView.reloadAll(context);
        return true;
    }
}

}  // namespace core
