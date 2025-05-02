#include "kamon_fourier.h"
#include "components/contourExtractor/contourExtractor.h"
#include "components/visualizer/visualizer.h"

#include <TGUI/TGUI.hpp>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

#include <kissfft/kiss_fft.h>

// ──────────────────────────────────────────────────────────────────────────────
// Anonymous namespace – local data / helpers
// ──────────────────────────────────────────────────────────────────────────────
namespace
{

// The single instance for our Fourier panel
tgui::Panel::Ptr g_fourierPanel = nullptr;

// Data for epicycle animation
static bool g_fourierInitialized = false;

// Fourier data
static std::vector<std::complex<float>> g_coeffs;   // Fourier coefficients
static std::vector<int>                 g_freqs;    // integer frequencies
static float                            g_time          = 0.f;
static const float                      g_speed         = 1.f;
static const int                        g_numComponents = 65;
static KamonFourier::Visualizer         g_visualizer(g_numComponents, g_speed);

// The contour we attempt to trace
static std::vector<sf::Vector2f> g_contourPoints;

// A separate “history” of points that the epicycle tip has traced out
static std::vector<sf::Vector2f> g_path;

// Window size
static const int W_WIDTH  = 900;
static const int W_HEIGHT = 700;

// ──────────────────────────────────────────────────────────────────────────────
// 1) Normalise points into the range [-1, 1]
// ──────────────────────────────────────────────────────────────────────────────
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

// ──────────────────────────────────────────────────────────────────────────────
// 2) Compute Fourier coefficients with KissFFT  (O(N log N))
//    We treat every point as the complex sample  z[n] = x[n] + i·y[n]
// ──────────────────────────────────────────────────────────────────────────────
void computeFourier(const std::vector<sf::Vector2f>& pts)
{
    const int N = static_cast<int>(pts.size());
    if (N < 2)
    {
        std::cerr << "[KamonFourier] Not enough points to compute FFT.\n";
        return;
    }

    // --- 2.1  Prepare input / output buffers for KissFFT ---------------------
    std::vector<kiss_fft_cpx> in (N);
    std::vector<kiss_fft_cpx> out(N);

    for (int n = 0; n < N; ++n)
    {
        in[n].r = pts[n].x;          // real  = x-coordinate
        in[n].i = pts[n].y;          // imag  = y-coordinate
    }

    kiss_fft_cfg cfg = kiss_fft_alloc(N, /*inverse=*/0, nullptr, nullptr);
    if (!cfg)
    {
        std::cerr << "[KamonFourier] KissFFT allocation failed.\n";
        return;
    }

    kiss_fft(cfg, in.data(), out.data());
    std::free(cfg);   // KissFFT was malloc()-based

    // --- 2.2  Copy + normalise to std::complex<float> ------------------------
    const float invN = 1.0f / static_cast<float>(N);
    std::vector<std::complex<float>> cplx(N);
    for (int k = 0; k < N; ++k)
        cplx[k] = std::complex<float>(out[k].r * invN, out[k].i * invN);

    // --- 2.3  Map indices to signed frequencies and sort by magnitude --------
    std::vector<std::pair<int,float>> magIndex;
    magIndex.reserve(N);
    for (int k = 0; k < N; ++k)
    {
        const int   freq      = (k <= N/2) ?  k : (k - N);  // wrap negatives
        const float magnitude = std::abs(cplx[k]);
        magIndex.emplace_back(freq, magnitude);
    }
    std::sort(magIndex.begin(), magIndex.end(),
              [](auto& a, auto& b){ return a.second > b.second; });

    // --- 2.4  Keep only the strongest g_numComponents harmonics --------------
    const int keep = std::min(g_numComponents, static_cast<int>(magIndex.size()));
    g_coeffs.resize(keep);
    g_freqs .resize(keep);

    for (int i = 0; i < keep; ++i)
    {
        const int freqIndex = magIndex[i].first;             // signed frequency
        const int rawIndex  = (freqIndex >= 0) ? freqIndex   // 0 … N-1 index
                                               : (freqIndex + N);
        g_coeffs[i] = cplx[rawIndex];
        g_freqs [i] = freqIndex;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// 3) Initialise (load contour → normalise → FFT → reset timers)
// ──────────────────────────────────────────────────────────────────────────────
void initFourierData()
{
    if (g_fourierInitialized) return;

    // Load from SVG first, fall back to PNG
    const std::string svgPath = "assets/kamon.svg";
    const std::string pngPath = "assets/kamon_fourier.png";

    using namespace KamonFourier::ContourExtractor;
    g_contourPoints = extractContourFromSVG(svgPath);
    if (g_contourPoints.empty())
    {
        std::cerr << "[KamonFourier] Falling back to PNG contour…\n";
        g_contourPoints = extractLargestContour(pngPath);
    }
    if (g_contourPoints.empty())
    {
        std::cerr << "[KamonFourier] Could not load any valid contour.\n";
        return;
    }

    normalize(g_contourPoints);      // scale to unit square
    computeFourier(g_contourPoints); // KissFFT

    g_fourierInitialized = true;
    g_path.clear();
    g_time = 0.f;
}

// ──────────────────────────────────────────────────────────────────────────────
// 4) Evaluate epicycle sum   f(t) = Σ C_k · e^{i·ω_k·t}
// ──────────────────────────────────────────────────────────────────────────────
std::complex<float> evalEpicycles(float t)
{
    std::complex<float> sum(0.f, 0.f);
    for (std::size_t i = 0; i < g_coeffs.size(); ++i)
    {
        const float               theta = g_freqs[i] * t;
        const std::complex<float> e(std::cos(theta), std::sin(theta));
        sum += g_coeffs[i] * e;
    }
    return sum;
}

} // ───── end anonymous namespace ──────────────────────────────────────────────



// ──────────────────────────────────────────────────────────────────────────────
// 5) Public API – unchanged except that it now benefits from KissFFT speed
// ──────────────────────────────────────────────────────────────────────────────
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

    auto backBtn = tgui::Button::create("Back to Home");
    backBtn->setPosition({0.f, 0.f});
    backBtn->onPress([onBackHome]{ if (onBackHome) onBackHome(); });
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
    btn->onPress([openCallback]{ if (openCallback) openCallback(); });
    panel->add(btn);

    return panel;
}

tgui::Panel::Ptr getFourierPanel() { return g_fourierPanel; }

void updateAndDraw(sf::RenderWindow& window)
{
    initFourierData();
    if (!g_fourierInitialized) return;

    g_visualizer.updateAndDraw(window, g_coeffs, g_freqs);
}

} // namespace KamonFourier
