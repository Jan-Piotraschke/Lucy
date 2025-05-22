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

#include "modules/kamon_fourier/kamon_fourier.h"
#include "modules/logs_report/logs_report.h"
#include "modules/mesh/mesh.h"
#include "modules/tile/hexagon_tile.h"
#include "modules/ai_inference/ModelProcessor.h"

#include "screens/welcome_screen.h"
#include "screens/homepage_screen.h"

enum class Screen
{
    Home,
    LogAnalysis,
    Mesh,
    KamonFourier
};

// Hide all container‑panels in a single call
static void hideAllScreens(const std::vector<tgui::Panel::Ptr>& containers)
{
    for (auto& c : containers)
        c->setVisible(false);
}

namespace RetroPalette
{
static const sf::Color LightGray     = sf::Color(192, 192, 192);
static const sf::Color DarkGray      = sf::Color(60, 60, 60);
static const sf::Color Indigo        = sf::Color(0, 51, 102);
static const sf::Color SpringGreen   = sf::Color(142, 182, 148);
static const sf::Color White         = sf::Color(255, 255, 255);
static const sf::Color PanelBg       = sf::Color(220, 220, 220);
static const sf::Color CrimsonRed    = sf::Color(184, 36, 49);
static const sf::Color BasaltGray    = sf::Color(45, 45, 45);
static const sf::Color RetroYellow   = sf::Color(252, 216, 0);
static const sf::Color SkyBlue       = sf::Color(112, 160, 201);
static const sf::Color TerminalGreen = sf::Color(0, 255, 0);
static const sf::Color DeepPurple    = sf::Color(68, 36, 116);
static const sf::Color SakuraPink    = sf::Color(254, 223, 225);
static const sf::Color DeepCrimson   = sf::Color(142, 53, 74);
static const sf::Color BlushRose     = sf::Color(232, 122, 144);
static const sf::Color CoralRed      = sf::Color(208, 90, 110);
static const sf::Color PlumRed       = sf::Color(225, 107, 140);
static const sf::Color RusticWine    = sf::Color(159, 53, 58);
static const sf::Color DustyBrown    = sf::Color(100, 54, 60);
static const sf::Color PaleBlush     = sf::Color(238, 169, 169);
} // namespace RetroPalette

static const unsigned WINDOW_WIDTH  = 900u;
static const unsigned WINDOW_HEIGHT = 1000u;

int main()
{
    // 0 - Model inference
    const std::string model_path = "assets/model/traced_model.pt";
    const std::string csv_output_path = "output.csv";
    const std::string image_output_path = "output_plot.png";

    ModelProcessor processor(model_path);

    if (!processor.run()) {
        std::cerr << "Failed to run model processing.\n";
        return -1;
    }

    std::cout << "Processing complete.\n";

    // ── 1) Window & GUI ───────────────────────────────────────
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)),
        "Let's make Lucy amazing!",
        sf::Style::Close);
    window.setFramerateLimit(60);

    tgui::Gui gui(window);
    gui.setFont("assets/font/ChicagoKare-Regular.ttf");

    // ── 2) Floating child‑windows (created first but *added* later)
    auto menuWindow    = tgui::ChildWindow::create("Menu");
    auto goodbyeWindow = tgui::ChildWindow::create("Goodbye");

    // ── 3) State variables ───────────────────────────────────
    Screen currentScreen = Screen::Home;

    bool showGoodbye = false;
    bool modeOnline  = false;
    bool loading     = false;

    sf::Clock      loadingClock;
    const sf::Time LOADING_DURATION = sf::seconds(2.f);

    // ── 4) Welcome screen ────────────────────────────────────
    WelcomeScreen welcome(gui, window.getSize());
    bool          welcomeHandled = false;

    // ── 5) Screen containers ─────────────────────────────────
    tgui::Panel::Ptr homeContainer;
    tgui::Panel::Ptr logAnalysisContainer;
    tgui::Panel::Ptr kamonFourierContainer;
    tgui::Panel::Ptr meshContainer;

    // Home screen (from new module)
    homeContainer = HomepageScreen::createHomepagePanel(
        window.getSize(),
        /* onLogsClick  */ [&]()
        {
            loading = true;
            loadingClock.restart();
            std::cout << "[LOGS] Start loading...\n";
        },
        /* onMeshClick  */ [&]()
        {
            currentScreen = Screen::Mesh;
            hideAllScreens({homeContainer, logAnalysisContainer, meshContainer, kamonFourierContainer});
            meshContainer->setVisible(true);
        },
        /* onFourierClick */ [&]()
        {
            currentScreen = Screen::KamonFourier;
            hideAllScreens({homeContainer, logAnalysisContainer, meshContainer, kamonFourierContainer});
            kamonFourierContainer->setVisible(true);
        },
        /* modeOnlineRef */ modeOnline,
        /* onMenuClick   */ [&]() { menuWindow->setVisible(true); },
        /* onShutdownClick */ [&]()
        {
            showGoodbye = true;
            menuWindow->setVisible(false);
        });

    logAnalysisContainer = Logs::createLogAnalysisContainer(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens({homeContainer, logAnalysisContainer, meshContainer, KamonFourier::getFourierPanel()});
            homeContainer->setVisible(true);
        });
    logAnalysisContainer->getRenderer()->setBackgroundColor(RetroPalette::PanelBg);
    logAnalysisContainer->setSize({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    logAnalysisContainer->setVisible(false);

    meshContainer = Mesh::createMeshContainer(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens({homeContainer, logAnalysisContainer, meshContainer, KamonFourier::getFourierPanel()});
            homeContainer->setVisible(true);
        });
    meshContainer->setSize({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    meshContainer->setVisible(false);

    kamonFourierContainer = KamonFourier::createKamonFourierContainer(
        [&]()
        {
            currentScreen = Screen::Home;
            hideAllScreens({homeContainer, logAnalysisContainer, meshContainer, kamonFourierContainer});
            homeContainer->setVisible(true);
        });
    kamonFourierContainer->setSize({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    kamonFourierContainer->setVisible(false);

    // ── 6) Add *panel* widgets first (they form the background) ─
    gui.add(homeContainer);
    gui.add(logAnalysisContainer);
    gui.add(meshContainer);
    gui.add(kamonFourierContainer);

    // ── 7) Now add the floating windows so they sit *on top* ──
    gui.add(menuWindow);
    gui.add(goodbyeWindow);

    // ── 8) Build Menu Window ─────────────────────────────────
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
    docBtn->onPress([]() { std::cout << "[Menu] Open Documentation.\n"; });
    menuPanel->add(docBtn);

    auto shutdownBtn = tgui::Button::create("Shutdown App");
    shutdownBtn->setPosition(10, 110);
    shutdownBtn->setSize(200, 30);
    shutdownBtn->getRenderer()->setBackgroundColor(RetroPalette::SpringGreen);
    shutdownBtn->onPress([
        &]()
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

    // ── 9) Build Goodbye Window ──────────────────────────────
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

    // ── 10) Main loop ────────────────────────────────────────
    while (window.isOpen())
    {
        // Event polling
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

        // Welcome screen logic
        if (!welcomeHandled)
        {
            welcome.update();
            if (!welcome.isActive())
            {
                homeContainer->setVisible(true);
                welcomeHandled = true;
            }
        }

        // Fake loading for LogAnalysis
        if (loading)
        {
            if (loadingClock.getElapsedTime() >= LOADING_DURATION)
            {
                loading       = false;
                currentScreen = Screen::LogAnalysis;
                hideAllScreens({homeContainer, logAnalysisContainer, meshContainer, kamonFourierContainer});
                logAnalysisContainer->setVisible(true);
                std::cout << "[LOGS] Done loading, switch to LogAnalysis screen.\n";
            }
        }

        goodbyeWindow->setVisible(showGoodbye);

        // Render
        window.clear(RetroPalette::LightGray);
        window.setView(window.getDefaultView());
        gui.draw();

        if (currentScreen == Screen::KamonFourier)
        {
            KamonFourier::updateAndDraw(window);
        }
        else if (currentScreen == Screen::Mesh)
        {
            Mesh::updateAndDraw(window);
        }

        window.display();
    }

    return 0;
}
