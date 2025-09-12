#pragma once

#include <ftxui/screen/color.hpp>

namespace ui
{

using ftxui::literals::operator""_rgb;

struct Palette
{
    using Color = ftxui::Color;

    static inline Color bg0 = 0x282828_rgb;
    static inline Color bg1 = 0x303030_rgb;
    static inline Color bg2 = 0x4e4e4e_rgb;
    static inline Color bg3 = 0x45403d_rgb;
    static inline Color bg4 = 0x353535_rgb;
    static inline Color bg5 = 0x606060_rgb;
    static inline Color bg6 = 0x808080_rgb;
    static inline Color fg0 = 0xaf8787_rgb;
    static inline Color fg1 = 0x7daea3_rgb;
    static inline Color fg3 = 0xd75f5f_rgb;

    struct StatusLine
    {
        static constexpr auto& commandFg = Palette::bg0;
        static constexpr auto& commandBg = Palette::fg1;
        static constexpr auto& normalFg  = Palette::bg0;
        static constexpr auto& normalBg  = Palette::fg0;
        static constexpr auto& visualFg  = Palette::bg0;
        static constexpr auto& visualBg  = Palette::fg3;
        static constexpr auto& bg1       = Palette::bg1;
        static constexpr auto& bg2       = Palette::bg2;
        static constexpr auto& bg3       = Palette::bg3;
    };

    struct TabLine
    {
        static constexpr auto& separatorBg      = Palette::bg0;
        static constexpr auto& activeBg         = Palette::fg0;
        static constexpr auto& activeFg         = Palette::bg0;
        static constexpr auto& inactiveBg       = Palette::bg2;
        static constexpr auto& activeLineMarker = Palette::fg1;
        static constexpr auto& activeLineBg     = Palette::bg4;
        static constexpr auto& inactiveLineBg   = Palette::bg0;
    };

    struct Window
    {
        static constexpr Color& inactiveLineNumberFg = Palette::bg2;
        static constexpr Color& activeLineNumberFg = Palette::bg6;
    };

    struct Picker
    {
        static constexpr auto& activeLineMarker = Palette::fg1;
        static constexpr auto& additionalInfoFg = Palette::bg2;
    };
};

}  // namespace ui
