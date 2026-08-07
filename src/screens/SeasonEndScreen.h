#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class SeasonEndScreen : public Screen {
public:
    SeasonEndScreen();
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    struct Button { sf::FloatRect bounds; std::string label; std::string action; bool primary = false; };
    std::vector<Button> m_buttons;
    int m_hoverIdx = -1, m_pressedIdx = -1;
};
