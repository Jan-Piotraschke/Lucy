#include "kamon_fourier.h"

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
// 1) Helper: load & extract largest contour from PNG (same as before)
// -----------------------------------------------------------------------------
std::vector<sf::Vector2f> extractLargestContour(const std::string& imagePath)
{
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
    if (img.empty())
    {
        std::cerr << "[KamonFourier] Failed to load image: " << imagePath << std::endl;
        return {};
    }

    cv::Mat thresh;
    cv::threshold(img, thresh, 127, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    if (contours.empty())
    {
        std::cerr << "[KamonFourier] No contours found.\n";
        return {};
    }

    auto largestIt = std::max_element(
        contours.begin(),
        contours.end(),
        [](auto& c1, auto& c2) { return cv::contourArea(c1) < cv::contourArea(c2); });

    std::vector<sf::Vector2f> pts;
    pts.reserve(largestIt->size());
    for (auto& pt : *largestIt)
    {
        // flip Y
        pts.emplace_back(float(pt.x), float(-pt.y));
    }
    return pts;
}

// -----------------------------------------------------------------------------
// 2) Helper: minimal SVG path parser to extract a single path from "kamon.svg".
//    This is extremely naive. It only parses M, L, and C commands with absolute
//    coordinates. If your SVG has arcs, transforms, etc., this won't handle it!
//
//    Returns vector of (x,y) in same coordinate space (with Y upward if we later
//    flip).
// -----------------------------------------------------------------------------
std::vector<sf::Vector2f> extractContourFromSVG(const std::string& svgPath)
{
    // 2.1) Read entire file into a string
    std::ifstream in(svgPath);
    if (!in.is_open())
    {
        std::cerr << "[KamonFourier] Failed to open SVG file: " << svgPath << "\n";
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // 2.2) Find the first <path ... d="..."> inside the file
    auto pathPos = content.find("<path");
    if (pathPos == std::string::npos)
    {
        std::cerr << "[KamonFourier] No <path> found in SVG.\n";
        return {};
    }
    // find d=
    auto dPos = content.find(" d=", pathPos);
    if (dPos == std::string::npos)
    {
        // some have d= without space, try that
        dPos = content.find("d=", pathPos);
        if (dPos == std::string::npos)
        {
            std::cerr << "[KamonFourier] No d= attribute found in <path>.\n";
            return {};
        }
    }

    // find the first quote after d=
    auto firstQuote = content.find('"', dPos);
    if (firstQuote == std::string::npos)
    {
        std::cerr << "[KamonFourier] No opening quote for path d=.\n";
        return {};
    }
    // find closing quote
    auto secondQuote = content.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos)
    {
        std::cerr << "[KamonFourier] No closing quote for path d=.\n";
        return {};
    }
    // extract the substring of path data
    std::string pathData = content.substr(firstQuote + 1, secondQuote - (firstQuote + 1));

    // 2.3) Parse the path commands. We'll handle M, L, C (absolute only).
    //      We'll store the resulting points in an array (with some sampling).
    std::vector<sf::Vector2f> points;

    auto skipSpaces = [&](std::istringstream& iss)
    {
        while (iss && std::isspace(iss.peek()))
        {
            iss.ignore(1);
        }
    };

    // We'll maintain a "current" position
    sf::Vector2f currentPos(0.f, 0.f);

    std::istringstream iss(pathData);
    char               cmd;
    while (iss >> cmd)
    {
        if (cmd == 'M') // MoveTo absolute
        {
            float x, y;
            iss >> x;
            skipSpaces(iss);
            iss >> y;
            skipSpaces(iss);

            currentPos.x = x;
            currentPos.y = y;
            points.push_back(currentPos);
        }
        else if (cmd == 'L') // LineTo absolute
        {
            float x, y;
            while (true)
            {
                if (!(iss >> x))
                    break;
                if (!(iss >> y))
                    break;
                currentPos.x = x;
                currentPos.y = y;
                points.push_back(currentPos);

                skipSpaces(iss);
                if (iss.peek() == 'M' || iss.peek() == 'C' || !iss.good())
                    break;
            }
        }
        else if (cmd == 'C') // Cubic Bézier absolute
        {
            // We read sets of 6 floats: (x1,y1, x2,y2, x3,y3) until next command
            const int NUM_SAMPLES_PER_BEZIER = 30;
            while (true)
            {
                float x1, y1, x2, y2, x3, y3;
                if (!(iss >> x1))
                    break;
                if (!(iss >> y1))
                    break;
                if (!(iss >> x2))
                    break;
                if (!(iss >> y2))
                    break;
                if (!(iss >> x3))
                    break;
                if (!(iss >> y3))
                    break;

                // We have a cubic from currentPos to (x3, y3),
                // control points (x1,y1), (x2,y2).
                // We'll subdivide the curve into many line segments.
                // param t=0..1
                for (int i = 1; i <= NUM_SAMPLES_PER_BEZIER; i++)
                {
                    float t  = float(i) / float(NUM_SAMPLES_PER_BEZIER);
                    float mt = 1.f - t;
                    // standard cubic interpolation:
                    //   B(t) = (mt^3)*P0 + 3*(mt^2)*t*C1 + 3*mt*(t^2)*C2 + (t^3)*P3
                    float bx = mt * mt * mt * currentPos.x + 3.f * mt * mt * t * x1
                               + 3.f * mt * (t * t) * x2 + t * t * t * x3;
                    float by = mt * mt * mt * currentPos.y + 3.f * mt * mt * t * y1
                               + 3.f * mt * (t * t) * y2 + t * t * t * y3;
                    points.emplace_back(bx, by);
                }

                // The new currentPos is the end of this cubic
                currentPos.x = x3;
                currentPos.y = y3;

                skipSpaces(iss);
                // if the next char is something else, break
                if (iss.peek() == 'M' || iss.peek() == 'L' || iss.peek() == 'C' || !iss.good())
                {
                    break;
                }
            }
        }
        else
        {
            // If we see a command we don't handle (like 'm','l','c' rel coords, 'A', etc.),
            // we skip or break. Real code would handle them or parse transformations, etc.
            std::cerr << "[KamonFourier] Unhandled SVG path command: " << cmd << "\n";
            break;
        }
    }

    // If we found zero points, we consider it an error
    if (points.size() < 2)
    {
        std::cerr << "[KamonFourier] No valid points in SVG path or parser too naive.\n";
        return {};
    }

    return points;
}

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

    g_contourPoints = extractContourFromSVG(svgPath);
    if (g_contourPoints.empty())
    {
        // fallback
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
    // 1) Make sure we've initialized data
    initFourierData();
    if (!g_fourierInitialized)
        return;

    // 2) Advance time
    g_time += g_speed * 0.02f; // “dt”

    // 3) Evaluate epicycles step by step to draw circles
    std::complex<float> sumPrev(0.f, 0.f);

    sf::Color circleColor = sf::Color(100, 100, 200, 80);
    for (int i = 0; i < g_numComponents; i++)
    {
        float freq   = float(g_freqs[i]);
        auto  c      = g_coeffs[i];
        float radius = std::abs(c);

        std::complex<float> sumCurrent(0, 0);
        {
            float theta = freq * g_time;
            float re    = c.real() * std::cos(theta) - c.imag() * std::sin(theta);
            float im    = c.real() * std::sin(theta) + c.imag() * std::cos(theta);
            sumCurrent  = sumPrev + std::complex<float>(re, im);
        }

        sf::CircleShape circle(radius * 200.f);
        circle.setOrigin({radius * 200.f, radius * 200.f});
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(circleColor);
        circle.setOutlineThickness(1.f);

        // Flip Y for SFML drawing
        float cx = (sumPrev.real() * 200.f) + 450.f;
        float cy = 700.f - ((sumPrev.imag() * 200.f) + 350.f);
        circle.setPosition({cx, cy});
        window.draw(circle);

        sumPrev = sumCurrent;
    }

    // 4) The final tip => add to path
    auto         tip = sumPrev;
    sf::Vector2f tipPos((tip.real() * 200.f) + 450.f, 700.f - ((tip.imag() * 200.f) + 350.f));
    g_path.push_back(tipPos);
    if (g_path.size() > 2000)
    {
        g_path.erase(g_path.begin());
    }

    // 5) Draw the path
    sf::VertexArray pathLines(sf::PrimitiveType::LineStrip, g_path.size());
    for (size_t i = 0; i < g_path.size(); i++)
    {
        pathLines[i].position = g_path[i];
        pathLines[i].color    = sf::Color::Black;
    }
    window.draw(pathLines);
}

} // namespace KamonFourier
