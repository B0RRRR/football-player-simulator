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

    // winner: 0 none, 1 home, 2 away. s1 = first leg / single-match score; s2 = second leg (empty
    // for single matches), each written "h:a".
    struct BMatch { std::string home, away, s1, s2; int winner; bool me; };
    struct BRound { std::string name, date; std::vector<BMatch> matches; };
    std::vector<BRound> m_rounds;
    bool m_isNat = false; // tournament teams are national sides (use flag textures)
    std::string m_statusStr, m_viewName, m_winnerStr;

    struct Btn { sf::FloatRect bounds; std::string label, action; };
    std::vector<Btn> m_tabs;
    sf::FloatRect m_backBtn, m_prevBtn, m_nextBtn;
    std::string m_hoverAction, m_pressedAction;

    int m_currentView; // 0 = CL, 1 = EL, 2 = Int
    int m_selectedYear = 0;
    int m_maxYear = 0;

    void dispatch(const std::string& action);
};
