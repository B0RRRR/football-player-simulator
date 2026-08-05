#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

// Pure geometry for the "draw a shot/pass path" mechanic, shared by the match (MatchScreen)
// and training (DrillArena) so the ball flies identically in both and there is one source of
// truth. No SFML rendering, no game state - just a polyline of world-space points.
namespace ShotPath {

    inline float length(const std::vector<sf::Vector2f>& path) {
        float len = 0.f;
        for (std::size_t i = 1; i < path.size(); ++i)
            len += std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
        return len;
    }

    // Approximate the raw trace with a single quadratic-Bezier arc bowed to the SAME side the
    // player actually drew (can't invert - the control point is placed on whichever side the
    // traced points lie). Guarantees one smooth, plausible curve.
    inline void constrainToArc(std::vector<sf::Vector2f>& path) {
        if (path.size() < 3 || length(path) < 12.f) return;

        sf::Vector2f p0 = path.front();
        sf::Vector2f p2 = path.back();
        sf::Vector2f chord = p2 - p0;
        float L = std::hypot(chord.x, chord.y);
        if (L < 8.f) return;
        sf::Vector2f perp(-chord.y / L, chord.x / L);

        float bestOff = 0.f;
        for (const auto& pt : path) {
            float off = (pt.x - p0.x) * perp.x + (pt.y - p0.y) * perp.y;
            if (std::fabs(off) > std::fabs(bestOff)) bestOff = off;
        }
        float maxOff = std::min(120.f, L * 0.6f);
        bestOff = std::clamp(bestOff, -maxOff, maxOff);

        sf::Vector2f mid = (p0 + p2) * 0.5f;
        sf::Vector2f p1 = mid + perp * (bestOff * 2.f);

        std::vector<sf::Vector2f> out;
        const int N = 20;
        for (int s = 0; s <= N; ++s) {
            float t = (float)s / N, u = 1.f - t;
            out.push_back(u * u * p0 + 2.f * u * t * p1 + t * t * p2);
        }
        path.swap(out);
    }

    // Chaikin corner-cutting, keeping the endpoints fixed, for a smooth curve.
    inline void chaikin(std::vector<sf::Vector2f>& path, int passes) {
        for (int pass = 0; pass < passes && path.size() >= 3; ++pass) {
            std::vector<sf::Vector2f> out;
            out.push_back(path.front());
            for (std::size_t i = 0; i + 1 < path.size(); ++i) {
                sf::Vector2f a = path[i], b = path[i + 1];
                out.push_back(a + (b - a) * 0.25f);
                out.push_back(a + (b - a) * 0.75f);
            }
            out.push_back(path.back());
            path.swap(out);
        }
    }

    // Position `dist` px along the path; once past its end, continue straight along `endDir`.
    inline sf::Vector2f pointAlong(const std::vector<sf::Vector2f>& path, sf::Vector2f from,
                                   sf::Vector2f endDir, float dist) {
        if (path.empty()) return from;
        if (path.size() == 1) return path[0] + endDir * dist;
        float acc = 0.f;
        for (std::size_t i = 1; i < path.size(); ++i) {
            sf::Vector2f a = path[i - 1], b = path[i];
            float seg = std::hypot(b.x - a.x, b.y - a.y);
            if (dist <= acc + seg) {
                float u = (seg > 0.f) ? (dist - acc) / seg : 0.f;
                return a + (b - a) * u;
            }
            acc += seg;
        }
        return path.back() + endDir * (dist - acc);
    }

}
