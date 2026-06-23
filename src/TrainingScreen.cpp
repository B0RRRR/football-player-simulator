#include "TrainingScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <cstdlib>
#include <cmath>

TrainingScreen::TrainingScreen() : m_state(TrainingState::Intro), m_score(0), m_maxScore(5), m_xpEarned(0), m_timeRemaining(15.f) {
}

void TrainingScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();
    
    m_mainText.setFont(font);
    m_mainText.setCharacterSize(36);
    m_mainText.setFillColor(sf::Color::White);
    m_mainText.setPosition(100.f, 50.f);
    
    m_infoText.setFont(font);
    m_infoText.setCharacterSize(24);
    m_infoText.setFillColor(sf::Color::Yellow);
    m_infoText.setPosition(100.f, 150.f);
    
    m_btnRect.setSize(sf::Vector2f(200.f, 50.f));
    m_btnRect.setPosition(300.f, 450.f);
    m_btnRect.setFillColor(sf::Color(100, 100, 100));
    m_btnText.setFont(font);
    m_btnText.setString("Start Drill");
    m_btnText.setCharacterSize(24);
    m_btnText.setFillColor(sf::Color::White);
    sf::FloatRect tr = m_btnText.getLocalBounds();
    m_btnText.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
    m_btnText.setPosition(400.f, 475.f);

    std::string role;
    if (p->position == PlayerPosition::Forward) role = "Shooting Drill";
    else if (p->position == PlayerPosition::Midfielder) role = "Passing Drill";
    else if (p->position == PlayerPosition::Defender) role = "Tackling Drill";
    else if (p->position == PlayerPosition::Goalkeeper) role = "Goalkeeping Drill";
    
    m_mainText.setString("Training: " + role);
    m_infoText.setString("Follow the instructions to earn XP.\nEnergy cost: -15.");
}

void TrainingScreen::initGame() {
    m_state = TrainingState::Playing;
    m_score = 0;
    Player* p = m_gameManager->getPlayer();
    
    if (p->position == PlayerPosition::Forward) {
        m_maxScore = 5;
        m_shotsTaken = 0;
        m_targetDir = 1.0f;
        m_powerDir = 1.0f;
        m_powerValue = 0.0f;
        
        m_goalRect.setSize(sf::Vector2f(400.f, 150.f));
        m_goalRect.setPosition(200.f, 100.f);
        m_goalRect.setFillColor(sf::Color::Transparent);
        m_goalRect.setOutlineThickness(5.f);
        m_goalRect.setOutlineColor(sf::Color::White);
        
        m_targetRect.setSize(sf::Vector2f(60.f, 60.f));
        m_targetRect.setPosition(370.f, 145.f);
        m_targetRect.setFillColor(sf::Color::Red);
        
        m_powerBarBg.setSize(sf::Vector2f(50.f, 200.f));
        m_powerBarBg.setPosition(100.f, 300.f);
        m_powerBarBg.setFillColor(sf::Color(50, 50, 50));
        
        m_powerBarFill.setSize(sf::Vector2f(50.f, 0.f));
        m_powerBarFill.setPosition(100.f, 500.f);
        m_powerBarFill.setFillColor(sf::Color::Green);
    } 
    else if (p->position == PlayerPosition::Midfielder) {
        m_maxScore = 10;
        m_passesAttempted = 0;
        m_spawnTimer = 0.f;
        m_teammates.clear();
    }
    else if (p->position == PlayerPosition::Defender) {
        m_maxScore = 5;
        m_tacklesAttempted = 0;
        m_tackleZone.setRadius(50.f);
        m_tackleZone.setPosition(400.f - 50.f, 400.f - 50.f);
        m_tackleZone.setFillColor(sf::Color::Transparent);
        m_tackleZone.setOutlineThickness(5.f);
        m_tackleZone.setOutlineColor(sf::Color::Green);
        
        m_attacker.setSize(sf::Vector2f(20.f, 20.f));
        m_attacker.setPosition(400.f - 10.f, 50.f);
        m_attacker.setFillColor(sf::Color::Red);
        m_attackerSpeed = 200.f;
    }
    else if (p->position == PlayerPosition::Goalkeeper) {
        m_maxScore = 10;
        m_savesAttempted = 0;
        m_ballSpawnTimer = 0.f;
        m_balls.clear();
        m_goalRect.setSize(sf::Vector2f(600.f, 200.f));
        m_goalRect.setPosition(100.f, 300.f);
        m_goalRect.setFillColor(sf::Color(50, 50, 50));
    }
}

void TrainingScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (m_state == TrainingState::Intro) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            if (m_btnRect.getGlobalBounds().contains(mousePos)) {
                initGame();
            }
        }
    } 
    else if (m_state == TrainingState::Playing) {
        Player* p = m_gameManager->getPlayer();
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
            if (p->position == PlayerPosition::Forward) {
                // Shoot! Check if target is near center and power is good
                float targetCenter = m_targetRect.getPosition().x + 30.f;
                float dist = std::abs(targetCenter - 400.f);
                
                if (m_powerValue > 40.f && m_powerValue < 90.f && dist < 100.f) {
                    m_score++;
                }
                m_shotsTaken++;
                m_powerValue = 0.f;
                
                if (m_shotsTaken >= m_maxScore) finishGame();
            }
            else if (p->position == PlayerPosition::Defender) {
                // Tackle! Check if attacker is inside zone
                float ay = m_attacker.getPosition().y + 10.f;
                float zCenter = m_tackleZone.getPosition().y + 50.f;
                
                if (std::abs(ay - zCenter) < 40.f) {
                    m_score++;
                }
                m_tacklesAttempted++;
                m_attacker.setPosition(400.f - 10.f, 50.f); // Reset
                m_attackerSpeed += 50.f;
                
                if (m_tacklesAttempted >= m_maxScore) finishGame();
            }
        }
        else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            
            if (p->position == PlayerPosition::Midfielder) {
                for (auto it = m_teammates.begin(); it != m_teammates.end(); ) {
                    if (it->shape.getGlobalBounds().contains(mousePos)) {
                        m_score++;
                        m_passesAttempted++;
                        it = m_teammates.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            else if (p->position == PlayerPosition::Goalkeeper) {
                for (auto it = m_balls.begin(); it != m_balls.end(); ) {
                    if (it->shape.getGlobalBounds().contains(mousePos)) {
                        m_score++;
                        m_savesAttempted++;
                        it = m_balls.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
    }
    else if (m_state == TrainingState::Result) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            if (m_btnRect.getGlobalBounds().contains(mousePos)) {
                // Advance day and exit
                m_gameManager->getCareerManager()->advanceDay();
                m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            }
        }
    }
}

void TrainingScreen::updateGame(float dt) {
    Player* p = m_gameManager->getPlayer();
    
    if (p->position == PlayerPosition::Forward) {
        // Target moving
        m_targetRect.move(m_targetDir * 300.f * dt, 0.f);
        if (m_targetRect.getPosition().x < 200.f) {
            m_targetRect.setPosition(200.f, m_targetRect.getPosition().y);
            m_targetDir = 1.f;
        } else if (m_targetRect.getPosition().x > 600.f - 60.f) {
            m_targetRect.setPosition(600.f - 60.f, m_targetRect.getPosition().y);
            m_targetDir = -1.f;
        }
        
        // Power bar
        m_powerValue += m_powerDir * 150.f * dt;
        if (m_powerValue > 100.f) {
            m_powerValue = 100.f;
            m_powerDir = -1.f;
        } else if (m_powerValue < 0.f) {
            m_powerValue = 0.f;
            m_powerDir = 1.f;
        }
        
        m_powerBarFill.setSize(sf::Vector2f(50.f, - (m_powerValue / 100.f) * 200.f));
    }
    else if (p->position == PlayerPosition::Midfielder) {
        m_spawnTimer -= dt;
        if (m_spawnTimer <= 0.f && m_passesAttempted + m_teammates.size() < m_maxScore) {
            Teammate t;
            t.shape.setRadius(20.f);
            t.shape.setFillColor(sf::Color::Green);
            float rx = 100.f + (rand() % 600);
            float ry = 100.f + (rand() % 400);
            t.shape.setPosition(rx, ry);
            t.timeAlive = 1.5f;
            m_teammates.push_back(t);
            m_spawnTimer = 0.8f;
        }
        
        for (auto it = m_teammates.begin(); it != m_teammates.end(); ) {
            it->timeAlive -= dt;
            if (it->timeAlive <= 0.f) {
                m_passesAttempted++;
                it = m_teammates.erase(it);
            } else {
                // Fade out
                sf::Color c = it->shape.getFillColor();
                c.a = static_cast<sf::Uint8>((it->timeAlive / 1.5f) * 255);
                it->shape.setFillColor(c);
                ++it;
            }
        }
        
        if (m_passesAttempted >= m_maxScore) finishGame();
    }
    else if (p->position == PlayerPosition::Defender) {
        m_attacker.move(0.f, m_attackerSpeed * dt);
        if (m_attacker.getPosition().y > 600.f) {
            m_tacklesAttempted++;
            m_attacker.setPosition(400.f - 10.f, 50.f);
            m_attackerSpeed += 50.f;
        }
        if (m_tacklesAttempted >= m_maxScore) finishGame();
    }
    else if (p->position == PlayerPosition::Goalkeeper) {
        m_ballSpawnTimer -= dt;
        if (m_ballSpawnTimer <= 0.f && m_savesAttempted + m_balls.size() < m_maxScore) {
            Ball b;
            b.shape.setRadius(15.f);
            b.shape.setFillColor(sf::Color::White);
            float sx = rand() % 800;
            b.shape.setPosition(sx, 0.f);
            
            float targetX = 100.f + (rand() % 600);
            float targetY = 400.f; // Middle of goal
            float dx = targetX - sx;
            float dy = targetY - 0.f;
            float len = std::sqrt(dx*dx + dy*dy);
            b.dir = sf::Vector2f(dx/len, dy/len);
            
            m_balls.push_back(b);
            m_ballSpawnTimer = 0.6f;
        }
        
        for (auto it = m_balls.begin(); it != m_balls.end(); ) {
            it->shape.move(it->dir.x * 300.f * dt, it->dir.y * 300.f * dt);
            if (it->shape.getPosition().y > 500.f) {
                m_savesAttempted++;
                it = m_balls.erase(it);
            } else {
                ++it;
            }
        }
        
        if (m_savesAttempted >= m_maxScore) finishGame();
    }
}

void TrainingScreen::update(sf::Time deltaTime) {
    if (m_state == TrainingState::Playing) {
        updateGame(deltaTime.asSeconds());
        if (m_state == TrainingState::Playing) {
            m_infoText.setString("Score: " + std::to_string(m_score) + " / " + std::to_string(m_maxScore));
        }
    }
}

void TrainingScreen::finishGame() {
    m_state = TrainingState::Result;
    
    // Calculate XP
    float accuracy = static_cast<float>(m_score) / m_maxScore;
    m_xpEarned = 5 + static_cast<int>(accuracy * 25.f); // 5 to 30 XP
    
    Player* p = m_gameManager->getPlayer();
    p->experience += m_xpEarned;
    
    m_mainText.setString("Training Completed!");
    m_infoText.setString("Score: " + std::to_string(m_score) + " / " + std::to_string(m_maxScore) + 
                         "\nXP Earned: +" + std::to_string(m_xpEarned) + "\nEnergy: -15");
    
    m_btnText.setString("Continue");
    sf::FloatRect tr = m_btnText.getLocalBounds();
    m_btnText.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
    m_btnText.setPosition(400.f, 475.f);
}

void TrainingScreen::drawGame(sf::RenderWindow& window) {
    Player* p = m_gameManager->getPlayer();
    
    if (p->position == PlayerPosition::Forward) {
        window.draw(m_goalRect);
        
        // Draw center marker for the goal to show where perfect timing is
        sf::RectangleShape centerLine(sf::Vector2f(2.f, 150.f));
        centerLine.setPosition(400.f, 100.f);
        centerLine.setFillColor(sf::Color(255, 255, 255, 100));
        window.draw(centerLine);
        
        window.draw(m_targetRect);
        window.draw(m_powerBarBg);
        
        // Draw valid power markers
        sf::RectangleShape markerLow(sf::Vector2f(50.f, 2.f));
        markerLow.setPosition(100.f, 500.f - 0.4f * 200.f); // 40%
        markerLow.setFillColor(sf::Color::Yellow);
        
        sf::RectangleShape markerHigh(sf::Vector2f(50.f, 2.f));
        markerHigh.setPosition(100.f, 500.f - 0.9f * 200.f); // 90%
        markerHigh.setFillColor(sf::Color::Red);
        
        window.draw(markerLow);
        window.draw(markerHigh);
        
        window.draw(m_powerBarFill);
    } 
    else if (p->position == PlayerPosition::Midfielder) {
        for (const auto& t : m_teammates) {
            window.draw(t.shape);
        }
    }
    else if (p->position == PlayerPosition::Defender) {
        window.draw(m_tackleZone);
        window.draw(m_attacker);
    }
    else if (p->position == PlayerPosition::Goalkeeper) {
        window.draw(m_goalRect);
        for (const auto& b : m_balls) {
            window.draw(b.shape);
        }
    }
}

void TrainingScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 40, 20));
    window.draw(m_mainText);
    window.draw(m_infoText);
    
    if (m_state == TrainingState::Intro || m_state == TrainingState::Result) {
        window.draw(m_btnRect);
        window.draw(m_btnText);
    } 
    else if (m_state == TrainingState::Playing) {
        drawGame(window);
    }
}
