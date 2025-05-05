#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/AllWidgets.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <functional>

namespace HomepageScreen
{
tgui::Panel::Ptr createHomepagePanel(
    const sf::Vector2u&   windowSize,
    std::function<void()> onLogsClick,
    std::function<void()> onMeshClick,
    std::function<void()> onFourierClick,
    bool&                 modeOnlineRef,
    std::function<void()> onMenuClick,
    std::function<void()> onShutdownClick);
}
