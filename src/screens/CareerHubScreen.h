#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class CareerHubScreen : public Screen {
public:
    CareerHubScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct Button {
        sf::FloatRect bounds;
        std::string label;
        std::string action;
        bool primary = false;   // accent-highlighted (Advance Day / Transfer)
    };

    std::vector<Button> m_buttons;   // main action column + debug row (index 0 is dynamic)
    Button m_transfer;
    bool m_showTransfer = false;

    std::string m_hoverAction, m_pressedAction;

    // Header text, rebuilt each frame in update().
    std::string m_clubTitle;
    std::string m_line1, m_line2, m_notice;
    sf::Color m_noticeColor = sf::Color::Yellow;

    void dispatch(sf::RenderWindow& window, const std::string& action);
};
