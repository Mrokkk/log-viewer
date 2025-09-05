#pragma once

#include <array>
#include <span>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/views.hpp"
#include "ui/config.hpp"
#include "ui/fwd.hpp"
#include "ui/view_node.hpp"
#include "utils/ring_buffer.hpp"

namespace ui
{

using Character = uint32_t;
using Characters = std::array<Character, 8>;

struct Glyph
{
    uint8_t    width:7;
    uint8_t    special:1;
    uint32_t   offset;
    Characters characters;
};

using Glyphs = std::vector<Glyph>;
using GlyphsSpan = std::span<Glyph>;

struct ColoredString
{
    GlyphsSpan   glyphs;
    ftxui::Color color;
};

using ColoredStrings = std::vector<ColoredString>;

struct Line
{
    size_t         lineNumber;
    size_t         absoluteLineNumber;
    Glyphs         glyphs;
    ColoredStrings segments;
};

using RingBuffer = utils::RingBuffer<Line>;

struct View final : ViewNode
{
    View(std::string name, core::ViewId viewDataId, const Config& config);
    static ViewPtr create(std::string name, core::ViewId viewDataId, const Config& config);

    bool           loaded;
    core::ViewId   dataId;
    size_t         lineCount;
    size_t         viewHeight;
    size_t         lineNrDigits;
    size_t         yoffset;
    size_t         xoffset;
    size_t         ycurrent;
    size_t         xcurrent;
    bool           selectionMode;
    size_t         selectionPivot;
    size_t         selectionStart;
    size_t         selectionEnd;
    const Config&  config;
    RingBuffer     ringBuffer;
    ftxui::Element viewRenderer;
};

void reloadView(View& view, Ftxui& ui, core::Context& context);

}  // namespace ui
