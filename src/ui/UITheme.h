#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace UITheme {
    // Color Palette
    const sf::Color BackgroundDark(15, 20, 35);
    const sf::Color BackgroundLight(35, 45, 75);
    const sf::Color ButtonNormal(50, 70, 110);
    const sf::Color ButtonHover(70, 100, 150);
    const sf::Color ButtonActive(100, 150, 200);
    const sf::Color TextWhite(240, 240, 245);
    const sf::Color TextDim(180, 190, 210);
    const sf::Color Highlight(255, 215, 0); // Gold for active/selection

    // Draw a vertical gradient background filling the window
    inline void drawGradientBackground(sf::RenderWindow& window) {
        sf::VertexArray bg(sf::Quads, 4);
        sf::Vector2f size(window.getView().getSize());
        
        bg[0].position = sf::Vector2f(0, 0);
        bg[1].position = sf::Vector2f(size.x, 0);
        bg[2].position = sf::Vector2f(size.x, size.y);
        bg[3].position = sf::Vector2f(0, size.y);

        bg[0].color = BackgroundDark;
        bg[1].color = BackgroundDark;
        bg[2].color = BackgroundLight;
        bg[3].color = BackgroundLight;

        window.draw(bg);
    }
}
