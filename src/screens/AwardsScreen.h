#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class AwardsScreen : public Screen {
public:
    AwardsScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct AwardResult {
        std::string title;
        std::string winnerName;
        std::string winnerClub;
        std::string statInfo;
        bool isRealPlayer;
    };
    
    std::vector<AwardResult> m_awards;
    int m_currentAwardIndex = 0;

    struct Button { sf::FloatRect bounds; std::string label; std::string action; };
    std::vector<Button> m_buttons;
    int m_hoverIdx = -1, m_pressedIdx = -1;

    void processAwards();
    void showCurrentAward();
};
