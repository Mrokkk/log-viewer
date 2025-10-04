#pragma once

#include <ftxui/dom/node.hpp>

#include "core/fwd.hpp"

namespace ui
{

struct BookmarksRenderer : ftxui::Node
{
    BookmarksRenderer(const core::Bookmarks& bookmarks, bool active);
    static ftxui::Element create(const core::Bookmarks& bookmarks, bool active);

private:
    void ComputeRequirement() override;
    void Render(ftxui::Screen& screen) override;

    const bool             mActive;
    const core::Bookmarks& mBookmarks;
};

}  // namespace ui
