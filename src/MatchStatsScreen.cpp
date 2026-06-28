#include "MatchStatsScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "AssetManager.h"
#include <iomanip>
#include <sstream>

MatchStatsScreen::MatchStatsScreen(std::shared_ptr<MatchEngine> engine) 
    : m_engine(engine) {}

void MatchStatsScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::Yellow);
    m_titleText.setPosition(250.f, 30.f);
    m_titleText.setString("Match Stats");
    
    const MatchStats& home = m_engine->isHome() ? m_engine->getPlayerTeamStats() : m_engine->getOpponentTeamStats();
    const MatchStats& away = m_engine->isHome() ? m_engine->getOpponentTeamStats() : m_engine->getPlayerTeamStats();
    
    std::string hName = m_engine->isHome() ? m_engine->getPlayerClub()->name : m_engine->getOpponentClub()->name;
    std::string aName = m_engine->isHome() ? m_engine->getOpponentClub()->name : m_engine->getPlayerClub()->name;
    
    std::stringstream ss;
    ss << std::left << std::setw(20) << hName << " vs " << aName << "\n\n";
    ss << std::left << std::setw(20) << std::to_string(home.goals) << " Goals " << std::to_string(away.goals) << "\n";
    ss << std::left << std::setw(20) << std::to_string(home.shots) << " Shots " << std::to_string(away.shots) << "\n";
    ss << std::left << std::setw(20) << std::to_string(home.yellowCards) << " Yellow Cards " << std::to_string(away.yellowCards) << "\n";
    ss << std::left << std::setw(20) << std::to_string(home.redCards) << " Red Cards " << std::to_string(away.redCards) << "\n";
    
    m_statsText.setFont(font);
    m_statsText.setCharacterSize(24);
    m_statsText.setFillColor(sf::Color::White);
    m_statsText.setPosition(150.f, 120.f);
    m_statsText.setString(ss.str());
    
    std::stringstream rss;
    rss << std::fixed << std::setprecision(1) << m_engine->getPlayerRating();
    
    // Accumulate rating
    Player* p = m_gameManager->getPlayer();
    if (p) {
        p->totalSeasonRating += m_engine->getPlayerRating();
        p->matchesPlayedThisSeason++;
    }
    
    m_ratingText.setFont(font);
    m_ratingText.setCharacterSize(30);
    m_ratingText.setFillColor(sf::Color::Cyan);
    m_ratingText.setPosition(250.f, 350.f);
    m_ratingText.setString("Your Rating: " + rss.str() + " / 10.0");
    
    m_btnContinue.setSize(sf::Vector2f(200.f, 50.f));
    m_btnContinue.setPosition(300.f, 450.f);
    m_btnContinue.setFillColor(sf::Color(100, 100, 100));
    
    m_btnText.setFont(font);
    m_btnText.setString("Continue");
    m_btnText.setCharacterSize(20);
    m_btnText.setFillColor(sf::Color::White);
    
    sf::FloatRect tr = m_btnText.getLocalBounds();
    m_btnText.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
    m_btnText.setPosition(m_btnContinue.getPosition().x + m_btnContinue.getSize().x/2.0f,
                          m_btnContinue.getPosition().y + m_btnContinue.getSize().y/2.0f);
                          
    // Update league points for both clubs
    Club* hc = m_engine->isHome() ? m_engine->getPlayerClub() : m_engine->getOpponentClub();
    Club* ac = m_engine->isHome() ? m_engine->getOpponentClub() : m_engine->getPlayerClub();
    
    if (m_gameManager->getCareerManager()->hasInternationalMatchToday()) {
        auto updateTournament = [&](Tournament& t) {
            if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
            for (auto& m : t.rounds[t.currentRoundIndex].matches) {
                if ((m.home == hc && m.away == ac) || (m.home == ac && m.away == hc)) {
                    bool isHomeLeg = (m.home == hc);
                    if (!m.leg1Played) {
                        m.homeGoalsLeg1 = isHomeLeg ? home.goals : away.goals;
                        m.awayGoalsLeg1 = isHomeLeg ? away.goals : home.goals;
                        m.leg1Played = true;
                        
                        m.winner = (home.goals > away.goals) ? hc : ((away.goals > home.goals) ? ac : nullptr);
                        if (!m.winner) {
                            // random pens if user drew
                            m.homePenalties = 4 + rand()%2;
                            m.awayPenalties = 3 + rand()%3;
                            if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                            m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                            m_statsText.setString(m_statsText.getString() + "\n\nTied! " + m.winner->name + " won on penalties.");
                        }
                    }
                }
            }
        };
        updateTournament(m_gameManager->getDatabase().getWorldCup());
        updateTournament(m_gameManager->getDatabase().getEuroCup());
    } else if (m_gameManager->getCareerManager()->hasEuropeanMatchToday()) {
        auto updateTournament = [&](Tournament& t) {
            if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
            for (auto& m : t.rounds[t.currentRoundIndex].matches) {
                if ((m.home == hc && m.away == ac) || (m.home == ac && m.away == hc)) {
                    // Match found!
                    bool isHomeLeg = (m.home == hc); // the original draw order
                    if (!m.leg1Played) {
                        m.homeGoalsLeg1 = isHomeLeg ? home.goals : away.goals;
                        m.awayGoalsLeg1 = isHomeLeg ? away.goals : home.goals;
                        m.leg1Played = true;
                        
                        if (m.isFinal) {
                            m.winner = (home.goals > away.goals) ? hc : ((away.goals > home.goals) ? ac : nullptr);
                            if (!m.winner) m.winner = (rand()%2==0) ? hc : ac; // random pens if user drew final
                        }
                    } else if (!m.leg2Played && !m.isFinal) {
                        m.homeGoalsLeg2 = isHomeLeg ? home.goals : away.goals;
                        m.awayGoalsLeg2 = isHomeLeg ? away.goals : home.goals;
                        m.leg2Played = true;
                        
                        int aggHome = m.homeGoalsLeg1 + m.homeGoalsLeg2;
                        int aggAway = m.awayGoalsLeg1 + m.awayGoalsLeg2;
                        
                        if (aggHome > aggAway) m.winner = m.home;
                        else if (aggAway > aggHome) m.winner = m.away;
                        else {
                            // Penalties simulated!
                            m.homePenalties = 4 + rand()%2;
                            m.awayPenalties = 3 + rand()%3;
                            if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                            m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                            
                            // Let the player know about the shootout
                            m_statsText.setString(m_statsText.getString() + "\n\nAggregate tied! " + m.winner->name + " won on penalties.");
                        }
                    }
                }
            }
        };
        updateTournament(m_gameManager->getDatabase().getChampionsLeague());
        updateTournament(m_gameManager->getDatabase().getEuropaLeague());
    } else {
        hc->goalsFor += home.goals;
        hc->goalsAgainst += away.goals;
        ac->goalsFor += away.goals;
        ac->goalsAgainst += home.goals;
        
        if (home.goals > away.goals) {
            hc->points += 3; hc->wins++;
            ac->losses++;
        } else if (away.goals > home.goals) {
            ac->points += 3; ac->wins++;
            hc->losses++;
        } else {
            hc->points += 1; hc->draws++;
            ac->points += 1; ac->draws++;
        }
    }
}

void MatchStatsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        if (m_btnContinue.getGlobalBounds().contains(mousePos)) {
            m_gameManager->getCareerManager()->advanceDay();
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        }
    }
}

void MatchStatsScreen::update(sf::Time deltaTime) {
}

void MatchStatsScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 30));
    window.draw(m_titleText);
    window.draw(m_statsText);
    window.draw(m_ratingText);
    window.draw(m_btnContinue);
    window.draw(m_btnText);
}
