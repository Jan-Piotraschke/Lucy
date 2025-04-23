#include "visualizer.h"

#include <cmath>

namespace KamonFourier
{
namespace
{
constexpr float TWO_PI = 6.283185307179586476925f;
}

Visualizer::Visualizer(int numComponents, float speed)
    : m_speed(speed), m_time(0.f), m_numComponents(numComponents)
{
    m_path.reserve(2000);
}

void Visualizer::reset()
{
    m_time = 0.f;
    m_path.clear();
}

void Visualizer::updateAndDraw(
    sf::RenderWindow&                       window,
    const std::vector<std::complex<float>>& coeffs,
    const std::vector<int>&                 freqs)
{
    // Defensive checks -------------------------------------------------------
    if (coeffs.empty() || freqs.empty() || coeffs.size() < static_cast<size_t>(m_numComponents))
        return;

    // 1) Advance time ---------------------------------------------------------
    m_time += m_speed * 0.02f; // dt (same constant as before)

    // 2) MAIN EPICYCLE DRAWING ------------------------------------------------
    std::complex<float> sumPrev(0.f, 0.f);
    const sf::Color     circleColor(100, 100, 200, 80);

    for (int i = 0; i < m_numComponents; ++i)
    {
        const float               freq   = static_cast<float>(freqs[i]);
        const auto&               c      = coeffs[i];
        const float               radius = std::abs(c);
        const float               theta  = freq * m_time;
        const std::complex<float> sumCur =
            sumPrev
            + std::complex<float>(
                c.real() * std::cos(theta) - c.imag() * std::sin(theta),
                c.real() * std::sin(theta) + c.imag() * std::cos(theta));

        // Circle of the current epicycle
        sf::CircleShape circle(radius * 200.f);
        circle.setOrigin({radius * 200.f, radius * 200.f});
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(circleColor);
        circle.setOutlineThickness(1.f);

        const float cx = (sumPrev.real() * 200.f) + 450.f;
        const float cy = 700.f - ((sumPrev.imag() * 200.f) + 350.f);
        circle.setPosition({cx, cy});
        window.draw(circle);

        sumPrev = sumCur;
    }

    // 3) Traced path ----------------------------------------------------------
    const sf::Vector2f tipPos(
        (sumPrev.real() * 200.f) + 450.f, 700.f - ((sumPrev.imag() * 200.f) + 350.f));
    m_path.push_back(tipPos);
    if (m_path.size() > 2000)
        m_path.erase(m_path.begin());

    sf::VertexArray pathLines(sf::PrimitiveType::LineStrip, m_path.size());
    for (size_t i = 0; i < m_path.size(); ++i)
    {
        pathLines[i].position = m_path[i];
        pathLines[i].color    = sf::Color::Black;
    }
    window.draw(pathLines);

    // 4) Clockwork ------------------------------------------------------------
    drawClockwork(window, coeffs, freqs);
}

void Visualizer::drawClockwork(
    sf::RenderWindow&                       window,
    const std::vector<std::complex<float>>& coeffs,
    const std::vector<int>&                 freqs)
{
    const int          clockCount = m_numComponents;
    const float        ringRadius = 250.f;
    const sf::Vector2f screenCenter(450.f, 350.f);

    for (int i = 0; i < clockCount; ++i)
    {
        // ---- positioning on the ring --------------------------------------
        const float        arrAng = TWO_PI * static_cast<float>(i) / static_cast<float>(clockCount);
        const sf::Vector2f center(
            screenCenter.x + std::cos(arrAng) * ringRadius,
            screenCenter.y + std::sin(arrAng) * ringRadius);

        // ---- clock face ----------------------------------------------------
        const float     radius = std::abs(coeffs[i]) * 35.f;
        sf::CircleShape face(radius);
        face.setOrigin(sf::Vector2f(radius, radius));
        face.setPosition(center);
        face.setFillColor(sf::Color::Transparent);
        face.setOutlineColor(sf::Color(50, 50, 50));
        face.setOutlineThickness(1.f);
        window.draw(face);

        // ---- tick marks ----------------------------------------------------
        constexpr int numTicks = 12;
        for (int t = 0; t < numTicks; ++t)
        {
            const float        tickAng = TWO_PI * t / numTicks;
            const sf::Vector2f outer(
                center.x + std::cos(tickAng) * radius, center.y + std::sin(tickAng) * radius);
            const sf::Vector2f inner(
                center.x + std::cos(tickAng) * radius * 0.85f,
                center.y + std::sin(tickAng) * radius * 0.85f);
            const sf::Vertex tickLine[] = {
                {outer, sf::Color(80, 80, 80)}, {inner, sf::Color(80, 80, 80)}};
            window.draw(tickLine, 2, sf::PrimitiveType::Lines);
        }

        // ---- hand ----------------------------------------------------------
        const float        theta = freqs[i] * m_time;
        const sf::Vector2f tip(
            center.x + std::cos(theta) * radius, center.y + std::sin(theta) * radius);
        const sf::Vertex hand[] = {{center, sf::Color::Red}, {tip, sf::Color::Red}};
        window.draw(hand, 2, sf::PrimitiveType::Lines);
    }
}

} // namespace KamonFourier
