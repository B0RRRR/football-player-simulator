#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class CareerHubScreen : public Screen {
public:
    CareerHubScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    sf::Text m_playerStatsText;
    sf::Text m_calendarText;

    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
        sf::Color baseColor = sf::Color(100, 100, 100);
        bool isHovered = false;
    };
    
    std::vector<Button> m_buttons;
};
