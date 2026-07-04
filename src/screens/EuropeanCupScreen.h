#pragma once
#include "Screen.h"
#include "Database.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class EuropeanCupScreen : public Screen {
public:
    EuropeanCupScreen();
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    void updateBracketVisuals();

    sf::Text m_titleText;
    sf::Text m_statusText;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    std::vector<Button> m_buttons;
    int m_currentView; // 0 = CL, 1 = EL, 2 = Int
    int m_selectedYear = 0;
    int m_maxYear = 0;
    
    std::vector<sf::Text> m_bracketTexts;
};
