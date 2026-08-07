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
    struct Offer { Club* club; int offeredSalary; };

    // One clickable widget. `kind` decides how it draws and what it does.
    struct Click {
        sf::FloatRect bounds;
        std::string label;
        std::string action;      // TAB_INBOX/TAB_SEARCH/BACK/PREV_LEAGUE/NEXT_LEAGUE/NEXT_PAGE/PREV_PAGE/APPLY/ACCEPT
        Club* targetClub = nullptr;
        Offer offer{nullptr, 0};
        std::string logo;        // club name for a crest (offer/club rows)
        std::string rightText;   // salary / STR shown on the right
        bool isCard = false;     // draw as a club/offer card rather than a plain button
    };

    std::vector<Click> m_click;
    std::vector<Offer> m_offers;
    int m_hoverIdx = -1, m_pressedIdx = -1;

    enum class Tab { Inbox, Search };
    Tab m_currentTab = Tab::Inbox;
    int m_searchLeagueIdx = 0;
    int m_searchPage = 0;

    std::string m_infoStr, m_messageStr, m_searchLeagueName;
    float m_messageTimer = 0.0f;

    void refreshTab();
    void buildInboxTab();
    void buildSearchTab();
    void generateOffersIfNeeded();
    void attemptTransfer(Club* targetClub);
    void dispatch(const Click& c);
};
