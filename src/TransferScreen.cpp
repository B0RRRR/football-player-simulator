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
    m_titleText.setPosition(100.f, 50.f);
    m_titleText.setString("Transfer Window Open!");
    
    m_infoText.setFont(font);
    m_infoText.setCharacterSize(24);
    m_infoText.setFillColor(sf::Color::White);
    m_infoText.setPosition(100.f, 120.f);
    m_infoText.setString("These clubs are interested in signing you:");

    generateOffers();
}

void TransferScreen::generateOffers() {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();
    Database& db = m_gameManager->getDatabase();

    int playerOverall = (p->shooting + p->passing + p->tackling + p->goalkeeping) / 4;
    // Add performance bonus
    int performanceBonus = (p->goals * 2 + p->assists) / 10;
    int targetStrength = playerOverall + performanceBonus;
    if (targetStrength > 90) targetStrength = 90;
    
    // We want clubs with strength within [targetStrength - 5, targetStrength + 5]
    std::vector<Club*> suitableClubs;
    const auto& leagues = db.getLeagues();
    for (const auto& l : leagues) {
        for (const auto& c : l.clubs) {
            // Can't move to current club
            if (p->currentClub && c.name == p->currentClub->name) continue;
            
            if (abs(c.strength - targetStrength) <= 8) {
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

    m_offers.clear();
    for (auto* c : suitableClubs) {
        Offer off;
        off.club = c;
        // Salary depends on club strength
        off.offeredSalary = c->strength * 50 + (rand() % 1000);
        m_offers.push_back(off);
    }

    float startY = 200.f;
    for (size_t i = 0; i < m_offers.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(600.f, 60.f));
        btn.rect.setPosition(100.f, startY + i * 80.f);
        btn.rect.setFillColor(sf::Color(70, 100, 70));
        
        btn.text.setFont(font);
        std::string label = m_offers[i].club->name + " (STR: " + std::to_string(m_offers[i].club->strength) + ") - Salary: $" + std::to_string(m_offers[i].offeredSalary) + "/week";
        btn.text.setString(sf::String::fromUtf8(label.begin(), label.end()));
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        
        btn.action = "ACCEPT";
        btn.offer = m_offers[i];
        m_buttons.push_back(btn);
    }

    // Reject All / Stay at current club
    Button btnBack;
    btnBack.rect.setSize(sf::Vector2f(200.f, 40.f));
    btnBack.rect.setPosition(100.f, 500.f);
    btnBack.rect.setFillColor(sf::Color(150, 50, 50));
    btnBack.text.setFont(font);
    btnBack.text.setString("Stay at Current Club");
    btnBack.text.setCharacterSize(18);
    btnBack.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = btnBack.text.getLocalBounds();
    btnBack.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnBack.text.setPosition(btnBack.rect.getPosition().x + btnBack.rect.getSize().x/2.0f,
                             btnBack.rect.getPosition().y + btnBack.rect.getSize().y/2.0f);
    btnBack.action = "CANCEL";
    m_buttons.push_back(btnBack);
}

void TransferScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "CANCEL") {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                    return;
                } else if (btn.action == "ACCEPT") {
                    Player* p = m_gameManager->getPlayer();
                    
                    // We need to fetch the real club pointer from Database
                    // because the one in suitableClubs might be a copy from getLeagues().
                    // Actually, getLeagues returns const reference, we casted to const_cast.
                    // To be completely safe:
                    Club* realClub = nullptr;
                    // We don't actually need the loop, just use getClub directly
                    p->currentClub = m_gameManager->getDatabase().getClub("", btn.offer.club->name);
                    if (!p->currentClub) {
                        // Fallback if league name is required but we passed empty.
                        // We will update Database::getClub to search all if leagueName is empty.
                        p->currentClub = btn.offer.club; // just use the pointer if fallback fails
                    }
                    p->salary = btn.offer.offeredSalary;
                    
                    // Reset stats for new club if needed, but not strictly necessary
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                    return;
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(sf::Color(100, 150, 100));
                if (btn.action == "CANCEL") btn.rect.setFillColor(sf::Color(200, 80, 80));
            } else {
                btn.rect.setFillColor(sf::Color(70, 100, 70));
                if (btn.action == "CANCEL") btn.rect.setFillColor(sf::Color(150, 50, 50));
            }
        }
    }
}

void TransferScreen::update(sf::Time deltaTime) {}

void TransferScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(30, 30, 50));
    window.draw(m_titleText);
    window.draw(m_infoText);
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
