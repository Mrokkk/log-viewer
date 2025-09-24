#include "picker.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/colored_string.hpp>

#include "core/context.hpp"
#include "core/main_picker.hpp"
#include "ui/ftxui.hpp"
#include "ui/picker_entry.hpp"
#include "ui/text_box.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"

using namespace ftxui;

namespace ui
{

static std::string pickerName(const core::MainPicker::Type type)
{
#define TYPE_CONVERT(type) \
    case core::MainPicker::Type::type: return #type
    switch (type)
    {
        TYPE_CONVERT(files);
        TYPE_CONVERT(commands);
        TYPE_CONVERT(variables);
        TYPE_CONVERT(messages);
        TYPE_CONVERT(logs);
        case core::MainPicker::Type::_last:
            break;
    }
    return "unknown";
}

static utils::Strings getPickerNames()
{
    utils::Strings names;
    for (int i = 0; i < static_cast<int>(core::MainPicker::Type::_last); ++i)
    {
        names.push_back(pickerName(static_cast<core::MainPicker::Type>(i)));
    }
    return names;
}

Picker::Picker(core::Context& context)
    : mTextBox({
        .content = context.mainPicker.readline().line(),
        .cursorPosition = &context.mainPicker.readline().cursor(),
    })
    , mTabs(
        Toggle(
            getPickerNames(),
            &context.mainPicker.currentPickerIndex()))
    , mPlaceholder(Input(""))
    , mWindow(
        Window({
            .inner = Container::LockedVertical({
                mTabs,
                mPlaceholder
            }),
            .title = "",
            .resize_left = false,
            .resize_right = false,
            .resize_top = false,
            .resize_down = false,
        }))
{
    mPlaceholder->TakeFocus();
}

Picker::~Picker() = default;

Element Picker::Picker::render(core::Context& context)
{
    auto& mainPicker = context.mainPicker;
    auto& picker = mainPicker.currentPicker();

    auto& ui = context.ui->cast<Ftxui>();
    const auto resx = ui.terminalSize.dimx / 2;

    utils::Buffer buf;

    buf << picker.filtered().size() << '/' << picker.data().size();

    Elements content;
    content.reserve(picker.filtered().size());

    size_t i = 0;
    for (const auto& string : picker.filtered())
    {
        const bool active = i++ == picker.cursor();
        auto element = renderPickerEntry(*string, active);
        if (active) element |= focus;
        content.emplace_back(std::move(element));
    }

    return window(
        text(""),
        vbox({
            mTabs->Render(),
            separator(),
            hbox({
                renderTextBox(mTextBox) | xflex,
                separator(),
                text(buf.str())
            }),
            separator(),
            vbox(std::move(content)) | yframe | yflex | size(HEIGHT, EQUAL, picker.height()),
        }))
            | size(WIDTH, EQUAL, resx)
            | clear_under
            | center
            | flex;
}

}  // namespace ui
