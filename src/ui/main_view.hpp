#pragma once

#include "ui/view.hpp"

namespace ui
{

struct MainView
{
    MainView();
    ViewNode root;
    View*    currentView = nullptr;
    int      activeLine  = 0;
};

}  // namespace ui
