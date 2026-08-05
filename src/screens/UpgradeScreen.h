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
    struct Row {
        sf::FloatRect bounds;
        std::string action;
        std::string icon;
    };

    std::vector<Row> m_rows;
    int m_hoverIdx = -1;
    int m_pressedIdx = -1;

    void dispatch(const std::string& action);
};
