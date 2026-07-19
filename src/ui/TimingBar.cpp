#include "TimingBar.h"
#include "UITheme.h"
#include "AssetManager.h"
#include <algorithm>
#include <cmath>

namespace {
    // Bar geometry, in screen coordinates.
    const float BAR_WIDTH = 320.f;
    const float BAR_HEIGHT = 26.f;
}

void TimingBar::start(int stat, bool hardMode, float sweeps) {
    float s = std::clamp(stat / 100.f, 0.f, 1.f);

    // Better stat -> wider sweet spot and slower marker. Mirrors the difficulty
    // curve already used by the shooting drill in TrainingScreen, where the bar
    // speed scales with (100 - stat).
    // Tightened from a very forgiving window (a Perfect used to be ~a quarter of the bar
    // at mid stats) - hitting the sweet spot every time was part of why scoring was
    // trivial. Still scales with the stat so a better player has a fairer target.
    m_perfectHalf = 0.035f + s * 0.06f;  // stat 1 -> ~0.035, stat 100 -> ~0.095
    m_goodHalf = m_perfectHalf + 0.08f + s * 0.06f;

    // The marker is deliberately unhurried: the skill is hitting the zone, not
    // having superhuman reflexes. The goalkeeper's flight time is tuned against
    // this in MatchScreen::initMinigame, so keep the ceiling in mind if changing it.
    m_speed = 1.3f - s * 0.35f;          // stat 1 -> ~1.3 sweeps/s, stat 100 -> ~0.95

    if (hardMode) {
        m_perfectHalf *= 0.6f;
        m_goodHalf *= 0.75f;
        m_speed *= 1.25f;
    }

    m_duration = sweeps / m_speed;
    m_elapsed = 0.f;
    m_value = 0.f;
    m_dir = 1.f;
    m_active = true;
    m_expired = false;
}

void TimingBar::update(float dt) {
    if (!m_active) return;

    m_elapsed += dt;
    if (m_elapsed >= m_duration) {
        // Ran out of time without a press - that's a miss.
        m_active = false;
        m_expired = true;
        return;
    }

    m_value += m_dir * m_speed * dt;
    if (m_value > 1.f) {
        m_value = 1.f;
        m_dir = -1.f;
    } else if (m_value < 0.f) {
        m_value = 0.f;
        m_dir = 1.f;
    }
}

QTEGrade TimingBar::gradeFor(float value) const {
    float dist = std::abs(value - 0.5f);
    if (dist <= m_perfectHalf) return QTEGrade::Perfect;
    if (dist <= m_goodHalf) return QTEGrade::Good;
    if (dist <= m_goodHalf + 0.15f) return QTEGrade::Poor;
    return QTEGrade::Miss;
}

QTEResult TimingBar::lock() {
    QTEResult result;
    result.value = m_value;
    result.grade = gradeFor(m_value);

    // quality: 1.0 dead centre, falling off to 0 at the edge of the bar.
    float dist = std::abs(m_value - 0.5f);
    result.quality = std::clamp(1.f - dist / 0.5f, 0.f, 1.f);

    m_active = false;
    m_expired = false;
    return result;
}

void TimingBar::cancel() {
    m_active = false;
    m_expired = false;
}

void TimingBar::draw(sf::RenderWindow& window, sf::Vector2f center) const {
    if (!m_active) return;

    float left = center.x - BAR_WIDTH / 2.f;
    float top = center.y - BAR_HEIGHT / 2.f;

    // Backdrop (the "poor / miss" region).
    sf::RectangleShape bg(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
    bg.setPosition(left, top);
    bg.setFillColor(sf::Color(30, 34, 48, 230));
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(10, 12, 20, 230));
    window.draw(bg);

    // Good zone.
    sf::RectangleShape good(sf::Vector2f(BAR_WIDTH * m_goodHalf * 2.f, BAR_HEIGHT));
    good.setPosition(left + BAR_WIDTH * (0.5f - m_goodHalf), top);
    good.setFillColor(sf::Color(190, 160, 40, 220));
    window.draw(good);

    // Perfect zone.
    sf::RectangleShape perfect(sf::Vector2f(BAR_WIDTH * m_perfectHalf * 2.f, BAR_HEIGHT));
    perfect.setPosition(left + BAR_WIDTH * (0.5f - m_perfectHalf), top);
    perfect.setFillColor(sf::Color(60, 200, 90, 235));
    window.draw(perfect);

    // Marker.
    sf::RectangleShape marker(sf::Vector2f(4.f, BAR_HEIGHT + 12.f));
    marker.setPosition(left + BAR_WIDTH * m_value - 2.f, top - 6.f);
    marker.setFillColor(sf::Color::White);
    window.draw(marker);

    // Time-remaining sliver under the bar, so the expiry isn't a surprise.
    float remaining = std::clamp(1.f - m_elapsed / m_duration, 0.f, 1.f);
    sf::RectangleShape timer(sf::Vector2f(BAR_WIDTH * remaining, 3.f));
    timer.setPosition(left, top + BAR_HEIGHT + 6.f);
    timer.setFillColor(sf::Color(200, 200, 210, 160));
    window.draw(timer);
}
