#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System.hpp>
#include <TGUI/AllWidgets.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <iostream>
#include <string>

class WelcomeScreen
{
  public:
    WelcomeScreen(
        tgui::Gui&          gui,
        const sf::Vector2u& windowSize,
        const std::string&  message   = "Welcome to Lucy!",
        sf::Color           bgColor   = sf::Color(255, 255, 255),
        sf::Color           textColor = sf::Color(0, 51, 102))
        : m_gui(gui)
        , m_duration(4.25f)
        , m_fadeDuration(0.3f)
        , m_videoLoaded(false)
        , m_lastFrameTime(0.f)
        , m_frameInterval(1.0f / 30.f) // fallback FPS
    {
        m_panel = tgui::Panel::create(
            {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
        m_panel->getRenderer()->setBackgroundColor(bgColor);
        m_panel->getRenderer()->setOpacity(0.f);
        m_gui.add(m_panel);

        m_videoDisplay = tgui::Picture::create();
        m_videoDisplay->setPosition({"(&.width - width)/2", "20%"});
        m_panel->add(m_videoDisplay);

        m_videoCap.open("assets/welcome_animation.mp4");
        if (m_videoCap.isOpened() && m_videoCap.read(m_cvFrame) && !m_cvFrame.empty())
        {
            const unsigned int frameWidth  = m_cvFrame.cols;
            const unsigned int frameHeight = m_cvFrame.rows;

            m_videoDisplay->setSize(
                {static_cast<float>(frameWidth), static_cast<float>(frameHeight)});
            m_cvFrameRGBA.create(frameHeight, frameWidth, CV_8UC4);
            cv::cvtColor(m_cvFrame, m_cvFrameRGBA, cv::COLOR_BGR2RGBA);

            m_sfmlVideoTexture = sf::Texture(sf::Vector2u(frameWidth, frameHeight));
            m_sfmlVideoTexture.update(m_cvFrameRGBA.data);

            m_tguiVideoTexture.loadFromPixelData({frameWidth, frameHeight}, m_cvFrameRGBA.data);
            m_videoDisplay->getRenderer()->setTexture(m_tguiVideoTexture);
            m_videoLoaded = true;

            double fps = m_videoCap.get(cv::CAP_PROP_FPS);
            if (fps > 0.1)
            {
                m_frameInterval = 1.0 / fps;
            }

            m_videoCap.set(cv::CAP_PROP_POS_FRAMES, 0);
        }
        else
        {
            std::cerr << "Info: WelcomeScreen - Could not open or read video, falling back.\n";
            m_videoLoaded = false;
        }

        if (!m_videoLoaded)
        {
            tgui::Texture fallbackTexture("assets/kamon_fourier.png");
            if (fallbackTexture.getImageSize() != tgui::Vector2u{0, 0})
            {
                m_videoDisplay->getRenderer()->setTexture(fallbackTexture);
                tgui::Vector2u fbSize = fallbackTexture.getImageSize();
                m_videoDisplay->setSize(
                    {static_cast<float>(fbSize.x), static_cast<float>(fbSize.y)});
            }
            else
            {
                std::cerr << "Error: WelcomeScreen - Fallback image load failed.\n";
                m_videoDisplay->setSize(180.f, 180.f);
            }
        }

        m_label = tgui::Label::create(message);
        m_label->setTextSize(48);
        m_label->getRenderer()->setTextColor(textColor);
        m_label->setPosition({"(&.width - width)/2", "60%"});
        m_panel->add(m_label);

        m_panel->setVisible(true);
        m_clock.restart();
    }

    void update()
    {
        if (!m_panel || !m_panel->isVisible())
            return;

        const float t = m_clock.getElapsedTime().asSeconds();

        // Fade logic
        if (t < m_fadeDuration)
            m_panel->getRenderer()->setOpacity(t / m_fadeDuration);
        else if (t > m_duration - m_fadeDuration)
            m_panel->getRenderer()->setOpacity(std::max(0.f, (m_duration - t) / m_fadeDuration));
        else
            m_panel->getRenderer()->setOpacity(1.f);

        // Video playback
        if (m_videoLoaded && m_videoCap.isOpened() && t < m_duration)
        {
            float frameTime = t - m_lastFrameTime;
            if (frameTime >= m_frameInterval)
            {
                if (m_videoCap.read(m_cvFrame) && !m_cvFrame.empty())
                {
                    m_lastFrameTime = t;
                    cv::cvtColor(m_cvFrame, m_cvFrameRGBA, cv::COLOR_BGR2RGBA);

                    if (m_cvFrameRGBA.empty())
                        return;

                    m_sfmlVideoTexture.update(m_cvFrameRGBA.data);
                    m_tguiVideoTexture.loadFromPixelData(
                        {static_cast<unsigned>(m_cvFrameRGBA.cols),
                         static_cast<unsigned>(m_cvFrameRGBA.rows)},
                        m_cvFrameRGBA.data);
                    m_videoDisplay->getRenderer()->setTexture(m_tguiVideoTexture);
                }
                else
                {
                    // End of video or error
                    m_videoCap.set(cv::CAP_PROP_POS_FRAMES, 0);
                }
            }
        }

        if (t >= m_duration)
        {
            m_panel->setVisible(false);
            if (m_videoCap.isOpened())
                m_videoCap.release();
            if (m_panel->getParent())
                m_gui.remove(m_panel);
        }
    }

    [[nodiscard]] bool isActive() const
    {
        return m_panel && m_panel->isVisible();
    }

  private:
    tgui::Gui&         m_gui;
    tgui::Panel::Ptr   m_panel;
    tgui::Label::Ptr   m_label;
    tgui::Picture::Ptr m_videoDisplay;

    sf::Clock   m_clock;
    const float m_duration;
    const float m_fadeDuration;

    // Video
    cv::VideoCapture m_videoCap;
    cv::Mat          m_cvFrame;
    cv::Mat          m_cvFrameRGBA;
    sf::Texture      m_sfmlVideoTexture;
    tgui::Texture    m_tguiVideoTexture;

    bool  m_videoLoaded;
    float m_lastFrameTime;
    float m_frameInterval;
};
