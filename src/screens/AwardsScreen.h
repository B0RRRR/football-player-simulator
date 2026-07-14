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
    
    sf::Text m_titleText;
    sf::Text m_awardTitleText;
    sf::Text m_winnerNameText;
    sf::Text m_statInfoText;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    
    std::vector<Button> m_buttons;
    
    void processAwards();
    void showCurrentAward();
};
