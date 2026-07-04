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
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };

    sf::Text m_titleText;
    sf::Text m_statsText;
    sf::Text m_infoText;
    std::vector<Button> m_buttons;
};
