#include "AwardsScreen.h"
#include "SeasonEndScreen.h"
#include "GameManager.h"
#include "Player.h"
#include "Database.h"
#include "AssetManager.h"
#include "UITheme.h"
#include "UIKit.h"
#include "CareerManager.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>

AwardsScreen::AwardsScreen() {}

void AwardsScreen::init() {
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
        rp.overall = p->overall();
        
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
    m_hoverIdx = m_pressedIdx = -1;
    if (m_currentAwardIndex < (int)m_awards.size())
        m_buttons.push_back({sf::FloatRect(540.f, 566.f, 200.f, 54.f), "Next Award", "NEXT"});
    else
        m_buttons.push_back({sf::FloatRect(480.f, 566.f, 320.f, 54.f), "Proceed to Season Summary", "FINISH"});
}

void AwardsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int rel = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].bounds.contains(m)) rel = (int)i;
        if (rel >= 0 && rel == m_pressedIdx) {
            if (m_buttons[rel].action == "NEXT") { m_currentAwardIndex++; showCurrentAward(); }
            else m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
        }
        m_pressedIdx = -1;
    }
}

void AwardsScreen::update(sf::Time) {}

void AwardsScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 44.f}, "Awards Ceremony", 40);

    auto ctext = [&](float y, const std::string& s, unsigned sz, sf::Color c, bool bold = false) {
        float w = UIKit::crispText(font, s, sz).getGlobalBounds().width;
        UIKit::drawText(window, font, {640.f - w * 0.5f, y}, s, sz, c, 1.0f, bold);
    };

    bool done = m_currentAwardIndex >= (int)m_awards.size();
    if (!done)
        ctext(150.f, "AWARD " + std::to_string(m_currentAwardIndex + 1) + " OF " + std::to_string((int)m_awards.size()), 15, UITheme::Accent);

    UIKit::drawPanel(window, {290.f, 196.f, 700.f, 320.f});
    if (!done) {
        const auto& aw = m_awards[m_currentAwardIndex];
        UIKit::drawIcon(window, "star", {640.f, 252.f}, 22.f, UITheme::Highlight);
        ctext(292.f, aw.title, 26, UITheme::Accent, true);
        ctext(346.f, aw.winnerName + "  (" + aw.winnerClub + ")", 32,
              aw.isRealPlayer ? sf::Color(90, 210, 120) : UITheme::TextWhite, true);
        ctext(408.f, aw.statInfo, 20, UITheme::TextDim);
        if (aw.isRealPlayer) ctext(446.f, "THAT'S YOU!", 15, sf::Color(90, 210, 120), true);
    } else {
        UIKit::drawIcon(window, "star", {640.f, 280.f}, 26.f, UITheme::Highlight);
        ctext(330.f, "Ceremony Concluded", 30, UITheme::TextWhite, true);
        ctext(384.f, "All awards have been distributed.", 18, UITheme::TextDim);
    }

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, m_buttons[i].bounds, m_buttons[i].label, st);
    }
}
