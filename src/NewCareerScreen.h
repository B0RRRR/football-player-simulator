#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class SetupState {
    InputName,
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
    int m_selectedPosition; // 0..3
    std::string m_selectedLeague;
    std::string m_selectedClub;

    sf::Text m_titleText;
    sf::Text m_infoText;
    sf::Text m_inputText; // For name input
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
    };
    
    std::vector<Button> m_buttons;
};
