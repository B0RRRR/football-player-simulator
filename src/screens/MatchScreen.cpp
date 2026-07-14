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
            int n = static_cast<int>(lg->clubs.size());
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
    if (m_engine->isUserSubbedOff()) {
        m_statusText.setString(m_engine->getUserStartReason());
        m_statusText.setFillColor(sf::Color(255, 100, 100));
    } else {
        m_statusText.setString("Status: On the pitch (Starter)");
    }
    
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
    
    // Setup views
    m_uiView = sf::View(sf::FloatRect(0, 0, 1280.f, 720.f));
    m_camera = m_uiView;
    m_currentZoom = 1.0f;
    
    m_btnSkipRect.setSize(sf::Vector2f(120.f, 40.f));
    m_btnSkipRect.setPosition(1100.f, 650.f);
    m_btnSkipRect.setFillColor(sf::Color(150, 50, 50));
    m_btnSkipText.setFont(font); m_btnSkipText.setCharacterSize(18); m_btnSkipText.setFillColor(sf::Color::White);
    m_btnSkipText.setString("Skip Match");
    m_btnSkipText.setPosition(1115.f, 660.f);

    resetToKickoff();
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
                        // 50% win rate for auto-sim, with randomized power/accuracy
                        MinigameResult autoResult;
                        autoResult.success = rand() % 2 == 0;
                        autoResult.kind = MinigameActionKind::Shot;
                        autoResult.power = (rand() % 100) / 100.f;
                        autoResult.accuracy = (rand() % 100) / 100.f;
                        m_engine->processMinigameResult(autoResult);
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
        // Use physical scancodes rather than localized Key codes: sf::Keyboard::W etc.
        // are remapped by the active keyboard layout (e.g. a Cyrillic layout), so on a
        // non-Latin layout WASD would silently stop matching while the arrow keys (which
        // are layout-independent) kept working.
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.scancode == sf::Keyboard::Scan::W || event.key.scancode == sf::Keyboard::Scan::Up) m_keyUp = true;
            if (event.key.scancode == sf::Keyboard::Scan::S || event.key.scancode == sf::Keyboard::Scan::Down) m_keyDown = true;
            if (event.key.scancode == sf::Keyboard::Scan::A || event.key.scancode == sf::Keyboard::Scan::Left) m_keyLeft = true;
            if (event.key.scancode == sf::Keyboard::Scan::D || event.key.scancode == sf::Keyboard::Scan::Right) m_keyRight = true;

            Player* p = m_gameManager->getPlayer();
            sf::Vector2f myPos = m_dots[m_userIdx].shape.getPosition();
            sf::Vector2f bPos = m_visualBall.getPosition();
            float distToBall = std::hypot(bPos.x - myPos.x, bPos.y - myPos.y);

            bool isSpace = (event.key.scancode == sf::Keyboard::Scan::Space);
            bool isShift = (event.key.scancode == sf::Keyboard::Scan::LShift || event.key.scancode == sf::Keyboard::Scan::RShift);
            bool isE = (event.key.scancode == sf::Keyboard::Scan::E);
            bool isQ = (event.key.scancode == sf::Keyboard::Scan::Q);
            bool ctrlHeld = event.key.control;

            // What Space does depends on whether we're ON the ball, not on our nominal
            // position: a defender who has just won it can shoot and pass like anyone else.
            bool hasBall = (m_pendingKind == MinigameActionKind::Shot || m_pendingKind == MinigameActionKind::Pass);

            if (m_qte.isActive()) {
                // A QTE is running: Space locks the marker. Everything else is ignored,
                // the action is already committed.
                if (isSpace) resolveQTE(m_qte.lock());
            } else if (isSpace) {
                if (!hasBall && p->position != PlayerPosition::Goalkeeper && distToBall < 25.f) {
                    // We're defending: go and win it back.
                    startQTE(MinigameActionKind::Tackle, ActionVariant::Slide, false);
                } else if (hasBall && distToBall < 20.f) {
                    bool finesse = ctrlHeld;
                    startQTE(MinigameActionKind::Shot,
                             finesse ? ActionVariant::Finesse : ActionVariant::Power,
                             finesse);
                }
            } else if (hasBall && distToBall < 20.f && (isE || isQ)) {
                // E = ground pass, Q = lofted/through ball (riskier, tighter window)
                startQTE(MinigameActionKind::Pass,
                         isQ ? ActionVariant::Lofted : ActionVariant::Default,
                         isQ);
            } else if (isShift && m_dashTimer <= 0.f) {
                // Shift = dash. The goalkeeper's save is now a reaction QTE, so the dash
                // is purely for repositioning before the shot comes in.
                if (p->position == PlayerPosition::Goalkeeper) {
                    m_dashTimer = 0.4f;
                    m_dashSpeedBonus = p->goalkeeping * 1.5f;
                } else {
                    m_dashTimer = 0.5f;
                    m_dashSpeedBonus = 0.f;
                }
            }
        } else if (event.type == sf::Event::KeyReleased) {
            if (event.key.scancode == sf::Keyboard::Scan::W || event.key.scancode == sf::Keyboard::Scan::Up) m_keyUp = false;
            if (event.key.scancode == sf::Keyboard::Scan::S || event.key.scancode == sf::Keyboard::Scan::Down) m_keyDown = false;
            if (event.key.scancode == sf::Keyboard::Scan::A || event.key.scancode == sf::Keyboard::Scan::Left) m_keyLeft = false;
            if (event.key.scancode == sf::Keyboard::Scan::D || event.key.scancode == sf::Keyboard::Scan::Right) m_keyRight = false;
        }
    }
}

int MatchScreen::statForAction(MinigameActionKind kind) const {
    Player* p = m_gameManager->getPlayer();
    switch (kind) {
        case MinigameActionKind::Shot:   return p->shooting;
        case MinigameActionKind::Pass:   return p->passing;
        case MinigameActionKind::Tackle: return p->tackling;
        case MinigameActionKind::Save:   return p->goalkeeping;
        default:                         return p->passing;
    }
}

void MatchScreen::startQTE(MinigameActionKind kind, ActionVariant variant, bool hardMode, float sweeps) {
    m_qteKind = kind;
    m_qteVariant = variant;
    m_qte.start(statForAction(kind), hardMode, sweeps);
}




void MatchScreen::endMinigame() {
    m_minigameActive = false;
    m_qte.cancel();
    m_ballFriction = 1.5f;
    m_currentZoom = 1.0f;
    m_camera = m_uiView;
}

void MatchScreen::resolveQTE(const QTEResult& result) {
    switch (m_qteKind) {
        case MinigameActionKind::Tackle: resolveTackleQTE(result); break;
        case MinigameActionKind::Save:   resolveSaveQTE(result); break;
        case MinigameActionKind::Pass:   resolvePassQTE(result); break;
        default:                         resolveShotQTE(result); break;
    }
}


void MatchScreen::update(sf::Time deltaTime) {
    if (!m_engine) return;
    
    if (m_engine->isUserSubbedOff()) {
        m_statusText.setString(m_engine->getUserStartReason());
        m_statusText.setFillColor(sf::Color(255, 100, 100));
    }

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
        if (m_pendingEvent.outcome == EventOutcome::TackleWon || m_pendingEvent.outcome == EventOutcome::Intercepted) {
            m_pendingEvent.isHome = originalIsHome; 
        }
        
        // A foul kills the play, whatever produced it. A mistimed slide tackle emits a
        // YellowCard from inside the episode, and that event used to land in the routing
        // below - which sent us to Beat::Shot and cheerfully animated the attacker
        // *shooting* after the whistle had gone. Restart from the spot instead.
        if (m_pendingEvent.type == EventType::Card || m_pendingEvent.type == EventType::Foul) {
            m_engine->commitEvent(m_pendingEvent);
            m_visibleLogs.push_back(m_pendingEvent);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
            setupFreeKick(m_pendingEvent.isHome);
            updateVisuals(deltaTime);
            return;
        }

        // A pass has already been delivered by resolvePassQTE - the ball is travelling to
        // the receiver's feet. Falling through to the routing below sent us to Beat::Shot,
        // which then animated a *shot*: it grabbed the ball and drove it at the goal (or,
        // on a miss, off the pitch entirely). That is why a perfectly timed pass kept
        // ending up in touch.
        if (m_pendingEvent.outcome == EventOutcome::PassGood || m_pendingEvent.outcome == EventOutcome::PassBad) {
            m_visualState = VisualState::NormalPlay;
            m_stateTimer = 0.f;
            updateVisuals(deltaTime);
            return;
        }

        Player* p = m_gameManager->getPlayer();
        bool isTackleMinigame = p && p->position != PlayerPosition::Goalkeeper && (originalIsHome != m_engine->isHome());

        if (m_attackPhase == Beat::MidPassResolve) {
            // Keep phase 41, it handles its own resolution
        } else if (m_attackPhase == Beat::MidTackleResolve) {
            // Keep phase 51, it handles its own resolution
        } else if (m_attackPhase == Beat::SoloRunResolve) {
            // Keep phase 61, it handles its own resolution
        } else if (isTackleMinigame) {
            m_attackPhase = Beat::DefTackleResolve; // Go to tackle resolution
        } else {
            m_attackPhase = Beat::Shot; // Go to shot resolution
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
                
                if (m_attackPhase == Beat::MidPassHold) m_attackPhase = Beat::MidPassResolve;
                else if (m_attackPhase == Beat::MidTackleChase) m_attackPhase = Beat::MidTackleResolve;
                else if (m_attackPhase == Beat::SoloRun) m_attackPhase = Beat::SoloRunResolve;
                else m_attackPhase = Beat::Resolve;
                
                m_stateTimer = 0.f;
            } 
            else if (m_pendingEvent.type == EventType::Goal || m_pendingEvent.type == EventType::Chance || m_pendingEvent.type == EventType::PendingMinigame) {
                m_visualState = VisualState::Attacking;
                m_attackPhase = Beat::Setup;
                m_stateTimer = 0.f;
                m_attackType = rand() % 3; // Choose 0 (Wing), 1 (Solo), 2 (Center)
                int attackerBase = m_pendingEvent.isHome ? 0 : 11;
                m_attackWingerIdx = attackerBase + ((rand()%2==0)?5:8);
                int options[] = {9, 10};
                m_attackFwdIdx = attackerBase + options[rand() % 2];
                
                Player* p = m_gameManager->getPlayer();
                if (p && p->position == PlayerPosition::Forward && m_pendingEvent.isHome == m_engine->isHome()) {
                    m_attackFwdIdx = attackerBase + 10;
                    if (m_pendingEvent.type == EventType::PendingMinigame && m_attackType == 1) {
                        m_attackType = (rand() % 2 == 0) ? 0 : 2;
                    }
                }
                
                m_shotTargetY = 290.f + (rand()%60 - 30.f);
            } else {
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());

                if (m_pendingEvent.type == EventType::Card || m_pendingEvent.type == EventType::Foul) {
                    // Play is dead. The ball used to just keep whatever velocity it had
                    // when the foul was given and sail off across the pitch; instead it
                    // sits at the spot and the fouled side restarts it.
                    setupFreeKick(m_pendingEvent.isHome);
                } else {
                    m_visualState = VisualState::NormalPlay;
                    m_stateTimer = 0.f;
                }
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

MinigameResult MatchScreen::buildMinigameResult(bool success, MinigameActionKind kind, ActionVariant variant) const {
    MinigameResult result;
    result.success = success;
    result.kind = kind;
    result.variant = variant;

    float speed = std::hypot(m_ballVelocity.x, m_ballVelocity.y);
    result.power = std::clamp(speed / 900.f, 0.f, 1.f);

    // Accuracy now comes from how well the player timed the QTE, not from where the
    // ball happened to end up geometrically.
    result.accuracy = std::clamp(m_qteAccuracy, 0.f, 1.f);

    return result;
}

void MatchScreen::resolveShotQTE(const QTEResult& qte) {
    // The QTE decides where the ball is aimed and how hard it's struck; the physics
    // below then plays it out and the existing goal-line detection resolves the
    // episode. So a Perfect strike is aimed inside the frame, a Miss is sprayed wide.
    bool attackingRight = m_engine->isHome();
    float goalX = attackingRight ? 860.f : 20.f;

    // Aim error grows as the timing gets worse. The goal spans roughly y 250..330.
    float aimError = 0.f;
    float power = 0.f;
    bool onTarget = true;

    // The goal mouth is y 250..330, i.e. +/-40 either side of 290. The aim is offset from
    // centre by (18 + aimError), so anything that pushes the total past 40 misses.
    switch (qte.grade) {
        case QTEGrade::Perfect: aimError = 0.f;  power = 520.f; break;
        case QTEGrade::Good:    aimError = 12.f; power = 470.f; break;
        case QTEGrade::Poor:    aimError = 34.f; power = 400.f; break; // clips the frame
        case QTEGrade::Miss:
        default:                aimError = 95.f; power = 330.f; onTarget = false; break;
    }

    if (m_qteVariant == ActionVariant::Finesse) {
        // Placement over power: tighter aim, less pace.
        aimError *= 0.6f;
        power *= 0.8f;
    }

    sf::Vector2f origin = m_visualBall.getPosition();
    float distToGoal = std::hypot(goalX - origin.x, 290.f - origin.y);

    // Range matters. Timing the bar perfectly means you struck the ball cleanly - it does
    // not mean you can pick out the top corner from your own half. Distance scatters the
    // shot, so a long-range effort is a genuine gamble even off a Perfect: the further out
    // you are, the more likely the scatter pushes it past the post.
    float scatter = (distToGoal / 26.f) * ((rand() % 100) / 100.f);
    aimError += scatter;

    // Aim for a corner on a good strike; error pushes it away from the frame.
    float side = (rand() % 2 == 0) ? -1.f : 1.f;
    float targetY = 290.f + side * (18.f + aimError);
    m_shotTargetY = targetY;

    sf::Vector2f dir(goalX - origin.x, targetY - origin.y);
    float len = std::hypot(dir.x, dir.y);
    if (len > 0.f) { dir.x /= len; dir.y /= len; }

    // A struck shot keeps its pace instead of decaying like a loose ball. Weight it by
    // range too - a shot from distance has to be hit properly, or (with the drag below)
    // it arrives as a gentle roll the keeper picks up, which is exactly how a strike from
    // the halfway line used to trickle in.
    m_ballFriction = 0.35f;
    power += distToGoal * 0.6f;
    float minPower = len * m_ballFriction * 1.4f;
    if (onTarget && power < minPower) power = minPower;

    m_ballVelocity = dir * power;
    m_ballStruck = true; // released from the dribble - physics takes over from here
    m_pendingKind = MinigameActionKind::Shot;
    m_pendingVariant = m_qteVariant;
    m_qteAccuracy = qte.quality;
}

int MatchScreen::pickPassTarget() const {
    // Prefer a team-mate in the direction the player is facing, and among those the
    // nearer one. Falls back to the closest team-mate if nobody is ahead.
    int ownBase = m_engine->isHome() ? 0 : 11;
    sf::Vector2f myPos = m_dots[m_userIdx].shape.getPosition();

    int best = -1;
    float bestScore = -1e9f;

    for (int i = ownBase; i < ownBase + 11; ++i) {
        if (i == m_userIdx) continue;
        if (i % 11 == 0) continue;         // don't pass back to our own keeper
        if (hasRedCard(i)) continue;

        sf::Vector2f toMate = m_dots[i].shape.getPosition() - myPos;
        float dist = std::hypot(toMate.x, toMate.y);
        if (dist < 1.f) continue;

        sf::Vector2f unit(toMate.x / dist, toMate.y / dist);
        float facing = unit.x * m_userMoveDir.x + unit.y * m_userMoveDir.y; // -1..1

        // Facing dominates; distance is a mild tie-breaker.
        float score = facing * 100.f - dist * 0.25f;
        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

void MatchScreen::resolvePassQTE(const QTEResult& qte) {
    // A pass resolves straight away rather than waiting on the goal line: the engine
    // turns a good pass into a chance (simulateAIEvent) and a bad one into a turnover.
    bool success = (qte.grade == QTEGrade::Perfect || qte.grade == QTEGrade::Good);
    bool lofted = (m_qteVariant == ActionVariant::Lofted);

    // Aim at an actual team-mate. (This used to fire the ball along m_userMoveDir - the
    // last WASD direction, defaulting to "right" if the player hadn't moved - so the
    // pass sailed off to nobody.)
    sf::Vector2f ballPos = m_visualBall.getPosition();
    int mate = pickPassTarget();

    sf::Vector2f dir;
    float dist;
    if (mate >= 0) {
        sf::Vector2f toMate = m_dots[mate].shape.getPosition() - ballPos;
        dist = std::hypot(toMate.x, toMate.y);
        dir = (dist > 0.f) ? sf::Vector2f(toMate.x / dist, toMate.y / dist) : m_userMoveDir;
    } else {
        dir = m_userMoveDir;
        dist = 200.f;
    }

    // A mistimed pass sprays off-line and gets cut out.
    if (!success) {
        float stray = ((rand() % 100) - 50) / 50.f * 0.45f;
        float c = std::cos(stray), s = std::sin(stray);
        dir = sf::Vector2f(dir.x * c - dir.y * s, dir.x * s + dir.y * c);
    }

    // Deliver the ball TO the team-mate rather than firing it off with physics. Weighting
    // a pass by hand never worked: with friction the ball travels ~v/1.5, so the old
    // `speed = dist * 1.5 * 1.35` overshot the receiver by 35% every time (and the 400
    // floor overshot a short pass several times over) - a perfectly timed pass would sail
    // straight past its target and out of play, while the log cheerfully called it
    // inch-perfect. A pass ends at a team-mate's feet, so aim it there and let the normal
    // ball-travel logic carry it.
    sf::Vector2f landing = (mate >= 0) ? m_dots[mate].shape.getPosition()
                                       : ballPos + dir * dist;
    if (!success) {
        // A mistimed pass falls short and off-line, where it can be cut out.
        landing = ballPos + dir * (dist * 0.7f);
    }
    landing.x = std::clamp(landing.x, 50.f, 830.f);
    landing.y = std::clamp(landing.y, 140.f, 440.f);

    m_ballTarget = landing;
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballStruck = true; // no longer at our feet
    if (lofted) m_ballLoftTimer = 0.6f;

    // Hand the ball to whoever ends up with it. Making him the carrier means the normal
    // ball-travel logic walks the ball to his feet and keeps it there - it physically
    // cannot end up in touch, which is what kept happening.
    if (success && mate >= 0) {
        m_ballCarrierIdx = mate;
    } else {
        // Cut out: the nearest opponent picks it off.
        int oppBase = m_engine->isHome() ? 11 : 0;
        int thief = -1;
        float best = 1e9f;
        for (int i = oppBase; i < oppBase + 11; ++i) {
            if (hasRedCard(i)) continue;
            sf::Vector2f d = m_dots[i].shape.getPosition() - landing;
            float dd = std::hypot(d.x, d.y);
            if (dd < best) { best = dd; thief = i; }
        }
        m_ballCarrierIdx = thief;
    }

    m_qteAccuracy = qte.quality;
    m_pendingKind = MinigameActionKind::Pass;
    m_pendingVariant = m_qteVariant;

    MinigameResult result = buildMinigameResult(success, MinigameActionKind::Pass, m_qteVariant);
    m_engine->processMinigameResult(result);
    endMinigame();
}

void MatchScreen::resolveTackleQTE(const QTEResult& qte) {
    // Replaces the old blind `rand() % 100 < chance` roll: the timing decides it.
    bool success = (qte.grade == QTEGrade::Perfect || qte.grade == QTEGrade::Good);

    m_qteAccuracy = qte.quality;

    if (success) {
        // We won it - so we KEEP it. The episode carries on with the ball at our feet and
        // we choose what to do with it (dribble, pass, shoot). Previously a successful
        // tackle just hoofed the ball clear and ended the episode, which meant winning the
        // duel gave you nothing to play with.
        //
        // Deliberately do NOT report to the engine yet: processMinigameResult() flips the
        // match back to Simulating and would tear the minigame down. The follow-up action
        // (the shot or pass) is what reports the outcome of this episode.
        m_pendingKind = MinigameActionKind::Shot;   // we're the attacker now
        m_pendingVariant = ActionVariant::Default;
        m_ballStruck = false;                        // back to dribbling
        m_ballVelocity = sf::Vector2f(0.f, 0.f);
        m_ballFriction = 1.5f;
        m_visualBall.setPosition(m_dots[m_userIdx].shape.getPosition() + m_userMoveDir * 12.f);

        // Fresh grace period, or the opponents (who are right on top of us) would rob us
        // back on the very next frame.
        m_minigameTimer = 0.f;
        m_tackleAttemptTimer = 0.f;
        return;
    }

    MinigameResult result = buildMinigameResult(false, MinigameActionKind::Tackle, ActionVariant::Slide);
    m_engine->processMinigameResult(result);
    endMinigame();
}

void MatchScreen::resolveSaveQTE(const QTEResult& qte) {
    // Time it right and the keeper parries; miss it and the shot goes in.
    bool saved = (qte.grade == QTEGrade::Perfect || qte.grade == QTEGrade::Good);

    m_qteAccuracy = qte.quality;

    if (saved) {
        // Parry the ball away from goal, back up the pitch.
        float dirX = m_engine->isHome() ? 1.f : -1.f;
        float lateral = ((rand() % 100) - 50) / 50.f * 200.f;
        m_ballVelocity = sf::Vector2f(dirX * 380.f, lateral);

        MinigameResult result = buildMinigameResult(true, MinigameActionKind::Save, ActionVariant::Dive);
        m_engine->processMinigameResult(result);
        endMinigame();
    } else {
        // Beaten - let the shot run on into the net.
        MinigameResult result = buildMinigameResult(false, MinigameActionKind::Save, ActionVariant::Dive);
        m_engine->processMinigameResult(result);
        endMinigame();
    }
}

void MatchScreen::initMinigame() {
    m_minigameTimer = 0.f;
    // Disconnect ball carrier so the ball becomes a physics object
    m_ballCarrierIdx = -1;
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballFriction = 1.5f; // default rolling drag; the GK shot below lowers it

    m_dashTimer = 0.f;
    m_dashSpeedBonus = 0.f;
    m_userMoveDir = sf::Vector2f(1.f, 0.f);
    m_pendingVariant = ActionVariant::Default;
    m_pendingKind = MinigameActionKind::Shot;
    m_ballLoftTimer = 0.f;
    m_visualBall.setScale(1.f, 1.f);
    m_qte.cancel();
    m_qteAccuracy = 0.f;
    m_tackleAttemptTimer = 0.f;
    m_ballStruck = false;

    m_keyUp = false;
    m_keyDown = false;
    m_keyLeft = false;
    m_keyRight = false;

    Player* p = m_gameManager->getPlayer();
    int userPosIdx = 0;
    if (p->position == PlayerPosition::Defender) userPosIdx = 3;
    else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
    else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
    m_userIdx = m_engine->isHome() ? userPosIdx : 11 + userPosIdx;

    // What kind of episode is this? Everything downstream keys off m_pendingKind: whether
    // we're dribbling, whether opponents may rob us, what Space does. It used to be hard
    // -wired to Shot, which meant a midfielder DEFENDING was treated as being on the ball
    // - the ball got glued to his feet while he was supposed to be winning it back.
    bool attacking = (m_pendingEvent.isHome == m_engine->isHome());
    if (p->position == PlayerPosition::Goalkeeper) {
        m_pendingKind = MinigameActionKind::Save;
    } else {
        m_pendingKind = attacking ? MinigameActionKind::Shot : MinigameActionKind::Tackle;
    }

    // An attacking player starts the episode ON the ball. The scripts leave it wherever
    // the build-up ended (mid-cross, at a team-mate's feet), which could be nowhere near
    // us - so put it at our feet instead of making us chase a loose ball.
    if (attacking && p->position != PlayerPosition::Goalkeeper) {
        m_visualBall.setPosition(m_dots[m_userIdx].shape.getPosition() + m_userMoveDir * 12.f);
        m_ballVelocity = sf::Vector2f(0.f, 0.f);
    }

    if (p->position == PlayerPosition::Goalkeeper && m_attackPhase == Beat::GkShotWindup) {
        // The GK doesn't get to gather a stray ball - a shot is already coming in.
        // Strike it from the edge of the box, on a clean straight line to goal.
        // (It used to spawn back at x=400, near halfway, so it ricocheted through the
        // whole crowd of frozen dots before ever reaching the keeper. Collisions with
        // everyone except the keeper are also suppressed for this flight - see
        // updateMinigame - so the shot arrives instead of pinballing.)
        bool userIsHome = m_engine->isHome();
        float goalX = userIsHome ? 35.f : 845.f;
        float originX = userIsHome ? goalX + 225.f : goalX - 225.f;

        sf::Vector2f origin(originX, m_shotTargetY);
        sf::Vector2f goalPoint(goalX, m_shotTargetY);
        m_visualBall.setPosition(origin);

        sf::Vector2f dir = goalPoint - origin;
        float len = std::hypot(dir.x, dir.y);
        if (len > 0.f) { dir.x /= len; dir.y /= len; }

        // A struck shot shouldn't decay like a loose ball, or it crawls to a halt short
        // of the line. With this drag and speed the ball covers the 225px in ~1.6s,
        // which comfortably outlasts the save QTE below (~0.9-1.3s depending on the
        // keeper's stat), so the bar always resolves before the ball arrives.
        m_ballFriction = 0.25f;
        m_ballVelocity = dir * 170.f;

        // The save is a reaction test: the bar arms itself the moment the shot is
        // struck. Lock it in the zone to parry, miss it (or let it expire) and it's a
        // goal. A single pass of the marker - the keeper gets one chance, like in life.
        startQTE(MinigameActionKind::Save, ActionVariant::Dive, false, 1.0f);
    }
}

void MatchScreen::updateMinigame(sf::Time deltaTime) {
    float dt = deltaTime.asSeconds();
    m_minigameTimer += dt;
    
    // Zoom camera towards the ball
    sf::Vector2f ballPos = m_visualBall.getPosition();
    
    // Target zoom is 0.5 (2x zoom)
    float targetZoom = 0.5f;
    m_currentZoom += (targetZoom - m_currentZoom) * 2.0f * dt;
    
    // Calculate target center, clamped to pitch boundaries so we don't show off-pitch area
    sf::Vector2f targetCenter = ballPos;
    float viewWidth = 1280.f * m_currentZoom;
    float viewHeight = 720.f * m_currentZoom;
    
    // Pitch bounds: X [40, 840], Y [130, 450]
    // To allow some margin, let's clamp center:
    float minX = 40.f + viewWidth / 2.f;
    float maxX = 840.f - viewWidth / 2.f;
    float minY = 130.f + viewHeight / 2.f;
    float maxY = 450.f - viewHeight / 2.f;
    
    if (minX > maxX) targetCenter.x = 440.f; // If view is wider than pitch, just center
    else targetCenter.x = std::clamp(targetCenter.x, minX, maxX);
    
    if (minY > maxY) targetCenter.y = 290.f;
    else targetCenter.y = std::clamp(targetCenter.y, minY, maxY);
    
    // Track the ball tightly. An exponential follow trails a moving ball by roughly
    // (ball speed / follow rate) pixels: at the old rate of 3 a struck shot outran the
    // camera by ~290px and left the zoomed view entirely. 8 keeps it comfortably framed.
    sf::Vector2f currentCenter = m_camera.getCenter();
    currentCenter += (targetCenter - currentCenter) * 8.0f * dt;

    m_camera.setCenter(currentCenter);
    m_camera.setSize(1280.f * m_currentZoom, 720.f * m_currentZoom);

    // Bring the other 21 players to life. updateMinigameAI may end the episode outright
    // (an opponent robbing us of the ball), so bail out before touching stale state.
    updateMinigameAI(dt);
    if (!m_minigameActive) return;
    updateDotMotion(dt);

    // Dribbling: until the player actually strikes the ball it stays at his feet. It
    // used to lie on the pitch as a loose physics object, so simply running into it
    // booted it away - after which it drifted off on its own and the player was left
    // chasing a ball bouncing around out of reach.
    // Purely kind-based: a defender who has just won the ball is on it just as much as a
    // forward is, and a midfielder who is defending is not.
    bool userHasBall = (m_pendingKind == MinigameActionKind::Shot || m_pendingKind == MinigameActionKind::Pass);
    bool dribbling = userHasBall && !m_ballStruck;

    if (dribbling) {
        m_visualBall.setPosition(m_dots[m_userIdx].shape.getPosition() + m_userMoveDir * 12.f);
        m_ballVelocity = sf::Vector2f(0.f, 0.f);
    } else {
        // Ball Physics
        m_visualBall.move(m_ballVelocity * dt);
        m_ballVelocity -= m_ballVelocity * m_ballFriction * dt; // Friction
    }

    // The user's own current velocity, used below to transfer momentum into the ball on contact.
    // Non-user dots are frozen while the minigame is active, so only the user's dot ever moves.
    sf::Vector2f userVelocity(0.f, 0.f);
    {
        sf::Vector2f dir(0.f, 0.f);
        if (m_keyUp) dir.y -= 1.f;
        if (m_keyDown) dir.y += 1.f;
        if (m_keyLeft) dir.x -= 1.f;
        if (m_keyRight) dir.x += 1.f;
        float len = std::hypot(dir.x, dir.y);
        bool moving = len > 0.f;
        if (moving) { dir.x /= len; dir.y /= len; } else { dir = m_userMoveDir; }
        if (moving || m_dashTimer > 0.f) {
            float spd = (m_dashTimer > 0.f) ? (400.f + m_dashSpeedBonus) : 150.f;
            userVelocity = dir * spd;
        }
    }

    // Collisions with players
    sf::Vector2f bPos = m_visualBall.getPosition();
    float bRadius = m_visualBall.getRadius();
    Player* userPlayer = m_gameManager->getPlayer();
    // An incoming shot at the keeper should reach the keeper. Everyone else is a frozen
    // dot standing in its path, and bouncing off them turned the shot into a pinball.
    bool saveFlight = (m_qte.isActive() && m_qteKind == MinigameActionKind::Save);

    // NOTE: this guard deliberately keys off `dribbling`, not "the ball is ours". While we
    // still have it at our feet the ball is glued to us and nobody may knock it away (an
    // opponent parked 15px from the carrier used to boot it clear on frame one). But the
    // moment it is STRUCK it becomes a real ball again and everyone can block it - keeping
    // the skip alive past the strike is what let shots sail straight through the keeper.

    for (size_t i = 0; i < m_dots.size(); ++i) {
        auto& d = m_dots[i];
        bool isUserDot = ((int)i == m_userIdx);
        if ((saveFlight || dribbling) && !isUserDot) continue;
        // Dribbling: the ball is glued to our feet, so bouncing it off ourselves would
        // just kick it out from under us every frame.
        if (dribbling && isUserDot) continue;

        sf::Vector2f dPos = d.shape.getPosition();
        float dRadius = d.shape.getRadius();

        float dist = std::hypot(bPos.x - dPos.x, bPos.y - dPos.y);
        if (dist > 0.01f && dist < bRadius + dRadius) {
            sf::Vector2f normal = sf::Vector2f(bPos.x - dPos.x, bPos.y - dPos.y);
            normal.x /= dist; normal.y /= dist;
            m_visualBall.setPosition(dPos.x + normal.x * (bRadius + dRadius), dPos.y + normal.y * (bRadius + dRadius));

            // Defenders make a controlled stop on contact; goalkeepers parry rather than
            // fully rebound. Everyone else keeps the original elastic-ish bounce.
            float restitution = 1.0f;
            float bump = 20.f;
            if (isUserDot && userPlayer->position == PlayerPosition::Defender) {
                restitution = 0.4f;
                bump = 10.f;
            } else if (isUserDot && userPlayer->position == PlayerPosition::Goalkeeper) {
                restitution = 0.6f;
                bump = 12.f;
            }

            float dotProduct = m_ballVelocity.x * normal.x + m_ballVelocity.y * normal.y;
            if (dotProduct < 0) {
                m_ballVelocity.x -= (1.f + restitution) * dotProduct * normal.x;
                m_ballVelocity.y -= (1.f + restitution) * dotProduct * normal.y;
            }
            m_ballVelocity += normal * bump;

            // A sprinting/dashing player redirects the ball harder than a standing one.
            if (isUserDot) {
                m_ballVelocity += userVelocity * 0.5f;
            }
        }
    }
    
    // Player Controls
    sf::Vector2f moveInput(0.f, 0.f);
    if (m_keyUp) moveInput.y -= 1.f;
    if (m_keyDown) moveInput.y += 1.f;
    if (m_keyLeft) moveInput.x -= 1.f;
    if (m_keyRight) moveInput.x += 1.f;
    
    if (m_dashTimer > 0) m_dashTimer -= dt;
    
    if (moveInput.x != 0 || moveInput.y != 0) {
        float len = std::hypot(moveInput.x, moveInput.y);
        m_userMoveDir = sf::Vector2f(moveInput.x / len, moveInput.y / len);
    }
    
    // While a QTE is running the action is already committed - the player is planting
    // their foot to strike/tackle/dive, so they don't get to keep jogging around.
    bool qteRunning = m_qte.isActive();

    // Move if input is pressed OR if we are dashing
    if (!qteRunning && ((moveInput.x != 0 || moveInput.y != 0) || m_dashTimer > 0.f)) {
        float speed = 150.f; // Base speed
        if (m_dashTimer > 0) speed = 400.f + m_dashSpeedBonus; // Dashing speed is faster

        m_dots[m_userIdx].shape.move(m_userMoveDir * speed * dt);
        // keep within pitch
        sf::Vector2f pos = m_dots[m_userIdx].shape.getPosition();
        pos.x = std::clamp(pos.x, 40.f, 840.f);
        pos.y = std::clamp(pos.y, 130.f, 450.f);
        m_dots[m_userIdx].shape.setPosition(pos);
    }

    // Tick the timing bar. Space (handled in handleInput) locks it; running out of
    // time is a miss - never a silent success, which is what the old 15s fallback did.
    if (qteRunning) {
        m_qte.update(dt);
        if (m_qte.isExpired()) {
            QTEResult missed;
            missed.grade = QTEGrade::Miss;
            missed.value = 0.f;
            missed.quality = 0.f;
            resolveQTE(missed);
        }
    }

    if (!m_minigameActive) {
        // An expiring QTE may have already resolved the episode this frame; endMinigame()
        // has restored the camera, so just bail out.
        return;
    }

    // Lofted pass visual: ball briefly "grows" mid-flight to sell the arc
    if (m_ballLoftTimer > 0.f) {
        m_ballLoftTimer -= dt;
        float t = std::clamp(m_ballLoftTimer / 0.6f, 0.f, 1.f);
        float scale = 1.f + std::sin(t * 3.14159f) * 0.6f;
        m_visualBall.setScale(scale, scale);
        if (m_ballLoftTimer <= 0.f) m_visualBall.setScale(1.f, 1.f);
    }

    // While a QTE is armed it is the sole authority on the outcome - the physics below
    // must not short-circuit it. Without this guard the ball (deflected off the frozen
    // dots) could leave the pitch mid-QTE and the out-of-bounds handler would hand the
    // keeper/defender an automatic success for having done nothing at all.
    if (m_qte.isActive()) {
        return;
    }

    // While dribbling, the ball is at our feet by construction. Letting the goal/out
    // checks below run would mean you could simply walk it over the line, or "lose" it
    // out of play by standing near the touchline. Nothing is decided until you strike.
    if (dribbling) {
        return;
    }

    // Goal detection
    if (bPos.y > 250.f && bPos.y < 330.f) {
        if (bPos.x < 50.f || bPos.x > 830.f) {
            bool success = (bPos.x < 50.f) ? !m_engine->isHome() : m_engine->isHome();
            Player* p = m_gameManager->getPlayer();
            MinigameActionKind kind = (p->position == PlayerPosition::Goalkeeper) ? MinigameActionKind::Save
                                     : (p->position == PlayerPosition::Defender) ? MinigameActionKind::Tackle
                                     : m_pendingKind;
            m_engine->processMinigameResult(buildMinigameResult(success, kind, m_pendingVariant));
            endMinigame();
        }
    }

    // Out of bounds detection
    if (bPos.x < 40.f || bPos.x > 840.f || bPos.y < 130.f || bPos.y > 450.f) {
        if (m_minigameActive) {
            Player* p = m_gameManager->getPlayer();
            if (p->position == PlayerPosition::Defender || p->position == PlayerPosition::Goalkeeper) {
                MinigameActionKind kind = (p->position == PlayerPosition::Goalkeeper) ? MinigameActionKind::Save : MinigameActionKind::Tackle;
                m_engine->processMinigameResult(buildMinigameResult(true, kind, m_pendingVariant)); // Success for defender/GK (cleared the ball)
            } else {
                m_engine->processMinigameResult(buildMinigameResult(false, m_pendingKind, m_pendingVariant)); // Fail for attacker
            }
            endMinigame();
        }
    }

    // Fallback: the player never committed to an action. This used to hand defenders
    // and keepers a free *success* ("survived the attack") for standing still, which
    // meant doing nothing was a winning strategy. Standing still is now a failure for
    // everyone - the outcome should come from the QTE, not from waiting it out.
    if (m_minigameTimer > 12.0f && m_minigameActive) {
        Player* p = m_gameManager->getPlayer();
        MinigameActionKind kind = (p->position == PlayerPosition::Goalkeeper) ? MinigameActionKind::Save
                                 : (p->position == PlayerPosition::Defender) ? MinigameActionKind::Tackle
                                 : m_pendingKind;
        m_qte.cancel();
        m_qteAccuracy = 0.f;
        m_engine->processMinigameResult(buildMinigameResult(false, kind, m_pendingVariant));
        endMinigame();
    }
}

void MatchScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    
    window.setView(m_camera);
    
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
    
    if (m_minigameActive) {
        sf::Vector2f userPos = m_dots[m_userIdx].shape.getPosition();

        // Aiming line. Power is no longer charged by holding a key - the QTE bar
        // (drawn in the UI view below) decides both power and placement.
        sf::Vertex line[] = {
            sf::Vertex(userPos, sf::Color(255, 255, 255, 100)),
            sf::Vertex(userPos + m_userMoveDir * 20.f, sf::Color(255, 255, 255, 0))
        };
        window.draw(line, 2, sf::Lines);
    }

    window.setView(m_uiView);
    
    window.draw(m_homeLogo); window.draw(m_awayLogo);
    window.draw(m_homeName); window.draw(m_awayName);
    window.draw(m_scoreText); window.draw(m_timeText);
    
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
    } else {
        // Minigame overlay: the QTE bar plus the controls for this role.
        sf::Text hint;
        hint.setFont(font);
        hint.setCharacterSize(15);
        hint.setFillColor(UITheme::TextDim);

        // The controls follow the ball, not the shirt number: once a defender wins it back
        // he gets the full attacking set.
        bool hasBall = (m_pendingKind == MinigameActionKind::Shot || m_pendingKind == MinigameActionKind::Pass);
        std::string controls;
        if (p->position == PlayerPosition::Goalkeeper) {
            controls = "SHIFT - rush off the line   |   SPACE - time the save";
        } else if (hasBall) {
            controls = "WASD - move   |   SPACE - shoot (CTRL: finesse)   |   E - pass   |   Q - through ball   |   SHIFT - sprint";
        } else {
            controls = "WASD - close down   |   SHIFT - sprint   |   SPACE - tackle";
        }

        if (m_qte.isActive()) {
            std::string label;
            switch (m_qteKind) {
                case MinigameActionKind::Save:   label = "SAVE IT!  -  press SPACE in the green"; break;
                case MinigameActionKind::Tackle: label = "TACKLE  -  press SPACE in the green"; break;
                case MinigameActionKind::Pass:   label = "PASS  -  press SPACE in the green"; break;
                default:                         label = "SHOOT  -  press SPACE in the green"; break;
            }
            hint.setString(label);
            hint.setCharacterSize(18);
            hint.setFillColor(UITheme::Highlight);
        } else {
            hint.setString(controls);
        }

        sf::FloatRect hb = hint.getLocalBounds();
        hint.setOrigin(hb.left + hb.width / 2.f, hb.top + hb.height / 2.f);
        hint.setPosition(440.f, 620.f);
        window.draw(hint);

        m_qte.draw(window, sf::Vector2f(440.f, 665.f));
    }
}
