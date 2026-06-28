#include "MatchScreen.h"
#include "MatchStatsScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <iostream>

MatchScreen::MatchScreen() : m_engine(nullptr), m_minigameActive(false), m_simTimer(0.f) {}

void MatchScreen::init() {
    Player* p = m_gameManager->getPlayer();
    
    // Find an opponent from the same league
    Club* opp = nullptr;
    bool isHomeMatch = true;
    Club* playerClub = p->currentClub;
    
    if (m_gameManager->getCareerManager()->hasInternationalMatchToday()) {
        opp = m_gameManager->getCareerManager()->getInternationalOpponent();
        isHomeMatch = m_gameManager->getCareerManager()->isHomeInternationalMatch();
        // Use national team as player's club
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
                if (c.name == p->currentClub->name) {
                    lg = &l;
                    break;
                }
            }
        }
        
        if (lg) {
            int n = lg->clubs.size();
            int r = p->weeksPlayed % (n - 1);
            
            int pIndex = -1;
            for (int i = 0; i < n; ++i) {
                if (lg->clubs[i].name == p->currentClub->name) {
                    pIndex = i;
                    break;
                }
            }
            
            auto rotate = [n, r](int x) {
                if (x == 0) return 0;
                return 1 + (x - 1 + r) % (n - 1);
            };
            
            for (int i = 0; i < n / 2; ++i) {
                int t1 = (i == 0) ? 0 : rotate(i);
                int t2 = rotate(n - 1 - i);
                
                if (t1 == pIndex) {
                    opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t2].name);
                    break;
                } else if (t2 == pIndex) {
                    opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t1].name);
                    isHomeMatch = false;
                    break;
                }
            }
        }
    }
    
    m_engine = std::make_shared<MatchEngine>(playerClub, opp, isHomeMatch, p);

    auto& font = AssetManager::get().getFont("MainFont");
    
    m_scoreText.setFont(font);
    m_scoreText.setCharacterSize(30);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setPosition(200.f, 20.f);
    
    m_timeText.setFont(font);
    m_timeText.setCharacterSize(24);
    m_timeText.setFillColor(sf::Color::Yellow);
    m_timeText.setPosition(380.f, 60.f);
    
    m_logText.setFont(font);
    m_logText.setCharacterSize(16);
    m_logText.setFillColor(sf::Color::Green);
    m_logText.setPosition(50.f, 100.f);
    
    m_statusText.setFont(font);
    m_statusText.setCharacterSize(20);
    m_statusText.setFillColor(sf::Color::Cyan);
    m_statusText.setPosition(50.f, 500.f);
    m_statusText.setString("Status: On the pitch (Starter)");
    
    m_pitchRect.setSize(sf::Vector2f(600.f, 300.f));
    m_pitchRect.setPosition(100.f, 250.f);
    m_pitchRect.setFillColor(sf::Color(34, 139, 34)); // Grass green
    m_pitchRect.setOutlineThickness(2.f);
    m_pitchRect.setOutlineColor(sf::Color::White);
    
    m_playerSprite.setRadius(10.f);
    m_playerSprite.setFillColor(sf::Color::Blue);
    m_ballSprite.setRadius(5.f);
    m_ballSprite.setFillColor(sf::Color::White);
    m_targetSprite.setRadius(30.f);
    m_targetSprite.setFillColor(sf::Color(255, 255, 255, 100)); // Semi transparent
    m_enemySprite.setRadius(10.f);
    m_enemySprite.setFillColor(sf::Color::Red);
    
    m_promptText.setFont(font);
    m_promptText.setCharacterSize(24);
    m_promptText.setFillColor(sf::Color::Yellow);
    m_promptText.setString("Press SPACE to Tackle!");
    m_promptText.setPosition(250.f, 200.f);
}

void MatchScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (m_minigameActive) {
        Player* p = m_gameManager->getPlayer();
        
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            
            if (p->position == PlayerPosition::Forward || p->position == PlayerPosition::Goalkeeper) {
                // Shoot or Save towards mouse
                if (m_targetSprite.getGlobalBounds().contains(mousePos)) {
                    m_engine->processMinigameResult(true);
                } else {
                    m_engine->processMinigameResult(false);
                }
                m_minigameActive = false;
            } else if (p->position == PlayerPosition::Midfielder) {
                // Shoot towards mouse, but check line of sight intersection with enemy
                if (m_targetSprite.getGlobalBounds().contains(mousePos)) {
                    // Check intersection
                    sf::Vector2f pPos = m_playerSprite.getPosition();
                    sf::Vector2f ePos = m_enemySprite.getPosition();
                    // Basic check: if enemy is in the middle of X and roughly similar Y
                    float minX = std::min(pPos.x, mousePos.x);
                    float maxX = std::max(pPos.x, mousePos.x);
                    if (ePos.x > minX - 20.f && ePos.x < maxX + 20.f && std::abs(ePos.y - mousePos.y) < 40.f) {
                        // Intercepted!
                        m_engine->processMinigameResult(false);
                    } else {
                        m_engine->processMinigameResult(true);
                    }
                } else {
                    m_engine->processMinigameResult(false);
                }
                m_minigameActive = false;
            }
        }
        else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
            if (p->position == PlayerPosition::Defender) {
                // Timing minigame
                if (m_enemySprite.getGlobalBounds().intersects(m_targetSprite.getGlobalBounds())) {
                    m_engine->processMinigameResult(true);
                } else {
                    m_engine->processMinigameResult(false);
                }
                m_minigameActive = false;
            }
        }
    }
}

void MatchScreen::update(sf::Time deltaTime) {
    if (!m_engine) return;
    
    if (m_engine->getState() == MatchState::Finished) {
        // We wait a second to let user read the final whistle log, then transition
        m_simTimer += deltaTime.asSeconds();
        if (m_simTimer > 2.0f) {
            m_gameManager->changeScreen(std::make_shared<MatchStatsScreen>(m_engine));
            return; // Fix the crash!
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
    
    if (m_engine->getState() == MatchState::Simulating) {
        m_simTimer += deltaTime.asSeconds();
        if (m_simTimer > 0.4f) { // Slowed down simulation further
            m_simTimer = 0.f;
            m_engine->updateMinute();
            
            while (m_engine->hasLogs()) {
                m_visibleLogs.push_back(m_engine->popRecentLog());
                if (m_visibleLogs.size() > 8) {
                    m_visibleLogs.erase(m_visibleLogs.begin());
                }
            }
        }
    }
    
    // Update texts
    std::string homeName = m_engine->isHome() ? m_engine->getPlayerClub()->name : m_engine->getOpponentClub()->name;
    std::string awayName = m_engine->isHome() ? m_engine->getOpponentClub()->name : m_engine->getPlayerClub()->name;
    int homeStr = m_engine->isHome() ? m_engine->getPlayerClub()->strength : m_engine->getOpponentClub()->strength;
    int awayStr = m_engine->isHome() ? m_engine->getOpponentClub()->strength : m_engine->getPlayerClub()->strength;

    std::string scoreStr = "[" + std::to_string(homeStr) + "] " + 
                           homeName + " " + std::to_string(m_engine->getHomeScore()) + 
                           " - " + std::to_string(m_engine->getAwayScore()) + " " + awayName + 
                           " [" + std::to_string(awayStr) + "]";
    m_scoreText.setString(sf::String::fromUtf8(scoreStr.begin(), scoreStr.end()));
    
    m_timeText.setString(std::to_string(m_engine->getMinute()) + "'");
    
    std::string fullLog = "";
    for (const auto& l : m_visibleLogs) {
        fullLog += l + "\n";
    }
    m_logText.setString(sf::String::fromUtf8(fullLog.begin(), fullLog.end()));
}

void MatchScreen::initMinigame() {
    Player* p = m_gameManager->getPlayer();
    m_minigameTimer = 0.f;
    
    // Scale difficulty
    int oppStrength = m_engine->getOpponentClub()->strength;
    
    if (p->position == PlayerPosition::Forward) {
        m_playerSprite.setPosition(380.f, 450.f);
        m_ballSprite.setPosition(400.f, 450.f);
        m_targetSprite.setPosition(380.f, 280.f); 
        m_targetDir = 1.f;
        // Make target smaller if shooting is low
        float r = 10.f + (p->shooting / 100.f) * 30.f;
        m_targetSprite.setRadius(r);
    } else if (p->position == PlayerPosition::Midfielder) {
        m_playerSprite.setPosition(380.f, 400.f);
        m_ballSprite.setPosition(400.f, 400.f);
        m_targetSprite.setPosition(150.f + (rand()%400), 280.f);
        m_targetSprite.setRadius(25.f);
        // Enemy defender spawns in the passing lane
        m_enemySprite.setPosition(m_targetSprite.getPosition().x, 340.f);
        m_enemyDir = (rand()%2 == 0) ? 1.f : -1.f;
        m_enemySpeed = 80.f + ((100.f - p->passing) * 3.f); // Made slightly faster
    } else if (p->position == PlayerPosition::Defender) {
        m_playerSprite.setPosition(380.f, 450.f);
        m_targetSprite.setPosition(380.f, 400.f);
        m_enemySprite.setPosition(380.f, 250.f);
        m_ballSprite.setPosition(m_enemySprite.getPosition());
        m_enemySpeed = 80.f + ((100.f - p->tackling) * 2.f) + (oppStrength * 1.5f);
    } else if (p->position == PlayerPosition::Goalkeeper) {
        m_playerSprite.setPosition(380.f, 280.f);
        m_ballSprite.setPosition(380.f, 450.f);
        m_targetSprite.setPosition(250.f + (rand()%260), 280.f + (rand()%50));
        m_targetSprite.setRadius(30.f);
    }
}

void MatchScreen::updateMinigame(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    float dt = deltaTime.asSeconds();
    
    if (p->position == PlayerPosition::Forward) {
        m_minigameTimer += dt;
        
        float speed = 50.f + ((100.f - p->shooting) * 2.f);
        m_targetSprite.move(m_targetDir * speed * dt, 0.f);
        if (m_targetSprite.getPosition().x < 120.f) m_targetDir = 1.f;
        if (m_targetSprite.getPosition().x > 680.f - m_targetSprite.getRadius()*2.f) m_targetDir = -1.f;
        
        if (m_minigameTimer > 5.0f) {
            m_engine->processMinigameResult(false);
            m_minigameActive = false;
        }
    } else if (p->position == PlayerPosition::Midfielder) {
        m_minigameTimer += dt;
        
        m_enemySprite.move(m_enemyDir * m_enemySpeed * dt, 0.f);
        if (m_enemySprite.getPosition().x < 150.f) m_enemyDir = 1.f;
        if (m_enemySprite.getPosition().x > 650.f) m_enemyDir = -1.f;
        
        if (m_minigameTimer > 5.0f) {
            m_engine->processMinigameResult(false);
            m_minigameActive = false;
        }
    } else if (p->position == PlayerPosition::Defender) {
        m_enemySprite.move(0, m_enemySpeed * dt);
        m_ballSprite.setPosition(m_enemySprite.getPosition());
        
        if (m_enemySprite.getPosition().y > 500.f) {
            m_engine->processMinigameResult(false);
            m_minigameActive = false;
        }
    } else if (p->position == PlayerPosition::Goalkeeper) {
        m_minigameTimer += dt;
        
        float ballSpeed = 1.f + ((100.f - p->goalkeeping) / 50.f);
        m_ballSprite.move((m_targetSprite.getPosition().x - 380.f) * dt / ballSpeed,
                          (m_targetSprite.getPosition().y - 450.f) * dt / ballSpeed);
                          
        if (m_minigameTimer > ballSpeed) { 
            m_engine->processMinigameResult(false);
            m_minigameActive = false;
        }
    }
}

void MatchScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 30));
    window.draw(m_scoreText);
    window.draw(m_timeText);
    window.draw(m_logText);
    window.draw(m_statusText);
    
    if (m_minigameActive) {
        window.draw(m_pitchRect);
        
        Player* p = m_gameManager->getPlayer();
        window.draw(m_targetSprite);
        window.draw(m_playerSprite);
        
        if (p->position == PlayerPosition::Defender || p->position == PlayerPosition::Midfielder) {
            window.draw(m_enemySprite);
        }
        if (p->position == PlayerPosition::Defender) {
            window.draw(m_promptText);
        }
        
        window.draw(m_ballSprite);
        
        auto& font = AssetManager::get().getFont("MainFont");
        sf::Text inst;
        inst.setFont(font);
        inst.setCharacterSize(18);
        inst.setFillColor(sf::Color::Yellow);
        inst.setPosition(120.f, 560.f);
        
        if (p->position == PlayerPosition::Forward) inst.setString("Click the target area to shoot! (5s)");
        else if (p->position == PlayerPosition::Midfielder) inst.setString("Click the target area to pass! (5s)");
        else if (p->position == PlayerPosition::Defender) inst.setString("Click ANYWHERE when the red attacker enters the target circle!");
        else if (p->position == PlayerPosition::Goalkeeper) inst.setString("Click the target area to save the shot! (3s)");
        
        window.draw(inst);
    }
}
