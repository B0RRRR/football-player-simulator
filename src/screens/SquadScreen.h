#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include "Player.h"
#include <string>
#include <vector>

class GameManager;

class SquadScreen : public Screen {
public:
    SquadScreen();
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct Row {
        std::string name, nat;
        PlayerPosition pos;
        int ovr, goals, assists;
        bool isMe;
    };
    std::vector<Row> m_rows;
    std::string m_title;

    sf::FloatRect m_backBtn;
    bool m_backHover = false, m_backPressed = false;

    float m_scroll = 0.f, m_maxScroll = 0.f;
    const float m_rowH = 46.f, m_bandTop = 210.f, m_bandBot = 596.f;
};
