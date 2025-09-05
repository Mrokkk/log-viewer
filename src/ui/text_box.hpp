#pragma once

#include <string>

#include <ftxui/fwd.hpp>

namespace ui
{

struct TextBox
{
    const std::string&  content;
    const size_t*       cursorPosition = nullptr;
    const std::string*  suggestion = nullptr;
    const ftxui::Color* suggestionColor = nullptr;
};

ftxui::Element renderTextBox(const TextBox& options);

}  // namespace ui
