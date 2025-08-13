#include "ui_component.hpp"

namespace ui
{

UIComponent::UIComponent(Type t)
    : type(t)
{
}

UIComponent::~UIComponent() = default;

void UIComponent::onExit()
{
}

void UIComponent::takeFocus()
{
}

bool UIComponent::handleEvent(const ftxui::Event&, Ftxui&, core::Context&)
{
    return false;
}

}  // namespace ui
