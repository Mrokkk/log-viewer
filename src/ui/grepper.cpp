#include "grepper.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>

#include "core/command.hpp"
#include "core/commands/grep.hpp"
#include "core/context.hpp"
#include "core/grep_options.hpp"
#include "core/input.hpp"
#include "core/mode.hpp"
#include "core/readline.hpp"
#include "ui/event_converter.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/text_box.hpp"
#include "ui/ui_component.hpp"

using namespace ftxui;

namespace ui
{

DEFINE_COMMAND(grepper)
{
    HELP() = "show grepper";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->get<Ftxui>();

        if (not ui.mainView.isViewLoaded())
        {
            return false;
        }

        switchFocus(UIComponent::grepper, ui, context);

        return true;
    }
}

struct Grepper::Impl final
{
    Impl();
    Element render();
    bool handleEvent(const ftxui::Event& event, core::Context& context);

    core::GrepOptions options;
    core::Readline    readline;
    TextBox           textBox;
    Components        checkboxes;
    Component         window;

private:
    void accept(core::Context& context);
};

Grepper::Impl::Impl()
    : textBox({
        .content = readline.line(),
        .cursorPosition = &readline.cursor(),
        .suggestion = &readline.suggestion(),
        .suggestionColor = &Palette::bg5
    })
{
    readline
        .enableSuggestions()
        .onAccept(
            [this](core::InputSource, core::Context& context)
            {
                accept(context);
            });

    auto regexCheckbox = Checkbox(
        CheckboxOption{
            .label = "regex (Alt+R)",
            .checked = &options.regex,
        });

    auto caseInsensitiveCheckbox = Checkbox(
        CheckboxOption{
            .label = "case insensitive (Alt+C)",
            .checked = &options.caseInsensitive,
        });

    auto invertedCheckbox = Checkbox(
        CheckboxOption{
            .label = "inverted (Alt+I)",
            .checked = &options.inverted,
        });

    window = Window({
        .inner = Container::LockedVertical({
            regexCheckbox,
            caseInsensitiveCheckbox,
            invertedCheckbox,
        }),
        .title = "",
        .resize_left = false,
        .resize_right = false,
        .resize_top = false,
        .resize_down = false,
    });

    checkboxes = {
        regexCheckbox,
        caseInsensitiveCheckbox,
        invertedCheckbox
    };
}

Element Grepper::Impl::render()
{
    Elements verticalBox = {
        renderTextBox(textBox),
        separator(),
    };

    for (auto& checkbox : checkboxes)
    {
        verticalBox.push_back(checkbox->Render());
    }

    return ::window(
        text(""),
        vbox(std::move(verticalBox)) | xflex) | clear_under | center | xflex;
}

bool Grepper::Impl::handleEvent(const ftxui::Event& event, core::Context& context)
{
    if (event == Event::Escape)
    {
        switchMode(core::Mode::normal, context);
        return true;
    }
    else if (event == Event::AltR)
    {
        options.regex ^= true;
        return true;
    }
    else if (event == Event::AltC)
    {
        options.caseInsensitive ^= true;
        return true;
    }
    else if (event == Event::AltI)
    {
        options.inverted ^= true;
        return true;
    }

    auto keyPress = convertEvent(event);

    if (not keyPress) [[unlikely]]
    {
        return false;
    }

    readline.handleKeyPress(keyPress.value(), core::InputSource::user, context);

    return true;
}

void Grepper::Impl::accept(core::Context& context)
{
    const auto& pattern = readline.line();

    if (pattern.empty())
    {
        return;
    }

    core::commands::grep(pattern, options, context);

    readline.clear();

    core::switchMode(core::Mode::normal, context);
}

Grepper::Grepper()
    : UIComponent(UIComponent::grepper)
    , mPimpl(new Impl)
{
}

Grepper::~Grepper()
{
    delete mPimpl;
}

ftxui::Element Grepper::render(core::Context&)
{
    return mPimpl->render();
}

bool Grepper::handleEvent(const ftxui::Event& event, Ftxui&, core::Context& context)
{
    return mPimpl->handleEvent(event, context);
}

Grepper::operator ftxui::Component&()
{
    return mPimpl->window;
}

}  // namespace ui
