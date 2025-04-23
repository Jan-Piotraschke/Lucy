#pragma once

#include <SFML/Graphics.hpp>
#include <complex>
#include <vector>

namespace KamonFourier
{
/**
 * @brief A self‑contained helper that draws the epicycle animation (main epicycle + “clockwork”).
 *
 * The class keeps its own animation state (time & traced path) so the caller only has to
 * hand it the current Fourier coefficients + frequencies every frame.
 */
class Visualizer
{
  public:
    /**
     * @param numComponents Number of Fourier components that will be drawn.
     * @param speed         Animation speed factor (same semantics as the old g_speed).
     */
    explicit Visualizer(int numComponents, float speed = 2.f);

    // Non‑copyable (the object stores large vectors)
    Visualizer(const Visualizer&)            = delete;
    Visualizer& operator=(const Visualizer&) = delete;

    /**
     * @brief Advance the internal time and render everything onto the window.
     *
     * @param window  Target SFML render window.
     * @param coeffs  Fourier coefficients (size >= numComponents).
     * @param freqs   Corresponding frequency indices (size >= numComponents).
     */
    void updateAndDraw(
        sf::RenderWindow&                       window,
        const std::vector<std::complex<float>>& coeffs,
        const std::vector<int>&                 freqs);

    /** Reset the animation (time = 0, path cleared). */
    void reset();

  private:
    void drawClockwork(
        sf::RenderWindow&                       window,
        const std::vector<std::complex<float>>& coeffs,
        const std::vector<int>&                 freqs);

    float                     m_speed{2.f};
    float                     m_time{0.f};
    int                       m_numComponents{0};
    std::vector<sf::Vector2f> m_path; // traced tip path (kept for ~2000 samples)
};
} // namespace KamonFourier
