#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include "MatchEngine.h"

class MatchStatsScreen : public Screen {
public:
    MatchStatsScreen(std::shared_ptr<MatchEngine> engine);
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    std::shared_ptr<MatchEngine> m_engine;
    
    sf::Text m_titleText;
    sf::Text m_statsText;
    sf::Text m_ratingText;
    
    sf::RectangleShape m_btnContinue;
    sf::Text m_btnText;
};
