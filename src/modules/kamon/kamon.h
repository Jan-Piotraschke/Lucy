#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <functional>
#include <string>
#include <vector>

namespace Kamon
{
// Return an SFML VertexArray of the kamon contour (already extracted + normalized).
sf::VertexArray createKamonContourShape();

// Create a TGUI Panel that represents the "Kamon Screen"
//   This panel typically has a label and "Back" button.
//   We take a callback for the back button.
tgui::Panel::Ptr createKamonContainer(const std::function<void()>& onBackHome);

// Create a TGUI Panel "tile" (300x150), with a button that calls openCallback when pressed.
//   This can be placed on your Home screen.
tgui::Panel::Ptr createKamonTile(tgui::Panel::Ptr tile, const std::function<void()>& openCallback);

// You may want a global panel pointer to re-hide or show
// (optional, but we do it to unify hideAllScreens).
// We'll store the one created in createKamonContainer.
tgui::Panel::Ptr getKamonPanel();
} // namespace Kamon
