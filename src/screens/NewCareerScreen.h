#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class SetupState {
    InputName,
    SelectNationality,
    SelectPosition,
    SelectLeague,
    SelectClub
};

class NewCareerScreen : public Screen {
public:
    NewCareerScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    void rebuildButtons();

    SetupState m_state;
    
    std::string m_playerName;
    std::string m_selectedNationality;
    int m_selectedPosition; // 0..3
    std::string m_selectedLeague;
    std::string m_selectedClub;

    std::string m_title;
    std::string m_info;

    struct Button {
        sf::FloatRect bounds;
        std::string label;
        std::string action;
        std::string flag;   // country name for a flag card (empty = plain button)
        std::string logo;   // club name for a logo card (empty = plain button)
    };

    std::vector<Button> m_buttons;
    int m_hoverIdx = -1;
    int m_pressedIdx = -1;
    float m_caret = 0.f; // blinking caret timer for the name field
};
