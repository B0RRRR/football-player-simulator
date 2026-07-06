#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct Club;

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
    sf::Text m_messageText;

    struct Offer {
        Club* club;
        int offeredSalary;
    };

    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
        Offer offer;
        Club* targetClub = nullptr;
        sf::Color baseColor;
        bool isHovered = false;
    };

    std::vector<Button> m_navButtons;
    std::vector<Button> m_contentButtons;
    std::vector<Offer> m_offers;

    enum class Tab {
        Inbox,
        Search
    };
    
    Tab m_currentTab = Tab::Inbox;
    
    int m_searchLeagueIdx = 0;
    int m_searchPage = 0;
    
    float m_messageTimer = 0.0f;

    void refreshTab();
    void buildInboxTab();
    void buildSearchTab();
    void generateOffersIfNeeded();
    void attemptTransfer(Club* targetClub);
};
