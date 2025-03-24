#include "logs_report.h"

// TGUI single-backend includes (for TGUI 1.8 + SFML 3.0)
#include <TGUI/AllWidgets.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>

namespace Logs
{

////////////////////////////////////////////////////////////
// 1) Create the tile content (no shape creation here)
////////////////////////////////////////////////////////////
tgui::Panel::Ptr createLogsTile(tgui::Panel::Ptr tile, const std::function<void()>& openCallback)
{
    // Add your labels/buttons just like a normal TGUI panel
    auto logsTitle = tgui::Label::create("Logs Report");
    logsTitle->setTextSize(18);
    logsTitle->getRenderer()->setTextColor(sf::Color::White);
    logsTitle->setPosition(10.f, 20.f);
    tile->add(logsTitle);

    auto logsDesc = tgui::Label::create("Generate Logs Report");
    logsDesc->setTextSize(14);
    logsDesc->getRenderer()->setTextColor(sf::Color::White);
    logsDesc->setPosition(10.f, 45.f);
    tile->add(logsDesc);

    auto openBtn = tgui::Button::create("OPEN");
    openBtn->setPosition(10.f, 80.f);
    openBtn->setSize(70.f, 30.f);
    openBtn->onPress(
        [openCallback]()
        {
            if (openCallback)
                openCallback();
        });
    tile->add(openBtn);

    return tile;
}

////////////////////////////////////////////////////////////
// 2) Create the "Log Analysis Container" panel/screen
////////////////////////////////////////////////////////////
tgui::Panel::Ptr createLogAnalysisContainer(const std::function<void()>& onBackHome)
{
    auto panel = tgui::Panel::create({"100%", "100%"});
    panel->setVisible(false);

    auto logAnalysisPanel = tgui::Panel::create({"100%", "100% - 50"});
    logAnalysisPanel->setPosition(0, 50);
    panel->add(logAnalysisPanel);

    auto logAnalysisTitle = tgui::Label::create("Logs Report Screen");
    logAnalysisTitle->setTextSize(24);
    logAnalysisTitle->getRenderer()->setTextColor(sf::Color(0, 51, 102));
    logAnalysisTitle->setPosition({"(&.width - width)/2", 200});
    logAnalysisPanel->add(logAnalysisTitle);

    auto logAnalysisDesc = tgui::Label::create("Logs report generated...");
    logAnalysisDesc->setPosition({"(&.width - width)/2", 250});
    logAnalysisPanel->add(logAnalysisDesc);

    auto backHomeBtn = tgui::Button::create("Back to Home");
    backHomeBtn->setPosition({"(&.width - width)/2", 320});
    backHomeBtn->onPress(
        [onBackHome]()
        {
            if (onBackHome)
                onBackHome();
        });
    logAnalysisPanel->add(backHomeBtn);

    return panel;
}

} // namespace Logs
