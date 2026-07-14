#include "AwardsScreen.h"
#include "SeasonEndScreen.h"
#include "GameManager.h"
#include "Player.h"
#include "Database.h"
#include "AssetManager.h"
#include "UITheme.h"
#include "CareerManager.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>

AwardsScreen::AwardsScreen() {}

void AwardsScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(100.f, 50.f);
    m_titleText.setString("End of Season Awards Ceremony");
    
    m_awardTitleText.setFont(font);
    m_awardTitleText.setCharacterSize(36);
    m_awardTitleText.setFillColor(sf::Color::Yellow);
    m_awardTitleText.setPosition(100.f, 200.f);
    
    m_winnerNameText.setFont(font);
    m_winnerNameText.setCharacterSize(48);
    m_winnerNameText.setFillColor(sf::Color::White);
    m_winnerNameText.setPosition(100.f, 300.f);
    
    m_statInfoText.setFont(font);
    m_statInfoText.setCharacterSize(24);
    m_statInfoText.setFillColor(sf::Color(200, 200, 200));
    m_statInfoText.setPosition(100.f, 400.f);
    
    processAwards();
    showCurrentAward();
}

void AwardsScreen::processAwards() {
    Player* p = m_gameManager->getPlayer();
    Database& db = m_gameManager->getDatabase();
    
    std::string playerLeague = "";
    if (p && p->currentClub) {
        for (const auto& l : db.getLeagues()) {
            for (const auto& c : l.clubs) {
                if (c.name == p->currentClub->name) {
                    playerLeague = l.name;
                    break;
                }
            }
        }
    }
    
    // Create a unified list of players for the player's league, and globally
    struct ProxyPlayer {
        std::string name;
        std::string club;
        PlayerPosition pos;
        int goals;
        int assists;
        float avgRating;
        bool isReal;
        std::string league;
        int overall;
    };
    
    std::vector<ProxyPlayer> globalPlayers;
    std::vector<ProxyPlayer> leaguePlayers;
    
    std::unordered_map<std::string, std::string> clubToLeague;
    for (const auto& l : db.getLeagues()) {
        for (const auto& c : l.clubs) {
            clubToLeague[c.name] = l.name;
        }
    }
    
    // Add real player
    if (p && p->currentClub) {
        ProxyPlayer rp;
        rp.name = p->name;
        rp.club = p->currentClub->name;
        rp.pos = p->position;
        rp.goals = p->goals;
        rp.assists = p->assists;
        rp.avgRating = p->matchesPlayedThisSeason > 0 ? (p->totalSeasonRating / p->matchesPlayedThisSeason) : 0.0f;
        rp.isReal = true;
        rp.league = clubToLeague[rp.club];
        rp.overall = (p->shooting + p->passing + p->tackling + p->goalkeeping) / 4;
        
        globalPlayers.push_back(rp);
        if (!playerLeague.empty()) {
            leaguePlayers.push_back(rp);
        }
    }
    
    // Add AI players
    for (const auto& aip_ptr : db.getPlayers()) {
        auto* aip = aip_ptr.get();
        if (!aip || !aip->currentClub) continue;
        
        ProxyPlayer ap;
        ap.name = aip->name;
        ap.club = aip->currentClub->name;
        ap.pos = aip->position;
        ap.goals = aip->goals;
        ap.assists = aip->assists;
        ap.avgRating = aip->avgRating;
        ap.isReal = false;
        ap.league = clubToLeague[ap.club];
        ap.overall = aip->overall;
        
        globalPlayers.push_back(ap);
        
        // Check if in same league
        if (!playerLeague.empty()) {
            for (const auto& l : db.getLeagues()) {
                if (l.name == playerLeague) {
                    bool inLeague = false;
                    for (const auto& c : l.clubs) {
                        if (c.name == ap.club) {
                            inLeague = true;
                            break;
                        }
                    }
                    if (inLeague) {
                        leaguePlayers.push_back(ap);
                    }
                    break;
                }
            }
        }
    }
    
    // Calculate Golden Boot (League)
    if (!leaguePlayers.empty()) {
        auto winner = std::max_element(leaguePlayers.begin(), leaguePlayers.end(), [](const ProxyPlayer& a, const ProxyPlayer& b) {
            return a.goals < b.goals;
        });
        m_awards.push_back({"Golden Boot (" + playerLeague + ")", winner->name, winner->club, std::to_string(winner->goals) + " Goals", winner->isReal});
    }
    
    // Playmaker (League)
    if (!leaguePlayers.empty()) {
        auto winner = std::max_element(leaguePlayers.begin(), leaguePlayers.end(), [](const ProxyPlayer& a, const ProxyPlayer& b) {
            return a.assists < b.assists;
        });
        m_awards.push_back({"Playmaker of the Year (" + playerLeague + ")", winner->name, winner->club, std::to_string(winner->assists) + " Assists", winner->isReal});
    }
    
    // Best Defender (League)
    if (!leaguePlayers.empty()) {
        ProxyPlayer* bestDef = nullptr;
        for (auto& ply : leaguePlayers) {
            if (ply.pos == PlayerPosition::Defender) {
                if (!bestDef || ply.avgRating > bestDef->avgRating) {
                    bestDef = &ply;
                }
            }
        }
        if (bestDef) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f Avg Rating", bestDef->avgRating);
            m_awards.push_back({"Best Defender (" + playerLeague + ")", bestDef->name, bestDef->club, buf, bestDef->isReal});
        }
    }
    
    // Best Goalkeeper (League)
    if (!leaguePlayers.empty()) {
        ProxyPlayer* bestGk = nullptr;
        for (auto& ply : leaguePlayers) {
            if (ply.pos == PlayerPosition::Goalkeeper) {
                if (!bestGk || ply.avgRating > bestGk->avgRating) {
                    bestGk = &ply;
                }
            }
        }
        if (bestGk) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f Avg Rating", bestGk->avgRating);
            m_awards.push_back({"Best Goalkeeper (" + playerLeague + ")", bestGk->name, bestGk->club, buf, bestGk->isReal});
        }
    }
    
    // Ballon d'Or (Global)
    std::vector<ProxyPlayer> bdCandidates;
    for (const auto& ply : globalPlayers) {
        if (ply.overall >= 80 && (
            ply.league == "Premier League" || ply.league == "La Liga" ||
            ply.league == "Serie A" || ply.league == "Bundesliga" || ply.league == "Ligue 1")) {
            bdCandidates.push_back(ply);
        }
    }
    
    if (!bdCandidates.empty()) {
        auto winner = std::max_element(bdCandidates.begin(), bdCandidates.end(), [](const ProxyPlayer& a, const ProxyPlayer& b) {
            float scoreA = (a.goals * 2.0f) + (a.assists * 1.5f) + (a.avgRating * 10.0f);
            float scoreB = (b.goals * 2.0f) + (b.assists * 1.5f) + (b.avgRating * 10.0f);
            return scoreA < scoreB;
        });
        char buf[64];
        snprintf(buf, sizeof(buf), "%d Goals | %d Assists | %.2f Avg Rating", winner->goals, winner->assists, winner->avgRating);
        m_awards.push_back({"Ballon d'Or (World Best Player)", winner->name, winner->club, buf, winner->isReal});
    }
    
    // Add achievements to real player
    if (p) {
        CareerManager* cm = m_gameManager->getCareerManager();
        int year = cm ? cm->getYear() : 1;
        for (const auto& aw : m_awards) {
            if (aw.isRealPlayer) {
                p->achievements.push_back(aw.title + " - Year " + std::to_string(year));
            }
        }
    }
}

void AwardsScreen::showCurrentAward() {
    m_buttons.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    if (m_currentAwardIndex < m_awards.size()) {
        const auto& aw = m_awards[m_currentAwardIndex];
        m_awardTitleText.setString(aw.title);
        m_winnerNameText.setString(aw.winnerName + " (" + aw.winnerClub + ")");
        if (aw.isRealPlayer) {
            m_winnerNameText.setFillColor(sf::Color::Green);
        } else {
            m_winnerNameText.setFillColor(sf::Color::White);
        }
        m_statInfoText.setString(aw.statInfo);
        
        Button btn;
        btn.rect.setSize(sf::Vector2f(200.f, 50.f));
        btn.rect.setPosition(100.f, 500.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        btn.text.setFont(font);
        btn.text.setString("Next Award");
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect tr = btn.text.getLocalBounds();
        btn.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        btn.action = "NEXT";
        m_buttons.push_back(btn);
    } else {
        m_awardTitleText.setString("Ceremony Concluded");
        m_winnerNameText.setString("");
        m_statInfoText.setString("All awards have been distributed.");
        
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(100.f, 500.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        btn.text.setFont(font);
        btn.text.setString("Proceed to Season Summary");
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect tr = btn.text.getLocalBounds();
        btn.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        btn.action = "FINISH";
        m_buttons.push_back(btn);
    }
}

void AwardsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "NEXT") {
                    m_currentAwardIndex++;
                    showCurrentAward();
                } else if (btn.action == "FINISH") {
                    m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(UITheme::ButtonHover);
            } else {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            }
        }
    }
}

void AwardsScreen::update(sf::Time deltaTime) {}

void AwardsScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    window.draw(m_titleText);
    window.draw(m_awardTitleText);
    window.draw(m_winnerNameText);
    window.draw(m_statInfoText);
    
    for (auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
