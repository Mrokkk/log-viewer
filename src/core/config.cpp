#include "config.hpp"

#include "core/context.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/variable.hpp"

namespace core
{

DEFINE_READWRITE_VARIABLE(maxThreads, integer, "Number of threads used for parallel grep")
{
    READER()
    {
        return long(context.config.maxThreads);
    }

    WRITER()
    {
        if (value.integer < 0 or value.integer > 16)
        {
            context.messageLine.error() << "Invalid maxThreads value: " << value.integer;
            return false;
        }
        context.config.maxThreads = value.integer;
        return true;
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
        if (value.integer < 0)
        {
            context.messageLine.error() << "Invalid linesPerThread value: " << value.integer;
            return false;
        }
        context.config.linesPerThread = value.integer;
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(showLineNumbers, boolean, "Show line numbers on the left")
{
    READER()
    {
        return context.config.showLineNumbers;
    }

    WRITER()
    {
        context.config.showLineNumbers = value.boolean;
        context.mainView.reloadAll(context);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(absoluteLineNumbers, boolean, "Print file absolute line numbers")
{
    READER()
    {
        return context.config.absoluteLineNumbers;
    }

    WRITER()
    {
        context.config.absoluteLineNumbers = value.boolean;
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
        if (value.integer < 1)
        {
            context.messageLine.error() << "Invalid scrollJump value: " << value.integer;
            return false;
        }
        context.config.scrollJump = value.integer;
        return true;
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
        context.config.scrollOff = value.integer & 0xff;
        return true;
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
        context.config.fastMoveLen = value.integer & 0xff;
        return true;
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
        if (value.integer > 8 or value.integer < 0)
        {
            context.messageLine.error() << "Invalid tab width: " << value.integer;
            return false;
        }
        context.config.tabWidth = value.integer;
        context.mainView.reloadAll(context);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(lineNumberSeparator, string, "Line number and view separator")
{
    READER()
    {
        return &context.config.lineNumberSeparator;
    }

    WRITER()
    {
        context.config.lineNumberSeparator = *value.string;
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(tabChar, string, "Tab character")
{
    READER()
    {
        return &context.config.tabChar;
    }

    WRITER()
    {
        context.config.tabChar = *value.string;
        context.mainView.reloadAll(context);
        return true;
    }
}

}  // namespace core
