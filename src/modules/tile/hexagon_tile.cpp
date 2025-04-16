#include "hexagon_tile.h"

#include <TGUI/Backend/SFML-Graphics.hpp> // For BackendRenderTargetSFML

HexagonTile::Ptr HexagonTile::create(const tgui::Layout2d& size, const sf::Color& fillColor)
{
    auto tile = std::make_shared<HexagonTile>();
    tile->setSize(size);
    tile->setFillColor(fillColor);
    return tile;
}

HexagonTile::HexagonTile()
{
    m_type = "HexagonTile"; // for TGUI internal identification
    getRenderer()->setBackgroundColor(sf::Color::Transparent);
    getRenderer()->setBorders({0, 0, 0, 0});

    // Default fill color
    m_fillColor = sf::Color::Blue;
}

void HexagonTile::setFillColor(const sf::Color& color)
{
    m_fillColor = color;
}

void HexagonTile::draw(tgui::BackendRenderTarget& backendTarget, tgui::RenderStates states) const
{
    // 1) Create the hex shape
    //    - 6 sides, radius=100 => pointy-topped hex
    sf::CircleShape hex(100.f, 6);
    hex.setFillColor(m_fillColor);
    hex.setOutlineColor(sf::Color::Black);
    hex.setOutlineThickness(2.f);

    // 2) Scale it to fill our panel’s size
    sf::Vector2f panelSize = getSize(); // e.g., {150, 150}
    // Original bounding box of the shape is ~200×173, so scale to fit
    // By default we consider 160 to match a smaller portion of the bounding box
    hex.setScale({panelSize.x / 160.f, panelSize.y / 160.f});

    // 3) Center the shape in the panel
    hex.setOrigin({100.f, 100.f});
    sf::Vector2f absolutePos = getAbsolutePosition();
    hex.setPosition({absolutePos.x + (panelSize.x / 2.f), absolutePos.y + (panelSize.y / 2.f)});

    // 4) Draw to the underlying SFML render target
    auto&             backendSFML  = static_cast<tgui::BackendRenderTargetSFML&>(backendTarget);
    sf::RenderTarget* renderTarget = backendSFML.getTarget();
    if (renderTarget)
        renderTarget->draw(hex);

    // 5) Let TGUI draw our child widgets (labels, buttons, etc.)
    Panel::draw(backendTarget, states);
}
