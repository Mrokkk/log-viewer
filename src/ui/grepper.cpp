#include "grepper.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>

#include "core/command.hpp"
#include "core/context.hpp"
#include "core/operations.hpp"
#include "ui/ftxui.hpp"

using namespace ftxui;

namespace ui
{

void createGrepper(Ftxui& ui)
{
    ui.grepper.input = Input(
        &ui.grepper.state.pattern,
        InputOption{
            .multiline = false,
        }
    );

    auto regexCheckbox = Checkbox(
        CheckboxOption{
            .label = "regex",
            .checked = &ui.grepper.state.regex,
        });

    auto caseInsensitiveCheckbox = Checkbox(
        CheckboxOption{
            .label = "case insensitive",
            .checked = &ui.grepper.state.caseInsensitive,
        });

    auto invertedCheckbox = Checkbox(
        CheckboxOption{
            .label = "inverted",
            .checked = &ui.grepper.state.inverted,
        });

    ui.grepper.window = Window({
        .inner = Container::LockedVertical({
            ui.grepper.input,
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

    ui.grepper.checkboxes = {
        regexCheckbox,
        caseInsensitiveCheckbox,
        invertedCheckbox
    };
}

void grepperAccept(Ftxui& ui, core::Context& context)
{
    if (ui.grepper.state.pattern.empty())
    {
        return;
    }

    std::string command = "grep ";

    if (ui.grepper.state.regex)
    {
        command += "-r ";
    }
    if (ui.grepper.state.caseInsensitive)
    {
        command += "-c ";
    }
    if (ui.grepper.state.inverted)
    {
        command += "-i ";
    }

    command += '\"';
    command += ui.grepper.state.pattern;
    command += '\"';

    core::executeCode(command, context);

    ui.active = UIElement::logView;
}

Element renderGrepper(Ftxui& ui)
{
    Elements verticalBox = {
        ui.grepper.input->Render(),
        separator(),
    };

    for (auto& checkbox : ui.grepper.checkboxes)
    {
        verticalBox.push_back(checkbox->Render());
    }

    return window(
        text(""),
        vbox(std::move(verticalBox)) | xflex) | clear_under | center | xflex;
}

namespace
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

        if (ui.active != UIElement::logView or not isViewLoaded(ui))
        {
            return false;
        }

        ui.active = UIElement::grepper;
        ui.grepper.input->TakeFocus();

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

        if (not isViewLoaded(ui))
        {
            ui << error << "no file loaded yet";
            return false;
        }

        core::GrepOptions options;

        if (flags.contains("c"))
        {
            options.caseInsensitive = true;
        }
        if (flags.contains("r"))
        {
            options.regex = true;
        }
        if (flags.contains("i"))
        {
            options.inverted = true;
        }

        auto viewToAdd = ui.currentView->isBase()
            ? ui.currentView->parent()
            : ui.currentView;

        auto& newLink = (*viewToAdd)
            .addChild(ViewNode::createLink(pattern))
            .setActive();

        auto& base = newLink
            .addChild(View::create("base"))
            .setActive()
            .cast<View>();

        auto parentView = ui.currentView;
        ui.currentView = &base;

        core::asyncGrep(
            pattern,
            options,
            parentView->lines,
            *parentView->file,
            [&base, &ui, &context, file = parentView->file](core::LineRefs lines, float time)
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

}  // namespace

}  // namespace ui
