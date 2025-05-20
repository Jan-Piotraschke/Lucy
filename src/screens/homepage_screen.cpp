#include "homepage_screen.h"
#include "../modules/kamon_fourier/kamon_fourier.h"
#include "../modules/logs_report/logs_report.h"
#include "../modules/mesh/mesh.h"
#include "../modules/tile/hexagon_tile.h"

namespace RetroPalette
{
const sf::Color LightGray   = sf::Color(192, 192, 192);
const sf::Color Indigo      = sf::Color(0, 51, 102);
const sf::Color SpringGreen = sf::Color(142, 182, 148);
const sf::Color CoralRed    = sf::Color(208, 90, 110);
} // namespace RetroPalette

namespace HomepageScreen
{
tgui::Panel::Ptr createHomepagePanel(
    const sf::Vector2u&   windowSize,
    std::function<void()> onLogsClick,
    std::function<void()> onMeshClick,
    std::function<void()> onFourierClick,
    bool&                 modeOnlineRef,
    std::function<void()> onMenuClick,
    std::function<void()> onShutdownClick)
{
    auto panel = tgui::Panel::create({(float)windowSize.x, (float)windowSize.y});
    panel->setVisible(false);

    auto topBar = tgui::Panel::create({"100%", 50});
    topBar->setPosition(0, 0);
    topBar->getRenderer()->setBackgroundColor(RetroPalette::LightGray);
    panel->add(topBar);

    auto homeBtn = tgui::Button::create("<H>");
    homeBtn->setPosition(10, 10);
    homeBtn->setSize(40, 30);
    homeBtn->onPress([panel]() { panel->setVisible(true); });
    topBar->add(homeBtn);

    auto titleLabel = tgui::Label::create("Lucy");
    titleLabel->setTextSize(24);
    titleLabel->setPosition({"(&.width - width)/2", 10});
    titleLabel->getRenderer()->setTextColor(RetroPalette::Indigo);
    topBar->add(titleLabel);

    auto menuBtn = tgui::Button::create("[|||]");
    menuBtn->setPosition({"&.width - 50", 10});
    menuBtn->setSize(40, 30);
    menuBtn->onPress(onMenuClick);
    topBar->add(menuBtn);

    auto homeContent = tgui::Panel::create({(float)windowSize.x, (float)(windowSize.y - 50)});
    homeContent->setPosition(0, 50);
    panel->add(homeContent);

    auto logo = tgui::Picture::create("assets/img/kamon_pixelated.png");
    logo->setSize(180, 180);
    logo->setPosition({"(&.width - width)/2", 50});
    homeContent->add(logo);

    auto titleHome = tgui::Label::create("Welcome to Lucy");
    titleHome->setTextSize(32);
    titleHome->getRenderer()->setTextColor(RetroPalette::Indigo);
    titleHome->setPosition({"(&.width - width)/2", 250});
    homeContent->add(titleHome);

    auto modePanel = tgui::Panel::create({400, 40});
    modePanel->setPosition({"(&.width - width)/2", 300});
    homeContent->add(modePanel);

    auto modeLabel = tgui::Label::create("Let's work with Lucy in offline mode");
    modeLabel->setTextSize(20);
    modeLabel->setAutoSize(true);
    modeLabel->setPosition({0, 5});
    modePanel->add(modeLabel);

    auto modeCheck = tgui::CheckBox::create();
    modeCheck->setPosition({modeLabel->getSize().x + 10, 5});
    modeCheck->onChange(
        [modeCheck, modeLabel, &modeOnlineRef]()
        {
            modeOnlineRef    = modeCheck->isChecked();
            std::string text = "Let's work with Lucy in ";
            text += (modeOnlineRef ? "online" : "offline");
            text += " mode";
            modeLabel->setText(text);
            modeCheck->setPosition({modeLabel->getSize().x + 10, 5});
        });
    modePanel->add(modeCheck);

    auto logsHexTile = HexagonTile::create({150.f, 150.f}, RetroPalette::CoralRed);
    auto logsTile    = Logs::createLogsTile(logsHexTile, onLogsClick);
    logsTile->setPosition({"(&.width - width)/2", 370});
    homeContent->add(logsTile);

    auto meshHexTile = HexagonTile::create({150.f, 150.f}, RetroPalette::Indigo);
    auto meshTile    = Mesh::createMeshTile(meshHexTile, onMeshClick);
    meshTile->setPosition({"(&.width - width)/2", 530});
    homeContent->add(meshTile);

    auto fourierTile = KamonFourier::createFourierTile(onFourierClick);
    fourierTile->setPosition({"(&.width - width)/2", 710});
    homeContent->add(fourierTile);

    return panel;
}
} // namespace HomepageScreen
