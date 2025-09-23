#pragma once

// IWYU pragma: always_keep
#include <ftxui/component/event.hpp>

template<>
struct std::hash<ftxui::Event>
{
    std::size_t operator()(const ftxui::Event& e) const noexcept
    {
        return std::hash<std::string>{}(e.input());
    }
};
