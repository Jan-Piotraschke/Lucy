#include "mesh.h"
#include "components/loader/loader.h"

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <unordered_set>

namespace fs = std::filesystem;
using namespace mesh::loader;

// ────────────────────────────────
//   ▌  Internal, file‑local state
// ────────────────────────────────
namespace
{

std::vector<sf::Vector2f>              verts2;
std::vector<std::vector<unsigned int>> faces2;
sf::VertexArray                        mesh2;
sf::VertexArray                        edges2;
bool                                   mesh2Loaded = false;

std::vector<sf::Vector3f>              verts3;
std::vector<std::vector<unsigned int>> faces3;
sf::VertexArray                        mesh3;
sf::VertexArray                        edges3;
bool                                   mesh3Loaded = false;
float                                  radius3     = 1.f;

std::vector<std::vector<sf::Vector2f>> allFrames;
std::vector<sf::Vector2f>              dataPoints2;
bool                                   data2Loaded     = false;
size_t                                 currentFrameIdx = 0;
bool                                   playing         = false;
sf::Clock                              frameClock;

std::vector<std::vector<sf::Vector3f>> allFrames3;
std::vector<sf::Vector3f>              dataPoints3;
bool                                   data3Loaded = false;

float angle      = 0.f;
int   lastMouseX = 0;
bool  dragging   = false;

using Edge = std::pair<unsigned, unsigned>;
struct EdgeHash
{
    size_t operator()(const Edge& e) const noexcept
    {
        unsigned long long a = std::min(e.first, e.second);
        unsigned long long b = std::max(e.first, e.second);
        return (a + b) * (a + b + 1) / 2 + b;
    }
};
using EdgeSet = std::unordered_set<Edge, EdgeHash>;

} // namespace

// ────────────────────────────────
//   ▌  UI‑building helpers
// ────────────────────────────────
tgui::Panel::Ptr Mesh::createMeshContainer(std::function<void()> goBackCallback)
{
    auto panel = tgui::Panel::create();
    panel->setSize({"100%", "100%"});
    panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    auto backBtn = tgui::Button::create("< Back");
    backBtn->setSize(100, 30);
    backBtn->setPosition(10, 10);
    backBtn->onPress(goBackCallback);
    panel->add(backBtn);

    auto startBtn = tgui::Button::create("Start");
    startBtn->setSize(100, 30);
    startBtn->setPosition(120, 10);
    startBtn->onPress(
        []
        {
            if (!allFrames.empty())
            {
                playing = true;
                frameClock.restart();
                std::cout << "Animation started.\n";
            }
        });
    panel->add(startBtn);

    // After the Start button:
    auto resetBtn = tgui::Button::create("Reset");
    resetBtn->setSize(100, 30);
    resetBtn->setPosition(230, 10);
    resetBtn->onPress(
        []
        {
            // stop playback
            playing = false;

            // rewind to first frame (if any)
            currentFrameIdx = 0;
            if (!allFrames.empty())
                dataPoints2 = allFrames[0];

            // optionally reset other runtime state
            frameClock.restart();
            angle = 0.f;

            std::cout << "Animation reset.\n";
        });
    panel->add(resetBtn);

    fs::path base      = fs::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    fs::path kachel    = base / "meshes" / "kachelmuster.off";
    fs::path ellipsoid = base / "meshes" / "ellipsoid.off";

    if (!mesh2Loaded && fs::exists(kachel))
    {
        mesh2  = sf::VertexArray(sf::PrimitiveType::Triangles);
        edges2 = sf::VertexArray(sf::PrimitiveType::Lines);
        if (loadOFF2D(kachel.string(), verts2, faces2))
        {
            EdgeSet used;
            for (const auto& f : faces2)
            {
                for (size_t i = 1; i + 1 < f.size(); ++i)
                {
                    mesh2.append({verts2[f[0]], sf::Color::White});
                    mesh2.append({verts2[f[i]], sf::Color::White});
                    mesh2.append({verts2[f[i + 1]], sf::Color::White});
                }
                for (size_t i = 0; i < f.size(); ++i)
                {
                    Edge e{
                        std::min(f[i], f[(i + 1) % f.size()]),
                        std::max(f[i], f[(i + 1) % f.size()])};
                    if (used.insert(e).second)
                    {
                        edges2.append({verts2[f[i]], sf::Color::Black});
                        edges2.append({verts2[f[(i + 1) % f.size()]], sf::Color::Black});
                    }
                }
            }
            mesh2Loaded = true;
            std::cout << "Loaded 2D mesh: " << kachel << '\n';
        }
    }

    if (!mesh3Loaded && fs::exists(ellipsoid))
    {
        if (loadOFF3D(ellipsoid.string(), verts3, faces3))
        {
            radius3 = 0.f;
            for (const auto& v : verts3)
                radius3 = std::max(radius3, std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
            mesh3Loaded = true;
            std::cout << "Loaded 3D mesh: " << ellipsoid << '\n';
        }
    }

    fs::path dataFolder = base / "src" / "modules" / "2DTissue" / "data";
    if (fs::exists(dataFolder))
    {
        loadAllFramesFromFolder(dataFolder, allFrames, dataPoints2, data2Loaded, currentFrameIdx);
        loadAllFrames3DFromFolder(
            dataFolder, allFrames3, dataPoints3, data3Loaded, currentFrameIdx);
    }

    return panel;
}

tgui::Panel::Ptr Mesh::createMeshTile(
    tgui::Panel::Ptr tile, const std::function<void()>& openCallback)
{
    auto kamonTitle = tgui::Label::create("2DTissue");
    kamonTitle->setTextSize(18);
    kamonTitle->getRenderer()->setTextColor(sf::Color::White);
    kamonTitle->setPosition(10, 10);
    tile->add(kamonTitle);

    auto kamonDesc = tgui::Label::create("Show the mesh");
    kamonDesc->setTextSize(14);
    kamonDesc->getRenderer()->setTextColor(sf::Color::White);
    kamonDesc->setPosition(10, 40);
    tile->add(kamonDesc);

    auto openBtn = tgui::Button::create("OPEN");
    openBtn->setPosition(10, 80);
    openBtn->setSize(70, 30);
    openBtn->onPress(
        [openCallback]()
        {
            if (openCallback)
                openCallback();
        });
    tile->add(openBtn);

    return tile;
}

// ────────────────────────────────
//   ▌  Runtime drawing
// ────────────────────────────────
void Mesh::updateAndDraw(sf::RenderWindow& window)
{
    // Animate CSV frames if playing
    if (playing && frameClock.getElapsedTime().asSeconds() > 0.01f)
    {
        frameClock.restart();
        if (currentFrameIdx + 1 < allFrames.size())
        {
            currentFrameIdx++;
            dataPoints2 = allFrames[currentFrameIdx];
            dataPoints3 = allFrames3[currentFrameIdx];
        }
        else
        {
            playing = false;
        }
    }

    constexpr float WIN_W    = 900.f;
    constexpr float WIN_H    = 900.f;
    constexpr float TOP_FRAC = 0.75f;
    constexpr float TOP_H    = WIN_H * TOP_FRAC;
    constexpr float BOTTOM_H = WIN_H - TOP_H;

    // Mouse drag rotation
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)
        && sf::Mouse::getPosition(window).y > TOP_H)
    {
        int dx = sf::Mouse::getPosition(window).x - lastMouseX;
        angle += dx * 0.005f;
        dragging = true;
    }
    else
        dragging = false;
    lastMouseX = sf::Mouse::getPosition(window).x;

    // Draw 2D mesh
    if (mesh2Loaded)
    {
        const auto b     = mesh2.getBounds();
        const auto sz    = b.size;
        float      scale = std::min(WIN_W / sz.x, TOP_H / sz.y) * 0.9f;

        sf::Transform tr;
        tr.translate({WIN_W * 0.5f, TOP_H * 0.5f});
        tr.scale({scale, -scale});
        tr.translate({-(b.position.x + sz.x * 0.5f), -(b.position.y + sz.y * 0.5f)});

        window.draw(mesh2, tr);
        window.draw(edges2, tr);

        // Draw CSV data points
        if (data2Loaded)
        {
            sf::CircleShape pt(2.f);
            pt.setFillColor(sf::Color::Red);
            pt.setOrigin({3.f, 3.f});
            for (const auto& p : dataPoints2)
            {
                pt.setPosition(tr.transformPoint(p));
                window.draw(pt);
            }
        }
    }

    // Draw 3D mesh
    if (mesh3Loaded)
    {
        const float        c = std::cos(angle), s = std::sin(angle);
        const float        scale = (std::min(WIN_W, BOTTOM_H) * 0.45f) / radius3;
        const sf::Vector2f centre{WIN_W * 0.5f, TOP_H + BOTTOM_H * 0.5f};

        std::vector<sf::Vector2f> proj(verts3.size());
        for (size_t i = 0; i < verts3.size(); ++i)
        {
            float x = verts3[i].x * c - verts3[i].z * s;
            float z = verts3[i].x * s + verts3[i].z * c;
            float y = verts3[i].y;
            proj[i] = {centre.x + x * scale, centre.y - y * scale};
        }

        mesh3.clear();
        mesh3.setPrimitiveType(sf::PrimitiveType::Triangles);
        edges3.clear();
        edges3.setPrimitiveType(sf::PrimitiveType::Lines);
        EdgeSet used;
        for (const auto& f : faces3)
        {
            for (size_t i = 1; i + 1 < f.size(); ++i)
            {
                mesh3.append({proj[f[0]], sf::Color(200, 200, 200)});
                mesh3.append({proj[f[i]], sf::Color(200, 200, 200)});
                mesh3.append({proj[f[i + 1]], sf::Color(200, 200, 200)});
            }
            for (size_t i = 0; i < f.size(); ++i)
            {
                Edge e{
                    std::min(f[i], f[(i + 1) % f.size()]), std::max(f[i], f[(i + 1) % f.size()])};
                if (used.insert(e).second)
                {
                    edges3.append({proj[f[i]], sf::Color::Black});
                    edges3.append({proj[f[(i + 1) % f.size()]], sf::Color::Black});
                }
            }
        }
        window.draw(mesh3);
        window.draw(edges3);

        // Draw 3-D CSV points
        if (data3Loaded)
        {
            sf::CircleShape dot(1.f);
            dot.setOrigin({3.f, 3.f});
            dot.setFillColor(sf::Color::Red);

            for (const auto& v : dataPoints3)
            {
                // same rotation + projection that we used for verts3
                float x = v.x * c - v.z * s;
                float z = v.x * s + v.z * c;
                float y = v.y;

                sf::Vector2f p{centre.x + x * scale, centre.y - y * scale};
                dot.setPosition(p);
                window.draw(dot);
            }
        }
    }
}
