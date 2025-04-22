#pragma once

#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>

namespace KamonFourier::ContourExtractor
{
// Load the largest contour from a grayscale PNG image
std::vector<sf::Vector2f> extractLargestContour(const std::string& imagePath);

// Load a single SVG path (only M, L, C commands supported — absolute only)
std::vector<sf::Vector2f> extractContourFromSVG(const std::string& svgPath);
} // namespace KamonFourier::ContourExtractor
