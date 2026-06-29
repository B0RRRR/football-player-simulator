#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class SettingsScreen : public Screen {
public:
    SettingsScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    sf::Text m_diffText;
    sf::Text m_speedText;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    
    std::vector<Button> m_buttons;
    
    void updateDifficultyText();
    void updateSpeedText();
};
