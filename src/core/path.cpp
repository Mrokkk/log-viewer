#include "core/buffer.hpp"
#include "core/context.hpp"
#include "core/main_view.hpp"
#include "core/variable.hpp"

namespace core
{

DEFINE_READONLY_VARIABLE(path, string, "Path to the opened file")
{
    READER()
    {
        if (not context.mainView.isCurrentWindowLoaded())
        {
            return nullptr;
        }

        auto buffer = context.mainView.currentBuffer();

        if (not buffer)
        {
            return nullptr;
        }

        return &buffer->filePath();
    }
}

}  // namespace core
