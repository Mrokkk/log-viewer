#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "core/buffers.hpp"
#include "utils/bitflag.hpp"
#include "utils/noncopyable.hpp"
#include "utils/ring_buffer.hpp"

namespace core
{

using Character = uint32_t;
using Characters = std::array<Character, 8>;

DEFINE_BITFLAG(GlyphFlags, uint8_t,
{
    whitespace,
    invalid,
    control,
});

struct Glyph
{
    uint8_t    width;
    GlyphFlags flags;
    uint32_t   offset;
    Characters characters;
};

using Glyphs = std::vector<Glyph>;
using GlyphsSpan = std::span<Glyph>;

struct ColoredString
{
    uint32_t   color:24;
    uint32_t   defColor:1;
    GlyphsSpan glyphs;
};

using ColoredStrings = std::vector<ColoredString>;

struct BufferLine
{
    size_t         lineNumber;
    size_t         absoluteLineNumber;
    Glyphs         glyphs;
    ColoredStrings segments;
};

using RingBuffer = utils::RingBuffer<BufferLine>;

struct Window final : utils::NonCopyable
{
    using ConfigPtr = const Config* const;

    Window();
    Window(BufferId id, Context& context);
    Window(Window&& other) = default;

    bool       initialized;
    bool       loaded;
    bool       pendingSearch;
    bool       foundAnything;
    BufferId   bufferId;
    size_t     lineCount;
    size_t     width;
    size_t     height;
    size_t     lineNrDigits;
    size_t     yoffset;
    size_t     xoffset;
    size_t     ycurrent;
    size_t     xcurrent;
    bool       selectionMode;
    size_t     selectionPivot;
    size_t     selectionStart;
    size_t     selectionEnd;
    Context*   context;
    ConfigPtr  config;
    RingBuffer ringBuffer;
};

}  // namespace core
