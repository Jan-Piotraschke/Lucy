#include "kamon_fourier.h"
#include "components/contourExtractor/contourExtractor.h"

#include <TGUI/TGUI.hpp>
#include <cmath>
#include <complex> // for std::complex
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

// Anonymous namespace for local data/functions
namespace
{

// The single instance for our Fourier panel
tgui::Panel::Ptr g_fourierPanel = nullptr;

// Data for epicycle animation
static bool g_fourierInitialized = false;

// Fourier data
static std::vector<std::complex<float>> g_coeffs; // Fourier coefficients
static std::vector<int>                 g_freqs;  // frequencies (indices)
static float                            g_time          = 0.f;
static const float                      g_speed         = 2.f;
static const int                        g_numComponents = 100;

// The contour we attempt to trace
static std::vector<sf::Vector2f> g_contourPoints;

// A separate “history” of points that the epicycle tip has traced out
static std::vector<sf::Vector2f> g_path;

// Window size
static const int W_WIDTH  = 900;
static const int W_HEIGHT = 700;

// -----------------------------------------------------------------------------
// 3) Normalize to [-1,1] range (same as before)
// -----------------------------------------------------------------------------
void normalize(std::vector<sf::Vector2f>& pts)
{
    if (pts.empty())
        return;

    double sumX = 0, sumY = 0;
    for (auto& p : pts)
    {
        sumX += p.x;
        sumY += p.y;
    }
    double meanX = sumX / pts.size();
    double meanY = sumY / pts.size();
    for (auto& p : pts)
    {
        p.x -= float(meanX);
        p.y -= float(meanY);
    }
    double maxVal = 0.0;
    for (auto& p : pts)
    {
        double mag = std::max(std::fabs(p.x), std::fabs(p.y));
        if (mag > maxVal)
            maxVal = mag;
    }
    if (maxVal > 0)
    {
        for (auto& p : pts)
        {
            p.x /= float(maxVal);
            p.y /= float(maxVal);
        }
    }
}

// -----------------------------------------------------------------------------
// 4) A naive O(N^2) DFT (same as your “fixed” approach) for correctness
// -----------------------------------------------------------------------------
void computeFourier(const std::vector<sf::Vector2f>& pts)
{
    int N = (int)pts.size();
    if (N < 2)
    {
        std::cerr << "[KamonFourier] Not enough points to compute FFT.\n";
        return;
    }

    std::vector<std::complex<float>> cplx(N, std::complex<float>(0, 0));
    const float                      TWO_PI = 6.28318530718f;

    for (int k = 0; k < N; k++)
    {
        std::complex<float> sum(0.f, 0.f);
        for (int n = 0; n < N; n++)
        {
            float xn    = pts[n].x;
            float yn    = pts[n].y;
            float angle = -TWO_PI * float(k) * float(n) / float(N);
            float cosA  = std::cos(angle);
            float sinA  = std::sin(angle);

            float re = xn * cosA + yn * sinA;
            float im = -xn * sinA + yn * cosA;
            sum += std::complex<float>(re, im);
        }
        sum /= float(N);
        cplx[k] = sum;
    }

    // interpret negative freq if k > N/2 => freq = k - N
    std::vector<std::pair<int, float>> magIndex;
    magIndex.reserve(N);
    for (int k = 0; k < N; k++)
    {
        int   freq      = (k <= N / 2) ? k : (k - N);
        float magnitude = std::abs(cplx[k]);
        magIndex.push_back({freq, magnitude});
    }
    std::sort(
        magIndex.begin(), magIndex.end(), [&](auto& a, auto& b) { return a.second > b.second; });

    g_coeffs.clear();
    g_freqs.clear();
    g_coeffs.resize(g_numComponents);
    g_freqs.resize(g_numComponents);

    for (int i = 0; i < g_numComponents; i++)
    {
        int freqIndex = magIndex[i].first;
        int cplxIndex = (freqIndex >= 0) ? freqIndex : (freqIndex + N);
        g_coeffs[i]   = cplx[cplxIndex];
        g_freqs[i]    = freqIndex;
    }
}

// -----------------------------------------------------------------------------
// 5) Initialize data once
// -----------------------------------------------------------------------------
void initFourierData()
{
    if (g_fourierInitialized)
        return;

    // Try to load from SVG first. If that fails, fallback to the PNG.
    std::string svgPath = "assets/kamon.svg";
    std::string pngPath = "assets/kamon_fourier.png";

    using namespace KamonFourier::ContourExtractor;

    g_contourPoints = extractContourFromSVG(svgPath);
    if (g_contourPoints.empty())
    {
        std::cerr << "[KamonFourier] Falling back to PNG contour...\n";
        g_contourPoints = extractLargestContour(pngPath);
    }


    if (g_contourPoints.empty())
    {
        std::cerr << "[KamonFourier] Could not load any valid contour from SVG or PNG.\n";
        return;
    }

    // 2) normalize to [-1,1]
    normalize(g_contourPoints);

    // 3) do naive DFT
    computeFourier(g_contourPoints);

    g_fourierInitialized = true;
    g_path.clear();
    g_time = 0.f;
}

// -----------------------------------------------------------------------------
// 6) Evaluate epicycles sum at time t, etc. (same as before)
// -----------------------------------------------------------------------------
std::complex<float> evalEpicycles(float t)
{
    std::complex<float> sum(0, 0);
    for (int i = 0; i < g_numComponents; i++)
    {
        float               theta = g_freqs[i] * t;
        std::complex<float> e(std::cos(theta), std::sin(theta));
        sum += g_coeffs[i] * e;
    }
    return sum;
}

} // end anonymous namespace

// -----------------------------------------------------------------------------
// 7) Public API in KamonFourier namespace
// -----------------------------------------------------------------------------
namespace KamonFourier
{

tgui::Panel::Ptr createKamonFourierContainer(const std::function<void()>& onBackHome)
{
    g_fourierPanel = tgui::Panel::create({"100%", "100%"});
    g_fourierPanel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    auto content = tgui::Panel::create({"100%", "100% - 50"});
    content->setPosition(0, 50);
    content->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
    g_fourierPanel->add(content);

    auto title = tgui::Label::create("Kamon Fourier Epicycle Screen");
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

    return g_fourierPanel;
}

tgui::Panel::Ptr createFourierTile(const std::function<void()>& openCallback)
{
    auto panel = tgui::Panel::create({300, 150});
    panel->getRenderer()->setBackgroundColor(sf::Color(20, 20, 150));
    panel->getRenderer()->setBorderColor(sf::Color::Black);
    panel->getRenderer()->setBorders({2, 2, 2, 2});

    auto lbl = tgui::Label::create("das Uhrwerk von Kamon");
    lbl->setTextSize(18);
    lbl->getRenderer()->setTextColor(sf::Color::White);
    lbl->setPosition(10, 10);
    panel->add(lbl);

    auto desc = tgui::Label::create("Show animation");
    desc->setTextSize(14);
    desc->getRenderer()->setTextColor(sf::Color::White);
    desc->setPosition(10, 40);
    panel->add(desc);

    auto btn = tgui::Button::create("OPEN");
    btn->setPosition(10, 80);
    btn->setSize(70, 30);
    btn->onPress(
        [openCallback]()
        {
            if (openCallback)
                openCallback();
        });
    panel->add(btn);

    return panel;
}

tgui::Panel::Ptr getFourierPanel()
{
    return g_fourierPanel;
}

void updateAndDraw(sf::RenderWindow& window)
{
    initFourierData();
    if (!g_fourierInitialized)
        return;

    // advance time
    g_time += g_speed * 0.02f; // dt

    // ========== 1) MAIN EPICYCLE DRAWING ==========
    std::complex<float> sumPrev(0.f, 0.f);
    sf::Color           circleColor = sf::Color(100, 100, 200, 80);

    for (int i = 0; i < g_numComponents; i++)
    {
        float freq   = float(g_freqs[i]);
        auto  c      = g_coeffs[i];
        float radius = std::abs(c);

        // compute this component's contribution
        float               theta      = freq * g_time;
        float               re         = c.real() * std::cos(theta) - c.imag() * std::sin(theta);
        float               im         = c.real() * std::sin(theta) + c.imag() * std::cos(theta);
        std::complex<float> sumCurrent = sumPrev + std::complex<float>(re, im);

        // draw the epicycle circle
        sf::CircleShape circle(radius * 200.f);
        circle.setOrigin({radius * 200.f, radius * 200.f});
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(circleColor);
        circle.setOutlineThickness(1.f);

        float cx = (sumPrev.real() * 200.f) + 450.f;
        float cy = 700.f - ((sumPrev.imag() * 200.f) + 350.f);
        circle.setPosition({cx, cy});
        window.draw(circle);

        sumPrev = sumCurrent;
    }

    // draw the traced path
    auto         tip = sumPrev;
    sf::Vector2f tipPos((tip.real() * 200.f) + 450.f, 700.f - ((tip.imag() * 200.f) + 350.f));
    g_path.push_back(tipPos);
    if (g_path.size() > 2000)
        g_path.erase(g_path.begin());

    sf::VertexArray pathLines(sf::PrimitiveType::LineStrip, g_path.size());
    for (size_t i = 0; i < g_path.size(); i++)
    {
        pathLines[i].position = g_path[i];
        pathLines[i].color    = sf::Color::Black;
    }
    window.draw(pathLines);

    // ========== 2) CLOCKWORK LAYOUT FOR INDIVIDUAL EPICYCLES ==========
    const int          clockCount = g_numComponents;
    const float        ringRadius = 250.f;         // distance from center
    const sf::Vector2f screenCenter(450.f, 350.f); // same center as main

    for (int i = 0; i < clockCount; ++i)
    {
        // 2.1 compute each clock’s center on a ring
        float        arrAng = 2 * M_PI * float(i) / float(clockCount);
        sf::Vector2f center = {
            screenCenter.x + std::cos(arrAng) * ringRadius,
            screenCenter.y + std::sin(arrAng) * ringRadius};

        // 2.2 radius of this epicycle “clock”
        float radius = std::abs(g_coeffs[i]) * 35.f;

        // ---- draw clock face ----
        sf::CircleShape face(radius);
        face.setOrigin(sf::Vector2f(radius, radius));
        face.setPosition(center);
        face.setFillColor(sf::Color::Transparent);
        face.setOutlineColor(sf::Color(50, 50, 50));
        face.setOutlineThickness(1.f);
        window.draw(face);

        // ---- draw tick marks ----
        const int numTicks = 12;
        for (int t = 0; t < numTicks; ++t)
        {
            float        tickAng = 2 * M_PI * t / numTicks;
            sf::Vector2f outer   = {
                center.x + std::cos(tickAng) * radius, center.y + std::sin(tickAng) * radius};
            sf::Vector2f inner = {
                center.x + std::cos(tickAng) * (radius * 0.85f),
                center.y + std::sin(tickAng) * (radius * 0.85f)};
            sf::Vertex tickLine[] = {
                {outer, sf::Color(80, 80, 80)}, {inner, sf::Color(80, 80, 80)}};
            window.draw(tickLine, 2, sf::PrimitiveType::Lines);
        }

        // ---- draw the rotating hand ----
        float        theta = g_freqs[i] * g_time;
        sf::Vector2f tip   = {
            center.x + std::cos(theta) * radius, center.y + std::sin(theta) * radius};
        sf::Vertex hand[] = {{center, sf::Color::Red}, {tip, sf::Color::Red}};
        window.draw(hand, 2, sf::PrimitiveType::Lines);
    }
}

} // namespace KamonFourier
