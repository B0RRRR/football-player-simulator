#include "MatchScreen.h"
#include "CareerHubScreen.h"
#include "CareerManager.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"

MatchScreen::MatchScreen() {
    m_player = nullptr;
    m_match = nullptr;
}

MatchScreen::~MatchScreen() {
    delete m_match;
}

void MatchScreen::init() {
    m_player = m_gameManager->getPlayer();
    m_match = new Match(m_player);

    auto& font = AssetManager::get().getFont("MainFont");
    
    m_scoreText.setFont(font);
    m_scoreText.setCharacterSize(36);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setPosition(300.f, 20.f);
    
    m_logsText.setFont(font);
    m_logsText.setCharacterSize(20);
    m_logsText.setFillColor(sf::Color::White);
    m_logsText.setPosition(50.f, 150.f);
    
    createActionButtons();
    
    // Setup back button
    m_backButton.rect.setSize(sf::Vector2f(200.f, 50.f));
    m_backButton.rect.setPosition(300.f, 500.f);
    m_backButton.rect.setFillColor(sf::Color(100, 100, 100));
    m_backButton.text.setFont(font);
    m_backButton.text.setString("Back to Menu");
    m_backButton.text.setCharacterSize(24);
    m_backButton.text.setFillColor(sf::Color::White);
    m_backButton.action = "Back";
    sf::FloatRect tr = m_backButton.text.getLocalBounds();
    m_backButton.text.setOrigin(tr.left + tr.width/2.f, tr.top + tr.height/2.f);
    m_backButton.text.setPosition(300.f + 100.f, 500.f + 25.f);
}

void MatchScreen::createActionButtons() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    std::vector<std::string> actions = {"Shoot", "Pass"};
    for(size_t i = 0; i < actions.size(); ++i) {
        Button b;
        b.rect.setSize(sf::Vector2f(150.f, 50.f));
        b.rect.setPosition(200.f + i * 250.f, 400.f);
        b.rect.setFillColor(sf::Color(200, 50, 50));
        b.text.setFont(font);
        b.text.setString(actions[i]);
        b.text.setCharacterSize(24);
        b.text.setFillColor(sf::Color::White);
        
        sf::FloatRect tr = b.text.getLocalBounds();
        b.text.setOrigin(tr.left + tr.width/2.f, tr.top + tr.height/2.f);
        b.text.setPosition(b.rect.getPosition().x + 75.f, b.rect.getPosition().y + 25.f);
        
        b.action = actions[i];
        m_actionButtons.push_back(b);
    }
}

void MatchScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        
        if (m_match->getState() == MatchState::KeyMoment) {
            for (auto& btn : m_actionButtons) {
                if (btn.rect.getGlobalBounds().contains(mousePos)) {
                    if (btn.action == "Shoot") m_match->playerShoot();
                    if (btn.action == "Pass") m_match->playerPass();
                }
            }
        }
        else if (m_match->getState() == MatchState::Finished) {
            if (m_backButton.rect.getGlobalBounds().contains(mousePos)) {
                if (m_backButton.action == "Back") {
                    // Record results
                    Club* c = m_gameManager->getPlayer()->currentClub;
                    if (c) {
                        int us = m_match->getScoreUs();
                        int them = m_match->getScoreThem();
                        if (us > them) {
                            c->points += 3; c->wins++;
                        } else if (us == them) {
                            c->points += 1; c->draws++;
                        } else {
                            c->losses++;
                        }
                        c->goalsFor += us;
                        c->goalsAgainst += them;
                    }
                    
                    m_gameManager->getCareerManager()->advanceDay();
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                }
            }
        }
    }
}

void MatchScreen::update(sf::Time deltaTime) {
    m_match->update(deltaTime.asSeconds());
    
    // Update texts
    std::string scoreStr = "US " + std::to_string(m_match->getScoreUs()) + " - " + std::to_string(m_match->getScoreThem()) + " THEM\n" + std::to_string(m_match->getMinute()) + "'";
    m_scoreText.setString(sf::String::fromUtf8(scoreStr.begin(), scoreStr.end()));
    
    std::string logs;
    for (const auto& l : m_match->getLogs()) {
        logs += l + "\n\n";
    }
    m_logsText.setString(sf::String::fromUtf8(logs.begin(), logs.end()));
}

void MatchScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(34, 139, 34)); // Grass green
    
    // Draw lines of pitch (mockup)
    sf::RectangleShape centerLine(sf::Vector2f(800.f, 5.f));
    centerLine.setPosition(0.f, 300.f);
    centerLine.setFillColor(sf::Color(255, 255, 255, 100));
    window.draw(centerLine);
    
    sf::CircleShape centerCircle(70.f);
    centerCircle.setPosition(400.f - 70.f, 300.f - 70.f);
    centerCircle.setFillColor(sf::Color::Transparent);
    centerCircle.setOutlineThickness(5.f);
    centerCircle.setOutlineColor(sf::Color(255, 255, 255, 100));
    window.draw(centerCircle);
    
    window.draw(m_scoreText);
    window.draw(m_logsText);
    
    if (m_match->getState() == MatchState::KeyMoment) {
        for (const auto& btn : m_actionButtons) {
            window.draw(btn.rect);
            window.draw(btn.text);
        }
    }
    else if (m_match->getState() == MatchState::Finished) {
        window.draw(m_backButton.rect);
        window.draw(m_backButton.text);
    }
}
