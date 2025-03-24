#include "kamon.h"

#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace
{
// The single instance of our kamon screen
tgui::Panel::Ptr g_kamonPanel = nullptr;

// Extract the largest contour from an image
std::vector<sf::Vector2f> extractLargestContour(const std::string& imagePath)
{
    // Load as grayscale
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
    if (img.empty())
    {
        std::cerr << "[OpenCV] Failed to load image: " << imagePath << std::endl;
        return {};
    }

    // Threshold (binary inverse)
    cv::Mat thresh;
    cv::threshold(img, thresh, 127, 255, cv::THRESH_BINARY_INV);

    // Find external contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    if (contours.empty())
    {
        std::cerr << "[OpenCV] No contours found in image: " << imagePath << std::endl;
        return {};
    }

    // Pick the largest contour
    auto largestIt = std::max_element(
        contours.begin(),
        contours.end(),
        [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2)
        { return cv::contourArea(c1) < cv::contourArea(c2); });

    // Convert to sf::Vector2f, flipping Y
    std::vector<sf::Vector2f> points;
    points.reserve(largestIt->size());
    for (const auto& pt : *largestIt)
    {
        points.emplace_back(
            static_cast<float>(pt.x),
            static_cast<float>(-pt.y) // flip Y
        );
    }

    return points;
}

// Normalize the points to fit inside [-1, 1] in both x/y
void normalizePoints(std::vector<sf::Vector2f>& points)
{
    if (points.empty())
        return;

    // Compute centroid
    double sumX = 0.0, sumY = 0.0;
    for (auto& p : points)
    {
        sumX += p.x;
        sumY += p.y;
    }
    double meanX = sumX / points.size();
    double meanY = sumY / points.size();

    // Shift so mean is (0, 0)
    for (auto& p : points)
    {
        p.x -= static_cast<float>(meanX);
        p.y -= static_cast<float>(meanY);
    }

    // Scale so all coords fit in [-1,1]
    double maxVal = 0.0;
    for (auto& p : points)
    {
        double mag = std::max(std::fabs(p.x), std::fabs(p.y));
        if (mag > maxVal)
            maxVal = mag;
    }
    if (maxVal > 0)
    {
        for (auto& p : points)
        {
            p.x /= static_cast<float>(maxVal);
            p.y /= static_cast<float>(maxVal);
        }
    }
}

// Create an SFML line-loop from the points, with optional scale/offset
sf::VertexArray createContourShapeInternal(
    const std::vector<sf::Vector2f>& points, float scale, sf::Vector2f offset, sf::Color color)
{
    sf::VertexArray shape(sf::PrimitiveType::LineStrip, points.size() + 1);

    for (size_t i = 0; i < points.size(); i++)
    {
        float x           = points[i].x * scale + offset.x;
        float y           = points[i].y * scale + offset.y;
        shape[i].position = {x, y};
        shape[i].color    = color;
    }

    // Close the contour (connect last to first)
    if (!points.empty())
    {
        shape[points.size()].position = shape[0].position;
        shape[points.size()].color    = color;
    }

    return shape;
}
} // end anonymous namespace

namespace Kamon
{
sf::VertexArray createKamonContourShape()
{
    static const std::string path   = "assets/kamon_fourier.png";
    auto                     points = extractLargestContour(path);
    normalizePoints(points);

    // Create shape (red line). Center at (450,350) in 900x700 window
    auto shape = createContourShapeInternal(points, 200.f, {450.f, 350.f}, sf::Color::Red);

    // Flip Y-axis to correct upside-down issue
    for (size_t i = 0; i < shape.getVertexCount(); ++i)
    {
        shape[i].position.y = 700.f - shape[i].position.y; // Adjust based on window height
    }

    return shape;
}

tgui::Panel::Ptr createKamonContainer(const std::function<void()>& onBackHome)
{
    g_kamonPanel = tgui::Panel::create({"100%", "100%"});

    // *** IMPORTANT: Make the panel transparent so we can see the SFML shape behind it ***
    g_kamonPanel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    // Create a sub-panel for content
    auto content = tgui::Panel::create({"100%", "100% - 50"});
    content->setPosition(0, 50);
    content->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
    g_kamonPanel->add(content);

    auto title = tgui::Label::create("Kamon Contour Screen");
    title->setTextSize(24);
    title->getRenderer()->setTextColor(sf::Color(0, 51, 102));
    title->setPosition({"(&.width - width)/2", 150});
    content->add(title);

    auto backBtn = tgui::Button::create("Back to Home");
    backBtn->setPosition({"(&.width - width)/2", 220});
    backBtn->onPress(
        [onBackHome]()
        {
            if (onBackHome)
                onBackHome();
        });
    content->add(backBtn);

    return g_kamonPanel;
}

tgui::Panel::Ptr createKamonTile(const std::function<void()>& openCallback)
{
    auto kamonTilePanel = tgui::Panel::create({300, 150});
    kamonTilePanel->getRenderer()->setBackgroundColor(sf::Color(0, 100, 0));
    kamonTilePanel->getRenderer()->setBorderColor(sf::Color::Black);
    kamonTilePanel->getRenderer()->setBorders({2, 2, 2, 2});

    auto kamonTitle = tgui::Label::create("Kamon");
    kamonTitle->setTextSize(18);
    kamonTitle->getRenderer()->setTextColor(sf::Color::White);
    kamonTitle->setPosition(10, 10);
    kamonTilePanel->add(kamonTitle);

    auto kamonDesc = tgui::Label::create("Show the kamon contour");
    kamonDesc->setTextSize(14);
    kamonDesc->getRenderer()->setTextColor(sf::Color::White);
    kamonDesc->setPosition(10, 40);
    kamonTilePanel->add(kamonDesc);

    auto openBtn = tgui::Button::create("OPEN");
    openBtn->setPosition(10, 80);
    openBtn->setSize(70, 30);
    openBtn->onPress(
        [openCallback]()
        {
            if (openCallback)
                openCallback();
        });
    kamonTilePanel->add(openBtn);

    return kamonTilePanel;
}

tgui::Panel::Ptr getKamonPanel()
{
    return g_kamonPanel;
}
} // namespace Kamon
