#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class EventScreen : public Screen {
public:
    EventScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    sf::Text m_descriptionText;

    struct EventOption {
        std::string text;
        int xpChange;
        int moraleChange;
        int energyChange;
    };

    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        EventOption option;
    };

    std::vector<Button> m_buttons;

    void generateRandomEvent();
};
