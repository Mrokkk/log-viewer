#pragma once

#include <utility>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "core/fwd.hpp"

namespace ui
{

struct WindowRenderer : ftxui::Node
{
    WindowRenderer(const core::Window& window);
    static ftxui::Element create(const core::Window& window);

private:
    void ComputeRequirement() override;
    void Render(ftxui::Screen& screen) override;
    void drawCursor(ftxui::Screen& screen, int cursorPosition, int cursorWidth, int y) const;
    std::pair<int, int> getCursorPositionAndWidth() const;

    const core::Window& mWindow;
};

}  // namespace ui
