#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System.hpp>
#include <TGUI/AllWidgets.hpp>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

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
        , m_duration(5.0f)
        , m_fadeDuration(0.3f)
        , m_animationDone(false)
        , m_lastFrameTime(0.f)
        , m_frameInterval(1.0f / 60.f) // animation frame rate
    {
        m_panel = tgui::Panel::create(
            {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
        m_panel->getRenderer()->setBackgroundColor(bgColor);
        m_panel->getRenderer()->setOpacity(0.f);
        m_gui.add(m_panel);

        m_videoDisplay = tgui::Picture::create();
        m_videoDisplay->setPosition({"(&.width - width)/2", "20%"});
        m_panel->add(m_videoDisplay);

        // Load input image
        cv::Mat grayImage = cv::imread("assets/kamon_fourier.png", cv::IMREAD_GRAYSCALE);
        if (grayImage.empty())
        {
            std::cerr << "Error: Could not load welcome image.\n";
            return;
        }

        cv::resize(grayImage, grayImage, cv::Size(160, 160), 0, 0, cv::INTER_NEAREST);
        m_blackMask = grayImage < 128;

        m_currentMask  = cv::Mat::zeros(m_blackMask.size(), CV_8U);
        int revealRows = static_cast<int>(0.6 * m_blackMask.rows);
        m_blackMask.rowRange(0, revealRows).copyTo(m_currentMask.rowRange(0, revealRows));

        m_cvFrameRGBA.create(160, 160, CV_8UC4);
        updateMaskTexture();

        m_videoDisplay->setSize({240, 240});

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
                    m_animationDone = true; // animation done
                }
                else
                {
                    m_currentMask.setTo(255, revealNow);
                    updateMaskTexture();
                }
            }
        }

        if (t >= m_duration)
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

  private:
    void updateMaskTexture()
    {
        // Create image: white background, black where revealed
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

    // Animation
    cv::Mat       m_blackMask;
    cv::Mat       m_currentMask;
    cv::Mat       m_cvFrameRGBA;
    sf::Texture   m_sfmlVideoTexture;
    tgui::Texture m_tguiVideoTexture;
    bool          m_animationDone;
    float         m_lastFrameTime;
    float         m_frameInterval;
};
