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
    struct Button {
        sf::FloatRect bounds;
        std::string label;
        std::string action;
    };

    std::vector<Button> m_buttons;
    int m_hoverIdx = -1;
    int m_pressedIdx = -1;

    std::string m_coachMsg;
    float m_messageTimer = 0.0f;
    bool m_showAchievements = false;

    void dispatch(const std::string& action);
};
