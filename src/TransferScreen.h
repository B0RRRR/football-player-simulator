#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Club;

class TransferScreen : public Screen {
public:
    TransferScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Text m_titleText;
    sf::Text m_infoText;

    struct Offer {
        Club* club;
        int offeredSalary;
    };

    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
        Offer offer;
    };

    std::vector<Button> m_buttons;
    std::vector<Offer> m_offers;

    void generateOffers();
};
