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

    std::string m_hName, m_aName;
    bool m_isNat = false;
    int m_hg = 0, m_ag = 0, m_hs = 0, m_as = 0, m_hy = 0, m_ay = 0, m_hr = 0, m_ar = 0;
    float m_rating = 6.0f;
    bool m_benched = false;
    int m_xpGain = 0, m_trustGain = 0;
    std::string m_penaltyNote;

    sf::FloatRect m_continueBtn;
    bool m_hover = false, m_pressed = false;
};
