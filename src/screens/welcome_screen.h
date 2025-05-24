#pragma once

#include <SFML/Graphics.hpp>
#include <TGUI/AllWidgets.hpp>
#include <cmath>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>

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
        , m_duration(5.0f)
        , m_fadeDuration(0.3f)
        , m_animationDone(false)
        , m_sparkleStarted(false)
        , m_sparkleDuration(1.5f)
        , m_lastFrameTime(0.f)
        , m_frameInterval(1.0f / 60.f)
    {
        m_panel = tgui::Panel::create(
            {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
        m_panel->getRenderer()->setBackgroundColor(bgColor);
        m_panel->getRenderer()->setOpacity(0.f);
        m_gui.add(m_panel);

        // Central image (reveal)
        m_videoDisplay = tgui::Picture::create();
        m_videoDisplay->setPosition({"(&.width - width)/2", "20%"});
        m_panel->add(m_videoDisplay);

        // Load and prepare image
        cv::Mat grayImage = cv::imread("assets/img/kamon_fourier.png", cv::IMREAD_GRAYSCALE);
        if (grayImage.empty())
        {
            std::cerr << "Error: Could not load welcome image.\n";
            return;
        }

        cv::resize(grayImage, grayImage, cv::Size(160, 160), 0, 0, cv::INTER_NEAREST);
        m_blackMask    = grayImage < 128;
        m_currentMask  = cv::Mat::zeros(m_blackMask.size(), CV_8U);
        int revealRows = static_cast<int>(0.6 * m_blackMask.rows);
        m_blackMask.rowRange(0, revealRows).copyTo(m_currentMask.rowRange(0, revealRows));

        m_cvFrameRGBA.create(160, 160, CV_8UC4);
        updateMaskTexture();
        m_videoDisplay->setSize({240, 240});

        // Label
        m_label = tgui::Label::create(message);
        m_label->setTextSize(48);
        m_label->getRenderer()->setTextColor(textColor);
        m_label->setPosition({"(&.width - width)/2", "60%"});
        m_panel->add(m_label);

        // Load sparkle texture
        if (!m_sparkleTexture.loadFromFile("assets/img/sparkle.png"))
        {
            std::cerr << "Failed to load sparkle image!\n";
        }
        else
        {
            const size_t sparkleCount = 6;           // Quarter-circle sparkles
            const float  quarterArc   = M_PI / 2.0f; // 90 degrees

            for (size_t i = 0; i < sparkleCount; ++i)
            {
                sf::Sprite sprite(m_sparkleTexture);
                sprite.setOrigin(sf::Vector2f(m_sparkleTexture.getSize()) / 2.f);

                // Store angle offset in sprite rotation (reused)
                float angleOffset = static_cast<float>(i) / sparkleCount * quarterArc;
                sprite.setRotation(sf::radians(angleOffset));

                m_sparkles.emplace_back(sprite);
            }
        }

        m_panel->setVisible(true);
        m_clock.restart();
    }

    bool shouldRenderSparkles() const
    {
        float t = m_clock.getElapsedTime().asSeconds();
        return m_sparkleStarted && t >= m_duration - 1.8f && t < m_duration + m_sparkleDuration;
    }

    void update(sf::RenderTarget& target)
    {
        if (!m_panel || !m_panel->isVisible())
            return;

        float t = m_clock.getElapsedTime().asSeconds();

        // Fade logic
        if (t < m_fadeDuration)
            m_panel->getRenderer()->setOpacity(t / m_fadeDuration);
        else if (t > m_duration + m_sparkleDuration - m_fadeDuration)
            m_panel->getRenderer()->setOpacity(
                std::max(0.f, (m_duration + m_sparkleDuration - t) / m_fadeDuration));
        else
            m_panel->getRenderer()->setOpacity(1.f);

        if (!m_animationDone && t < m_duration)
        {
            float frameTime = t - m_lastFrameTime;
            if (frameTime >= m_frameInterval)
            {
                m_lastFrameTime = t;

                cv::Mat candidates, neighbours, revealNow;
                cv::bitwise_and(m_blackMask, ~m_currentMask, candidates);
                neighbours = getNeighbourVisiblePixels(m_currentMask);
                cv::bitwise_and(candidates, neighbours, revealNow);

                if (cv::countNonZero(revealNow) == 0)
                {
                    m_animationDone = true;
                    m_sparkleClock.restart();
                }
                else
                {
                    m_currentMask.setTo(255, revealNow);
                    updateMaskTexture();
                }
            }
        }

        if (m_animationDone && !m_sparkleStarted)
        {
            m_sparkleStarted = true;
            m_sparkleClock.restart();
        }

        if (m_sparkleStarted && t >= m_duration && t < m_duration + m_sparkleDuration)
        {
            renderSparkles();
        }

        if (t >= m_duration + m_sparkleDuration)
        {
            m_panel->setVisible(false);
            if (m_panel->getParent())
                m_gui.remove(m_panel);
        }
    }

    [[nodiscard]] bool isActive() const
    {
        return m_panel && m_panel->isVisible();
    }

    void renderSparkles()
    {
        float time   = m_sparkleClock.getElapsedTime().asSeconds();
        float radius = 140.0f;

        sf::Vector2f videoPos  = m_videoDisplay->getAbsolutePosition();
        sf::Vector2f videoSize = m_videoDisplay->getSize();
        sf::Vector2f center    = videoPos + videoSize / 2.f;

        auto* renderWindow = dynamic_cast<sf::RenderWindow*>(m_gui.getWindow());
        if (!renderWindow)
            return;

        for (auto& sparkle : m_sparkles)
        {
            float angleOffset = sparkle.getRotation().asRadians();
            float angle       = angleOffset + time * 3.0f;
            float x           = center.x + radius * std::cos(angle);
            float y           = center.y + radius * std::sin(angle);
            sparkle.setPosition({x, y});
            sparkle.setScale({0.8f, 0.8f});
            renderWindow->draw(sparkle);
        }
    }

  private:
    void updateMaskTexture()
    {
        cv::Mat frameGray(160, 160, CV_8U, cv::Scalar(255));
        frameGray.setTo(0, m_currentMask);

        cv::Mat rgba;
        cv::cvtColor(frameGray, rgba, cv::COLOR_GRAY2RGBA);
        m_sfmlVideoTexture.update(rgba.data);
        m_tguiVideoTexture.loadFromPixelData({160, 160}, rgba.data);
        m_videoDisplay->getRenderer()->setTexture(m_tguiVideoTexture);
    }

    cv::Mat getNeighbourVisiblePixels(const cv::Mat& mask)
    {
        cv::Mat up, down, left, right;
        cv::Mat result = cv::Mat::zeros(mask.size(), CV_8U);

        cv::copyMakeBorder(mask, up, 1, 0, 0, 0, cv::BORDER_CONSTANT, 0);
        up = up.rowRange(0, mask.rows);
        cv::copyMakeBorder(mask, down, 0, 1, 0, 0, cv::BORDER_CONSTANT, 0);
        down = down.rowRange(1, mask.rows + 1);
        cv::copyMakeBorder(mask, left, 0, 0, 1, 0, cv::BORDER_CONSTANT, 0);
        left = left.colRange(0, mask.cols);
        cv::copyMakeBorder(mask, right, 0, 0, 0, 1, cv::BORDER_CONSTANT, 0);
        right = right.colRange(1, mask.cols + 1);

        result |= up;
        result |= down;
        result |= left;
        result |= right;

        return result;
    }

  private:
    tgui::Gui&         m_gui;
    tgui::Panel::Ptr   m_panel;
    tgui::Label::Ptr   m_label;
    tgui::Picture::Ptr m_videoDisplay;

    sf::Clock   m_clock;
    const float m_duration;
    const float m_fadeDuration;
    float       m_lastFrameTime;
    float       m_frameInterval;
    bool        m_animationDone;

    // Sparkle animation
    sf::Texture             m_sparkleTexture;
    std::vector<sf::Sprite> m_sparkles;
    bool                    m_sparkleStarted;
    float                   m_sparkleDuration;
    sf::Clock               m_sparkleClock;

    // OpenCV
    cv::Mat       m_blackMask;
    cv::Mat       m_currentMask;
    cv::Mat       m_cvFrameRGBA;
    sf::Texture   m_sfmlVideoTexture;
    tgui::Texture m_tguiVideoTexture;
};
