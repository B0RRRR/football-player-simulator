#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// Shared drawing for anything that happens ON the pitch - used by the match and by training
// drills so both look identical. THIS is the single place future art plugs into: when players
// become sprites, the ball spins, etc., we change it here once and every screen that draws
// through PitchRenderer (match + all drills) gets it, with no external engine.
//
// World coordinates match the match: pitch rect 40,130 .. 840,450; goals at x~35 / ~845;
// goal mouth y 250..330. Everything is drawn in the caller's current view.
namespace PitchRenderer {

    inline void drawPitch(sf::RenderWindow& window) {
        sf::RectangleShape pitch(sf::Vector2f(800.f, 320.f));
        pitch.setPosition(40.f, 130.f);
        pitch.setFillColor(sf::Color(40, 140, 60));
        pitch.setOutlineThickness(3.f);
        pitch.setOutlineColor(sf::Color(200, 200, 200));
        window.draw(pitch);

        sf::RectangleShape halfway(sf::Vector2f(4.f, 320.f));
        halfway.setPosition(440.f, 130.f);
        halfway.setFillColor(sf::Color(200, 200, 200));
        window.draw(halfway);

        sf::CircleShape centre(40.f);
        centre.setPosition(400.f, 250.f);
        centre.setFillColor(sf::Color::Transparent);
        centre.setOutlineThickness(4.f);
        centre.setOutlineColor(sf::Color(200, 200, 200));
        window.draw(centre);

        for (float gx : {10.f, 840.f}) {
            sf::RectangleShape goal(sf::Vector2f(30.f, 80.f));
            goal.setPosition(gx, 250.f);
            goal.setFillColor(sf::Color::Transparent);
            goal.setOutlineThickness(3.f);
            goal.setOutlineColor(sf::Color::White);
            window.draw(goal);
        }
    }

    // A player dot. `home` picks the shirt colour (blue/red), matching the match.
    inline void drawDot(sf::RenderWindow& window, sf::Vector2f pos, bool home, bool highlight = false) {
        sf::CircleShape dot(6.f);
        dot.setOrigin(6.f, 6.f);
        dot.setPosition(pos);
        dot.setFillColor(home ? sf::Color(50, 50, 250) : sf::Color(250, 50, 50));
        window.draw(dot);
        if (highlight) {
            sf::CircleShape hl(10.f);
            hl.setOrigin(10.f, 10.f);
            hl.setPosition(pos);
            hl.setFillColor(sf::Color::Transparent);
            hl.setOutlineColor(sf::Color::Yellow);
            hl.setOutlineThickness(2.f);
            window.draw(hl);
        }
    }

    inline void drawBall(sf::RenderWindow& window, sf::Vector2f pos) {
        sf::CircleShape ball(4.f);
        ball.setOrigin(4.f, 4.f);
        ball.setPosition(pos);
        ball.setFillColor(sf::Color::White);
        window.draw(ball);
    }

    // The drawn shot/pass path (the yellow trace).
    inline void drawPath(sf::RenderWindow& window, const std::vector<sf::Vector2f>& path,
                         sf::Uint8 alpha = 210) {
        if (path.size() < 2) return;
        sf::VertexArray line(sf::LineStrip, path.size());
        for (std::size_t s = 0; s < path.size(); ++s) {
            line[s].position = path[s];
            line[s].color = sf::Color(255, 230, 90, alpha);
        }
        window.draw(line);
    }

}
