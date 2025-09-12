#define LOG_HEADER "ui::MainView"
#include "main_view.hpp"

#include <algorithm>
#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

#include "core/context.hpp"
#include "core/main_view.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/ui_component.hpp"
#include "ui/window_renderer.hpp"

using namespace ftxui;

namespace ui
{

MainView::MainView()
    : UIComponent(UIComponent::mainView)
    , mPlaceholder(Container::Vertical({}))
{
}

MainView::~MainView() = default;

void MainView::takeFocus()
{
}

bool MainView::handleEvent(const ftxui::Event&, Ftxui&, core::Context&)
{
    return false;
}

Element MainView::render(core::Context& context)
{
    auto current = context.mainView.currentWindowNode();

    if (not current) [[unlikely]]
    {
        return vbox({
            text("Hello!") | center,
            text("Type :files<Enter> to open file picker") | center,
            text("Alternatively, type :e <file><Enter> to open given file") | center,
        }) | center | flex;
    }

    auto vertical = renderTablines(context);

    if (current->loaded()) [[likely]]
    {
        vertical.push_back(std::make_shared<WindowRenderer>(current->window()));
    }
    else
    {
        vertical.push_back(vbox({text("Loading...") | center}) | center | flex);
    }

    return vbox(std::move(vertical)) | flex;
}

MainView::operator ftxui::Component&()
{
    return mPlaceholder;
}

static Element wrapActiveLineIf(Element line, bool condition)
{
    if (not condition)
    {
        return line;
    }
    return hbox({
        line,
        text("")
            | color(Palette::TabLine::inactiveLineBg)
            | bgcolor(Palette::TabLine::activeLineMarker),
        text("")
            | color(Palette::TabLine::activeLineMarker)
            | bgcolor(Palette::TabLine::activeLineBg)
            | xflex,
    });
}

static Element renderTab(const std::string& label, int index, bool active)
{
    auto string = ' ' + std::to_string(index) + ' ' + label + ' ';

    if (active)
    {
        if (index != 0)
        {
            return hbox({
                text("")
                    | bgcolor(Palette::TabLine::activeBg)
                    | color(Palette::TabLine::activeFg),
                text(std::move(string))
                    | bgcolor(Palette::TabLine::activeBg)
                    | color(Palette::TabLine::activeFg)
                    | bold,
                text("")
                    | color(Palette::TabLine::activeBg)
                    | bgcolor(Palette::TabLine::separatorBg),
            });
        }
        else
        {
            return hbox({
                text(std::move(string))
                    | bgcolor(Palette::TabLine::activeBg)
                    | color(Palette::TabLine::activeFg)
                    | bold,
                text("")
                    | color(Palette::TabLine::activeBg)
                    | bgcolor(Palette::TabLine::activeFg),
            });
        }
    }
    else
    {
        if (index != 0)
        {
            return hbox({
                text("")
                    | bgcolor(Palette::TabLine::inactiveBg)
                    | color(Palette::TabLine::separatorBg),
                text(std::move(string))
                    | bgcolor(Palette::TabLine::inactiveBg),
                text("")
                    | color(Palette::TabLine::inactiveBg)
                    | bgcolor(Palette::TabLine::separatorBg),
            });
        }
        else
        {
            return hbox({
                text(std::move(string))
                    | bgcolor(Palette::TabLine::inactiveBg),
                text("")
                    | color(Palette::TabLine::inactiveBg)
                    | bgcolor(Palette::TabLine::separatorBg),
            });
        }
    }
}

Elements MainView::renderTablines(core::Context& context)
{
    Elements vertical;
    vertical.reserve(10);

    const auto& root = context.mainView.root();

    Elements horizontal;
    horizontal.reserve(std::max(root.children().size(), 10uz));

    int i = 0;
    int index = 0;
    int activeTabline = context.mainView.activeTabline();

    for (const auto& child : root.children())
    {
        horizontal.push_back(renderTab(child->name(), i, root.activeChild() == child));
        ++i;
    }

    vertical.push_back(wrapActiveLineIf(hbox(std::move(horizontal)), index++ == activeTabline));

    auto node = root.activeChild();

    if (not node) [[unlikely]]
    {
        return vertical;
    }

    while (node->activeChild())
    {
        horizontal.reserve(10);
        int i = 0;
        for (const auto& child : node->children())
        {
            horizontal.push_back(renderTab(child->name(), i, child == node->activeChild()));
            ++i;
        }

        vertical.push_back(wrapActiveLineIf(hbox(std::move(horizontal)), index++ == activeTabline));

        node = node->activeChild();
    }

    return vertical;
}

}  // namespace ui
