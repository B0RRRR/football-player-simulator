#include "MyStatusScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "UITheme.h"
#include <iomanip>
#include <sstream>

MyStatusScreen::MyStatusScreen() {
}

void MyStatusScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(50.f, 30.f);
    m_titleText.setString("My Status");
    
    m_infoText.setFont(font);
    m_infoText.setCharacterSize(24);
    m_infoText.setFillColor(sf::Color(200, 200, 200));
    m_infoText.setPosition(50.f, 100.f);
    
    m_coachResponseText.setFont(font);
    m_coachResponseText.setCharacterSize(20);
    m_coachResponseText.setFillColor(sf::Color::Yellow);
    m_coachResponseText.setPosition(50.f, 350.f);
    m_coachResponseText.setString("");

    Button btnBack;
    btnBack.rect.setSize(sf::Vector2f(200.f, 40.f));
    btnBack.rect.setPosition(50.f, 600.f);
    btnBack.baseColor = sf::Color(150, 50, 50);
    btnBack.text.setFont(font);
    btnBack.text.setString("Back");
    btnBack.text.setCharacterSize(20);
    btnBack.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = btnBack.text.getLocalBounds();
    btnBack.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnBack.text.setPosition(btnBack.rect.getPosition().x + btnBack.rect.getSize().x/2.0f,
                             btnBack.rect.getPosition().y + btnBack.rect.getSize().y/2.0f);
    btnBack.action = "Back";
    m_buttons.push_back(btnBack);
    
    Button btnRequestTransfer;
    btnRequestTransfer.rect.setSize(sf::Vector2f(250.f, 40.f));
    btnRequestTransfer.rect.setPosition(450.f, 100.f);
    btnRequestTransfer.baseColor = sf::Color(50, 50, 150);
    btnRequestTransfer.text.setFont(font);
    btnRequestTransfer.text.setString("Request Transfer List");
    btnRequestTransfer.text.setCharacterSize(18);
    btnRequestTransfer.text.setFillColor(sf::Color::White);
    textRect = btnRequestTransfer.text.getLocalBounds();
    btnRequestTransfer.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnRequestTransfer.text.setPosition(btnRequestTransfer.rect.getPosition().x + btnRequestTransfer.rect.getSize().x/2.0f,
                                        btnRequestTransfer.rect.getPosition().y + btnRequestTransfer.rect.getSize().y/2.0f);
    btnRequestTransfer.action = "RequestTransfer";
    m_buttons.push_back(btnRequestTransfer);
    
    Button btnRequestPlayTime;
    btnRequestPlayTime.rect.setSize(sf::Vector2f(250.f, 40.f));
    btnRequestPlayTime.rect.setPosition(450.f, 160.f);
    btnRequestPlayTime.baseColor = sf::Color(50, 150, 50);
    btnRequestPlayTime.text.setFont(font);
    btnRequestPlayTime.text.setString("Request Playing Time");
    btnRequestPlayTime.text.setCharacterSize(18);
    btnRequestPlayTime.text.setFillColor(sf::Color::White);
    textRect = btnRequestPlayTime.text.getLocalBounds();
    btnRequestPlayTime.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnRequestPlayTime.text.setPosition(btnRequestPlayTime.rect.getPosition().x + btnRequestPlayTime.rect.getSize().x/2.0f,
                                        btnRequestPlayTime.rect.getPosition().y + btnRequestPlayTime.rect.getSize().y/2.0f);
    btnRequestPlayTime.action = "RequestPlayTime";
    m_buttons.push_back(btnRequestPlayTime);
    
    Button btnToggleAchievements;
    btnToggleAchievements.rect.setSize(sf::Vector2f(250.f, 40.f));
    btnToggleAchievements.rect.setPosition(450.f, 220.f);
    btnToggleAchievements.baseColor = sf::Color(150, 150, 50);
    btnToggleAchievements.text.setFont(font);
    btnToggleAchievements.text.setString("Toggle Achievements");
    btnToggleAchievements.text.setCharacterSize(18);
    btnToggleAchievements.text.setFillColor(sf::Color::White);
    textRect = btnToggleAchievements.text.getLocalBounds();
    btnToggleAchievements.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnToggleAchievements.text.setPosition(btnToggleAchievements.rect.getPosition().x + btnToggleAchievements.rect.getSize().x/2.0f,
                                        btnToggleAchievements.rect.getPosition().y + btnToggleAchievements.rect.getSize().y/2.0f);
    btnToggleAchievements.action = "ToggleAchievements";
    m_buttons.push_back(btnToggleAchievements);
}

void MyStatusScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                Player* p = m_gameManager->getPlayer();
                
                if (btn.action == "Back") {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                } else if (btn.action == "RequestTransfer") {
                    if (p->isTransferListed) {
                        m_coachResponseText.setString("Coach: You are already on the transfer list.");
                    } else {
                        if (p->coachTrust > 60.0f) {
                            m_coachResponseText.setString("Coach: I'm disappointed, but I'll respect your wish.\nYou are now transfer listed. Your trust drops.");
                            p->isTransferListed = true;
                            p->coachTrust -= 20.0f; // Drop trust because player wants to leave
                            if (p->coachTrust < 0.0f) p->coachTrust = 0.0f;
                        } else {
                            m_coachResponseText.setString("Coach: We're not selling you right now. Get back to training!");
                            p->coachTrust -= 10.0f;
                            if (p->coachTrust < 0.0f) p->coachTrust = 0.0f;
                        }
                    }
                    m_messageTimer = 5.0f;
                } else if (btn.action == "RequestPlayTime") {
                    if (p->coachTrust >= 70.0f) {
                        m_coachResponseText.setString("Coach: You are already a key player for us!");
                    } else if (p->coachTrust > 40.0f) {
                        m_coachResponseText.setString("Coach: Keep working hard in training and you'll get your chance.");
                    } else {
                        m_coachResponseText.setString("Coach: You haven't earned it. Stop complaining!");
                        p->coachTrust -= 5.0f;
                        if (p->coachTrust < 0.0f) p->coachTrust = 0.0f;
                    }
                    m_messageTimer = 5.0f;
                } else if (btn.action == "ToggleAchievements") {
                    m_showAchievements = !m_showAchievements;
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(sf::Color(btn.baseColor.r + 30, btn.baseColor.g + 30, btn.baseColor.b + 30));
            } else {
                btn.rect.setFillColor(btn.baseColor);
            }
        }
    }
}

void MyStatusScreen::update(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    if (!p) return;
    
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= deltaTime.asSeconds();
        if (m_messageTimer <= 0.0f) {
            m_coachResponseText.setString("");
        }
    }

    std::string info = "";
    if (m_showAchievements) {
        info = "=== ACHIEVEMENTS ===\n\n";
        if (p->achievements.empty()) {
            info += "No achievements yet. Keep playing!\n";
        } else {
            for (const auto& ach : p->achievements) {
                info += "- " + ach + "\n";
            }
        }
    } else {
        info = "Club: " + (p->currentClub ? p->currentClub->name : "None") + "\n\n";
        info += "Coach Trust: " + std::to_string((int)p->coachTrust) + " / 100\n";
        if (p->coachTrust < 30.0f) info += "Status: BENCHED\n";
        else info += "Status: ACTIVE\n";
        
        info += "\nContract: " + std::to_string(p->contractYearsLeft) + " years left\n";
        info += "Salary: $" + std::to_string(p->salary) + "/w\n";
        
        if (p->isTransferListed) {
            info += "\nTransfer Status: LISTED\n";
        } else {
            info += "\nTransfer Status: NOT FOR SALE\n";
        }
    }
    
    m_infoText.setString(info);
}

void MyStatusScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    
    window.draw(m_titleText);
    window.draw(m_infoText);
    window.draw(m_coachResponseText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
