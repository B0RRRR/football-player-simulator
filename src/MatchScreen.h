#pragma once
#include "Screen.h"
#include "Match.h"
#include "Player.h"
#include <SFML/Graphics.hpp>
#include <vector>

class MatchScreen : public Screen {
public:
    MatchScreen();
    ~MatchScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Player* m_player;
    Match* m_match;
    
    sf::Text m_scoreText;
    sf::Text m_logsText;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    
    std::vector<Button> m_actionButtons; // Shoot, Pass
    Button m_backButton; // After match finishes
    
    void createActionButtons();
};
