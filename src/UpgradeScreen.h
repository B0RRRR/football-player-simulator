#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class UpgradeScreen : public Screen {
public:
    UpgradeScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    sf::Text m_xpText;
    sf::Text m_statsText;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    
    std::vector<Button> m_buttons;
    
    const int UPGRADE_COST = 100;
};
