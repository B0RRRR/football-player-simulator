#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class LeagueTableScreen : public Screen {
public:
    LeagueTableScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    std::vector<sf::Text> m_tableRows;
    std::vector<sf::Sprite> m_tableLogos;

    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    
    Button m_backButton;
};
