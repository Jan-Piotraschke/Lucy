// modules/mesh/components/loader/loader.cpp
#include "loader.h"

#include <algorithm>
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
            allFrames.push_back(std::move(frame));
    }

    std::cout << "Loaded " << allFrames.size() << " frames.\n";
    if (!allFrames.empty())
    {
        currentPoints  = allFrames[0];
        dataLoadedFlag = true;
    }
    currentFrameIdx = 0;
}

} // namespace mesh::loader
