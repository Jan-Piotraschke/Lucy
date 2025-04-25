// modules/mesh/components/loader/loader.cpp
#include "loader.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace mesh::loader
{

bool loadOFF2D(
    const std::string&                      file,
    std::vector<sf::Vector2f>&              verts,
    std::vector<std::vector<unsigned int>>& faces)
{
    std::ifstream in(file);
    if (!in)
        return false;

    std::string header;
    in >> header;
    if (header != "OFF")
        return false;

    size_t nv, nf, ne;
    in >> nv >> nf >> ne;
    verts.resize(nv);
    for (auto& v : verts)
    {
        float z;
        in >> v.x >> v.y >> z;
    }

    faces.clear();
    faces.reserve(nf);
    for (size_t i = 0; i < nf; ++i)
    {
        unsigned cnt;
        in >> cnt;
        std::vector<unsigned> f(cnt);
        for (auto& idx : f)
            in >> idx;
        faces.push_back(std::move(f));
    }
    return true;
}

bool loadOFF3D(
    const std::string&                      file,
    std::vector<sf::Vector3f>&              verts,
    std::vector<std::vector<unsigned int>>& faces)
{
    std::ifstream in(file);
    if (!in)
        return false;

    std::string header;
    in >> header;
    if (header != "OFF")
        return false;

    size_t nv, nf, ne;
    in >> nv >> nf >> ne;
    verts.resize(nv);
    for (auto& v : verts)
        in >> v.x >> v.y >> v.z;

    faces.clear();
    faces.reserve(nf);
    for (size_t i = 0; i < nf; ++i)
    {
        unsigned cnt;
        in >> cnt;
        std::vector<unsigned> f(cnt);
        for (auto& idx : f)
            in >> idx;
        faces.push_back(std::move(f));
    }
    return true;
}

bool loadCSV2D(const std::string& file, std::vector<sf::Vector2f>& pts)
{
    std::ifstream in(file);
    if (!in)
        return false;

    pts.clear();
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream ss(line);
        float              x, y;
        char               comma;
        if (ss >> x >> comma >> y)
        {
            pts.emplace_back(x, y);
        }
    }
    return true;
}

// Helper function to rotate and translate a 2D point
sf::Vector2f customRotate(const sf::Vector2f& pt, float angleDegrees, float shiftX, float shiftY)
{
    const float angleRadians = angleDegrees * static_cast<float>(M_PI) / 180.f;
    const float cosTheta     = std::cos(angleRadians);
    const float sinTheta     = std::sin(angleRadians);
    const float threshold    = 1e-6f;

    float x_prime = pt.x * cosTheta - pt.y * sinTheta;
    float y_prime = pt.x * sinTheta + pt.y * cosTheta;

    if (std::abs(x_prime) < threshold)
        x_prime = 0.f;
    if (std::abs(y_prime) < threshold)
        y_prime = 0.f;

    return {x_prime + shiftX, y_prime + shiftY};
}

void loadAllFramesFromFolder(
    const std::filesystem::path&            folder,
    std::vector<std::vector<sf::Vector2f>>& allFrames,
    std::vector<sf::Vector2f>&              currentPoints,
    bool&                                   dataLoadedFlag,
    size_t&                                 currentFrameIdx)
{
    std::vector<std::pair<int, std::filesystem::path>> files;
    std::regex                                         pattern("r_data_(\\d+)\\.csv");

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        std::smatch match;
        std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, match, pattern))
        {
            int idx = std::stoi(match[1]);
            files.emplace_back(idx, entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    allFrames.clear();

    for (const auto& [_, path] : files)
    {
        std::vector<sf::Vector2f> frame;
        if (loadCSV2D(path.string(), frame))
        {
            std::vector<sf::Vector2f> augmented;

            // Original
            augmented.insert(augmented.end(), frame.begin(), frame.end());

            // // Rotated & translated variants
            // for (const auto& pt : frame)
            //     augmented.push_back(customRotate(pt, 90.f, 0.f, 0.f));
            // for (const auto& pt : frame)
            //     augmented.push_back(customRotate(pt, 90.f, 2.f, 0.f));
            // for (const auto& pt : frame)
            //     augmented.push_back(customRotate(pt, 270.f, 0.f, 2.f));
            // for (const auto& pt : frame)
            //     augmented.push_back(customRotate(pt, 270.f, 0.f, 0.f));

            allFrames.push_back(std::move(augmented));
        }
    }

    std::cout << "Loaded " << allFrames.size() << " augmented frames.\n";
    if (!allFrames.empty())
    {
        currentPoints  = allFrames[0];
        dataLoadedFlag = true;
    }
    currentFrameIdx = 0;
}

bool loadCSV3D(const std::string& file, std::vector<sf::Vector3f>& pts)
{
    std::ifstream in(file);
    if (!in)
        return false;

    pts.clear();
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream ss(line);
        float              x, y, z;
        char               c1, c2;
        if (ss >> x >> c1 >> y >> c2 >> z) // supports “x,y,z”
            pts.emplace_back(x, y, z);
    }
    return true;
}

void loadAllFrames3DFromFolder(
    const std::filesystem::path&            folder,
    std::vector<std::vector<sf::Vector3f>>& allFrames,
    std::vector<sf::Vector3f>&              currentPoints,
    bool&                                   dataLoadedFlag,
    size_t&                                 currentFrameIdx)
{
    std::vector<std::pair<int, std::filesystem::path>> files;
    std::regex                                         pattern(R"(r_data_3D_(\d+)\.csv)");

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        std::smatch m;
        std::string name = entry.path().filename().string();
        if (std::regex_match(name, m, pattern))
            files.emplace_back(std::stoi(m[1]), entry.path());
    }
    std::sort(files.begin(), files.end());

    allFrames.clear();
    for (const auto& [_, p] : files)
    {
        std::vector<sf::Vector3f> frame;
        if (loadCSV3D(p.string(), frame))
            allFrames.push_back(std::move(frame));
    }

    std::cout << "Loaded " << allFrames.size() << " 3-D frames.\n";
    if (!allFrames.empty())
    {
        currentPoints  = allFrames[0];
        dataLoadedFlag = true;
    }
    currentFrameIdx = 0;
}

// ─── 1-D CSV with integers ───────────────────────────────────────────────
bool loadColorCodes(const std::string& file, std::vector<int>& codes)
{
    std::ifstream in(file);
    if (!in)
        return false;

    codes.clear();
    int value;
    while (in >> value) // one integer per line (optionally separated by whitespace)
        codes.push_back(value);

    return true;
}

// ─── Folder scan:  particles_color_<N>.csv  → one frame ──────────────────
void loadAllColorFramesFromFolder(
    const std::filesystem::path&   folder,
    std::vector<std::vector<int>>& allColorFrames,
    std::vector<int>&              currentCodes,
    bool&                          dataLoadedFlag,
    size_t&                        currentFrameIdx)
{
    std::vector<std::pair<int, std::filesystem::path>> files;
    std::regex                                         pattern(R"(particles_color_(\d+)\.csv)");

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        std::smatch       m;
        const std::string name = entry.path().filename().string();
        if (std::regex_match(name, m, pattern))
            files.emplace_back(std::stoi(m[1]), entry.path());
    }
    std::sort(files.begin(), files.end());

    allColorFrames.clear();
    for (const auto& [_, p] : files)
    {
        std::vector<int> frame;
        if (loadColorCodes(p.string(), frame))
            allColorFrames.push_back(std::move(frame));
    }

    std::cout << "Loaded " << allColorFrames.size() << " colour frames.\n";
    if (!allColorFrames.empty())
    {
        currentCodes    = allColorFrames.front();
        dataLoadedFlag  = true;
        currentFrameIdx = 0;
    }
}

} // namespace mesh::loader
