#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include "UITheme.h"
#include "Database.h"

class GameManager;

class SquadScreen : public Screen {
public:
    SquadScreen();
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Font m_font;
    sf::Text m_titleText;
    
    struct PlayerRow {
        sf::Text name;
        sf::Text pos;
        sf::Text nat;
        sf::Text ovr;
        sf::Text goals;
        sf::Text assists;
        sf::RectangleShape bg;
    };
    
    std::vector<PlayerRow> m_rows;
    
    // UI
    sf::RectangleShape m_btnBack;
    sf::Text m_btnBackText;
    bool m_btnBackHovered = false;
    
    sf::Text m_headerName;
    sf::Text m_headerPos;
    sf::Text m_headerNat;
    sf::Text m_headerOvr;
    sf::Text m_headerGoals;
    sf::Text m_headerAssists;
    
    float m_scrollOffset = 0.f;
    float m_maxScroll = 0.f;
    float m_rowHeight = 40.f;
    float m_startY = 150.f;
};
