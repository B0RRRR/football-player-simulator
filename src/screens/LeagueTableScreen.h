#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class LeagueTableScreen : public Screen {
public:
    LeagueTableScreen();

    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct Standing {
        int pos; std::string club; bool isNat;
        int pts, w, d, l, gf, ga, gd; bool isMe;
        int zone; // 0 none, 1 green (CL/promotion), 2 orange (EL), 3 red (relegation)
    };
    bool m_topTier = false, m_secondTier = false;
    std::vector<Standing> m_standings;
    std::string m_titleStr;

    struct Tab { sf::FloatRect bounds; std::string name; };
    std::vector<Tab> m_leagueTabs;

    sf::FloatRect m_backBtn, m_prevBtn, m_nextBtn;
    std::string m_hoverAction, m_pressedAction;

    int m_viewedYear = 0;
    std::string m_viewedLeagueName;

    void dispatch(const std::string& action);
};
