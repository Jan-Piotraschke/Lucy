#pragma once

#include <SFML/System.hpp>
#include <TGUI/AllWidgets.hpp>

// ────────────────────────────────────────────────────────────────────────────────
// WelcomeScreen — a radically minimalistic startup sequence in retro spirit
// Displays a "Welcome to Lucy!" message and logo for 3 seconds (fade-in/out).
// ────────────────────────────────────────────────────────────────────────────────
class WelcomeScreen
{
  public:
    WelcomeScreen(
        tgui::Gui&          gui,
        const sf::Vector2u& windowSize,
        const std::string&  message   = "Welcome to Lucy!",
        sf::Color           bgColor   = sf::Color(255, 255, 255),
        sf::Color           textColor = sf::Color(0, 51, 102))
        : m_gui(gui), m_duration(3.f), m_fadeDuration(0.3f)
    {
        // Full-screen overlay panel
        m_panel = tgui::Panel::create(
            {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
        m_panel->getRenderer()->setBackgroundColor(bgColor);
        m_panel->getRenderer()->setOpacity(0.f); // Start transparent
        m_gui.add(m_panel);

        // Logo centered at top
        m_logo = tgui::Picture::create("assets/kamon_fourier.png");
        m_logo->setSize(180, 180);
        m_logo->setPosition({"(&.width - width)/2", "20%"});
        m_panel->add(m_logo);

        // Centered welcome label
        m_label = tgui::Label::create(message);
        m_label->setTextSize(48);
        m_label->getRenderer()->setTextColor(textColor);
        m_label->setPosition({"(&.width - width)/2", "60%"});
        m_panel->add(m_label);

        m_panel->setVisible(true);
    }

    void update()
    {
        if (!m_panel->isVisible())
            return;

        const float t = m_clock.getElapsedTime().asSeconds();

        if (t < m_fadeDuration)
        {
            m_panel->getRenderer()->setOpacity(t / m_fadeDuration);
        }
        else if (t > m_duration - m_fadeDuration)
        {
            m_panel->getRenderer()->setOpacity((m_duration - t) / m_fadeDuration);
        }
        else
        {
            m_panel->getRenderer()->setOpacity(1.f);
        }

        if (t >= m_duration)
        {
            m_panel->setVisible(false);
            m_gui.remove(m_panel);
        }
    }

    [[nodiscard]] bool isActive() const
    {
        return m_panel->isVisible();
    }

  private:
    tgui::Gui&         m_gui;
    tgui::Panel::Ptr   m_panel;
    tgui::Label::Ptr   m_label;
    tgui::Picture::Ptr m_logo;
    sf::Clock          m_clock;
    const float        m_duration;
    const float        m_fadeDuration;
};
