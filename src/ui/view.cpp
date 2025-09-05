#define LOG_HEADER "ui::View"
#include "view.hpp"

#include <cctype>
#include <cstring>
#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

#include "core/logger.hpp"
#include "core/views.hpp"
#include "ui/palette.hpp"
#include "utils/buffer.hpp"
#include "utils/time.hpp"

using namespace ftxui;

namespace ui
{

View::View(std::string name, core::ViewId viewDataId, const Config& cfg)
    : ViewNode(ViewNode::Type::view, std::move(name))
    , loaded(false)
    , dataId(viewDataId)
    , viewHeight(0)
    , lineNrDigits(0)
    , yoffset(0)
    , xoffset(0)
    , ycurrent(0)
    , xcurrent(0)
    , selectionMode(false)
    , selectionPivot(0)
    , selectionStart(0)
    , selectionEnd(0)
    , config(cfg)
    , ringBuffer(0)
    , viewRenderer(std::make_shared<ViewRenderer>(*this))
{
}

ViewPtr View::create(std::string name, core::ViewId viewDataId, const Config& config)
{
    return std::make_shared<View>(std::move(name), viewDataId, config);
}

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
    char tmp[5];
    std::memcpy(tmp, &value, 4);
    tmp[4] = 0;
    return std::string(tmp);
}

void ViewRenderer::Render(ftxui::Screen& screen)
{
    auto t = utils::startTimeMeasurement();
    int y = box_.y_min;

    if (y > box_.y_max) [[unlikely]]
    {
        return;
    }

    int xcurrent = mView.xcurrent + box_.x_min;
    int ycurrent = mView.ycurrent + y;
    int selectionStart = mView.selectionStart - mView.yoffset + y;
    int selectionEnd = mView.selectionEnd - mView.yoffset + y;
    bool selectionMode = mView.selectionMode;

    mView.ringBuffer.forEach(
        [this, &y, &screen, xcurrent, ycurrent, selectionStart, selectionEnd, selectionMode](const Line& line)
        {
            if (y > box_.y_max)
            {
                return;
            }

            int x = box_.x_min;
            int xmin = box_.x_min;

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
                buf << (lineNumber | utils::leftPadding(mView.lineNrDigits + 1)) << ' ';

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
                    if (x > box_.x_max)
                    {
                        goto next_line;
                    }
                    switch (glyph)
                    {
                        case 0x7f ... 0xff:
                            for (const auto c : utils::Buffer() << '<' << (glyph | utils::hex(utils::noshowbase)) << '>')
                            {
                                auto& pixel = screen.PixelAt(x, y);
                                pixel.character = c;
                                pixel.background_color = bgColor;
                                pixel.foreground_color = Palette::bg5;
                                ++x;
                            }
                            break;

                        case '\t':
                        {
                            auto& pixel = screen.PixelAt(x, y);
                            pixel.character = "›";
                            pixel.background_color = bgColor;
                            pixel.foreground_color = Palette::bg5;
                            ++x;

                            for (int i = 0; i < mView.config.tabWidth - 1; ++i)
                            {
                                auto& pixel = screen.PixelAt(x, y);
                                pixel.character = ' ';
                                pixel.background_color = bgColor;
                                pixel.foreground_color = segment.color;
                                ++x;
                            }
                            break;
                        }

                        default:
                            if (std::iscntrl(glyph))
                            {
                                utils::Buffer buf;
                                buf << '^' << char(glyph + 0x40);
                                for (const auto c : buf)
                                {
                                    auto& pixel = screen.PixelAt(x, y);
                                    pixel.character = c;
                                    pixel.background_color = bgColor;
                                    pixel.foreground_color = Palette::bg5;
                                    ++x;
                                }
                            }
                            else
                            {
                                auto& pixel = screen.PixelAt(x, y);
                                pixel.character = convertToString(glyph);
                                pixel.background_color = bgColor;
                                pixel.foreground_color = segment.color;
                                ++x;
                            }
                            break;
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
                // Draw cursor
                if (y == ycurrent)
                {
                    auto& pixel = screen.PixelAt(xcurrent + xmin, y);
                    pixel.background_color = Color::White;
                    pixel.foreground_color = Color::Black;
                }
                ++y;
            }
        });

    logger.debug() << "took: " << 1000 * t.elapsed() << " ms";
}

}  // namespace ui
