#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <TGUI/Widgets/Button.hpp>
#include <TGUI/Widgets/Panel.hpp>

namespace Mesh
{
tgui::Panel::Ptr createMeshContainer(std::function<void()> goBackCallback);
tgui::Panel::Ptr createMeshTile(tgui::Panel::Ptr tile, const std::function<void()>& openCallback);
void             updateAndDraw(sf::RenderWindow& window);
} // namespace Mesh
