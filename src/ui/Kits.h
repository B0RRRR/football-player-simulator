#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>
#include <functional>

// Club kit colours. There is no per-club colour data, so a stable colour is derived from the
// club's name (same club -> same kit every time). Each club has a home and an away kit; when the
// two sides would look alike, the away side switches kits (and, if still too close, falls back to
// a guaranteed-contrasting colour). Greens are avoided so shirts never blend into the pitch.
namespace Kits {

    inline sf::Color hsv(float h, float s, float v) {
        h = std::fmod(std::fmod(h, 360.f) + 360.f, 360.f);
        float c = v * s;
        float x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
        float m = v - c;
        float r = 0, g = 0, b = 0;
        if (h < 60)       { r = c; g = x; }
        else if (h < 120) { r = x; g = c; }
        else if (h < 180) { g = c; b = x; }
        else if (h < 240) { g = x; b = c; }
        else if (h < 300) { r = x; b = c; }
        else              { r = c; b = x; }
        return sf::Color((sf::Uint8)((r + m) * 255), (sf::Uint8)((g + m) * 255), (sf::Uint8)((b + m) * 255));
    }

    // Push a hue out of the pitch-green band so shirts stay legible on grass.
    inline float avoidGreen(float h) {
        return (h > 75.f && h < 165.f) ? std::fmod(h + 180.f, 360.f) : h;
    }

    inline float hueOf(const std::string& name) {
        return avoidGreen((float)(std::hash<std::string>{}(name) % 360u));
    }

    inline sf::Color home(const std::string& name) { return hsv(hueOf(name), 0.62f, 0.82f); }
    inline sf::Color away(const std::string& name) {
        return hsv(avoidGreen(hueOf(name) + 150.f), 0.55f, 0.95f); // shifted hue, brighter
    }

    inline int dist2(sf::Color a, sf::Color b) {
        int dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
        return dr * dr + dg * dg + db * db;
    }

    // A keeper kit: a high-visibility colour as far as possible from the given colours (both
    // teams' shirts, the other keeper) and always from the pitch green.
    inline sf::Color keeper(std::initializer_list<sf::Color> avoid) {
        static const sf::Color palette[] = {
            sf::Color(245, 140, 20),   // orange
            sf::Color(238, 208, 45),   // gold
            sf::Color(225, 55, 175),   // magenta
            sf::Color(45, 200, 225),   // cyan
            sf::Color(38, 42, 56),     // charcoal
        };
        const sf::Color green(40, 140, 60);
        sf::Color best = palette[0];
        int bestScore = -1;
        for (const auto& c : palette) {
            int minD = dist2(c, green);
            for (const auto& a : avoid) minD = std::min(minD, dist2(c, a));
            if (minD > bestScore) { bestScore = minD; best = c; }
        }
        return best;
    }

    inline bool clash(sf::Color a, sf::Color b) {
        int dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
        return (dr * dr + dg * dg + db * db) < 8100; // ~90 units apart
    }

    // Pick distinct shirts for a fixture: home wears its home kit; away switches to its away kit
    // (then to a hard fallback) if it would clash.
    inline void resolve(const std::string& homeClub, const std::string& awayClub,
                        sf::Color& homeShirt, sf::Color& awayShirt) {
        homeShirt = home(homeClub);
        awayShirt = home(awayClub);
        if (clash(homeShirt, awayShirt)) awayShirt = away(awayClub);
        if (clash(homeShirt, awayShirt)) {
            int lum = (homeShirt.r * 3 + homeShirt.g * 6 + homeShirt.b) / 10;
            awayShirt = (lum > 128) ? sf::Color(22, 30, 82) : sf::Color(238, 238, 244);
        }
    }

}
