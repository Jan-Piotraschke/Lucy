#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <TGUI/AllWidgets.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

#include "modules/kamon/kamon.h"
#include "modules/kamon_fourier/kamon_fourier.h"
#include "modules/logs_report/logs_report.h"
#include "modules/tile/hexagon_tile.h"

enum class Screen
{
    Home,
    LogAnalysis,
    Kamon,
    KamonFourier
};

// Hide all container-panels in a single call
static void hideAllScreens(const std::vector<tgui::Panel::Ptr>& containers)
{
    for (auto& c : containers)
        c->setVisible(false);
}

namespace RetroPalette
{
static const sf::Color LightGray     = sf::Color(192, 192, 192); // existing
static const sf::Color DarkGray      = sf::Color(60, 60, 60);    // existing
static const sf::Color Indigo        = sf::Color(0, 51, 102);    // Japan - traditional blue
static const sf::Color SpringGreen   = sf::Color(142, 182, 148); // existing
static const sf::Color White         = sf::Color(255, 255, 255); // existing
static const sf::Color PanelBg       = sf::Color(220, 220, 220); // existing
static const sf::Color CrimsonRed    = sf::Color(184, 36, 49);   // bold, historic red
static const sf::Color BasaltGray    = sf::Color(45, 45, 45);    // darker neutral than DarkGray
static const sf::Color RetroYellow   = sf::Color(252, 216, 0);
static const sf::Color SkyBlue       = sf::Color(112, 160, 201);
static const sf::Color TerminalGreen = sf::Color(0, 255, 0);
static const sf::Color DeepPurple    = sf::Color(68, 36, 116);
static const sf::Color SakuraPink    = sf::Color(254, 223, 225); // 桜
static const sf::Color DeepCrimson   = sf::Color(142, 53, 74);   // 蘇芳
static const sf::Color BlushRose     = sf::Color(232, 122, 144); // 薄紅
static const sf::Color CoralRed      = sf::Color(208, 90, 110);  // 今様
static const sf::Color PlumRed       = sf::Color(225, 107, 140); // 紅梅
static const sf::Color RusticWine    = sf::Color(159, 53, 58);   // 燕脂
static const sf::Color DustyBrown    = sf::Color(100, 54, 60);   // 桑染
static const sf::Color PaleBlush     = sf::Color(238, 169, 169); // 鴇
} // namespace RetroPalette

// Fixed window resolution
static const unsigned WINDOW_WIDTH  = 900u;
static const unsigned WINDOW_HEIGHT = 900u;

int main()
{
    // -----------------------------------------
    // 1) Create SFML window + TGUI GUI
    // -----------------------------------------
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)),
        "Let's make Lucy amazing!",
        sf::Style::Close);
    window.setFramerateLimit(60);

    tgui::Gui gui(window);
    gui.setFont("assets/PixelOperatorMono.ttf");

    // -----------------------------------------
    // 2) State variables
    // -----------------------------------------
    Screen currentScreen = Screen::Home;

    bool showMenu    = false;
    bool showGoodbye = false;
    bool modeOnline  = false;
    bool loading     = false;

    sf::Clock      loadingClock;
    const sf::Time LOADING_DURATION = sf::seconds(2.f);

    // -----------------------------------------
    // 3) Create main containers for each screen
    // -----------------------------------------
    // The home screen
    tgui::Panel::Ptr homeContainer =
        tgui::Panel::create({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    homeContainer->setVisible(true);

    tgui::Panel::Ptr logAnalysisContainer;
    tgui::Panel::Ptr kamonContainer;
    tgui::Panel::Ptr kamonFourierContainer;

    logAnalysisContainer = Logs::createLogAnalysisContainer(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer,
                 logAnalysisContainer,
                 Kamon::getKamonPanel(),
                 KamonFourier::getFourierPanel()});
            homeContainer->setVisible(true);
        });
    logAnalysisContainer->getRenderer()->setBackgroundColor(RetroPalette::PanelBg);
    logAnalysisContainer->setSize({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    logAnalysisContainer->setVisible(false);

    kamonContainer = Kamon::createKamonContainer(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer,
                 logAnalysisContainer,
                 kamonContainer,
                 KamonFourier::getFourierPanel()});
            homeContainer->setVisible(true);
        });
    kamonContainer->setSize({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    kamonContainer->setVisible(false);

    kamonFourierContainer = KamonFourier::createKamonFourierContainer(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            homeContainer->setVisible(true);
        });
    kamonFourierContainer->setSize({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    kamonFourierContainer->setVisible(false);

    // Add them to the GUI
    gui.add(homeContainer);
    gui.add(logAnalysisContainer);
    gui.add(kamonContainer);
    gui.add(kamonFourierContainer);

    // Child windows:
    auto menuWindow    = tgui::ChildWindow::create("Menu");
    auto goodbyeWindow = tgui::ChildWindow::create("Goodbye");
    gui.add(menuWindow);
    gui.add(goodbyeWindow);

    // -----------------------------------------
    // 4) Build your Home screen
    // -----------------------------------------
    auto topBar = tgui::Panel::create({"100%", 50});
    topBar->setPosition(0, 0);
    topBar->getRenderer()->setBackgroundColor(RetroPalette::LightGray);
    homeContainer->add(topBar);

    auto homeBtn = tgui::Button::create("<H>");
    homeBtn->setPosition(10, 10);
    homeBtn->setSize(40, 30);
    topBar->add(homeBtn);

    auto titleLabel = tgui::Label::create("Lucy");
    titleLabel->setTextSize(24);
    titleLabel->setPosition({"(&.width - width)/2", 10});
    titleLabel->getRenderer()->setTextColor(RetroPalette::Indigo);
    topBar->add(titleLabel);

    auto menuBtn = tgui::Button::create("[|||]");
    menuBtn->setPosition({"&.width - 50", 10});
    menuBtn->setSize(40, 30);
    topBar->add(menuBtn);

    homeBtn->onPress(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            homeContainer->setVisible(true);
        });

    menuBtn->onPress(
        [&]()
        {
            showMenu = true;
            menuWindow->setVisible(true);
        });

    // -----------------------------------------
    // 5) Build the Menu ChildWindow
    // -----------------------------------------
    menuWindow->setSize(300, 220);
    menuWindow->setPosition({"(parent.width - width)/2", "(parent.height - height)/2"});
    menuWindow->getRenderer()->setTitleBarHeight(30);
    menuWindow->getRenderer()->setTitleBarColor(RetroPalette::White);
    menuWindow->getRenderer()->setBackgroundColor(RetroPalette::PanelBg);
    menuWindow->setVisible(false);

    auto menuPanel = tgui::Panel::create({"100%", "100%"});
    menuWindow->add(menuPanel);

    auto sysStatusLabel = tgui::Label::create("SYSTEM STATUS\nCPU: 0.0%\nMEM: 0.0%");
    sysStatusLabel->setTextSize(16);
    sysStatusLabel->setPosition(10, 10);
    menuPanel->add(sysStatusLabel);

    auto docBtn = tgui::Button::create("Open Documentation");
    docBtn->setPosition(10, 70);
    docBtn->setSize(200, 30);
    docBtn->getRenderer()->setBackgroundColor(RetroPalette::SpringGreen);
    docBtn->onPress([&]() { std::cout << "[Menu] Open Documentation.\n"; });
    menuPanel->add(docBtn);

    auto shutdownBtn = tgui::Button::create("Shutdown App");
    shutdownBtn->setPosition(10, 110);
    shutdownBtn->setSize(200, 30);
    shutdownBtn->getRenderer()->setBackgroundColor(RetroPalette::SpringGreen);
    shutdownBtn->onPress(
        [&]()
        {
            showGoodbye = true;
            menuWindow->setVisible(false);
        });
    menuPanel->add(shutdownBtn);

    auto closeMenuBtn = tgui::Button::create("Close Menu");
    closeMenuBtn->setPosition(10, 150);
    closeMenuBtn->setSize(200, 30);
    closeMenuBtn->onPress([&]() { menuWindow->setVisible(false); });
    menuPanel->add(closeMenuBtn);

    // -----------------------------------------
    // 6) Build the Goodbye Window
    // -----------------------------------------
    goodbyeWindow->setSize(350, 150);
    goodbyeWindow->setPosition((WINDOW_WIDTH - 350) / 2, (WINDOW_HEIGHT - 150) / 2);
    goodbyeWindow->getRenderer()->setTitleBarHeight(30);
    goodbyeWindow->getRenderer()->setTitleBarColor(RetroPalette::White);
    goodbyeWindow->getRenderer()->setBackgroundColor(RetroPalette::PanelBg);
    goodbyeWindow->setTitle("Goodbye");
    goodbyeWindow->setVisible(false);

    auto goodbyePanel = tgui::Panel::create({"100%", "100%"});
    goodbyeWindow->add(goodbyePanel);

    auto goodbyeLabel = tgui::Label::create("Thanks for using the App!\nPlease close this window.");
    goodbyeLabel->setPosition(10, 10);
    goodbyePanel->add(goodbyeLabel);

    auto exitBtn = tgui::Button::create("Exit");
    exitBtn->setPosition(10, 80);
    exitBtn->onPress([&]() { window.close(); });
    goodbyePanel->add(exitBtn);

    // -----------------------------------------
    // 7) Build Home screen content
    // -----------------------------------------
    auto homeContent = tgui::Panel::create({(float)WINDOW_WIDTH, (float)(WINDOW_HEIGHT - 50)});
    homeContent->setPosition(0, 50);
    homeContainer->add(homeContent);

    // Center logo horizontally at y=50
    auto logo = tgui::Picture::create("assets/kamon.png");
    logo->setSize(180, 180);
    logo->setPosition({"(&.width - width)/2", 50});
    homeContent->add(logo);

    // Center main title horizontally at y=250
    auto titleHome = tgui::Label::create("Welcome to Lucy");
    titleHome->setTextSize(32);
    titleHome->getRenderer()->setTextColor(RetroPalette::Indigo);
    titleHome->setPosition({"(&.width - width)/2", 250});
    homeContent->add(titleHome);

    // Mode panel for label + checkbox
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
        [&]()
        {
            modeOnline       = modeCheck->isChecked();
            std::string text = "Let's work with Lucy in ";
            text += (modeOnline ? "online" : "offline");
            text += " mode";
            modeLabel->setText(text);
            modeCheck->setPosition({modeLabel->getSize().x + 10, 5});
        });
    modePanel->add(modeCheck);

    // "Logs" tile, centered horizontally at y=370
    auto logsHexTile = HexagonTile::create({150.f, 150.f}, RetroPalette::CoralRed);
    auto logsTile    = Logs::createLogsTile(
        logsHexTile,
        [&]()
        {
            // Trigger loading of logs
            loading = true;
            loadingClock.restart();
            std::cout << "[LOGS] Start loading...\n";
        });
    logsTile->setPosition({"(&.width - width)/2", 370});
    homeContent->add(logsTile);

    // "Kamon" tile, centered horizontally at y=530
    auto logsKamonHexTile = HexagonTile::create({150.f, 150.f}, RetroPalette::Indigo);
    auto kamonTile        = Kamon::createKamonTile(
        logsKamonHexTile,
        [&]()
        {
            currentScreen = Screen::Kamon;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            kamonContainer->setVisible(true);
        });
    kamonTile->setPosition({"(&.width - width)/2", 530});
    homeContent->add(kamonTile);

    // "Kamon Fourier" tile, centered horizontally at y=710
    auto fourierTile = KamonFourier::createFourierTile(
        [&]()
        {
            currentScreen = Screen::KamonFourier;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            kamonFourierContainer->setVisible(true);
        });
    fourierTile->setPosition({"(&.width - width)/2", 710});
    homeContent->add(fourierTile);

    // -----------------------------------------
    // 8) Main loop
    // -----------------------------------------
    while (window.isOpen())
    {
        while (true)
        {
            std::optional<sf::Event> optEvent = window.pollEvent();
            if (!optEvent.has_value())
                break;

            // Use the new SFML event approach
            if (optEvent->is<sf::Event::Closed>())
            {
                window.close();
                break;
            }

            gui.handleEvent(*optEvent);
        }
        if (!window.isOpen())
            break;

        // Check if we finished "loading" logs
        if (loading)
        {
            sf::Time elapsed = loadingClock.getElapsedTime();
            if (elapsed >= LOADING_DURATION)
            {
                loading       = false;
                currentScreen = Screen::LogAnalysis;
                hideAllScreens(
                    {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
                logAnalysisContainer->setVisible(true);
                std::cout << "[LOGS] Done loading, switch to LogAnalysis screen.\n";
            }
        }

        // Toggle "Goodbye" popup
        goodbyeWindow->setVisible(showGoodbye);

        window.clear(RetroPalette::LightGray);

        // If on KamonFourier screen, update/draw epicycles
        if (currentScreen == Screen::KamonFourier)
        {
            KamonFourier::updateAndDraw(window);
        }
        else if (currentScreen == Screen::Kamon)
        {
            // If on normal Kamon screen, draw the red contour
            sf::VertexArray kamonBorder = Kamon::createKamonContourShape();
            window.draw(kamonBorder);
        }

        gui.draw();
        window.display();
    }

    return 0;
}
