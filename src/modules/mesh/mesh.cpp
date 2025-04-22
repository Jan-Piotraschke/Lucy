#include "mesh.h"

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>

namespace fs = std::filesystem;

// ────────────────────────────────
//   ▌  Internal, file‑local state
// ────────────────────────────────
namespace
{
std::vector<sf::Vector2f>              verts2;
std::vector<std::vector<unsigned int>> faces2;
sf::VertexArray                        mesh2;  // 2‑D triangles
sf::VertexArray                        edges2; // 2‑D wireframe
bool                                   mesh2Loaded = false;

std::vector<sf::Vector3f>              verts3;
std::vector<std::vector<unsigned int>> faces3;
sf::VertexArray                        mesh3;  // 3‑D triangles (projected)
sf::VertexArray                        edges3; // 3‑D wireframe (projected)
bool                                   mesh3Loaded = false;
float                                  radius3     = 1.f; // bounding radius

float angle      = 0.f; // Y‑rotation
int   lastMouseX = 0;
bool  dragging   = false;

using Edge = std::pair<unsigned, unsigned>;
struct EdgeHash
{
    size_t operator()(const Edge& e) const noexcept
    {
        unsigned long long a = std::min(e.first, e.second);
        unsigned long long b = std::max(e.first, e.second);
        return (a + b) * (a + b + 1) / 2 + b; // Cantor pairing
    }
};
using EdgeSet = std::unordered_set<Edge, EdgeHash>;

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
} // unnamed namespace

// ────────────────────────────────
//   ▌  UI‑building helpers
// ────────────────────────────────
tgui::Panel::Ptr Mesh::createMeshContainer(std::function<void()> goBackCallback)
{
    // Root panel
    auto panel = tgui::Panel::create();
    panel->setSize({"100%", "100%"});
    panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent); // <‑‑ add this

    // Back button
    auto backBtn = tgui::Button::create("< Back");
    backBtn->setSize(100, 30);
    backBtn->setPosition(10, 10);
    backBtn->onPress(goBackCallback);
    panel->add(backBtn);

    // ------------------------------------------------------------------
    // Load meshes once (kept in the anonymous‑namespace statics above)
    // ------------------------------------------------------------------
    fs::path base = fs::path(__FILE__).parent_path().parent_path().parent_path().parent_path();

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
                // Filled triangles (fan)
                for (size_t i = 1; i + 1 < f.size(); ++i)
                {
                    mesh2.append({verts2[f[0]], sf::Color::White});
                    mesh2.append({verts2[f[i]], sf::Color::White});
                    mesh2.append({verts2[f[i + 1]], sf::Color::White});
                }
                // Wireframe (unique edges only)
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
            std::cout << "Loaded 2‑D mesh: " << kachel << '\n';
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
            std::cout << "Loaded 3‑D mesh: " << ellipsoid << '\n';
        }
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
    // ── overall window geometry ──────────────────────────────────────
    constexpr float WIN_W    = 900.f;
    constexpr float WIN_H    = 900.f;
    constexpr float TOP_FRAC = 0.75f; // 75 % for 2‑D view
    constexpr float TOP_H    = WIN_H * TOP_FRAC;
    constexpr float BOTTOM_H = WIN_H - TOP_H;

    // ── basic mouse‑drag rotation (bottom panel only) ────────────────
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)
        && sf::Mouse::getPosition(window).y > TOP_H)
    {
        int dx = sf::Mouse::getPosition(window).x - lastMouseX;
        angle += dx * 0.005f; // 0.005 rad / px
        dragging = true;
    }
    else
        dragging = false;
    lastMouseX = sf::Mouse::getPosition(window).x;

    // ── draw 2‑D mesh (TOP panel) ────────────────────────────────────
    if (mesh2Loaded)
    {
        const auto b     = mesh2.getBounds(); // FloatRect
        const auto sz    = b.size;
        float      scale = std::min(WIN_W / sz.x, TOP_H / sz.y) * 0.9f;

        sf::Transform tr;
        tr.translate({WIN_W * 0.5f, TOP_H * 0.5f}); // centre of top panel
        tr.scale({scale, -scale});
        tr.translate({-(b.position.x + sz.x * 0.5f), -(b.position.y + sz.y * 0.5f)});

        window.draw(mesh2, tr);
        window.draw(edges2, tr);
    }

    // ── draw 3‑D mesh (BOTTOM panel) ─────────────────────────────────
    if (mesh3Loaded)
    {
        const float        c = std::cos(angle), s = std::sin(angle);
        const float        scale = (std::min(WIN_W, BOTTOM_H) * 0.45f) / radius3;
        const sf::Vector2f centre{WIN_W * 0.5f, TOP_H + BOTTOM_H * 0.5f};

        // project vertices each frame
        std::vector<sf::Vector2f> proj(verts3.size());
        for (size_t i = 0; i < verts3.size(); ++i)
        {
            float x = verts3[i].x * c - verts3[i].z * s;
            float z = verts3[i].x * s + verts3[i].z * c;
            (void)z;
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
            // Filled tris
            for (size_t i = 1; i + 1 < f.size(); ++i)
            {
                mesh3.append({proj[f[0]], sf::Color(200, 200, 200)});
                mesh3.append({proj[f[i]], sf::Color(200, 200, 200)});
                mesh3.append({proj[f[i + 1]], sf::Color(200, 200, 200)});
            }
            // Wireframe
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
    }
}
