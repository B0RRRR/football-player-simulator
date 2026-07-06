#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class MyStatusScreen : public Screen {
public:
    MyStatusScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    sf::Text m_infoText;
    sf::Text m_coachResponseText;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
        sf::Color baseColor;
    };
    
    std::vector<Button> m_buttons;
    
    float m_messageTimer = 0.0f;
};
