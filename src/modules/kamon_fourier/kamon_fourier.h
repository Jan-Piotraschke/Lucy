#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <functional>

namespace KamonFourier
{
// Create the TGUI Panel for the “Kamon Fourier” screen (with label, back button).
tgui::Panel::Ptr createKamonFourierContainer(const std::function<void()>& onBackHome);

// Create a tile (similar to your logs or Kamon tile) to open the Fourier screen.
tgui::Panel::Ptr createFourierTile(const std::function<void()>& openCallback);

// Return the panel pointer so we can hide/show it.
tgui::Panel::Ptr getFourierPanel();

// Called every frame while we are on the KamonFourier screen
// to update the epicycle animation and draw it to the window.
void updateAndDraw(sf::RenderWindow& window);
} // namespace KamonFourier
