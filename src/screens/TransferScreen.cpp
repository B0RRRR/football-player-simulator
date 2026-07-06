#include "UITheme.h"
#include "TransferScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "Database.h"
#include "Player.h"
#include "AssetManager.h"
#include <algorithm>
#include <cstdlib>
#include <random>

TransferScreen::TransferScreen() {
}

void TransferScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::Yellow);
    m_titleText.setPosition(50.f, 30.f);
    m_titleText.setString("Transfer Center");
    
    m_infoText.setFont(font);
    m_infoText.setCharacterSize(20);
    m_infoText.setFillColor(sf::Color::White);
    m_infoText.setPosition(50.f, 90.f);

    m_messageText.setFont(font);
    m_messageText.setCharacterSize(20);
    m_messageText.setFillColor(sf::Color::Cyan);
    m_messageText.setPosition(500.f, 90.f);
    m_messageText.setString("");

    // Nav buttons
    Button btnInbox;
    btnInbox.rect.setSize(sf::Vector2f(200.f, 40.f));
    btnInbox.rect.setPosition(50.f, 130.f);
    btnInbox.baseColor = sf::Color(100, 100, 100);
    btnInbox.text.setFont(font);
    btnInbox.text.setString("Inbox / Offers");
    btnInbox.text.setCharacterSize(18);
    btnInbox.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = btnInbox.text.getLocalBounds();
    btnInbox.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnInbox.text.setPosition(btnInbox.rect.getPosition().x + btnInbox.rect.getSize().x/2.0f,
                              btnInbox.rect.getPosition().y + btnInbox.rect.getSize().y/2.0f);
    btnInbox.action = "TAB_INBOX";
    m_navButtons.push_back(btnInbox);

    Button btnSearch;
    btnSearch.rect.setSize(sf::Vector2f(200.f, 40.f));
    btnSearch.rect.setPosition(260.f, 130.f);
    btnSearch.baseColor = sf::Color(100, 100, 100);
    btnSearch.text.setFont(font);
    btnSearch.text.setString("Search Clubs");
    btnSearch.text.setCharacterSize(18);
    btnSearch.text.setFillColor(sf::Color::White);
    textRect = btnSearch.text.getLocalBounds();
    btnSearch.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnSearch.text.setPosition(btnSearch.rect.getPosition().x + btnSearch.rect.getSize().x/2.0f,
                               btnSearch.rect.getPosition().y + btnSearch.rect.getSize().y/2.0f);
    btnSearch.action = "TAB_SEARCH";
    m_navButtons.push_back(btnSearch);

    Button btnBack;
    btnBack.rect.setSize(sf::Vector2f(200.f, 40.f));
    btnBack.rect.setPosition(1000.f, 30.f);
    btnBack.baseColor = sf::Color(150, 50, 50);
    btnBack.text.setFont(font);
    btnBack.text.setString("Back to Hub");
    btnBack.text.setCharacterSize(18);
    btnBack.text.setFillColor(sf::Color::White);
    textRect = btnBack.text.getLocalBounds();
    btnBack.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnBack.text.setPosition(btnBack.rect.getPosition().x + btnBack.rect.getSize().x/2.0f,
                             btnBack.rect.getPosition().y + btnBack.rect.getSize().y/2.0f);
    btnBack.action = "BACK";
    m_navButtons.push_back(btnBack);

    generateOffersIfNeeded();
    refreshTab();
}

void TransferScreen::generateOffersIfNeeded() {
    if (!m_offers.empty()) return; // Already generated
    
    Player* p = m_gameManager->getPlayer();
    Database& db = m_gameManager->getDatabase();

    // Chances of getting random offers depend on isTransferListed
    int baseChance = p->isTransferListed ? 60 : 15;
    if (rand() % 100 > baseChance) return; // No random offers this window

    int playerOverall = (p->shooting + p->passing + p->tackling + p->goalkeeping) / 4;
    int targetStrength = playerOverall + ((p->goals + p->assists) / 5);
    if (targetStrength > 90) targetStrength = 90;
    
    std::vector<Club*> suitableClubs;
    const auto& leagues = db.getLeagues();
    for (const auto& l : leagues) {
        for (const auto& c : l.clubs) {
            if (p->currentClub && c.name == p->currentClub->name) continue;
            // Transfer listed -> willing to accept slightly lower clubs too
            int margin = p->isTransferListed ? 12 : 6;
            if (abs(c.strength - targetStrength) <= margin) {
                suitableClubs.push_back(db.getClub(l.name, c.name));
            }
        }
    }

    if (suitableClubs.size() > 3) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(suitableClubs.begin(), suitableClubs.end(), g);
        suitableClubs.resize(3);
    }

    for (auto* c : suitableClubs) {
        Offer off;
        off.club = c;
        off.offeredSalary = c->strength * 50 + (rand() % 1000);
        m_offers.push_back(off);
    }
}

void TransferScreen::refreshTab() {
    m_contentButtons.clear();
    auto& font = AssetManager::get().getFont("MainFont");

    // Update nav colors
    for (auto& btn : m_navButtons) {
        if (btn.action == "TAB_INBOX") {
            btn.baseColor = (m_currentTab == Tab::Inbox) ? sf::Color(50, 150, 50) : sf::Color(100, 100, 100);
        } else if (btn.action == "TAB_SEARCH") {
            btn.baseColor = (m_currentTab == Tab::Search) ? sf::Color(50, 150, 50) : sf::Color(100, 100, 100);
        }
    }

    if (m_currentTab == Tab::Inbox) {
        buildInboxTab();
    } else {
        buildSearchTab();
    }
}

void TransferScreen::buildInboxTab() {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();

    if (m_offers.empty()) {
        m_infoText.setString("No pending offers at the moment.\nYour agent says you should wait or apply manually.");
        return;
    }
    
    m_infoText.setString("You have " + std::to_string(m_offers.size()) + " pending offers:");

    float startY = 220.f;
    for (size_t i = 0; i < m_offers.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(600.f, 60.f));
        btn.rect.setPosition(50.f, startY + i * 80.f);
        btn.baseColor = UITheme::ButtonNormal;
        
        btn.text.setFont(font);
        std::string label = m_offers[i].club->name + " (STR: " + std::to_string(m_offers[i].club->strength) + ") - Salary: $" + std::to_string(m_offers[i].offeredSalary) + "/w";
        btn.text.setString(sf::String::fromUtf8(label.begin(), label.end()));
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        
        btn.action = "ACCEPT";
        btn.offer = m_offers[i];
        m_contentButtons.push_back(btn);
    }
}

void TransferScreen::buildSearchTab() {
    auto& font = AssetManager::get().getFont("MainFont");
    Database& db = m_gameManager->getDatabase();
    const auto& leagues = db.getLeagues();

    m_infoText.setString("Browse clubs and declare your interest.");

    if (leagues.empty()) return;

    if (m_searchLeagueIdx < 0) m_searchLeagueIdx = leagues.size() - 1;
    if (m_searchLeagueIdx >= leagues.size()) m_searchLeagueIdx = 0;

    const League& currentLeague = leagues[m_searchLeagueIdx];

    Button prevLeague;
    prevLeague.rect.setSize(sf::Vector2f(120.f, 30.f));
    prevLeague.rect.setPosition(50.f, 200.f);
    prevLeague.baseColor = sf::Color(100, 100, 100);
    prevLeague.text.setFont(font);
    prevLeague.text.setString("< League");
    prevLeague.text.setCharacterSize(16);
    prevLeague.text.setFillColor(sf::Color::White);
    sf::FloatRect tr = prevLeague.text.getLocalBounds();
    prevLeague.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
    prevLeague.text.setPosition(prevLeague.rect.getPosition().x + prevLeague.rect.getSize().x/2.0f,
                                prevLeague.rect.getPosition().y + prevLeague.rect.getSize().y/2.0f);
    prevLeague.action = "PREV_LEAGUE";
    m_contentButtons.push_back(prevLeague);

    Button nextLeague;
    nextLeague.rect.setSize(sf::Vector2f(120.f, 30.f));
    nextLeague.rect.setPosition(180.f, 200.f);
    nextLeague.baseColor = sf::Color(100, 100, 100);
    nextLeague.text.setFont(font);
    nextLeague.text.setString("League >");
    nextLeague.text.setCharacterSize(16);
    nextLeague.text.setFillColor(sf::Color::White);
    tr = nextLeague.text.getLocalBounds();
    nextLeague.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
    nextLeague.text.setPosition(nextLeague.rect.getPosition().x + nextLeague.rect.getSize().x/2.0f,
                                nextLeague.rect.getPosition().y + nextLeague.rect.getSize().y/2.0f);
    nextLeague.action = "NEXT_LEAGUE";
    m_contentButtons.push_back(nextLeague);

    sf::Text leagueTitle;
    leagueTitle.setFont(font);
    leagueTitle.setString(currentLeague.name);
    leagueTitle.setCharacterSize(24);
    leagueTitle.setFillColor(sf::Color::Yellow);
    leagueTitle.setPosition(320.f, 200.f);
    // Not a button, just render it in draw loop or store it. Since we re-create, we can just let it be.
    // Wait, let's make a dummy button to hold text for simplicity.
    Button lbl;
    lbl.rect.setSize(sf::Vector2f(0,0));
    lbl.text = leagueTitle;
    lbl.action = "NONE";
    lbl.baseColor = sf::Color::Transparent;
    m_contentButtons.push_back(lbl);

    float startY = 250.f;
    int clubsPerPage = 7;
    int startIndex = m_searchPage * clubsPerPage;
    
    for (int i = 0; i < clubsPerPage && (startIndex + i) < currentLeague.clubs.size(); ++i) {
        const Club& c = currentLeague.clubs[startIndex + i];
        
        Button applyBtn;
        applyBtn.rect.setSize(sf::Vector2f(500.f, 40.f));
        applyBtn.rect.setPosition(50.f, startY + i * 50.f);
        applyBtn.baseColor = sf::Color(70, 70, 70);
        
        applyBtn.text.setFont(font);
        applyBtn.text.setString(c.name + " (STR: " + std::to_string(c.strength) + ")");
        applyBtn.text.setCharacterSize(18);
        applyBtn.text.setFillColor(sf::Color::White);
        tr = applyBtn.text.getLocalBounds();
        applyBtn.text.setOrigin(0, tr.top + tr.height/2.0f);
        applyBtn.text.setPosition(60.f, applyBtn.rect.getPosition().y + applyBtn.rect.getSize().y/2.0f);
        
        applyBtn.action = "APPLY";
        applyBtn.targetClub = db.getClub(currentLeague.name, c.name);
        m_contentButtons.push_back(applyBtn);
    }

    if (startIndex + clubsPerPage < currentLeague.clubs.size()) {
        Button nextP;
        nextP.rect.setSize(sf::Vector2f(100.f, 30.f));
        nextP.rect.setPosition(500.f, 620.f);
        nextP.baseColor = sf::Color(100, 100, 100);
        nextP.text.setFont(font);
        nextP.text.setString("Next Page");
        nextP.text.setCharacterSize(16);
        nextP.text.setFillColor(sf::Color::White);
        tr = nextP.text.getLocalBounds();
        nextP.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
        nextP.text.setPosition(nextP.rect.getPosition().x + nextP.rect.getSize().x/2.0f,
                                nextP.rect.getPosition().y + nextP.rect.getSize().y/2.0f);
        nextP.action = "NEXT_PAGE";
        m_contentButtons.push_back(nextP);
    }
    
    if (m_searchPage > 0) {
        Button prevP;
        prevP.rect.setSize(sf::Vector2f(100.f, 30.f));
        prevP.rect.setPosition(380.f, 620.f);
        prevP.baseColor = sf::Color(100, 100, 100);
        prevP.text.setFont(font);
        prevP.text.setString("Prev Page");
        prevP.text.setCharacterSize(16);
        prevP.text.setFillColor(sf::Color::White);
        tr = prevP.text.getLocalBounds();
        prevP.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
        prevP.text.setPosition(prevP.rect.getPosition().x + prevP.rect.getSize().x/2.0f,
                                prevP.rect.getPosition().y + prevP.rect.getSize().y/2.0f);
        prevP.action = "PREV_PAGE";
        m_contentButtons.push_back(prevP);
    }
}

void TransferScreen::attemptTransfer(Club* targetClub) {
    if (!targetClub) return;
    Player* p = m_gameManager->getPlayer();
    
    if (p->currentClub && targetClub->name == p->currentClub->name) {
        m_messageText.setString("You are already playing for " + targetClub->name + "!");
        m_messageTimer = 3.0f;
        return;
    }

    int playerOverall = (p->shooting + p->passing + p->tackling + p->goalkeeping) / 4;
    int targetStrength = playerOverall + ((p->goals + p->assists) / 5);
    
    // Check if club is interested
    int diff = targetStrength - targetClub->strength;
    
    if (diff < -8) {
        m_messageText.setString("Refused! " + targetClub->name + " thinks you are not good enough.");
        m_messageTimer = 4.0f;
    } else if (diff > 20) {
        m_messageText.setString("Refused! " + targetClub->name + " cannot afford your wages.");
        m_messageTimer = 4.0f;
    } else {
        // Send offer to Inbox
        // Check if offer already exists
        for (const auto& off : m_offers) {
            if (off.club->name == targetClub->name) {
                m_messageText.setString("You already have a pending offer from " + targetClub->name + "!");
                m_messageTimer = 3.0f;
                return;
            }
        }
        
        Offer off;
        off.club = targetClub;
        off.offeredSalary = targetClub->strength * 50 + (rand() % 1000);
        
        // If you force a transfer, salary might be slightly worse
        if (p->isTransferListed) {
            off.offeredSalary = (int)(off.offeredSalary * 0.9f);
        }
        
        m_offers.push_back(off);
        m_messageText.setString("Success! " + targetClub->name + " sent an offer to your inbox.");
        m_messageTimer = 4.0f;
    }
}

void TransferScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_navButtons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "BACK") {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                    return;
                } else if (btn.action == "TAB_INBOX") {
                    m_currentTab = Tab::Inbox;
                    refreshTab();
                } else if (btn.action == "TAB_SEARCH") {
                    m_currentTab = Tab::Search;
                    refreshTab();
                }
            }
        }

        for (auto& btn : m_contentButtons) {
            if (btn.action != "NONE" && btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "PREV_LEAGUE") {
                    m_searchLeagueIdx--;
                    m_searchPage = 0;
                    refreshTab();
                } else if (btn.action == "NEXT_LEAGUE") {
                    m_searchLeagueIdx++;
                    m_searchPage = 0;
                    refreshTab();
                } else if (btn.action == "NEXT_PAGE") {
                    m_searchPage++;
                    refreshTab();
                } else if (btn.action == "PREV_PAGE") {
                    m_searchPage--;
                    refreshTab();
                } else if (btn.action == "APPLY") {
                    attemptTransfer(btn.targetClub);
                } else if (btn.action == "ACCEPT") {
                    Player* p = m_gameManager->getPlayer();
                    p->currentClub = m_gameManager->getDatabase().getClub("", btn.offer.club->name);
                    if (!p->currentClub) p->currentClub = btn.offer.club; // fallback
                    
                    p->salary = btn.offer.offeredSalary;
                    p->coachTrust = 50.0f;
                    p->isTransferListed = false;
                    
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                    return;
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        auto applyHover = [&](std::vector<Button>& buttons) {
            for (auto& btn : buttons) {
                if (btn.action != "NONE") {
                    if (btn.rect.getGlobalBounds().contains(mousePos)) {
                        btn.rect.setFillColor(sf::Color(btn.baseColor.r + 30, btn.baseColor.g + 30, btn.baseColor.b + 30));
                    } else {
                        btn.rect.setFillColor(btn.baseColor);
                    }
                }
            }
        };
        applyHover(m_navButtons);
        applyHover(m_contentButtons);
    }
}

void TransferScreen::update(sf::Time deltaTime) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= deltaTime.asSeconds();
        if (m_messageTimer <= 0.0f) {
            m_messageText.setString("");
        }
    }
}

void TransferScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    
    window.draw(m_titleText);
    window.draw(m_infoText);
    window.draw(m_messageText);
    
    for (const auto& btn : m_navButtons) {
        if (btn.action != "NONE" && btn.rect.getSize().x > 0) window.draw(btn.rect);
        window.draw(btn.text);
    }
    
    for (const auto& btn : m_contentButtons) {
        if (btn.action != "NONE" && btn.rect.getSize().x > 0) window.draw(btn.rect);
        window.draw(btn.text);
    }
}
