#include "bookmarks_renderer.hpp"

#include <memory>

#include "core/bookmarks.hpp"
#include "ui/palette.hpp"
#include "utils/math.hpp"

using namespace ftxui;

namespace ui
{

BookmarksRenderer::BookmarksRenderer(const core::Bookmarks& bookmarks, bool active)
    : mActive(active)
    , mBookmarks(bookmarks)
{
}

ftxui::Element BookmarksRenderer::create(const core::Bookmarks& bookmarks, bool active)
{
    return std::make_shared<BookmarksRenderer>(bookmarks, active);
}

void BookmarksRenderer::ComputeRequirement()
{
    size_t maxLen = sizeof("Bookmarks") - 1;
    for (const auto& bookmark : mBookmarks)
    {
        maxLen = utils::max(bookmark.name.size(), maxLen);
    }
    requirement_.min_x = maxLen;
    requirement_.min_y = 0;
    requirement_.flex_grow_y = 1;
    requirement_.flex_grow_x = 1;
    requirement_.flex_shrink_x = 1;
}

static void writeTitle(Screen& screen, int x, int y)
{
    for (auto c : "Bookmarks")
    {
        if (not c)
        {
            return;
        }
        auto& pixel = screen.PixelAt(x++, y);
        pixel.character = c;
        pixel.bold = true;
        pixel.foreground_color = Palette::fg0;
    }
}

void BookmarksRenderer::Render(ftxui::Screen& screen)
{
    int y = box_.y_min;

    if (y > box_.y_max) [[unlikely]]
    {
        return;
    }

    writeTitle(screen, box_.x_min, y++);

    int i = 0;
    for (const auto& bookmark : mBookmarks)
    {
        if (y > box_.y_max)
        {
            return;
        }

        int x = box_.x_min;
        Color bgColor = i == mBookmarks.currentIndex
            ? Palette::bg2
            : Color::Default;

        for (auto c : bookmark.name)
        {
            if (x > box_.x_max)
            {
                break;
            }
            auto& pixel = screen.PixelAt(x++, y);
            pixel.character = c;
            pixel.background_color = bgColor;
        }

        for (; x <= box_.x_max; ++x)
        {
            auto& pixel = screen.PixelAt(x, y);
            pixel.background_color = bgColor;
        }

        ++i;
        ++y;
    }

    if (mActive and mBookmarks.currentIndex + box_.y_min + 1 <= box_.y_max)
    {
        auto& pixel = screen.PixelAt(box_.x_min, mBookmarks.currentIndex + box_.y_min + 1);
        pixel.background_color = Color::White;
        pixel.foreground_color = Color::Black;
    }
}

}  // namespace ui
