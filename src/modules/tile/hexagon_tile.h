#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>

// A simple class that draws itself as a hexagon. It inherits from tgui::Panel
// so you can add child widgets (like labels, buttons, etc.) on top.
class HexagonTile : public tgui::Panel
{
  public:
    using Ptr = std::shared_ptr<HexagonTile>;

    // Create function, allows specifying size and fill color
    static Ptr create(
        const tgui::Layout2d& size      = {"100%", "100%"},
        const sf::Color&      fillColor = sf::Color::Blue);

    // Constructor
    HexagonTile();

    // Optionally set (or re-set) the fill color
    void setFillColor(const sf::Color& color);

  protected:
    // Overridden draw function from TGUI
    void draw(tgui::BackendRenderTarget& target, tgui::RenderStates states) const override;

  private:
    sf::Color m_fillColor;
};
