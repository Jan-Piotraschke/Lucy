#include "kamon_fourier.h"
#include "components/contourExtractor/contourExtractor.h"
#include "components/visualizer/visualizer.h"

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <kissfft/kiss_fft.h>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

// TODO: wir sollten in der Gingko Blatt Kerbe mit der Simulation anfangen!

// ──────────────────────────────────────────────────────────────────────────────
// Local constants & aliases
// ──────────────────────────────────────────────────────────────────────────────
namespace
{
using complexf = std::complex<float>;

constexpr float kSpeed         = 1.0f;
constexpr int   kNumComponents = 48;  // bei "24" sieht man gerade noch das kamon

// ──────────────────────────────────────────────────────────────────────────────
// RAII helper for kiss_fft_cfg
// ──────────────────────────────────────────────────────────────────────────────
struct KissFftDeleter
{
    void operator()(kiss_fft_state* cfg) const noexcept
    {
        std::free(cfg);
    }
};
using KissFftPtr = std::unique_ptr<kiss_fft_state, KissFftDeleter>;

struct FourierState
{
    tgui::Panel::Ptr          panel;
    bool                      initialized = false;
    std::vector<complexf>     coeffs;
    std::vector<int>          freqs;
    std::vector<sf::Vector2f> contourPts;
    std::vector<sf::Vector2f> path;
    float                     time = 0.0f;
    KamonFourier::Visualizer  visualizer{kNumComponents, kSpeed};
};

FourierState g_state;

// ──────────────────────────────────────────────────────────────────────────────
// Normalise input points to a −1..1 range around the origin
// ──────────────────────────────────────────────────────────────────────────────
void normalize(std::vector<sf::Vector2f>& pts)
{
    if (pts.empty())
        return;

    const sf::Vector2f mean = std::accumulate(
                                  pts.begin(),
                                  pts.end(),
                                  sf::Vector2f{},
                                  [](sf::Vector2f acc, const sf::Vector2f& p)
                                  {
                                      acc.x += p.x;
                                      acc.y += p.y;
                                      return acc;
                                  })
                              / static_cast<float>(pts.size());

    for (auto& p : pts)
        p -= mean;

    const float maxVal = std::accumulate(
        pts.begin(),
        pts.end(),
        0.0f,
        [](float m, const sf::Vector2f& p)
        { return std::max(m, std::max(std::fabs(p.x), std::fabs(p.y))); });

    if (maxVal > 0.0f)
    {
        for (auto& p : pts)
            p /= maxVal;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Compute Fourier coefficients using KissFFT
// ──────────────────────────────────────────────────────────────────────────────
void computeFourier(const std::vector<sf::Vector2f>& pts)
{
    const int N = static_cast<int>(pts.size());
    if (N < 2)
    {
        std::cerr << "[KamonFourier] Not enough points for FFT.\n";
        return;
    }

    std::vector<kiss_fft_cpx> in(N), out(N);
    for (int i = 0; i < N; ++i)
    {
        in[i].r = pts[i].x;
        in[i].i = pts[i].y;
    }

    KissFftPtr cfg{kiss_fft_alloc(N, 0, nullptr, nullptr)};
    if (!cfg)
    {
        std::cerr << "[KamonFourier] KissFFT allocation failed.\n";
        return;
    }

    kiss_fft(cfg.get(), in.data(), out.data());

    const float           invN = 1.0f / static_cast<float>(N);
    std::vector<complexf> cplx(N);
    for (int i = 0; i < N; ++i)
        cplx[i] = {out[i].r * invN, out[i].i * invN};

    // Magnitude-frequency pairs, sorted by magnitude (descending)
    struct MagIdx
    {
        int   freq;
        float mag;
    };
    std::vector<MagIdx> magIndex;
    magIndex.reserve(N);

    for (int k = 0; k < N; ++k)
    {
        const int freq = (k <= N / 2) ? k : k - N;
        magIndex.push_back({freq, std::abs(cplx[k])});
    }

    std::partial_sort(
        magIndex.begin(),
        magIndex.begin() + std::min(kNumComponents, N),
        magIndex.end(),
        [](const MagIdx& a, const MagIdx& b) { return a.mag > b.mag; });

    const int keep = std::min(kNumComponents, N);
    g_state.coeffs.resize(keep);
    g_state.freqs.resize(keep);

    for (int i = 0; i < keep; ++i)
    {
        const int freq = magIndex[i].freq;
        const int idx  = (freq >= 0) ? freq : freq + N;

        g_state.coeffs[i] = cplx[idx];
        g_state.freqs[i]  = freq;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Load contour and pre-compute Fourier data
// ──────────────────────────────────────────────────────────────────────────────
void initFourierData()
{
    if (g_state.initialized)
        return;

    using namespace KamonFourier::ContourExtractor;

    constexpr char kSvgPath[] = "assets/img/kamon.svg";
    constexpr char kPngPath[] = "assets/img/kamon_fourier.png";

    g_state.contourPts = extractContourFromSVG(kSvgPath);
    if (g_state.contourPts.empty())
    {
        std::cerr << "[KamonFourier] Falling back to PNG contour…\n";
        g_state.contourPts = extractLargestContour(kPngPath);
    }

    if (g_state.contourPts.empty())
    {
        std::cerr << "[KamonFourier] Failed to load contour.\n";
        return;
    }

    normalize(g_state.contourPts);
    computeFourier(g_state.contourPts);

    g_state.initialized = true;
    g_state.path.clear();
    g_state.time = 0.0f;
}

// ──────────────────────────────────────────────────────────────────────────────
// Evaluate truncated Fourier series (epicycles) at angle t
// ──────────────────────────────────────────────────────────────────────────────
[[nodiscard]] complexf evalEpicycles(float t) noexcept
{
    complexf sum{};
    for (std::size_t i = 0; i < g_state.coeffs.size(); ++i)
    {
        const float theta = static_cast<float>(g_state.freqs[i]) * t;
        sum += g_state.coeffs[i] * complexf{std::cos(theta), std::sin(theta)};
    }
    return sum;
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// KamonFourier Public API
// ──────────────────────────────────────────────────────────────────────────────
namespace KamonFourier
{

tgui::Panel::Ptr createKamonFourierContainer(const std::function<void()>& onBackHome)
{
    g_state.panel = tgui::Panel::create({"100%", "100%"});
    g_state.panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    auto content = tgui::Panel::create({"100%", "100% - 50"});
    content->setPosition(0, 50);
    content->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
    g_state.panel->add(content);

    auto backBtn = tgui::Button::create("Back to Home");
    backBtn->setPosition({0.f, 0.f});
    backBtn->onPress(onBackHome);
    content->add(backBtn);

    return g_state.panel;
}

tgui::Panel::Ptr createFourierTile(const std::function<void()>& openCallback)
{
    auto panel = tgui::Panel::create({300, 150});

    panel->getRenderer()->setBackgroundColor(sf::Color(20, 20, 150));
    panel->getRenderer()->setBorderColor(sf::Color::Black);
    panel->getRenderer()->setBorders({2, 2, 2, 2});

    auto title = tgui::Label::create("das Uhrwerk von Kamon");
    title->setTextSize(18);
    title->getRenderer()->setTextColor(sf::Color::White);
    title->setPosition(10, 10);
    panel->add(title);

    auto desc = tgui::Label::create("Show animation");
    desc->setTextSize(14);
    desc->getRenderer()->setTextColor(sf::Color::White);
    desc->setPosition(10, 40);
    panel->add(desc);

    auto btn = tgui::Button::create("OPEN");
    btn->setPosition(10, 80);
    btn->setSize(70, 30);
    btn->onPress(openCallback);
    panel->add(btn);

    return panel;
}

tgui::Panel::Ptr getFourierPanel()
{
    return g_state.panel;
}

void updateAndDraw(sf::RenderWindow& window)
{
    initFourierData();
    if (!g_state.initialized)
        return;

    g_state.visualizer.updateAndDraw(window, g_state.coeffs, g_state.freqs);
}

} // namespace KamonFourier
