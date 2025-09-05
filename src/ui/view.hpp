#pragma once

#include <memory>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/views.hpp"
#include "ui/config.hpp"
#include "ui/fwd.hpp"
#include "ui/view_node.hpp"
#include "utils/ring_buffer.hpp"
#include "utils/string.hpp"

namespace ui
{

struct View;

using Glyphs = std::vector<uint32_t>;

struct ColoredString
{
    Glyphs       glyphs;
    ftxui::Color color;
};

using ColoredStrings = std::vector<ColoredString>;

struct Line
{
    size_t         lineNumber;
    size_t         absoluteLineNumber;
    ColoredStrings segments;
    std::string    data;
};

using RingBuffer = utils::RingBuffer<Line>;

struct ViewRenderer : ftxui::Node
{
    ViewRenderer(const View& view);

private:
    void ComputeRequirement() override;
    void Render(ftxui::Screen& screen) override;

    const View& mView;
};

using ViewRendererPtr = std::shared_ptr<ViewRenderer>;

struct View final : ViewNode
{
    View(std::string name, core::ViewId viewDataId, const Config& config);
    static ViewPtr create(std::string name, core::ViewId viewDataId, const Config& config);

    bool            loaded;
    core::ViewId    dataId;
    size_t          lineCount;
    size_t          viewHeight;
    size_t          lineNrDigits;
    size_t          yoffset;
    size_t          xoffset;
    size_t          ycurrent;
    size_t          xcurrent;
    bool            selectionMode;
    size_t          selectionPivot;
    size_t          selectionStart;
    size_t          selectionEnd;
    const Config&   config;
    RingBuffer      ringBuffer;
    ViewRendererPtr viewRenderer;
};

void reloadView(View& view, Ftxui& ui, core::Context& context);

}  // namespace ui
