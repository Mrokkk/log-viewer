#pragma once

#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "utils/immobile.hpp"

namespace ui
{

struct UIComponent : utils::Immobile
{
    virtual ~UIComponent() = default;
    virtual ftxui::Element render(core::Context& context) = 0;
};

}  // namespace ui
