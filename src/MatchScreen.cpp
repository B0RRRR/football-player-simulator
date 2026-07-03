#include "MatchScreen.h"
#include "MatchStatsScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Settings.h"
#include "UITheme.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>

MatchScreen::MatchScreen() : m_engine(nullptr), m_minigameActive(false), m_isMinigameResultPending(false), m_simTimer(0.f), m_visualState(VisualState::Kickoff), m_stateTimer(0.f), m_ballCarrierIdx(-1) {}

bool MatchScreen::hasRedCard(int globalIdx) const {
    if (!m_engine) return false;
    int localIdx = globalIdx % 11;
    bool isHomeTeam = (globalIdx < 11);
    int userPosIdx = (int)m_gameManager->getPlayer()->position - 1;
    
    if (isHomeTeam) {
        for (int r : m_engine->getHomeRedCards()) {
            if (r == -1 && localIdx == userPosIdx && m_engine->isHome()) return true;
            else if (r == localIdx) return true;
        }
    } else {
        for (int r : m_engine->getAwayRedCards()) {
            if (r == -1 && localIdx == userPosIdx && !m_engine->isHome()) return true;
            else if (r == localIdx) return true;
        }
    }
    return false;
}

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
                } else if (m_attackPhase == 40) {
            // Midfielder Attacking: Wait a bit with the ball
            m_dots[m_ballCarrierIdx].targetPos = m_dots[m_ballCarrierIdx].shape.getPosition(); // Stand still
            if (m_stateTimer > 1.0f) {
                m_attackPhase = 41;
                m_stateTimer = 0.f;
                m_engine->triggerMinigame();
            }
        } else if (m_attackPhase == 41) {
            // Minigame resolved, process the pass
            if (m_engine->hasLogs()) m_pendingEvent = m_engine->popRecentLog();
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            
            bool isSuccess = (m_pendingEvent.text.find("Great pass") != std::string::npos || m_pendingEvent.text.find("GOAL") != std::string::npos);
            
            if (isSuccess) {
                // Pass to forward!
                m_ballCarrierIdx = -1;
                m_dots[m_attackFwdIdx].targetPos = m_dots[m_attackFwdIdx].shape.getPosition(); // Stand still
                m_ballTarget = m_dots[m_attackFwdIdx].shape.getPosition();
                m_attackPhase = 42;
                m_stateTimer = 0.f;
            } else {
                // Bad pass, intercepted by opponent
                m_ballCarrierIdx = -1;
                m_attackPhase = 43; // Fail resolution
                m_stateTimer = 0.f;
                int oppDefenderIdx = (m_engine->isHome() ? 11 : 0) + 3;
                m_dots[oppDefenderIdx].targetPos = m_dots[oppDefenderIdx].shape.getPosition(); // Stand still
                m_ballTarget = m_dots[oppDefenderIdx].shape.getPosition();
            }
        } else if (m_attackPhase == 42) {
            // Ball traveling to Forward
            float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
            if (dist < 10.f) {
                // Forward shoots!
                m_attackPhase = 2;
                m_stateTimer = 0.f;
                m_ballCarrierIdx = -1;
                m_shotTargetY = 290.f + (rand()%60 - 30.f);
            }
        } else if (m_attackPhase == 43) {
            // Ball intercepted by opponent
            float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
            if (dist < 10.f) {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
        } else if (m_attackPhase == 50) {
            // Midfielder Defending: Sprint to opponent
            int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
            m_dots[myMidIdx].targetPos = m_dots[m_ballCarrierIdx].shape.getPosition();
            m_dots[myMidIdx].speed = 150.f; // Realistic sprint
            
            float dist = std::hypot(m_dots[myMidIdx].shape.getPosition().x - m_dots[m_ballCarrierIdx].shape.getPosition().x, 
                                    m_dots[myMidIdx].shape.getPosition().y - m_dots[m_ballCarrierIdx].shape.getPosition().y);
            
            if (dist < 15.f) {
                m_attackPhase = 51;
                m_stateTimer = 0.f;
                m_engine->triggerMinigame();
            }
        } else if (m_attackPhase == 51) {
            // Minigame resolved, process the tackle
            if (m_engine->hasLogs()) m_pendingEvent = m_engine->popRecentLog();
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            
            bool isSuccess = (m_pendingEvent.text.find("Great tackle") != std::string::npos || m_pendingEvent.text.find("interception") != std::string::npos);
            int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
            m_dots[myMidIdx].speed = 100.f; // Reset speed
            
            if (isSuccess) {
                m_ballCarrierIdx = myMidIdx;
                m_attackPhase = 52;
                m_stateTimer = 0.f;
            } else {
                // Failed tackle, opponent passes to forward
                m_attackPhase = 1; 
                m_stateTimer = 0.f;
                m_ballCarrierIdx = -1;
                m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(700.f, m_shotTargetY) : sf::Vector2f(180.f, m_shotTargetY);
                int attackerBase = m_pendingEvent.isHome ? 0 : 11;
                do {
                    m_attackFwdIdx = attackerBase + 9 + (rand()%2);
                } while(hasRedCard(m_attackFwdIdx));
                m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
                m_dots[m_attackFwdIdx].speed = 100.f;
            }
        } else if (m_attackPhase == 52) {
            // Succcessful tackle wait a bit
            if (m_stateTimer > 1.0f) {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
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
    
    m_btnSkipRect.setSize(sf::Vector2f(120.f, 40.f));
    m_btnSkipRect.setPosition(1100.f, 20.f); // Top right corner
    m_btnSkipRect.setFillColor(sf::Color(150, 50, 50));
    
    m_btnSkipText.setFont(font);
    m_btnSkipText.setString("Skip Match");
    m_btnSkipText.setCharacterSize(18);
    m_btnSkipText.setFillColor(sf::Color::White);
    sf::FloatRect sr = m_btnSkipText.getLocalBounds();
    m_btnSkipText.setOrigin(sr.left + sr.width/2.0f, sr.top + sr.height/2.0f);
    m_btnSkipText.setPosition(m_btnSkipRect.getPosition().x + m_btnSkipRect.getSize().x/2.0f,
                              m_btnSkipRect.getPosition().y + m_btnSkipRect.getSize().y/2.0f);
    
    std::vector<std::string> speedLabels = {"Speed: 1x", "Speed: 2x", "Speed: 3x"};
    for (int i = 0; i < 3; ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(100.f, 30.f));
        btn.rect.setPosition(1120.f, 70.f + i * 40.f);
        btn.baseColor = sf::Color(100, 100, 150);
        btn.rect.setFillColor(btn.baseColor);
        
        btn.text.setFont(font);
        btn.text.setString(speedLabels[i]);
        btn.text.setCharacterSize(14);
        btn.text.setFillColor(sf::Color::White);
        sf::FloatRect sr2 = btn.text.getLocalBounds();
        btn.text.setOrigin(sr2.left + sr2.width/2.0f, sr2.top + sr2.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        btn.action = speedLabels[i];
        m_speedButtons.push_back(btn);
    }
    
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
    m_leftGoal.setSize(sf::Vector2f(30.f, 80.f));
    m_leftGoal.setPosition(10.f, 250.f);
    m_leftGoal.setFillColor(sf::Color::Transparent);
    m_leftGoal.setOutlineThickness(3.f);
    m_leftGoal.setOutlineColor(sf::Color::White);
    
    m_rightGoal.setSize(sf::Vector2f(30.f, 80.f));
    m_rightGoal.setPosition(840.f, 250.f);
    m_rightGoal.setFillColor(sf::Color::Transparent);
    m_rightGoal.setOutlineThickness(3.f);
    m_rightGoal.setOutlineColor(sf::Color::White);
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    
    for(int i=0; i<11; ++i) {
        PlayerDot d; d.shape.setRadius(6.f); d.shape.setFillColor(sf::Color(50, 50, 250)); d.isHome = true;
        d.targetPos = sf::Vector2f(50.f + form[i][0] * 780.f, 140.f + form[i][1] * 300.f);
        d.shape.setPosition(d.targetPos);
        d.speed = 100.f; m_dots.push_back(d);
    }
    for(int i=0; i<11; ++i) {
        PlayerDot d; d.shape.setRadius(6.f); d.shape.setFillColor(sf::Color(250, 50, 50)); d.isHome = false;
        d.targetPos = sf::Vector2f(830.f - form[i][0] * 780.f, 140.f + form[i][1] * 300.f);
        d.shape.setPosition(d.targetPos);
        d.speed = 100.f; m_dots.push_back(d);
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
    
    m_btnSkipRect.setSize(sf::Vector2f(120.f, 40.f));
    m_btnSkipRect.setPosition(1100.f, 650.f);
    m_btnSkipRect.setFillColor(sf::Color(150, 50, 50));
    m_btnSkipText.setFont(font); m_btnSkipText.setCharacterSize(18); m_btnSkipText.setFillColor(sf::Color::White);
    m_btnSkipText.setString("Skip Match");
    m_btnSkipText.setPosition(1115.f, 660.f);

    resetToKickoff();
}

void MatchScreen::resetToKickoff() {
    m_visualState = VisualState::Kickoff;
    m_stateTimer = 0.f;
    
    int kickerIdx = m_pendingEvent.isHome ? 11 + 9 : 9; // Away team kicks off if home scored
    if (m_engine->getMinute() == 0) kickerIdx = 9; // At 0' Home kicks off
    
    m_ballTarget = sf::Vector2f(440.f, 290.f);
    m_visualBall.setPosition(m_ballTarget);
    m_ballCarrierIdx = kickerIdx;
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    for(int i=0; i<11; ++i) {
        float tx = 50.f + form[i][0] * 780.f;
        if (tx > 435.f) tx = 435.f;
        m_dots[i].targetPos = sf::Vector2f(tx, 140.f + form[i][1] * 300.f);
        m_dots[i].shape.setPosition(m_dots[i].targetPos);
    }
    for(int i=0; i<11; ++i) {
        float tx = 830.f - form[i][0] * 780.f;
        if (tx < 445.f) tx = 445.f;
        m_dots[i+11].targetPos = sf::Vector2f(tx, 140.f + form[i][1] * 300.f);
        m_dots[i+11].shape.setPosition(m_dots[i+11].targetPos);
    }
    m_dots[kickerIdx].targetPos = sf::Vector2f(440.f, 290.f);
    m_dots[kickerIdx].shape.setPosition(m_dots[kickerIdx].targetPos);
}

void MatchScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (!m_minigameActive) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); 
            sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
            
            if (m_btnSkipRect.getGlobalBounds().contains(mousePos)) {
                // Auto-sim the rest of the match
                while (m_engine->getState() != MatchState::Finished) {
                    if (m_engine->getState() == MatchState::Simulating || m_engine->getState() == MatchState::Finished) {
                        m_engine->updateMinute();
                    } else if (m_engine->getState() == MatchState::MinigameTriggered) {
                        m_engine->processMinigameResult(rand() % 2 == 0); // 50% win rate for auto-sim
                    }
                    
                    while (m_engine->hasLogs()) {
                        m_engine->commitEvent(m_engine->popRecentLog());
                    }
                }
                m_gameManager->changeScreen(std::make_shared<MatchStatsScreen>(m_engine));
                return;
            }
            
            for (size_t i = 0; i < m_speedButtons.size(); ++i) {
                if (m_speedButtons[i].rect.getGlobalBounds().contains(mousePos)) {
                    m_matchSpeedMode = i; // 0, 1, or 2
                }
            }
        }
        
        if (event.type == sf::Event::MouseMoved) {
            sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); 
            sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
            if (m_btnSkipRect.getGlobalBounds().contains(mousePos)) {
                m_btnSkipRect.setFillColor(sf::Color(200, 50, 50));
            } else {
                m_btnSkipRect.setFillColor(sf::Color(150, 50, 50));
            }
        }
    }
    
    if (m_minigameActive) {
        Player* p = m_gameManager->getPlayer();
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
            
            if (p->position == PlayerPosition::Forward || p->position == PlayerPosition::Goalkeeper) {
                if (m_targetSprite.getGlobalBounds().contains(mousePos)) { m_engine->processMinigameResult(true); } 
                else { m_engine->processMinigameResult(false); }
                m_isMinigameResultPending = true;
                m_minigameActive = false;
            } else if (p->position == PlayerPosition::Midfielder) {
                int actionType = 0;
                if (m_attackPhase == 61) actionType = 1;
                else if (m_attackPhase == 41 && m_attackType == 3) actionType = 2;
                
                if (m_targetSprite.getGlobalBounds().contains(mousePos)) {
                    sf::Vector2f pPos = m_playerSprite.getPosition();
                    sf::Vector2f ePos = m_enemySprite.getPosition();
                    float minX = std::min(pPos.x, mousePos.x); float maxX = std::max(pPos.x, mousePos.x);
                    if (ePos.x > minX - 20.f && ePos.x < maxX + 20.f && std::abs(ePos.y - mousePos.y) < 40.f) {
                        m_engine->processMinigameResult(false, actionType);
                    } else { m_engine->processMinigameResult(true, actionType); }
                } else { m_engine->processMinigameResult(false, actionType); }
                m_isMinigameResultPending = true;
                m_minigameActive = false;
            }
        }
        else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
            if (p->position == PlayerPosition::Defender) {
                if (m_enemySprite.getGlobalBounds().intersects(m_targetSprite.getGlobalBounds())) {
                    m_engine->processMinigameResult(true);
                } else { m_engine->processMinigameResult(false); }
                m_isMinigameResultPending = true;
                m_minigameActive = false;
            }
        }
    }
}

void MatchScreen::updateVisuals(sf::Time deltaTime) {
    float dt = deltaTime.asSeconds();
    
    float animSpeedMult = 1.0f;
    if (g_settings.matchSpeed == 2) animSpeedMult = 2.0f;
    if (g_settings.matchSpeed == 3) animSpeedMult = 8.0f;
    
    dt *= animSpeedMult;
    
    m_stateTimer += dt;
    float mom = 0.0f;
    if (!m_engine->getMomentumHistory().empty()) mom = m_engine->getMomentumHistory().back();
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    
    if (m_visualState == VisualState::Kickoff) {
        if (m_stateTimer > 2.0f) {
            m_visualState = VisualState::NormalPlay;
            m_stateTimer = 0.f;
            int kickerIdx = m_pendingEvent.isHome ? 11 + 9 : 9;
            if (m_engine->getMinute() <= 1) kickerIdx = 9;
            m_ballCarrierIdx = -1;
            m_ballTarget = m_dots[kickerIdx + 1].shape.getPosition(); // Pass back
            
            // Pop the Kick-Off event if it's the very first minute so it doesn't stay in queue
            if (m_engine->getMinute() == 0 && m_engine->hasLogs()) {
                m_pendingEvent = m_engine->popRecentLog();
                m_visibleLogs.push_back(m_pendingEvent);
            }
        }
    }
    else if (m_visualState == VisualState::GoalKick) {
        int gkIdx = m_pendingEvent.isHome ? 11 : 0; // Defending GK takes the kick
        m_ballTarget = m_dots[gkIdx].shape.getPosition();
        float distToGk = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
        if (distToGk < 10.f) {
            m_ballCarrierIdx = gkIdx;
            if (m_stateTimer > 1.5f) {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
                int teamBase = (gkIdx == 0) ? 0 : 11;
                m_ballCarrierIdx = teamBase + 1 + (rand() % 10);
            }
        } else {
            m_stateTimer = 0.f; // Freeze timer while ball travels
        }
    }
    else if (m_visualState == VisualState::NormalPlay) {
        // Intercept logic for user
        if (m_ballCarrierIdx != -1) {
            bool carrierIsOpponent = (m_ballCarrierIdx < 11 && !m_engine->isHome()) || (m_ballCarrierIdx >= 11 && m_engine->isHome());
            if (carrierIsOpponent && !m_minigameActive && m_visualState != VisualState::WaitingForMinigame) {
                int userPosIdx = (int)m_gameManager->getPlayer()->position - 1;
                int userIdx = m_engine->isHome() ? userPosIdx : 11 + userPosIdx;
                float distToUser = std::hypot(m_visualBall.getPosition().x - m_dots[userIdx].shape.getPosition().x, m_visualBall.getPosition().y - m_dots[userIdx].shape.getPosition().y);
                if (distToUser < 30.f) {
                    Player* p = m_gameManager->getPlayer();
                    if (p->position == PlayerPosition::Midfielder) {
                        m_attackPhase = 51;
                        m_engine->triggerMinigame();
                    } else if (p->position == PlayerPosition::Defender) {
                        m_attackPhase = 12;
                        m_engine->triggerMinigame();
                    }
                }
            }
        }
        
        static float s_idleTime = 0.f;
        s_idleTime += dt;
        
        float shift = mom * 1.5f;
        for (size_t i = 0; i < 22; ++i) {
            int idx = i % 11;
            float tx = (i < 11) ? 50.f + form[idx][0] * 780.f : 830.f - form[idx][0] * 780.f;
            if (idx != 0) tx += (i < 11) ? shift : shift;
            float ty = 140.f + form[idx][1] * 300.f;
            
            float swayX = std::sin(s_idleTime * 2.0f + i) * 6.0f;
            float swayY = std::cos(s_idleTime * 1.5f + i) * 6.0f;
            
            m_dots[i].targetPos = sf::Vector2f(tx + swayX, ty + swayY);
        }
        
        if (m_ballCarrierIdx == -1 || (rand() % 100 < 2 && std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y) < 10.f)) {
            bool pickHome = (mom > 0); if (mom == 0) pickHome = (rand()%2 == 0);
            do {
                m_ballCarrierIdx = (pickHome ? 0 : 11) + 1 + (rand() % 8);
            } while(hasRedCard(m_ballCarrierIdx));
        }
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
        
    } 
    else if (m_visualState == VisualState::Attacking) {
        int attackerBase = m_pendingEvent.isHome ? 0 : 11;
        int defenderBase = m_pendingEvent.isHome ? 11 : 0;
        
        bool isGoal = (m_pendingEvent.type == EventType::Goal);
        bool isSave = (m_pendingEvent.text.find("Saved") != std::string::npos || m_pendingEvent.text.find("save") != std::string::npos);
        bool isMiss = (!isGoal && !isSave);
        
        float ballDist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
        
        if (m_attackPhase == 0) {
            // Phase 0: Setup and build up
            Player* p = m_gameManager->getPlayer();
            
            bool isDefenderTackleMinigame = p && p->position == PlayerPosition::Defender && (m_pendingEvent.isHome != m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;
            bool isMidfielderTackleMinigame = p && p->position == PlayerPosition::Midfielder && (m_pendingEvent.isHome != m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;
            bool isMidfielderPassMinigame = p && p->position == PlayerPosition::Midfielder && (m_pendingEvent.isHome == m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;
            
            if (isDefenderTackleMinigame) {
                m_attackPhase = 9;
                m_stateTimer = 0.f;
                int options[] = {9, 10};
                m_attackFwdIdx = attackerBase + options[rand() % 2];
                
                m_attackWingerIdx = attackerBase + 5; // Use winger index as passer
                m_ballCarrierIdx = m_attackWingerIdx; 
                
                int userPosIdx = 3;
                int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;
                
                sf::Vector2f defPos = m_dots[myDefenderIdx].shape.getPosition();
                sf::Vector2f attPos = m_dots[m_attackFwdIdx].shape.getPosition();
                
                // Pass target is the midpoint
                m_ballTarget = sf::Vector2f((defPos.x + attPos.x) / 2.f, (defPos.y + attPos.y) / 2.f);
                return;
            } else if (isMidfielderTackleMinigame) {
                m_attackPhase = 50;
                m_stateTimer = 0.f;
                int options[] = {7, 8, 9, 10};
                m_ballCarrierIdx = attackerBase + options[rand() % 4];
                m_attackFwdIdx = attackerBase + 10;
                m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition(); // user will sprint here
                return;
            } else if (isMidfielderPassMinigame) {
                if (rand() % 100 < 20) {
                    m_attackPhase = 60; // Solo Run
                    m_midfielderSoloRun = true;
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = (m_engine->isHome() ? 0 : 11) + 7;
                    m_dots[m_ballCarrierIdx].targetPos = m_engine->isHome() ? sf::Vector2f(720.f, m_shotTargetY) : sf::Vector2f(160.f, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 180.f;
                } else {
                    m_attackPhase = 40; // Pass
                    m_midfielderSoloRun = false;
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = (m_engine->isHome() ? 0 : 11) + 7; // User midfielder
                    
                    int myBase = m_engine->isHome() ? 0 : 11;
                    if (rand() % 100 < 50) {
                        m_attackFwdIdx = myBase + 9 + (rand() % 2); // Forward
                        m_attackType = 2; // Forward pass
                    } else {
                        m_attackFwdIdx = myBase + 2 + (rand() % 6); // Defs/Mids
                        if (m_attackFwdIdx == myBase + 7) m_attackFwdIdx = myBase + 8; // Avoid self
                        m_attackType = 3; // Backward/Sideways pass
                    }
                    
                    m_attackWingerIdx = 1 + (rand() % 2); // Store number of defenders to run (1 or 2)
                }
                return;
            }
            
            if (m_attackType == 0) { // Wing Attack
                m_ballCarrierIdx = m_attackWingerIdx;
                m_dots[m_attackWingerIdx].targetPos = m_pendingEvent.isHome ? sf::Vector2f(700.f, m_attackWingerIdx%11==5 ? 160.f : 420.f) : sf::Vector2f(180.f, m_attackWingerIdx%11==5 ? 160.f : 420.f);
                if (m_stateTimer > 1.0f) {
                    m_attackPhase = 1; 
                    m_ballCarrierIdx = -1; // Release ball for cross
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(700.f, m_shotTargetY) : sf::Vector2f(180.f, m_shotTargetY);
                    m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
                }
            } else if (m_attackType == 1) { // Solo Run
                m_ballCarrierIdx = attackerBase + 7;
                m_dots[m_ballCarrierIdx].targetPos = m_pendingEvent.isHome ? sf::Vector2f(720.f, m_shotTargetY) : sf::Vector2f(160.f, m_shotTargetY);
                m_dots[m_ballCarrierIdx].speed = 180.f; // Faster run!
                if (m_stateTimer > 1.5f) { m_attackPhase = 2; m_stateTimer = 0.f; }
            } else { // Center Attack
                m_ballCarrierIdx = m_attackFwdIdx;
                float attackX = m_pendingEvent.isHome ? (700.f + (rand()%20 - 10.f)) : (180.f + (rand()%20 - 10.f));
                m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(attackX, m_shotTargetY);
                if (m_stateTimer > 1.5f) { m_attackPhase = 2; m_stateTimer = 0.f; }
            }
            
        } else if (m_attackPhase == 1) {
            // Phase 1: Wait for ball to arrive (Wing Cross)
            m_ballCarrierIdx = -1;
            m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
            
            float fwdDist = std::hypot(m_dots[m_attackFwdIdx].shape.getPosition().x - m_ballTarget.x, m_dots[m_attackFwdIdx].shape.getPosition().y - m_ballTarget.y);
            
            if (ballDist < 10.f && fwdDist < 15.f) {
                // If there are pending events (like the shot outcome), pop it to use for the shot visualization
                if (m_engine->hasLogs()) {
                    m_pendingEvent = m_engine->popRecentLog();
                }
                m_attackPhase = 2;
                m_stateTimer = 0.f;
            }
            
        } else if (m_attackPhase == 9) {
            // Wait for ball to arrive to the passer smoothly before starting the tackle minigame sequence
            m_ballCarrierIdx = m_attackWingerIdx;
            if (m_stateTimer > 1.0f) {
                m_attackPhase = 10;
                m_stateTimer = 0.f;
                
                Player* p = m_gameManager->getPlayer();
                int userPosIdx = 0;
                if (p->position == PlayerPosition::Defender) userPosIdx = 3;
                else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
                else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
                int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;
                
                sf::Vector2f defPos = m_dots[myDefenderIdx].shape.getPosition();
                sf::Vector2f attPos = m_dots[m_attackFwdIdx].shape.getPosition();
                
                // Pass target is the midpoint
                m_ballTarget = sf::Vector2f((defPos.x + attPos.x) / 2.f, (defPos.y + attPos.y) / 2.f);
            }
        } else if (m_attackPhase == 10) {
            // Defender Minigame Phase 1: Ball is passed to forward, defender runs to intercept!
            Player* p = m_gameManager->getPlayer();
            int userPosIdx = 0;
            if (p->position == PlayerPosition::Defender) userPosIdx = 3;
            else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
            else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
            int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;
            
            m_ballCarrierIdx = -1;
            
            // Attacker runs to the pass target
            m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
            m_dots[m_attackFwdIdx].speed = 100.f; 
            
            // Defender target is slightly offset to not overlap perfectly
            float offsetX = m_pendingEvent.isHome ? 12.f : -12.f; 
            sf::Vector2f defTarget(m_ballTarget.x + offsetX, m_ballTarget.y + 8.f);
            m_dots[myDefenderIdx].targetPos = defTarget;
            
            // Defender reaction time: wait slightly before sprinting
            if (m_stateTimer < 0.4f) {
                m_dots[myDefenderIdx].speed = 0.f;
            } else {
                m_dots[myDefenderIdx].speed = 190.f; // Sprint faster to catch up!
            }
            
            float defDist = std::hypot(m_dots[myDefenderIdx].shape.getPosition().x - defTarget.x, m_dots[myDefenderIdx].shape.getPosition().y - defTarget.y);
            float bDist = std::hypot(m_visualBall.getPosition().x - m_ballTarget.x, m_visualBall.getPosition().y - m_ballTarget.y);
            
            // When defender and ball reach their targets, trigger the minigame!
            if ((defDist < 15.f && bDist < 15.f) || m_stateTimer > 3.0f) {
                m_engine->triggerMinigame();
                m_attackPhase = 11; 
                m_stateTimer = 0.f;
            }
            
        } else if (m_attackPhase == 11) {
            // Defender Minigame Phase 2: Pause while minigame is active
            Player* p = m_gameManager->getPlayer();
            int userPosIdx = 0;
            if (p->position == PlayerPosition::Defender) userPosIdx = 3;
            else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
            else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
            int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;
            
            m_dots[m_attackFwdIdx].speed = 0.f;
            m_dots[myDefenderIdx].speed = 0.f;
            
        } else if (m_attackPhase == 12) {
            // Defender Minigame Phase 3: Resolution
            bool isTackle = (m_pendingEvent.text.find("tackle") != std::string::npos || m_pendingEvent.text.find("interception") != std::string::npos);
            Player* p = m_gameManager->getPlayer();
            int userPosIdx = 0;
            if (p->position == PlayerPosition::Defender) userPosIdx = 3;
            else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
            else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
            int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;
            
            if (isTackle) {
                // Success! Defender takes the ball
                m_ballCarrierIdx = myDefenderIdx;
                m_dots[myDefenderIdx].speed = 100.f;
                m_dots[m_attackFwdIdx].speed = 100.f;
                
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
                
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            } else {
                // Failure! Attacker shoots immediately!
                m_attackPhase = 2; 
                m_stateTimer = 0.f;
                m_ballCarrierIdx = -1;
                m_shotTargetY = 290.f + (rand()%60 - 30.f);
                m_dots[myDefenderIdx].speed = 100.f;
                m_dots[m_attackFwdIdx].speed = 100.f;
            }
            
        } else if (m_attackPhase == 2) {
            // Phase 2: The Shot
            m_ballCarrierIdx = -1;
            float targetY = m_shotTargetY;
            
            if (m_pendingEvent.type == EventType::PendingMinigame) {
                // Freeze ball mid-air so animation can finish AFTER minigame
                m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(750.f, targetY) : sf::Vector2f(130.f, targetY);
                if (m_stateTimer > 0.3f) {
                    m_engine->triggerMinigame();
                    m_stateTimer = 0.f;
                    return; 
                }
            } else {
                if (isGoal) {
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(860.f, targetY) : sf::Vector2f(20.f, targetY);
                    m_dots[defenderBase].targetPos = m_pendingEvent.isHome ? sf::Vector2f(810.f, targetY > 290.f ? 250.f : 330.f) : sf::Vector2f(70.f, targetY > 290.f ? 250.f : 330.f);
                } else if (isSave) {
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(810.f, targetY) : sf::Vector2f(70.f, targetY);
                    m_dots[defenderBase].targetPos = m_ballTarget;
                } else {
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(860.f, targetY > 290.f ? 360.f : 220.f) : sf::Vector2f(20.f, targetY > 290.f ? 360.f : 220.f);
                    m_dots[defenderBase].targetPos = m_pendingEvent.isHome ? sf::Vector2f(810.f, 290.f) : sf::Vector2f(70.f, 290.f);
                }
                
                if (isSave) {
                    float distToTarget = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
                    if (distToTarget < 15.f) {
                        int gkIdx = m_pendingEvent.isHome ? 11 : 0;
                        m_ballCarrierIdx = gkIdx;
                    }
                    if (distToTarget < 10.f) {
                        m_attackPhase = 3;
                        m_stateTimer = 0.f;
                    }
                }
                
                if (m_stateTimer > 1.2f) { // slightly longer to ensure trajectory is seen
                    m_attackPhase = 3;
                    m_stateTimer = 0.f;
                }
            }
            
        } else if (m_attackPhase == 3) {
            // Resolution
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            
            if (isGoal) {
                m_visualState = VisualState::GoalCelebration;
                m_stateTimer = 0.f;
                float targetY = m_shotTargetY;
                m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(860.f, targetY > 290.f ? 330.f : 250.f) : sf::Vector2f(20.f, targetY > 290.f ? 330.f : 250.f);
            } else {
                m_visualState = VisualState::GoalKick;
                m_stateTimer = 0.f;
                int gkIdx = m_pendingEvent.isHome ? 11 : 0;
                m_visualBall.setPosition(m_dots[gkIdx].shape.getPosition()); // Snap ball to GK feet
            }
        } else if (m_attackPhase == 40) {
            // Midfielder Attacking: Wait a bit with the ball
            m_dots[m_ballCarrierIdx].targetPos = m_dots[m_ballCarrierIdx].shape.getPosition(); // Stand still
            if (m_stateTimer > 1.0f) {
                m_attackPhase = 41;
                m_stateTimer = 0.f;
                m_engine->triggerMinigame();
            }
        } else if (m_attackPhase == 41) {
            // Minigame resolved, process the pass
            if (m_engine->hasLogs()) m_pendingEvent = m_engine->popRecentLog();
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            
            bool isSuccess = (m_pendingEvent.text.find("Great pass") != std::string::npos || m_pendingEvent.text.find("GOAL") != std::string::npos);
            
            if (isSuccess) {
                // Pass to forward!
                m_ballCarrierIdx = -1;
                m_ballTarget = m_dots[m_attackFwdIdx].shape.getPosition();
                m_attackPhase = 42;
                m_stateTimer = 0.f;
            } else {
                // Bad pass, intercepted by opponent
                m_ballCarrierIdx = -1;
                m_attackPhase = 43; // Fail resolution
                m_stateTimer = 0.f;
                int oppDefenderIdx = (m_engine->isHome() ? 11 : 0) + 3;
                m_ballTarget = m_dots[oppDefenderIdx].shape.getPosition();
            }
        } else if (m_attackPhase == 42) {
            // Ball traveling to Forward
            float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
            if (dist < 10.f) {
                if (m_attackType == 3) {
                    m_attackPhase = 44;
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = m_attackFwdIdx; // Receiver gets the ball
                } else {
                    // Forward shoots!
                    if (m_engine->hasLogs()) {
                        m_pendingEvent = m_engine->popRecentLog();
                    }
                    m_attackPhase = 2;
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = -1;
                    m_shotTargetY = 290.f + (rand()%60 - 30.f);
                }
            }
        } else if (m_attackPhase == 43) {
            // Ball intercepted by opponent
            float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
            if (dist < 10.f) {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
        } else if (m_attackPhase == 50) {
            // Midfielder Defending: Sprint to opponent
            int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
            m_dots[myMidIdx].targetPos = m_dots[m_ballCarrierIdx].shape.getPosition();
            m_dots[myMidIdx].speed = 160.f; // Realistic sprint
            
            float dist = std::hypot(m_dots[myMidIdx].shape.getPosition().x - m_dots[m_ballCarrierIdx].shape.getPosition().x, 
                                    m_dots[myMidIdx].shape.getPosition().y - m_dots[m_ballCarrierIdx].shape.getPosition().y);
            
            if (dist < 15.f) {
                m_attackPhase = 51;
                m_stateTimer = 0.f;
                m_engine->triggerMinigame();
            }
        } else if (m_attackPhase == 51) {
            // Minigame resolved, process the tackle
            if (m_engine->hasLogs()) m_pendingEvent = m_engine->popRecentLog();
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            
            bool isSuccess = (m_pendingEvent.text.find("Great tackle") != std::string::npos || m_pendingEvent.text.find("interception") != std::string::npos);
            int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
            m_dots[myMidIdx].speed = 100.f; // Reset speed
            
            if (isSuccess) {
                m_ballCarrierIdx = myMidIdx;
                m_attackPhase = 52;
                m_stateTimer = 0.f;
            } else {
                // Failed tackle, opponent passes to forward
                m_attackPhase = 1; 
                m_stateTimer = 0.f;
                m_ballCarrierIdx = -1;
                m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(700.f, m_shotTargetY) : sf::Vector2f(180.f, m_shotTargetY);
                int attackerBase = m_pendingEvent.isHome ? 0 : 11;
                do {
                    m_attackFwdIdx = attackerBase + 9 + (rand()%2);
                } while(hasRedCard(m_attackFwdIdx));
                m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
                m_dots[m_attackFwdIdx].speed = 100.f;
            }
        } else if (m_attackPhase == 52) {
            // Succcessful tackle wait a bit
            if (m_stateTimer > 1.0f) {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
        } else if (m_attackPhase == 60) {
            // Midfielder Solo Run
            if (m_stateTimer > 1.5f) {
                m_attackPhase = 61;
                m_stateTimer = 0.f;
                m_engine->triggerMinigame();
            }
        } else if (m_attackPhase == 61) {
            // Minigame resolved, process the solo run
            bool isSuccess = (m_pendingEvent.text.find("GOAL") != std::string::npos);
            int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
            m_dots[myMidIdx].speed = 100.f; // Reset speed
            
            // Go to Phase 2 for shot trajectory
            m_attackPhase = 2;
            m_stateTimer = 0.f;
            m_ballCarrierIdx = -1;
            m_shotTargetY = 290.f + (rand()%60 - 30.f);
        } else if (m_attackPhase == 44) {
            // Receiver gets the ball
            if (m_stateTimer > 0.5f) {
                if (m_attackType == 3) {
                    // Backward/Sideways pass: no shot, just commit and normal play
                    m_engine->commitEvent(m_pendingEvent);
                    m_visibleLogs.push_back(m_pendingEvent);
                    if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
                    
                    m_pendingEvent = MatchEvent{"", EventType::Normal, true};
                    m_visualState = VisualState::NormalPlay;
                    m_stateTimer = 0.f;
                } else {
                    // Forward pass: go to shot
                    m_attackPhase = 2; // Go to shot
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = -1;
                }
            }
        }
        
        // Defend and ball attachment logic
        if ((m_attackPhase < 2 || m_attackPhase == 9 || m_attackPhase == 40 || m_attackPhase == 44 || m_attackPhase == 50) && m_ballCarrierIdx != -1) {
            m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
            
            int numDefendersToRun = 4;
            if (m_attackPhase == 40 || m_attackPhase == 44) {
                numDefendersToRun = m_attackWingerIdx; // the value we stored (1 or 2)
            }
            
            for (int i = 1; i <= numDefendersToRun; ++i) {
                sf::Vector2f carrierPos = m_dots[m_ballCarrierIdx].shape.getPosition();
                float offsetX = m_pendingEvent.isHome ? (i * 15.f) : -(i * 15.f);
                float offsetY = (i%2==0) ? (i * 20.f) : -(i * 20.f);
                m_dots[defenderBase + i].targetPos = sf::Vector2f(carrierPos.x + offsetX, carrierPos.y + offsetY);
            }
        }
        if (m_attackPhase < 2) m_dots[defenderBase].targetPos = m_pendingEvent.isHome ? sf::Vector2f(810.f, 290.f) : sf::Vector2f(70.f, 290.f);
    } 
    else if (m_visualState == VisualState::GoalCelebration) {
        if (m_stateTimer > 3.0f) {
            resetToKickoff();
        }
    }
    else if (m_visualState == VisualState::WaitingForMinigame) {
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
            m_engine->triggerMinigame();
            m_visualState = VisualState::NormalPlay;
        }
    }
    
    // Move dots
    for(auto& d : m_dots) {
        if (d.targetPos.x < 50.f) d.targetPos.x = 50.f; if (d.targetPos.x > 820.f) d.targetPos.x = 820.f;
        if (d.targetPos.y < 140.f) d.targetPos.y = 140.f; if (d.targetPos.y > 440.f) d.targetPos.y = 440.f;
        sf::Vector2f dir = d.targetPos - d.shape.getPosition();
        float len = std::hypot(dir.x, dir.y);
        
        float currentSpeed = d.speed;
        if (m_visualState == VisualState::Attacking) currentSpeed = 150.f; // Sprinting during attack
        
        if (len > 0) d.shape.move((dir.x / len) * currentSpeed * dt, (dir.y / len) * currentSpeed * dt);
    }
    
    // Move ball globally
    if (m_ballCarrierIdx != -1) {
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
    }
    
    float globalBDist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
    if (m_ballCarrierIdx != -1 && globalBDist < 10.f) {
        m_visualBall.setPosition(m_ballTarget); // Snap rigidly if close and held
    } else if (globalBDist > 0.f) {
        sf::Vector2f bdir = m_ballTarget - m_visualBall.getPosition();
        float bspeed = (m_visualState == VisualState::Attacking) ? 500.f : 400.f;
        float moveDist = bspeed * dt;
        if (moveDist >= globalBDist) {
            m_visualBall.setPosition(m_ballTarget);
        } else {
            m_visualBall.move((bdir.x / globalBDist) * moveDist, (bdir.y / globalBDist) * moveDist);
        }
    }
}

void MatchScreen::update(sf::Time deltaTime) {
    if (!m_engine) return;
    
    if (m_engine->getState() == MatchState::Finished) {
        if (!m_engine->hasLogs() && m_visualState == VisualState::NormalPlay) {
            m_scriptTimer += deltaTime.asSeconds();
            if (m_scriptTimer > 2.0f) {
                m_gameManager->changeScreen(std::make_shared<MatchStatsScreen>(m_engine));
                return;
            }
        }
    }
    
    if (m_engine->getState() == MatchState::MinigameTriggered) {
        if (!m_minigameActive) {
            initMinigame();
            m_minigameActive = true;
        }
        updateMinigame(deltaTime);
        return;
    } else if (m_visualState == VisualState::Attacking && m_pendingEvent.type == EventType::PendingMinigame && m_engine->getState() == MatchState::Simulating && m_engine->hasLogs()) {
        bool originalIsHome = m_pendingEvent.isHome;
        m_pendingEvent = m_engine->popRecentLog();
        
        // If the minigame resulted in a tackle, preserve the attacking side 
        // so the opponent attack resolves correctly visually on the same side
        if (m_pendingEvent.text.find("tackle") != std::string::npos || m_pendingEvent.text.find("interception") != std::string::npos) {
            m_pendingEvent.isHome = originalIsHome; 
        }
        
        Player* p = m_gameManager->getPlayer();
        bool isTackleMinigame = p && p->position != PlayerPosition::Goalkeeper && (originalIsHome != m_engine->isHome());
        
        if (m_attackPhase == 41) {
            // Keep phase 41, it handles its own resolution
        } else if (m_attackPhase == 51) {
            // Keep phase 51, it handles its own resolution
        } else if (m_attackPhase == 61) {
            // Keep phase 61, it handles its own resolution
        } else if (isTackleMinigame) {
            m_attackPhase = 12; // Go to tackle resolution
        } else {
            m_attackPhase = 2; // Go to shot resolution
        }
        m_stateTimer = 0.f;
    }
    
    updateVisuals(deltaTime);
    
    if (m_engine->getState() == MatchState::Simulating || m_engine->getState() == MatchState::Finished) {
        float speedDelay = 0.4f;
        if (g_settings.matchSpeed == 0) speedDelay = 1.0f;
        if (g_settings.matchSpeed == 2) speedDelay = 0.1f;
        if (g_settings.matchSpeed == 3) speedDelay = 0.0f;
        
        m_simTimer += deltaTime.asSeconds();
        if (m_simTimer > speedDelay) {
            m_simTimer = 0.f;
            if (m_engine->getState() == MatchState::Simulating && m_visualState == VisualState::NormalPlay && !m_engine->hasLogs()) {
                m_engine->updateMinute();
            }
        }
        
        if (m_engine->hasLogs() && m_visualState == VisualState::NormalPlay) {
            float distToCarrier = 0.f;
            if (m_ballCarrierIdx != -1) distToCarrier = std::hypot(m_visualBall.getPosition().x - m_dots[m_ballCarrierIdx].shape.getPosition().x, m_visualBall.getPosition().y - m_dots[m_ballCarrierIdx].shape.getPosition().y);
            if (distToCarrier < 10.f) {
                m_pendingEvent = m_engine->popRecentLog();
            
            if (m_isMinigameResultPending) {
                m_isMinigameResultPending = false;
                
                if (m_engine->hasLogs()) {
                    m_pendingEvent = m_engine->popRecentLog();
                }
                
                m_visualState = VisualState::Attacking;
                m_attackPhase = 3;
                m_stateTimer = 0.f;
            } 
            else if (m_pendingEvent.type == EventType::Goal || m_pendingEvent.type == EventType::Chance || m_pendingEvent.type == EventType::PendingMinigame) {
                m_visualState = VisualState::Attacking;
                m_attackPhase = 0;
                m_stateTimer = 0.f;
                m_attackType = rand() % 3; // Choose 0 (Wing), 1 (Solo), 2 (Center)
                int attackerBase = m_pendingEvent.isHome ? 0 : 11;
                m_attackWingerIdx = attackerBase + ((rand()%2==0)?5:8);
                int options[] = {9, 10};
                m_attackFwdIdx = attackerBase + options[rand() % 2];
                m_shotTargetY = 290.f + (rand()%60 - 30.f);
            } else {
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
                
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
            } // Close distToCarrier
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
    
    m_scoreText.setString(std::to_string(m_engine->getHomeScore()) + " - " + std::to_string(m_engine->getAwayScore()));
    m_timeText.setString(std::to_string(m_engine->getMinute()) + "'");
    
    std::string fullLog = "";
    for (const auto& l : m_visibleLogs) { fullLog += l.text + "\n"; }
    m_logText.setString(sf::String::fromUtf8(fullLog.begin(), fullLog.end()));
    
    auto homeStats = m_engine->isHome() ? m_engine->getPlayerTeamStats() : m_engine->getOpponentTeamStats();
    auto awayStats = m_engine->isHome() ? m_engine->getOpponentTeamStats() : m_engine->getPlayerTeamStats();
    
    m_homeStatsText.setString(
        "Shots: " + std::to_string(homeStats.shots) + "\n" +
        "Cards: " + std::to_string(homeStats.yellowCards) + "Y " + std::to_string(homeStats.redCards) + "R\n"
    );
    m_awayStatsText.setString(
        "Shots: " + std::to_string(awayStats.shots) + "\n" +
        "Cards: " + std::to_string(awayStats.yellowCards) + "Y " + std::to_string(awayStats.redCards) + "R\n"
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
        if (m_pendingEvent.isHome != m_engine->isHome()) {
            m_playerSprite.setPosition(600.f, 450.f); m_targetSprite.setPosition(600.f, 400.f);
            m_enemySprite.setPosition(600.f, 250.f); m_ballSprite.setPosition(m_enemySprite.getPosition());
            m_enemySpeed = 80.f + ((100.f - p->tackling) * 2.f) + (oppStrength * 1.5f);
        } else if (m_midfielderSoloRun) {
            m_playerSprite.setPosition(600.f, 450.f); m_ballSprite.setPosition(620.f, 450.f);
            m_targetSprite.setPosition(600.f, 280.f); m_targetDir = 1.f;
            float r = 10.f + (p->shooting / 100.f) * 30.f; m_targetSprite.setRadius(r);
        } else {
            m_playerSprite.setPosition(600.f, 400.f); m_ballSprite.setPosition(620.f, 400.f);
            m_targetSprite.setPosition(350.f + (rand()%400), 280.f); m_targetSprite.setRadius(25.f);
            m_enemySprite.setPosition(m_targetSprite.getPosition().x, 340.f); m_enemyDir = (rand()%2 == 0) ? 1.f : -1.f;
            m_enemySpeed = 80.f + ((100.f - p->passing) * 3.f);
        }
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
        if (m_minigameTimer > 5.0f) { m_engine->processMinigameResult(false, false); m_minigameActive = false; }
    } else if (p->position == PlayerPosition::Midfielder) {
        if (m_pendingEvent.isHome != m_engine->isHome()) {
            m_enemySprite.move(0, m_enemySpeed * dt); m_ballSprite.setPosition(m_enemySprite.getPosition());
            if (m_enemySprite.getPosition().y > 500.f) { m_engine->processMinigameResult(false, false); m_minigameActive = false; }
        } else if (m_midfielderSoloRun) {
            m_minigameTimer += dt;
            float speed = 50.f + ((100.f - p->shooting) * 2.f); m_targetSprite.move(m_targetDir * speed * dt, 0.f);
            if (m_targetSprite.getPosition().x < 320.f) m_targetDir = 1.f;
            if (m_targetSprite.getPosition().x > 880.f - m_targetSprite.getRadius()*2.f) m_targetDir = -1.f;
            if (m_minigameTimer > 5.0f) { m_engine->processMinigameResult(false, true); m_minigameActive = false; }
        } else {
            m_minigameTimer += dt; m_enemySprite.move(m_enemyDir * m_enemySpeed * dt, 0.f);
            if (m_enemySprite.getPosition().x < 350.f) m_enemyDir = 1.f; if (m_enemySprite.getPosition().x > 850.f) m_enemyDir = -1.f;
            if (m_minigameTimer > 5.0f) { m_engine->processMinigameResult(false, false); m_minigameActive = false; }
        }
    } else if (p->position == PlayerPosition::Defender) {
        m_enemySprite.move(0, m_enemySpeed * dt); m_ballSprite.setPosition(m_enemySprite.getPosition());
        if (m_enemySprite.getPosition().y > 500.f) { m_engine->processMinigameResult(false, false); m_minigameActive = false; }
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
        int localIdx = (int)i % 11;
        if (!hasRedCard(i)) {
            window.draw(m_dots[i].shape);
        }
        
        if (m_dots[i].isHome == m_engine->isHome() && localIdx == userPosIdx && !m_engine->isUserSubbedOff()) {
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
    if (!m_minigameActive) {
        window.draw(m_btnSkipRect);
        window.draw(m_btnSkipText);
        for (size_t i = 0; i < m_speedButtons.size(); ++i) {
            sf::RectangleShape r = m_speedButtons[i].rect;
            if (m_matchSpeedMode == (int)i) r.setOutlineThickness(2.f);
            else r.setOutlineThickness(0.f);
            r.setOutlineColor(sf::Color::Yellow);
            window.draw(r);
            window.draw(m_speedButtons[i].text);
        }
    }
    
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
