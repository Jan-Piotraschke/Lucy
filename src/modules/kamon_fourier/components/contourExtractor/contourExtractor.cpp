#include "contourExtractor.h"

#include <SFML/System/Vector2.hpp>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
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
        contours.begin(),
        contours.end(),
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
    // ---------- load SVG file ------------------------------------------------
    std::ifstream in(svgPath);
    if (!in.is_open())
    {
        std::cerr << "[KamonFourier] Failed to open SVG file: " << svgPath << '\n';
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // ---------- pull out the first path's ‘d’ attribute ----------------------
    const std::size_t pathPos = content.find("<path");
    if (pathPos == std::string::npos)
    {
        std::cerr << "[KamonFourier] No <path> element found in SVG.\n";
        return {};
    }
    std::size_t dPos = content.find(" d=", pathPos);
    if (dPos == std::string::npos)
        dPos = content.find("d=", pathPos);
    if (dPos == std::string::npos)
    {
        std::cerr << "[KamonFourier] No d= attribute in <path>.\n";
        return {};
    }

    const std::size_t firstQuote  = content.find('"', dPos);
    const std::size_t secondQuote = content.find('"', firstQuote + 1);
    if (firstQuote == std::string::npos || secondQuote == std::string::npos)
    {
        std::cerr << "[KamonFourier] Malformed path data string.\n";
        return {};
    }

    std::string pathData = content.substr(firstQuote + 1, secondQuote - firstQuote - 1);

    // Replace commas with spaces so we can treat both uniformly
    std::replace(pathData.begin(), pathData.end(), ',', ' ');

    // ---------- helpers ------------------------------------------------------
    auto skipDelim = [](std::istringstream& iss)
    {
        while (iss && (std::isspace(iss.peek()) || iss.peek() == ','))
            iss.ignore();
    };

    // ---------- main parse loop ---------------------------------------------
    std::vector<sf::Vector2f> points;
    sf::Vector2f              currentPos(0.f, 0.f);
    char                      cmd = 0; // current command (persists if coords repeat)

    std::istringstream iss(pathData);
    while (iss)
    {
        skipDelim(iss);
        if (!iss.good())
            break;

        // If next char is a letter, that's the new command
        if (std::isalpha(iss.peek()))
        {
            iss >> cmd;
            skipDelim(iss);
        }
        if (!cmd)
            break; // malformed data

        const bool relative = std::islower(cmd);
        const char op       = static_cast<char>(std::toupper(cmd));

        // ---------- M / m  (move-to) ----------------------------------------
        if (op == 'M')
        {
            float x, y;
            if (!(iss >> x >> y))
                break;

            if (relative)
            {
                x += currentPos.x;
                y += currentPos.y;
            }
            currentPos = {x, y};
            points.push_back(currentPos);

            // Any additional coord‐pairs after an M/m are treated as L/l
            cmd = relative ? 'l' : 'L';
        }
        // ---------- L / l  (line-to) ----------------------------------------
        else if (op == 'L')
        {
            float x, y;
            while (iss >> x >> y)
            {
                if (relative)
                {
                    x += currentPos.x;
                    y += currentPos.y;
                }
                currentPos = {x, y};
                points.push_back(currentPos);

                skipDelim(iss);
                if (std::isalpha(iss.peek()))
                    break; // next explicit command
            }
        }
        // ---------- C / c  (cubic bézier) -----------------------------------
        else if (op == 'C')
        {
            const int NUM_SAMPLES = 30;
            float     x1, y1, x2, y2, x3, y3;
            while (iss >> x1 >> y1 >> x2 >> y2 >> x3 >> y3)
            {
                if (relative)
                {
                    x1 += currentPos.x;
                    y1 += currentPos.y;
                    x2 += currentPos.x;
                    y2 += currentPos.y;
                    x3 += currentPos.x;
                    y3 += currentPos.y;
                }
                // Sample the curve
                for (int i = 1; i <= NUM_SAMPLES; ++i)
                {
                    const float t  = static_cast<float>(i) / NUM_SAMPLES;
                    const float mt = 1.f - t;
                    const float bx = mt * mt * mt * currentPos.x + 3 * mt * mt * t * x1
                                     + 3 * mt * t * t * x2 + t * t * t * x3;
                    const float by = mt * mt * mt * currentPos.y + 3 * mt * mt * t * y1
                                     + 3 * mt * t * t * y2 + t * t * t * y3;
                    points.emplace_back(bx, by);
                }
                currentPos = {x3, y3};

                skipDelim(iss);
                if (std::isalpha(iss.peek()))
                    break;
            }
        }
        // ---------- Z / z  (close path) -------------------------------------
        else if (op == 'Z')
        {
            if (!points.empty())
                points.push_back(points.front()); // optional: close contour
            cmd = 0;                              // force consumption of next explicit command
        }
        // ---------- unsupported --------------------------------------------
        else
        {
            std::cerr << "[KamonFourier] Unhandled SVG path command: " << cmd << '\n';
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
