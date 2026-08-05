#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class MenuScreen : public Screen {
public:
    MenuScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct Button {
        sf::FloatRect bounds;
        std::string label;   // shown text
        std::string action;  // what it does
    };

    std::vector<Button> m_buttons;
    int m_hoverIdx = -1;
    int m_pressedIdx = -1;
};
