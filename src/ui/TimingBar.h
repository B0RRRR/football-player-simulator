#pragma once
#include <SFML/Graphics.hpp>

// A reusable timing minigame (QTE): a marker sweeps back and forth across a bar
// and the player locks it with a key press. How close the marker is to the sweet
// spot at the centre decides the grade.
//
// The player's relevant stat (shooting/passing/tackling/goalkeeping) controls the
// difficulty - a better player gets a wider sweet spot and a slower marker - so
// outcomes come from the player's timing rather than a hidden dice roll.

enum class QTEGrade {
    Perfect,
    Good,
    Poor,
    Miss
};

struct QTEResult {
    QTEGrade grade = QTEGrade::Miss;
    float value = 0.f;   // 0..1, where the marker was locked
    float quality = 0.f; // 0..1, closeness to the sweet spot centre (1 = dead centre)
};

class TimingBar {
public:
    // stat: 1..100, the attribute governing this action.
    // hardMode: shrinks the zones further (risky actions like a lofted through ball).
    // sweeps: how many full back-and-forth passes before the QTE expires as a miss.
    void start(int stat, bool hardMode = false, float sweeps = 2.0f);
    void update(float dt);
    QTEResult lock(); // locks the marker, deactivates the bar, returns the grade
    void cancel();

    bool isActive() const { return m_active; }
    bool isExpired() const { return m_expired; }

    void draw(sf::RenderWindow& window, sf::Vector2f center) const;

private:
    QTEGrade gradeFor(float value) const;

    bool m_active = false;
    bool m_expired = false;

    float m_value = 0.f;      // 0..1, marker position
    float m_dir = 1.f;        // sweep direction
    float m_speed = 1.0f;     // full traversals per second
    float m_elapsed = 0.f;
    float m_duration = 0.f;   // seconds before expiry

    float m_perfectHalf = 0.08f; // half-width of the perfect zone, in 0..1 units
    float m_goodHalf = 0.20f;    // half-width of the good zone
};
