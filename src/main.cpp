#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

// TGUI single-backend includes
#include <TGUI/AllWidgets.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

// Existing modules
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

struct Palette
{
    static constexpr tgui::Color Spring_Green = {0x8E, 0xB6, 0x94}; // #8EB694
};

int main()
{
    // -----------------------------------------
    // 1) Create SFML window + TGUI GUI
    // -----------------------------------------
    sf::RenderWindow window(sf::VideoMode({1000u, 1000u}), "Let's make Lucy amazing!");
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
    tgui::Panel::Ptr homeContainer;
    homeContainer = tgui::Panel::create({"100%", "100%"});

    auto menuWindow    = tgui::ChildWindow::create("Menu");
    auto goodbyeWindow = tgui::ChildWindow::create("Goodbye");

    tgui::Panel::Ptr logAnalysisContainer;
    logAnalysisContainer = Logs::createLogAnalysisContainer(
        [&]()
        {
            // onBackHome
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer,
                 logAnalysisContainer,
                 Kamon::getKamonPanel(),
                 KamonFourier::getFourierPanel()});
            homeContainer->setVisible(true);
        });

    tgui::Panel::Ptr kamonContainer;
    kamonContainer = Kamon::createKamonContainer(
        [&]()
        {
            // onBackHome
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer,
                 logAnalysisContainer,
                 kamonContainer,
                 KamonFourier::getFourierPanel()});
            homeContainer->setVisible(true);
        });

    tgui::Panel::Ptr kamonFourierContainer;
    kamonFourierContainer = KamonFourier::createKamonFourierContainer(
        [&]()
        {
            // onBackHome
            currentScreen = Screen::Home;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            homeContainer->setVisible(true);
        });

    gui.add(homeContainer);
    gui.add(logAnalysisContainer);
    gui.add(kamonContainer);
    gui.add(kamonFourierContainer);
    gui.add(menuWindow);
    gui.add(goodbyeWindow);

    homeContainer->setVisible(true);
    logAnalysisContainer->setVisible(false);
    kamonContainer->setVisible(false);
    kamonFourierContainer->setVisible(false);

    // -----------------------------------------
    // 4) Build your Home screen
    // -----------------------------------------
    auto topBar = tgui::Panel::create({"100%", 50});
    topBar->getRenderer()->setBackgroundColor(sf::Color(240, 240, 240));
    homeContainer->add(topBar);

    auto homeBtn = tgui::Button::create("<H>");
    homeBtn->setPosition(10, 10);
    homeBtn->setSize(40, 30);
    topBar->add(homeBtn);

    auto titleLabel = tgui::Label::create("Lucy");
    titleLabel->setTextSize(24);
    titleLabel->setPosition({"(&.width - width)/2", 10});
    titleLabel->getRenderer()->setTextColor(sf::Color(0, 51, 102));
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

    // -----------------------------
    // 5) Build the Menu ChildWindow
    // -----------------------------
    menuWindow->setSize(300, 220);
    menuWindow->setPosition({"(parent.width - width)/2", "(parent.height - height)/2"});
    menuWindow->getRenderer()->setTitleBarHeight(30);
    menuWindow->getRenderer()->setTitleBarColor(sf::Color(255, 255, 255));
    menuWindow->getRenderer()->setBackgroundColor(sf::Color(220, 220, 220));
    menuWindow->setVisible(false);
    menuWindow->setKeepInParent(true);

    auto menuLayout = tgui::VerticalLayout::create();
    menuLayout->setSize("100% - 10", "100% - 10");
    menuLayout->setPosition(5, 5);
    menuWindow->add(menuLayout);

    auto sysStatusLabel = tgui::Label::create("SYSTEM STATUS\nCPU: 0.0%\nMEM: 0.0%");
    sysStatusLabel->setTextSize(16);
    menuLayout->add(sysStatusLabel);

    auto docBtn = tgui::Button::create("Open Documentation");
    docBtn->getRenderer()->setBackgroundColor(Palette::Spring_Green);
    menuLayout->add(docBtn);

    auto shutdownBtn = tgui::Button::create("Shutdown App");
    shutdownBtn->getRenderer()->setBackgroundColor(Palette::Spring_Green);
    menuLayout->add(shutdownBtn);

    auto closeMenuBtn = tgui::Button::create("Close Menu");
    menuLayout->add(closeMenuBtn);

    docBtn->onPress([&]() { std::cout << "[Menu] Open Documentation (shell-open a PDF here).\n"; });

    shutdownBtn->onPress(
        [&]()
        {
            showGoodbye = true;
            menuWindow->setVisible(false);
        });

    closeMenuBtn->onPress([&]() { menuWindow->setVisible(false); });

    // -----------------------------
    // 6) Build the Goodbye Window
    // -----------------------------
    goodbyeWindow->setSize(350, 150);
    goodbyeWindow->setPosition({"(parent.width - width)/2", "(parent.height - height)/2"});
    goodbyeWindow->getRenderer()->setTitleBarHeight(30);
    goodbyeWindow->setTitle("Goodbye");
    goodbyeWindow->setVisible(false);

    auto goodbyePanel = tgui::Panel::create({"100%", "100%"});
    goodbyeWindow->add(goodbyePanel);

    auto goodbyeLabel = tgui::Label::create("Thanks for using the App!\nPlease close this window.");
    goodbyeLabel->setPosition(10, 10);
    goodbyePanel->add(goodbyeLabel);

    auto exitBtn = tgui::Button::create("Exit");
    exitBtn->setPosition(10, 80);
    goodbyePanel->add(exitBtn);

    exitBtn->onPress([&]() { window.close(); });

    // -----------------------------
    // 7) Build Home screen content
    // -----------------------------
    auto homeContent = tgui::Panel::create({"100%", "100% - 50"});
    homeContent->setPosition(0, 50);
    homeContainer->add(homeContent);

    auto logo = tgui::Picture::create("assets/kamon.png");
    logo->setSize(180, 180);
    logo->setPosition({"(&.width - width)/2", 50});
    homeContent->add(logo);

    auto titleHome = tgui::Label::create("Welcome to Lucy");
    titleHome->setTextSize(32);
    titleHome->getRenderer()->setTextColor(sf::Color(0, 51, 102));
    titleHome->setPosition({"(&.width - width)/2", 250});
    homeContent->add(titleHome);

    auto modeLabel = tgui::Label::create("Let's work with Lucy in offline mode");
    modeLabel->setTextSize(20);
    modeLabel->setPosition({"(&.width - width)/2 - 50", 300});
    homeContent->add(modeLabel);

    auto modeCheck = tgui::CheckBox::create();
    modeCheck->setPosition({"(&.width - width)/2 + 180", 297});
    modeCheck->onChange(
        [&]()
        {
            modeOnline       = modeCheck->isChecked();
            std::string text = "Let's work with Lucy in ";
            text += (modeOnline ? "online" : "offline");
            text += " mode";
            modeLabel->setText(text);
        });
    homeContent->add(modeCheck);

    // "Logs" tile: we create the hex shape first, then let logs_report populate it.
    auto logsHexTile = HexagonTile::create({150.f, 150.f}, sf::Color(0, 51, 102));
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

    // "Kamon" tile
    auto kamonTile = Kamon::createKamonTile(
        [&]()
        {
            currentScreen = Screen::Kamon;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            kamonContainer->setVisible(true);
        });
    kamonTile->setPosition({"(&.width - width)/2", 530});
    homeContent->add(kamonTile);

    // "Kamon Fourier" tile
    auto fourierTile = KamonFourier::createFourierTile(
        [&]()
        {
            currentScreen = Screen::KamonFourier;
            hideAllScreens(
                {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
            kamonFourierContainer->setVisible(true);
        });
    // Place below the Kamon tile
    fourierTile->setPosition({"(&.width - width)/2", 530 + 180});
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

            if (optEvent->is<sf::Event::Closed>())
            {
                window.close();
                break;
            }
            gui.handleEvent(*optEvent);
        }
        if (!window.isOpen())
            break;

        // Check if we're "loading" logs
        if (loading)
        {
            sf::Time elapsed = loadingClock.getElapsedTime();
            if (elapsed >= LOADING_DURATION)
            {
                loading = false;
                // Show logs screen
                currentScreen = Screen::LogAnalysis;
                hideAllScreens(
                    {homeContainer, logAnalysisContainer, kamonContainer, kamonFourierContainer});
                logAnalysisContainer->setVisible(true);
                std::cout << "[LOGS] Done loading, switch to LogAnalysis screen.\n";
            }
        }

        goodbyeWindow->setVisible(showGoodbye);

        window.clear(sf::Color(240, 240, 240));

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
