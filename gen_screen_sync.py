import os

header = """#pragma once
#include "Screen.h"
#include "MatchEngine.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

enum class VisualState {
    Kickoff,
    NormalPlay,
    Attacking,
    GoalCelebration,
    WaitingForMinigame
};

struct PlayerDot {
    sf::CircleShape shape;
    sf::Vector2f targetPos;
    float speed;
    bool isHome;
};

class MatchScreen : public Screen {
public:
    MatchScreen();
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    void initMinigame();
    void updateMinigame(sf::Time deltaTime);
    void updateVisuals(sf::Time deltaTime);
    void resetToKickoff();

    std::shared_ptr<MatchEngine> m_engine;
    
    // UI Elements
    sf::Sprite m_homeLogo;
    sf::Sprite m_awayLogo;
    sf::Text m_homeName;
    sf::Text m_awayName;
    sf::Text m_scoreText;
    sf::Text m_timeText;
    
    sf::Text m_logText;
    sf::Text m_statusText;
    sf::Text m_statsTitle;
    sf::Text m_homeStatsText;
    sf::Text m_awayStatsText;
    
    std::vector<MatchEvent> m_visibleLogs;
    std::vector<sf::RectangleShape> m_momentumBars;
    
    // 2D Pitch Elements
    sf::RectangleShape m_pitchRect;
    sf::RectangleShape m_pitchLines;
    sf::CircleShape m_pitchCenter;
    sf::RectangleShape m_leftGoal;
    sf::RectangleShape m_rightGoal;
    
    std::vector<PlayerDot> m_dots;
    sf::CircleShape m_visualBall;
    sf::Vector2f m_ballTarget;
    int m_ballCarrierIdx;
    
    VisualState m_visualState;
    float m_stateTimer;
    MatchEvent m_pendingEvent;
    
    float m_simTimer;
    
    // Minigame Elements
    bool m_minigameActive;
    float m_minigameTimer;
    sf::RectangleShape m_minigameOverlay;
    sf::CircleShape m_playerSprite;
    sf::CircleShape m_enemySprite;
    sf::CircleShape m_ballSprite;
    sf::CircleShape m_targetSprite;
    sf::Text m_promptText;
    
    float m_targetDir;
    float m_enemyDir;
    float m_enemySpeed;
};
"""

with open("src/MatchScreen.h", "w") as f:
    f.write(header)

cpp_code = """#include "MatchScreen.h"
#include "MatchStatsScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Settings.h"
#include "UITheme.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

MatchScreen::MatchScreen() : m_engine(nullptr), m_minigameActive(false), m_simTimer(0.f), m_visualState(VisualState::Kickoff), m_stateTimer(0.f), m_ballCarrierIdx(-1) {}

void MatchScreen::init() {
    Player* p = m_gameManager->getPlayer();
    
    Club* opp = nullptr;
    bool isHomeMatch = true;
    Club* playerClub = p->currentClub;
    
    if (m_gameManager->getCareerManager()->hasInternationalMatchToday()) {
        opp = m_gameManager->getCareerManager()->getInternationalOpponent();
        isHomeMatch = m_gameManager->getCareerManager()->isHomeInternationalMatch();
        const League* nats = m_gameManager->getDatabase().getNationalTeams();
        if (nats) {
            for (const auto& c : nats->clubs) {
                if (c.name == p->nationality) {
                    playerClub = const_cast<Club*>(&c);
                    break;
                }
            }
        }
    } else if (m_gameManager->getCareerManager()->hasEuropeanMatchToday()) {
        opp = m_gameManager->getCareerManager()->getTodayOpponent();
        isHomeMatch = m_gameManager->getCareerManager()->isHomeMatchToday();
    } else {
        const League* lg = nullptr;
        for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
            for (const auto& c : l.clubs) {
                if (c.name == p->currentClub->name) { lg = &l; break; }
            }
        }
        if (lg) {
            int n = lg->clubs.size();
            int r = p->weeksPlayed % (n - 1);
            int pIndex = -1;
            for (int i = 0; i < n; ++i) {
                if (lg->clubs[i].name == p->currentClub->name) { pIndex = i; break; }
            }
            auto rotate = [n, r](int x) { if (x == 0) return 0; return 1 + (x - 1 + r) % (n - 1); };
            for (int i = 0; i < n / 2; ++i) {
                int t1 = (i == 0) ? 0 : rotate(i);
                int t2 = rotate(n - 1 - i);
                if (t1 == pIndex) { opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t2].name); break; }
                else if (t2 == pIndex) { opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t1].name); isHomeMatch = false; break; }
            }
        }
    }
    
    m_engine = std::make_shared<MatchEngine>(playerClub, opp, isHomeMatch, p);

    auto& font = AssetManager::get().getFont("MainFont");
    
    std::string homeNameStr = m_engine->isHome() ? m_engine->getPlayerClub()->name : m_engine->getOpponentClub()->name;
    std::string awayNameStr = m_engine->isHome() ? m_engine->getOpponentClub()->name : m_engine->getPlayerClub()->name;
    bool isNat = m_gameManager->getCareerManager()->hasInternationalMatchToday();
    
    m_homeLogo.setTexture(AssetManager::get().getTexture(homeNameStr, isNat));
    m_homeLogo.setPosition(350.f, 20.f);
    m_homeLogo.setScale(64.f / m_homeLogo.getTexture()->getSize().x, 64.f / m_homeLogo.getTexture()->getSize().y);
    
    m_awayLogo.setTexture(AssetManager::get().getTexture(awayNameStr, isNat));
    m_awayLogo.setPosition(850.f, 20.f);
    m_awayLogo.setScale(64.f / m_awayLogo.getTexture()->getSize().x, 64.f / m_awayLogo.getTexture()->getSize().y);
    
    m_homeName.setFont(font); m_homeName.setCharacterSize(24); m_homeName.setFillColor(UITheme::TextWhite);
    m_homeName.setString(homeNameStr); m_homeName.setPosition(340.f - m_homeName.getGlobalBounds().width, 40.f);
    
    m_awayName.setFont(font); m_awayName.setCharacterSize(24); m_awayName.setFillColor(UITheme::TextWhite);
    m_awayName.setString(awayNameStr); m_awayName.setPosition(930.f, 40.f);
    
    m_scoreText.setFont(font); m_scoreText.setCharacterSize(48); m_scoreText.setFillColor(UITheme::Highlight);
    m_scoreText.setPosition(580.f, 25.f);
    
    m_timeText.setFont(font); m_timeText.setCharacterSize(20); m_timeText.setFillColor(UITheme::TextDim);
    m_timeText.setPosition(610.f, 85.f);
    
    m_logText.setFont(font); m_logText.setCharacterSize(14); m_logText.setFillColor(UITheme::TextWhite);
    m_logText.setPosition(30.f, 500.f);
    
    m_statusText.setFont(font); m_statusText.setCharacterSize(16); m_statusText.setFillColor(sf::Color(100, 255, 100));
    m_statusText.setPosition(30.f, 470.f);
    m_statusText.setString("Status: On the pitch (Starter)");
    
    m_statsTitle.setFont(font); m_statsTitle.setCharacterSize(20); m_statsTitle.setFillColor(UITheme::Highlight);
    m_statsTitle.setString("LIVE STATS"); m_statsTitle.setPosition(1000.f, 150.f);
    
    m_homeStatsText.setFont(font); m_homeStatsText.setCharacterSize(16); m_homeStatsText.setFillColor(UITheme::TextWhite);
    m_homeStatsText.setPosition(900.f, 200.f);
    m_awayStatsText.setFont(font); m_awayStatsText.setCharacterSize(16); m_awayStatsText.setFillColor(UITheme::TextWhite);
    m_awayStatsText.setPosition(1100.f, 200.f);
    
    // 2D Pitch
    m_pitchRect.setSize(sf::Vector2f(800.f, 320.f));
    m_pitchRect.setPosition(40.f, 130.f);
    m_pitchRect.setFillColor(sf::Color(40, 140, 60)); 
    m_pitchRect.setOutlineThickness(3.f);
    m_pitchRect.setOutlineColor(sf::Color(200, 200, 200));
    
    m_pitchLines.setSize(sf::Vector2f(4.f, 320.f));
    m_pitchLines.setPosition(440.f, 130.f);
    m_pitchLines.setFillColor(sf::Color(200, 200, 200));
    
    m_pitchCenter.setRadius(40.f);
    m_pitchCenter.setPosition(400.f, 250.f);
    m_pitchCenter.setFillColor(sf::Color::Transparent);
    m_pitchCenter.setOutlineThickness(4.f);
    m_pitchCenter.setOutlineColor(sf::Color(200, 200, 200));
    
    // Goals
    m_leftGoal.setSize(sf::Vector2f(20.f, 80.f));
    m_leftGoal.setPosition(20.f, 250.f);
    m_leftGoal.setFillColor(sf::Color(200, 200, 200));
    m_leftGoal.setOutlineThickness(2.f);
    m_leftGoal.setOutlineColor(sf::Color::Black);
    
    m_rightGoal.setSize(sf::Vector2f(20.f, 80.f));
    m_rightGoal.setPosition(840.f, 250.f);
    m_rightGoal.setFillColor(sf::Color(200, 200, 200));
    m_rightGoal.setOutlineThickness(2.f);
    m_rightGoal.setOutlineColor(sf::Color::Black);
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    
    for(int i=0; i<11; ++i) {
        PlayerDot d; d.shape.setRadius(6.f); d.shape.setFillColor(sf::Color(50, 50, 250)); d.isHome = true;
        d.targetPos = sf::Vector2f(50.f + form[i][0] * 780.f, 140.f + form[i][1] * 300.f);
        d.shape.setPosition(d.targetPos);
        d.speed = 80.f; m_dots.push_back(d);
    }
    for(int i=0; i<11; ++i) {
        PlayerDot d; d.shape.setRadius(6.f); d.shape.setFillColor(sf::Color(250, 50, 50)); d.isHome = false;
        d.targetPos = sf::Vector2f(830.f - form[i][0] * 780.f, 140.f + form[i][1] * 300.f);
        d.shape.setPosition(d.targetPos);
        d.speed = 80.f; m_dots.push_back(d);
    }
    m_visualBall.setRadius(4.f); m_visualBall.setFillColor(sf::Color::White);
    
    // Minigame Overlay
    m_minigameOverlay.setSize(sf::Vector2f(1280.f, 720.f));
    m_minigameOverlay.setFillColor(sf::Color(0, 0, 0, 200));
    
    m_playerSprite.setRadius(12.f); m_playerSprite.setFillColor(sf::Color::Blue);
    m_ballSprite.setRadius(6.f); m_ballSprite.setFillColor(sf::Color::White);
    m_targetSprite.setRadius(30.f); m_targetSprite.setFillColor(sf::Color(255, 255, 255, 100));
    m_enemySprite.setRadius(12.f); m_enemySprite.setFillColor(sf::Color::Red);
    
    m_promptText.setFont(font); m_promptText.setCharacterSize(28); m_promptText.setFillColor(UITheme::Highlight);
    m_promptText.setString("MINIGAME ACTIVE!");
    m_promptText.setPosition(500.f, 50.f);
    
    resetToKickoff();
}

void MatchScreen::resetToKickoff() {
    m_visualState = VisualState::Kickoff;
    m_stateTimer = 0.f;
    m_ballTarget = sf::Vector2f(440.f, 290.f);
    m_visualBall.setPosition(m_ballTarget);
    m_ballCarrierIdx = -1;
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    for(int i=0; i<11; ++i) { m_dots[i].targetPos = sf::Vector2f(50.f + form[i][0] * 780.f, 140.f + form[i][1] * 300.f); }
    for(int i=0; i<11; ++i) { m_dots[i+11].targetPos = sf::Vector2f(830.f - form[i][0] * 780.f, 140.f + form[i][1] * 300.f); }
}

void MatchScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (m_minigameActive) {
        Player* p = m_gameManager->getPlayer();
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
            
            if (p->position == PlayerPosition::Forward || p->position == PlayerPosition::Goalkeeper) {
                if (m_targetSprite.getGlobalBounds().contains(mousePos)) { m_engine->processMinigameResult(true); } 
                else { m_engine->processMinigameResult(false); }
                m_minigameActive = false;
            } else if (p->position == PlayerPosition::Midfielder) {
                if (m_targetSprite.getGlobalBounds().contains(mousePos)) {
                    sf::Vector2f pPos = m_playerSprite.getPosition();
                    sf::Vector2f ePos = m_enemySprite.getPosition();
                    float minX = std::min(pPos.x, mousePos.x); float maxX = std::max(pPos.x, mousePos.x);
                    if (ePos.x > minX - 20.f && ePos.x < maxX + 20.f && std::abs(ePos.y - mousePos.y) < 40.f) {
                        m_engine->processMinigameResult(false);
                    } else { m_engine->processMinigameResult(true); }
                } else { m_engine->processMinigameResult(false); }
                m_minigameActive = false;
            }
        }
        else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
            if (p->position == PlayerPosition::Defender) {
                if (m_enemySprite.getGlobalBounds().intersects(m_targetSprite.getGlobalBounds())) {
                    m_engine->processMinigameResult(true);
                } else { m_engine->processMinigameResult(false); }
                m_minigameActive = false;
            }
        }
    }
}

void MatchScreen::updateVisuals(sf::Time deltaTime) {
    float dt = deltaTime.asSeconds();
    m_stateTimer += dt;
    float mom = 0.0f;
    if (!m_engine->getMomentumHistory().empty()) mom = m_engine->getMomentumHistory().back();
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    
    if (m_visualState == VisualState::Kickoff) {
        if (m_stateTimer > 2.0f) m_visualState = VisualState::NormalPlay;
    } 
    else if (m_visualState == VisualState::NormalPlay) {
        float shift = mom * 1.5f;
        for (size_t i = 0; i < 22; ++i) {
            int idx = i % 11;
            float tx = (i < 11) ? 50.f + form[idx][0] * 780.f : 830.f - form[idx][0] * 780.f;
            if (idx != 0) tx += (i < 11) ? shift : shift;
            float ty = 140.f + form[idx][1] * 300.f;
            
            if (std::hypot(m_dots[i].targetPos.x - m_dots[i].shape.getPosition().x, m_dots[i].targetPos.y - m_dots[i].shape.getPosition().y) < 5.f) {
                float jitterR = (idx == 0) ? 5.f : 15.f;
                m_dots[i].targetPos = sf::Vector2f(tx + (rand()%int(jitterR*2))-jitterR, ty + (rand()%int(jitterR*2))-jitterR);
            }
        }
        
        if (m_ballCarrierIdx == -1 || rand() % 100 < 2) {
            bool pickHome = (mom > 0); if (mom == 0) pickHome = (rand()%2 == 0);
            m_ballCarrierIdx = (pickHome ? 0 : 11) + 1 + (rand() % 8);
        }
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
        
    } 
    else if (m_visualState == VisualState::Attacking) {
        // Attack sequence before Goal/Chance resolves
        if (m_stateTimer < 1.0f) {
            // Ball passed to forward
            int baseIdx = m_pendingEvent.isHome ? 0 : 11;
            int options[] = {5, 8, 9, 10};
            if (m_ballCarrierIdx < baseIdx || m_ballCarrierIdx > baseIdx+10) m_ballCarrierIdx = baseIdx + options[rand() % 4];
            m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
        } else if (m_stateTimer > 1.0f && m_stateTimer < 2.0f) {
            // Shot taken!
            m_ballCarrierIdx = -1;
            if (m_pendingEvent.type == EventType::Goal) {
                m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(810.f, 290.f) : sf::Vector2f(70.f, 290.f);
            } else {
                // Chance (Miss or Save)
                m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(840.f, 200.f + rand()%180) : sf::Vector2f(40.f, 200.f + rand()%180);
            }
        } else if (m_stateTimer >= 2.0f) {
            // Animation done, commit event to engine
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            
            if (m_pendingEvent.type == EventType::Goal) {
                m_visualState = VisualState::GoalCelebration;
                m_stateTimer = 0.f;
            } else {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
        }
    } 
    else if (m_visualState == VisualState::GoalCelebration) {
        if (m_stateTimer < 3.0f) {
            if (m_pendingEvent.isHome) {
                for(int i=5;i<11;i++) m_dots[i].targetPos = sf::Vector2f(700.f + (rand()%40), 250.f + (rand()%80));
                for(int i=16;i<22;i++) m_dots[i].targetPos = sf::Vector2f(440.f, 290.f);
            } else {
                for(int i=16;i<22;i++) m_dots[i].targetPos = sf::Vector2f(150.f + (rand()%40), 250.f + (rand()%80));
                for(int i=5;i<11;i++) m_dots[i].targetPos = sf::Vector2f(440.f, 290.f);
            }
        } else {
            resetToKickoff();
        }
    }
    else if (m_visualState == VisualState::WaitingForMinigame) {
        // Move ball to the player's zone before triggering
        int userPosIdx = 0;
        PlayerPosition pos = m_gameManager->getPlayer()->position;
        if (pos == PlayerPosition::Defender) userPosIdx = 3;
        else if (pos == PlayerPosition::Midfielder) userPosIdx = 7;
        else if (pos == PlayerPosition::Forward) userPosIdx = 10;
        else if (pos == PlayerPosition::Goalkeeper) userPosIdx = 0;
        
        int userIdx = m_engine->isHome() ? userPosIdx : 11 + userPosIdx;
        m_ballTarget = m_dots[userIdx].shape.getPosition();
        
        float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
        if (dist < 10.f) {
            // Trigger it!
            m_engine->triggerMinigame();
            m_visualState = VisualState::NormalPlay; // Resume normal play when it finishes
        }
    }
    
    // Move dots
    for(auto& d : m_dots) {
        if (d.targetPos.x < 50.f) d.targetPos.x = 50.f; if (d.targetPos.x > 820.f) d.targetPos.x = 820.f;
        if (d.targetPos.y < 140.f) d.targetPos.y = 140.f; if (d.targetPos.y > 440.f) d.targetPos.y = 440.f;
        sf::Vector2f dir = d.targetPos - d.shape.getPosition();
        float len = std::hypot(dir.x, dir.y);
        if (len > 0) d.shape.move((dir.x / len) * d.speed * dt, (dir.y / len) * d.speed * dt);
    }
    
    // Move ball
    sf::Vector2f bdir = m_ballTarget - m_visualBall.getPosition();
    float blen = std::hypot(bdir.x, bdir.y);
    float bspeed = (m_visualState == VisualState::Attacking && m_stateTimer > 1.0f) ? 500.f : 300.f;
    if (blen > 0) m_visualBall.move((bdir.x / blen) * bspeed * dt, (bdir.y / blen) * bspeed * dt);
}

void MatchScreen::update(sf::Time deltaTime) {
    if (!m_engine) return;
    
    if (m_engine->getState() == MatchState::Finished) {
        m_simTimer += deltaTime.asSeconds();
        if (m_simTimer > 2.0f) {
            m_gameManager->changeScreen(std::make_shared<MatchStatsScreen>(m_engine));
            return;
        }
    }
    
    if (m_engine->getState() == MatchState::MinigameTriggered) {
        if (!m_minigameActive) {
            initMinigame();
            m_minigameActive = true;
        }
        updateMinigame(deltaTime);
        return;
    }
    
    updateVisuals(deltaTime);
    
    if (m_engine->getState() == MatchState::Simulating) {
        // Only tick the clock if we are in NormalPlay
        if (m_visualState == VisualState::NormalPlay || m_visualState == VisualState::Kickoff) {
            float speedDelay = 0.4f;
            if (g_settings.matchSpeed == 0) speedDelay = 1.0f;
            if (g_settings.matchSpeed == 2) speedDelay = 0.1f;
            if (g_settings.matchSpeed == 3) speedDelay = 0.0f;
            
            m_simTimer += deltaTime.asSeconds();
            if (m_simTimer > speedDelay) {
                m_simTimer = 0.f;
                m_engine->updateMinute();
            }
        }
        
        // Check for new logs from the engine
        if (m_engine->hasLogs() && m_visualState == VisualState::NormalPlay) {
            m_pendingEvent = m_engine->popRecentLog();
            
            if (m_pendingEvent.type == EventType::Goal || m_pendingEvent.type == EventType::Chance) {
                m_visualState = VisualState::Attacking;
                m_stateTimer = 0.f;
            } else if (m_pendingEvent.type == EventType::PendingMinigame) {
                m_visualState = VisualState::WaitingForMinigame;
            } else {
                // Normal log (Card, etc)
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            }
        }
        
        m_momentumBars.clear();
        const auto& hist = m_engine->getMomentumHistory();
        float barWidth = 400.f / 90.f;
        for (size_t i = 0; i < hist.size(); ++i) {
            float h = (hist[i] / 100.f) * 80.f;
            sf::RectangleShape r;
            r.setSize(sf::Vector2f(barWidth - 1.f, std::abs(h)));
            if (h > 0) {
                r.setPosition(450.f + i * barWidth, 620.f - h);
                r.setFillColor(sf::Color(50, 200, 50));
            } else {
                r.setPosition(450.f + i * barWidth, 620.f);
                r.setFillColor(sf::Color(50, 100, 250));
            }
            m_momentumBars.push_back(r);
        }
    }
    
    m_scoreText.setString(std::to_string(m_engine->getHomeScore()) + " - " + std::to_string(m_engine->getAwayScore()));
    m_timeText.setString(std::to_string(m_engine->getMinute()) + "'");
    
    std::string fullLog = "";
    for (const auto& l : m_visibleLogs) { fullLog += l.text + "\\n"; }
    m_logText.setString(sf::String::fromUtf8(fullLog.begin(), fullLog.end()));
    
    auto homeStats = m_engine->isHome() ? m_engine->getPlayerTeamStats() : m_engine->getOpponentTeamStats();
    auto awayStats = m_engine->isHome() ? m_engine->getOpponentTeamStats() : m_engine->getPlayerTeamStats();
    
    m_homeStatsText.setString(
        "Shots: " + std::to_string(homeStats.shots) + "\\n" +
        "Cards: " + std::to_string(homeStats.yellowCards) + "Y " + std::to_string(homeStats.redCards) + "R\\n"
    );
    m_awayStatsText.setString(
        "Shots: " + std::to_string(awayStats.shots) + "\\n" +
        "Cards: " + std::to_string(awayStats.yellowCards) + "Y " + std::to_string(awayStats.redCards) + "R\\n"
    );
}

void MatchScreen::initMinigame() {
    Player* p = m_gameManager->getPlayer();
    m_minigameTimer = 0.f;
    int oppStrength = m_engine->getOpponentClub()->strength;
    
    if (p->position == PlayerPosition::Forward) {
        m_playerSprite.setPosition(600.f, 450.f); m_ballSprite.setPosition(620.f, 450.f);
        m_targetSprite.setPosition(600.f, 280.f); m_targetDir = 1.f;
        float r = 10.f + (p->shooting / 100.f) * 30.f; m_targetSprite.setRadius(r);
    } else if (p->position == PlayerPosition::Midfielder) {
        m_playerSprite.setPosition(600.f, 400.f); m_ballSprite.setPosition(620.f, 400.f);
        m_targetSprite.setPosition(350.f + (rand()%400), 280.f); m_targetSprite.setRadius(25.f);
        m_enemySprite.setPosition(m_targetSprite.getPosition().x, 340.f); m_enemyDir = (rand()%2 == 0) ? 1.f : -1.f;
        m_enemySpeed = 80.f + ((100.f - p->passing) * 3.f);
    } else if (p->position == PlayerPosition::Defender) {
        m_playerSprite.setPosition(600.f, 450.f); m_targetSprite.setPosition(600.f, 400.f);
        m_enemySprite.setPosition(600.f, 250.f); m_ballSprite.setPosition(m_enemySprite.getPosition());
        m_enemySpeed = 80.f + ((100.f - p->tackling) * 2.f) + (oppStrength * 1.5f);
    } else if (p->position == PlayerPosition::Goalkeeper) {
        m_playerSprite.setPosition(600.f, 280.f); m_ballSprite.setPosition(600.f, 450.f);
        m_targetSprite.setPosition(450.f + (rand()%260), 280.f + (rand()%50)); m_targetSprite.setRadius(30.f);
    }
}

void MatchScreen::updateMinigame(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    float dt = deltaTime.asSeconds();
    if (p->position == PlayerPosition::Forward) {
        m_minigameTimer += dt;
        float speed = 50.f + ((100.f - p->shooting) * 2.f); m_targetSprite.move(m_targetDir * speed * dt, 0.f);
        if (m_targetSprite.getPosition().x < 320.f) m_targetDir = 1.f;
        if (m_targetSprite.getPosition().x > 880.f - m_targetSprite.getRadius()*2.f) m_targetDir = -1.f;
        if (m_minigameTimer > 5.0f) { m_engine->processMinigameResult(false); m_minigameActive = false; }
    } else if (p->position == PlayerPosition::Midfielder) {
        m_minigameTimer += dt; m_enemySprite.move(m_enemyDir * m_enemySpeed * dt, 0.f);
        if (m_enemySprite.getPosition().x < 350.f) m_enemyDir = 1.f; if (m_enemySprite.getPosition().x > 850.f) m_enemyDir = -1.f;
        if (m_minigameTimer > 5.0f) { m_engine->processMinigameResult(false); m_minigameActive = false; }
    } else if (p->position == PlayerPosition::Defender) {
        m_enemySprite.move(0, m_enemySpeed * dt); m_ballSprite.setPosition(m_enemySprite.getPosition());
        if (m_enemySprite.getPosition().y > 500.f) { m_engine->processMinigameResult(false); m_minigameActive = false; }
    } else if (p->position == PlayerPosition::Goalkeeper) {
        m_minigameTimer += dt; float ballSpeed = 1.f + ((100.f - p->goalkeeping) / 50.f);
        m_ballSprite.move((m_targetSprite.getPosition().x - 600.f) * dt / ballSpeed, (m_targetSprite.getPosition().y - 450.f) * dt / ballSpeed);
        if (m_minigameTimer > ballSpeed) { m_engine->processMinigameResult(false); m_minigameActive = false; }
    }
}

void MatchScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    
    window.draw(m_homeLogo); window.draw(m_awayLogo);
    window.draw(m_homeName); window.draw(m_awayName);
    window.draw(m_scoreText); window.draw(m_timeText);
    
    window.draw(m_pitchRect); window.draw(m_pitchLines); window.draw(m_pitchCenter);
    window.draw(m_leftGoal); window.draw(m_rightGoal);
    
    Player* p = m_gameManager->getPlayer();
    int userPosIdx = 0;
    if (p->position == PlayerPosition::Defender) userPosIdx = 3;
    else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
    else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
    
    for (size_t i = 0; i < m_dots.size(); ++i) {
        window.draw(m_dots[i].shape);
        if (m_dots[i].isHome == m_engine->isHome() && (int)i % 11 == userPosIdx) {
            sf::CircleShape hl(10.f);
            hl.setFillColor(sf::Color::Transparent);
            hl.setOutlineColor(sf::Color::Yellow);
            hl.setOutlineThickness(2.f);
            hl.setPosition(m_dots[i].shape.getPosition().x - 4.f, m_dots[i].shape.getPosition().y - 4.f);
            window.draw(hl);
        }
    }
    window.draw(m_visualBall);
    
    window.draw(m_logText);
    for (const auto& bar : m_momentumBars) window.draw(bar);
    
    sf::Text momTxt;
    auto& font = AssetManager::get().getFont("MainFont");
    momTxt.setFont(font); momTxt.setCharacterSize(14); momTxt.setFillColor(UITheme::Highlight);
    momTxt.setPosition(450.f, 530.f); momTxt.setString("Match Momentum");
    window.draw(momTxt);
    
    window.draw(m_statusText);
    window.draw(m_statsTitle);
    window.draw(m_homeStatsText);
    window.draw(m_awayStatsText);
    
    if (m_minigameActive) {
        window.draw(m_minigameOverlay);
        window.draw(m_targetSprite); window.draw(m_playerSprite);
        if (p->position == PlayerPosition::Defender || p->position == PlayerPosition::Midfielder) window.draw(m_enemySprite);
        if (p->position == PlayerPosition::Defender) window.draw(m_promptText);
        window.draw(m_ballSprite);
        
        sf::Text inst; inst.setFont(font); inst.setCharacterSize(24); inst.setFillColor(UITheme::Highlight);
        inst.setPosition(400.f, 600.f);
        if (p->position == PlayerPosition::Forward) inst.setString("Click the target area to shoot! (5s)");
        else if (p->position == PlayerPosition::Midfielder) inst.setString("Click the target area to pass! (5s)");
        else if (p->position == PlayerPosition::Defender) inst.setString("Press SPACE when the red attacker enters the target circle!");
        else if (p->position == PlayerPosition::Goalkeeper) inst.setString("Click the target area to save the shot! (3s)");
        window.draw(inst);
    }
}
"""

with open("src/MatchScreen.cpp", "w") as f:
    f.write(cpp_code)
print("MatchScreen.cpp updated.")
