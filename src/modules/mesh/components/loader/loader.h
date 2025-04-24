#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace mesh::loader
{

bool loadOFF2D(
    const std::string&                      file,
    std::vector<sf::Vector2f>&              verts,
    std::vector<std::vector<unsigned int>>& faces);

bool loadOFF3D(
    const std::string&                      file,
    std::vector<sf::Vector3f>&              verts,
    std::vector<std::vector<unsigned int>>& faces);

bool loadCSV2D(const std::string& file, std::vector<sf::Vector2f>& pts);

bool loadCSV3D(const std::string& file, std::vector<sf::Vector3f>& pts);

void loadAllFrames3DFromFolder(
    const std::filesystem::path&            folder,
    std::vector<std::vector<sf::Vector3f>>& allFrames,
    std::vector<sf::Vector3f>&              currentPoints,
    bool&                                   dataLoadedFlag,
    size_t&                                 currentFrameIdx);

void loadAllFramesFromFolder(
    const std::filesystem::path&            folder,
    std::vector<std::vector<sf::Vector2f>>& allFrames,
    std::vector<sf::Vector2f>&              currentPoints,
    bool&                                   dataLoadedFlag,
    size_t&                                 currentFrameIdx);

} // namespace mesh::loader
