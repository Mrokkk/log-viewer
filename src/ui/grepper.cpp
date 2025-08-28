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
    void takeFocus();

    core::GrepOptions options;
    core::Readline    readline;
    ftxui::Component  input;
    ftxui::Components checkboxes;
    ftxui::Component  window;

private:
    void accept(core::Context& context);
};

Grepper::Impl::Impl()
    : input(TextBox({
        .content = &readline.lineRef(),
        .cursorPosition = &readline.cursorRef(),
        .suggestion = &readline.suggestionRef(),
        .suggestionColor = Palette::bg5
    }))
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
            input,
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
        input->Render(),
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

void Grepper::Impl::takeFocus()
{
    input->TakeFocus();
}

void Grepper::Impl::accept(core::Context& context)
{
    const auto& pattern = readline.lineRef();

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
    , pimpl_(new Impl)
{
}

Grepper::~Grepper()
{
    delete pimpl_;
}

void Grepper::takeFocus()
{
    pimpl_->takeFocus();
}

ftxui::Element Grepper::render(core::Context&)
{
    return pimpl_->render();
}

bool Grepper::handleEvent(const ftxui::Event& event, Ftxui&, core::Context& context)
{
    return pimpl_->handleEvent(event, context);
}

Grepper::operator ftxui::Component&()
{
    return pimpl_->window;
}

}  // namespace ui
