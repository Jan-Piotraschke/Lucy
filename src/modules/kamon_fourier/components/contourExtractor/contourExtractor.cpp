#include "contourExtractor.h"

#include <SFML/System/Vector2.hpp>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace KamonFourier::ContourExtractor
{

std::vector<sf::Vector2f> extractLargestContour(const std::string& imagePath)
{
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
    if (img.empty())
    {
        std::cerr << "[KamonFourier] Failed to load image: " << imagePath << std::endl;
        return {};
    }

    cv::Mat thresh;
    cv::threshold(img, thresh, 127, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    if (contours.empty())
    {
        std::cerr << "[KamonFourier] No contours found.\n";
        return {};
    }

    auto largestIt = std::max_element(
        contours.begin(), contours.end(),
        [](const auto& a, const auto& b) { return cv::contourArea(a) < cv::contourArea(b); });

    std::vector<sf::Vector2f> points;
    points.reserve(largestIt->size());
    for (const auto& pt : *largestIt)
    {
        points.emplace_back(static_cast<float>(pt.x), static_cast<float>(-pt.y)); // flip Y
    }

    return points;
}

std::vector<sf::Vector2f> extractContourFromSVG(const std::string& svgPath)
{
    std::ifstream in(svgPath);
    if (!in.is_open())
    {
        std::cerr << "[KamonFourier] Failed to open SVG file: " << svgPath << "\n";
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    auto pathPos = content.find("<path");
    if (pathPos == std::string::npos)
    {
        std::cerr << "[KamonFourier] No <path> found in SVG.\n";
        return {};
    }

    auto dPos = content.find(" d=", pathPos);
    if (dPos == std::string::npos)
        dPos = content.find("d=", pathPos);
    if (dPos == std::string::npos)
    {
        std::cerr << "[KamonFourier] No d= attribute found in <path>.\n";
        return {};
    }

    auto firstQuote = content.find('"', dPos);
    auto secondQuote = content.find('"', firstQuote + 1);
    if (firstQuote == std::string::npos || secondQuote == std::string::npos)
    {
        std::cerr << "[KamonFourier] Malformed path d= string.\n";
        return {};
    }

    std::string pathData = content.substr(firstQuote + 1, secondQuote - (firstQuote + 1));

    std::vector<sf::Vector2f> points;
    auto skipSpaces = [](std::istringstream& iss)
    {
        while (iss && std::isspace(iss.peek())) iss.ignore();
    };

    sf::Vector2f currentPos(0.f, 0.f);
    std::istringstream iss(pathData);
    char cmd;

    while (iss >> cmd)
    {
        if (cmd == 'M')
        {
            float x, y;
            iss >> x; skipSpaces(iss); iss >> y; skipSpaces(iss);
            currentPos = {x, y};
            points.push_back(currentPos);
        }
        else if (cmd == 'L')
        {
            float x, y;
            while (iss >> x)
            {
                skipSpaces(iss);
                if (!(iss >> y)) break;
                currentPos = {x, y};
                points.push_back(currentPos);
                skipSpaces(iss);
                if (iss.peek() == 'M' || iss.peek() == 'C' || !iss.good())
                    break;
            }
        }
        else if (cmd == 'C')
        {
            const int NUM_SAMPLES = 30;
            float x1, y1, x2, y2, x3, y3;
            while (iss >> x1 >> y1 >> x2 >> y2 >> x3 >> y3)
            {
                for (int i = 1; i <= NUM_SAMPLES; i++)
                {
                    float t  = float(i) / NUM_SAMPLES;
                    float mt = 1.f - t;
                    float bx = mt * mt * mt * currentPos.x + 3 * mt * mt * t * x1 +
                               3 * mt * t * t * x2 + t * t * t * x3;
                    float by = mt * mt * mt * currentPos.y + 3 * mt * mt * t * y1 +
                               3 * mt * t * t * y2 + t * t * t * y3;
                    points.emplace_back(bx, by);
                }
                currentPos = {x3, y3};
                skipSpaces(iss);
                if (iss.peek() == 'M' || iss.peek() == 'L' || iss.peek() == 'C' || !iss.good())
                    break;
            }
        }
        else
        {
            std::cerr << "[KamonFourier] Unhandled SVG path command: " << cmd << "\n";
            break;
        }
    }

    if (points.size() < 2)
    {
        std::cerr << "[KamonFourier] No valid points extracted from SVG.\n";
        return {};
    }

    return points;
}

} // namespace KamonFourier::ContourExtractor
