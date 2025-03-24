#pragma once

#include <TGUI/TGUI.hpp>
#include <functional>

namespace Logs
{
// Instead of creating the tile ourselves, we accept a tile (Panel) that
// has already been shaped/styled. We'll just add text labels and a button.
tgui::Panel::Ptr createLogsTile(tgui::Panel::Ptr tile, const std::function<void()>& openCallback);

// Create the "Log Analysis Container" panel/screen.
// The onBackHome callback is called when we press "Back to Home."
tgui::Panel::Ptr createLogAnalysisContainer(const std::function<void()>& onBackHome);
} // namespace Logs
