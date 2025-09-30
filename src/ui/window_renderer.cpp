#define LOG_HEADER "ui::WindowRenderer"
#include "window_renderer.hpp"

#include <cstdint>
#include <cstring>

#include <ftxui/screen/color.hpp>

#include "core/config.hpp"
#include "core/logger.hpp"
#include "core/window.hpp"
#include "ui/palette.hpp"
#include "utils/buffer.hpp"
#include "utils/time.hpp"

using namespace ftxui;

namespace ui
{

WindowRenderer::WindowRenderer(const core::Window& window)
    : mWindow(window)
{
}

void WindowRenderer::ComputeRequirement()
{
    requirement_.min_x = 0;
    requirement_.min_y = mWindow.height;
    requirement_.flex_grow_x = 1;
    requirement_.flex_grow_y = 1;
}

static std::string convertToString(uint32_t value)
{
    return std::string((const char*)&value, strnlen((const char*)&value, 4));
}

static Color convertToColor(uint32_t value)
{
    return Color(
        (value >> 16) & 0xff,
        (value >>  8) & 0xff,
        (value      ) & 0xff
    );
}

void WindowRenderer::Render(ftxui::Screen& screen)
{
    auto t = utils::startTimeMeasurement();
    int y = box_.y_min;

    if (y > box_.y_max) [[unlikely]]
    {
        return;
    }

    const auto& w = mWindow;

    int xmin = box_.x_min;
    int ycurrent = w.ycurrent + y;
    int selectionStart = w.selectionStart - w.yoffset + y;
    int selectionEnd = w.selectionEnd - w.yoffset + y;
    bool selectionMode = w.selectionMode;
    size_t xoffset = w.xoffset;
    const bool hasBookmarks = w.bookmarks->size() != 0;

    if (w.ringBuffer.size() == 0) [[unlikely]]
    {
        return;
    }

    w.ringBuffer.forEach(
        [this, &xmin, &y, &screen, xoffset, ycurrent, selectionStart, selectionEnd, selectionMode, hasBookmarks](const core::BufferLine& line)
        {
            if (y > box_.y_max) [[unlikely]]
            {
                return;
            }

            int x = box_.x_min;

            const auto bgColor = y == ycurrent
                ? Palette::bg3
                : selectionMode and y >= selectionStart and y <= selectionEnd
                    ? Palette::bg2
                    : Color::Default;

            if (hasBookmarks)
            {
                if (mWindow.bookmarks->find(line.absoluteLineNumber))
                {
                    auto& pixel = screen.PixelAt(x, y);
                    pixel.character = "●";
                    pixel.foreground_color = Palette::fg3;
                }
                xmin = x += 2;
            }

            if (mWindow.config->showLineNumbers)
            {
                const auto lineNumber = mWindow.config->absoluteLineNumbers
                    ? line.absoluteLineNumber
                    : line.lineNumber;

                utils::Buffer buf;
                buf << (lineNumber | utils::leftPadding(mWindow.lineNrDigits + 1)) << mWindow.config->lineNumberSeparator;

                const auto fgColor = y == ycurrent
                    ? Palette::Window::activeLineNumberFg
                    : Palette::Window::inactiveLineNumberFg;

                for (const auto c : buf)
                {
                    auto& pixel = screen.PixelAt(x, y);
                    pixel.character = c;
                    pixel.foreground_color = fgColor;
                    ++x;
                }

                xmin = x;
            }

            size_t position = 0;

            // Draw text line
            for (const auto& segment : line.segments)
            {
                const auto segmentFgColor = convertToColor(segment.color);

                const Color fgColorSelector[2] = {
                    segmentFgColor,
                    Palette::bg5,
                };

                for (const auto& glyph : segment.glyphs)
                {
                    if (position < xoffset)
                    {
                        ++position;
                        continue;
                    }
                    const bool isSpecialGlyph = glyph.flags & (core::GlyphFlags::control | core::GlyphFlags::invalid);
                    const auto& color = fgColorSelector[isSpecialGlyph];
                    for (uint32_t i = 0; i < glyph.width; ++i)
                    {
                        if (x > box_.x_max)
                        {
                            goto next_line;
                        }

                        auto& pixel = screen.PixelAt(x, y);
                        pixel.character = convertToString(glyph.characters[i]);
                        pixel.background_color = bgColor;
                        pixel.foreground_color = color;
                        ++x;
                    }
                    ++position;
                }
            }

            for (; x <= box_.x_max; ++x)
            {
                auto& pixel = screen.PixelAt(x, y);
                pixel.background_color = bgColor;
            }

            next_line:
            {
                ++y;
            }
        });

    const auto [cursorPosition, cursorWidth] = getCursorPositionAndWidth();

    drawCursor(screen, cursorPosition + xmin, cursorWidth, mWindow.ycurrent + box_.y_min);

    logger.debug() << "took: " << 1000 * t.elapsed() << " ms";
}

void WindowRenderer::drawCursor(ftxui::Screen& screen, int cursorPosition, int cursorWidth, int y) const
{
    for (int i = cursorPosition; i < cursorPosition + cursorWidth; ++i)
    {
        auto& pixel = screen.PixelAt(i, y);
        pixel.background_color = Color::White;
        pixel.foreground_color = Color::Black;
    }
}

std::pair<int, int> WindowRenderer::getCursorPositionAndWidth() const
{
    const auto& currentLine = mWindow.ringBuffer[mWindow.ycurrent];

    int cursorPosition = 0;
    for (size_t i = mWindow.xoffset; i < currentLine.glyphs.size(); ++i)
    {
        const auto& glyph = currentLine.glyphs[i];
        if (i == mWindow.xcurrent + mWindow.xoffset)
        {
            return {cursorPosition, glyph.width};
        }
        cursorPosition += glyph.width;
    }

    return {cursorPosition, 1};
}

}  // namespace ui
