#define LOG_HEADER "ui::ViewRenderer"
#include "view_renderer.hpp"

#include <cstdint>
#include <cstring>

#include <ftxui/screen/color.hpp>

#include "core/logger.hpp"
#include "ui/palette.hpp"
#include "ui/view.hpp"
#include "utils/buffer.hpp"
#include "utils/time.hpp"

using namespace ftxui;

namespace ui
{

ViewRenderer::ViewRenderer(const View& view)
    : mView(view)
{
}

void ViewRenderer::ComputeRequirement()
{
    requirement_.min_x = 0;
    requirement_.min_y = mView.viewHeight;
    requirement_.flex_grow_x = 1;
    requirement_.flex_grow_y = 1;
}

static std::string convertToString(uint32_t value)
{
    return std::string((const char*)&value, strnlen((const char*)&value, 4));
}

void ViewRenderer::Render(ftxui::Screen& screen)
{
    auto t = utils::startTimeMeasurement();
    int y = box_.y_min;

    if (y > box_.y_max) [[unlikely]]
    {
        return;
    }

    int xmin = box_.x_min;
    int ycurrent = mView.ycurrent + y;
    int selectionStart = mView.selectionStart - mView.yoffset + y;
    int selectionEnd = mView.selectionEnd - mView.yoffset + y;
    bool selectionMode = mView.selectionMode;

    mView.ringBuffer.forEach(
        [this, &xmin, &y, &screen, ycurrent, selectionStart, selectionEnd, selectionMode](const Line& line)
        {
            if (y > box_.y_max)
            {
                return;
            }

            int x = box_.x_min;

            auto bgColor = y == ycurrent
                ? Palette::bg3
                : selectionMode and y >= selectionStart and y <= selectionEnd
                    ? Palette::bg2
                    : Color::Default;

            if (mView.config.showLineNumbers)
            {
                const auto lineNumber = mView.config.absoluteLineNumbers
                    ? line.absoluteLineNumber
                    : line.lineNumber;

                utils::Buffer buf;
                buf << (lineNumber | utils::leftPadding(mView.lineNrDigits + 1)) << mView.config.lineNumberSeparator;

                const auto fgColor = y == ycurrent
                    ? Palette::View::activeLineNumberFg
                    : Palette::View::inactiveLineNumberFg;

                for (const auto c : buf)
                {
                    auto& pixel = screen.PixelAt(x, y);
                    pixel.character = c;
                    pixel.foreground_color = fgColor;
                    ++x;
                }

                xmin = x;
            }

            // Draw line
            for (const auto& segment : line.segments)
            {
                for (const auto& glyph : segment.glyphs)
                {
                    if (glyph.special)
                    {
                        for (uint32_t i = 0; i < glyph.width; ++i)
                        {
                            if (x > box_.x_max)
                            {
                                goto next_line;
                            }

                            auto& pixel = screen.PixelAt(x, y);
                            pixel.character = convertToString(glyph.characters[i]);
                            pixel.background_color = bgColor;
                            pixel.foreground_color = Palette::bg5;
                            ++x;
                        }
                    }
                    else
                    {
                        for (uint32_t i = 0; i < glyph.width; ++i)
                        {
                            if (x > box_.x_max)
                            {
                                goto next_line;
                            }

                            auto& pixel = screen.PixelAt(x, y);
                            pixel.character = convertToString(glyph.characters[i]);
                            pixel.background_color = bgColor;
                            pixel.foreground_color = segment.color;
                            ++x;
                        }
                    }
                }

                for (; x <= box_.x_max; ++x)
                {
                    auto& pixel = screen.PixelAt(x, y);
                    pixel.background_color = bgColor;
                }
            }

            next_line:
            {
                ++y;
            }
        });

    const auto [cursorPosition, cursorWidth] = getCursorPositionAndWidth();

    drawCursor(screen, cursorPosition + xmin, cursorWidth, mView.ycurrent + box_.y_min);

    logger.debug() << "took: " << 1000 * t.elapsed() << " ms";
}

void ViewRenderer::drawCursor(ftxui::Screen& screen, int cursorPosition, int cursorWidth, int y) const
{
    for (int i = cursorPosition; i < cursorPosition + cursorWidth; ++i)
    {
        auto& pixel = screen.PixelAt(i, y);
        pixel.background_color = Color::White;
        pixel.foreground_color = Color::Black;
    }
}

std::pair<int, int> ViewRenderer::getCursorPositionAndWidth() const
{
    const auto& currentLine = mView.ringBuffer[mView.ycurrent];

    int cursorPosition = 0;
    for (size_t i = 0; i < currentLine.glyphs.size(); ++i)
    {
        const auto& glyph = currentLine.glyphs[i];
        if (i == mView.xcurrent)
        {
            return {cursorPosition, glyph.width};
        }
        cursorPosition += glyph.width;
    }

    return {cursorPosition, 1};
}

}  // namespace ui
