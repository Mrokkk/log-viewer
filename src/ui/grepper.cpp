#include "grepper.hpp"

#include <spanstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>

#include "core/command.hpp"
#include "core/context.hpp"
#include "core/grep.hpp"
#include "core/interpreter.hpp"
#include "ui/ftxui.hpp"
#include "ui/ui_component.hpp"
#include "ui/view.hpp"

using namespace ftxui;

namespace ui
{

struct State final
{
    std::string   pattern;
    bool          regex           = false;
    bool          caseInsensitive = false;
    bool          inverted        = false;
};

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

        if (ui.active->type != UIComponent::mainView or not ui.mainView.isViewLoaded())
        {
            return false;
        }

        switchFocus(UIComponent::grepper, ui, context);

        return true;
    }
}

DEFINE_COMMAND(grep)
{
    HELP() = "filter current view";

    FLAGS()
    {
        return {
            "c",
            "i",
            "r",
        };
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "pattern")
        };
    }

    EXECUTOR()
    {
        const auto& pattern = args[0].string;
        auto& ui = context.ui->get<Ftxui>();

        if (not ui.mainView.isViewLoaded())
        {
            ui << error << "no file loaded yet";
            return false;
        }

        std::string optionsString;

        core::GrepOptions options;

        if (flags.contains("r"))
        {
            optionsString += 'r';
            options.regex = true;
        }
        if (flags.contains("c"))
        {
            optionsString += 'c';
            options.caseInsensitive = true;
        }
        if (flags.contains("i"))
        {
            optionsString += 'i';
            options.inverted = true;
        }

        char buffer[128];
        std::spanstream ss(buffer);

        ss << pattern;

        if (not optionsString.empty())
        {
            ss << " [" << optionsString << ']';
        }

        auto& prevCurrentView = *ui.mainView.currentView();
        auto file = prevCurrentView.file;

        auto& parent = prevCurrentView.isBase()
            ? *prevCurrentView.parent()
            : prevCurrentView;

        auto& newView = ui.mainView.createView(
            std::string(ss.span().begin(), ss.span().end()),
            &parent);

        auto& base = newView.base().cast<View>();

        core::asyncGrep(
            pattern,
            options,
            prevCurrentView.lines,
            *file,
            [&base, &ui, &context, file](core::LineRefs lines, float time)
            {
                ui.screen.Post(
                    [&ui, &base, &context, lines = std::move(lines), file, time]
                    {
                        ui << info << "found " << lines.size() << " matches; took "
                           << std::fixed << std::setprecision(3) << time << " s";

                        base.file = file;
                        base.lineCount = lines.size();
                        base.lines = std::move(lines);

                        reloadView(base, ui, context);
                    });
                ui.screen.RequestAnimationFrame();
            });

        return true;
    }
}

namespace commands
{

bool grep(const State& params, core::Context& context)
{
    std::string command = "grep ";

    if (params.regex)
    {
        command += "-r ";
    }
    if (params.caseInsensitive)
    {
        command += "-c ";
    }
    if (params.inverted)
    {
        command += "-i ";
    }

    command += '\"';
    command += params.pattern;
    command += '\"';

    return core::executeCode(command, context);
}

}  // namespace commands

struct Grepper::Impl final
{
    Impl();
    Element render();
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context);

    State             state;
    ftxui::Component  input;
    ftxui::Components checkboxes;
    ftxui::Component  window;

private:
    bool accept(Ftxui& ui, core::Context& context);
};

Grepper::Impl::Impl()
    : input(Input(&state.pattern, InputOption{.multiline = false}))
{
    auto regexCheckbox = Checkbox(
        CheckboxOption{
            .label = "regex (Alt+R)",
            .checked = &state.regex,
        });

    auto caseInsensitiveCheckbox = Checkbox(
        CheckboxOption{
            .label = "case insensitive (Alt+C)",
            .checked = &state.caseInsensitive,
        });

    auto invertedCheckbox = Checkbox(
        CheckboxOption{
            .label = "inverted (Alt+I)",
            .checked = &state.inverted,
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

bool Grepper::Impl::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    if (event == Event::Return)
    {
        return accept(ui, context);
    }
    else if (event == Event::AltR)
    {
        state.regex ^= true;
        return true;
    }
    else if (event == Event::AltC)
    {
        state.caseInsensitive ^= true;
        return true;
    }
    else if (event == Event::AltI)
    {
        state.inverted ^= true;
        return true;
    }

    return false;
}

bool Grepper::Impl::accept(Ftxui& ui, core::Context& context)
{
    if (state.pattern.empty())
    {
        return true;
    }

    commands::grep(state, context);

    switchFocus(UIComponent::mainView, ui, context);

    return true;
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
    pimpl_->input->TakeFocus();
}

ftxui::Element Grepper::render(core::Context&)
{
    return pimpl_->render();
}

bool Grepper::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    return pimpl_->handleEvent(event, ui, context);
}

Grepper::operator ftxui::Component&()
{
    return pimpl_->window;
}

}  // namespace ui
