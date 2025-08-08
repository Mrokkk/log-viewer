#include "scroll.hpp"

#include "utils/math.hpp"

namespace ui
{

bool scrollLeft(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    if (view.xoffset == 0)
    {
        return true;
    }

    view.xoffset--;
    reloadLines(view, context);
    return true;
}

bool scrollRight(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    view.xoffset++;
    reloadLines(view, context);
    return true;
}

bool scrollLineDown(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    if (view.ringBuffer.size() == 0)
    {
        return true;
    }

    if (view.yoffset + view.viewHeight >= view.lineCount)
    {
        return true;
    }

    view.yoffset++;
    auto line = getLine(view, view.yoffset + view.viewHeight - 1, context);
    view.ringBuffer.pushBack(std::move(line));

    return true;
}

bool scrollLineUp(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    if (view.yoffset == 0)
    {
        return true;
    }

    view.yoffset--;
    view.ringBuffer.pushFront(getLine(view, view.yoffset, context));

    return true;
}

bool scrollPageDown(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    if (view.ringBuffer.size() == 0)
    {
        return true;
    }

    if (view.yoffset + view.viewHeight >= view.lineCount)
    {
        return true;
    }

    view.yoffset = (view.yoffset + view.viewHeight)
        | utils::clamp(0ul, view.lineCount - view.viewHeight);

    reloadLines(view, context);

    return true;
}

bool scrollPageUp(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    if (view.yoffset == 0)
    {
        return true;
    }

    view.yoffset -= view.viewHeight;

    if (static_cast<ssize_t>(view.yoffset) < 0)
    {
        view.yoffset = 0;
    }

    reloadLines(view, context);

    return true;
}

bool scrollToEnd(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return true;
    }

    auto& view = *ui.mainView.currentView;

    const auto lastViewLine = view.lineCount - view.viewHeight;

    if (view.yoffset >= lastViewLine)
    {
        return true;
    }

    view.yoffset = lastViewLine;

    reloadLines(view, context);

    return true;
}

void scrollTo(Ftxui& ui, ssize_t lineNumber, core::Context& context)
{
    if (not isViewLoaded(ui))
    {
        return;
    }

    auto& view = *ui.mainView.currentView;

    if (lineNumber == -1)
    {
        scrollToEnd(ui, context);
        return;
    }

    lineNumber = (size_t)lineNumber | utils::clamp(0ul, view.lineCount - view.viewHeight);
    view.yoffset = lineNumber;
    reloadLines(view, context);
}

}  // namespace ui
