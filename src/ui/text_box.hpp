#pragma once

#include <cstddef>
#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/util/ref.hpp>

namespace ui
{

struct TextBoxOptions
{
    using ConstStringRef = ftxui::Ref<const std::string>;

    ConstStringRef     content;
    const size_t*      cursorPosition;
    const std::string* suggestion;
    ftxui::Color       suggestionColor = ftxui::Color();
};

ftxui::Component TextBox(TextBoxOptions option);

}  // namespace ui
