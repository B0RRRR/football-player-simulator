#include "SaveManager.h"
#include "json.hpp"
#include "Logger.h"
#include "Player.h"
#include "CareerManager.h"
#include "Database.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

using json = nlohmann::json;

// A signature over the whole save (FNV-1a + a salt). Any manual edit changes the content, so the
// recomputed signature no longer matches and the save is rejected on load - you can't hand out
// yourself 5000 in every stat by editing the file. (Not cryptographically strong - it just makes
// casual save-editing not work.)
static std::string saveSig(const json& withoutSig) {
    static const std::string SALT = "fps.save.v1.$9wK2r7QxZ";
    std::string s = withoutSig.dump();
    s += SALT;
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= (uint64_t)c; h *= 1099511628211ULL; }
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
    return std::string(buf);
}

static json serializeTournament(const Tournament& t) {
    json jt = {
        {"name", t.name},
        {"currentRoundIndex", t.currentRoundIndex},
        {"isFinished", t.isFinished},
        {"winner", t.winner ? t.winner->name : ""}
    };
    jt["rounds"] = json::array();
    for (const auto& r : t.rounds) {
        json jr = {
            {"name", r.name},
            {"isCompleted", r.isCompleted},
            {"matches", json::array()}
        };
        for (const auto& m : r.matches) {
            jr["matches"].push_back({
                {"home", m.home ? m.home->name : ""},
                {"away", m.away ? m.away->name : ""},
                {"homeGoalsLeg1", m.homeGoalsLeg1},
                {"awayGoalsLeg1", m.awayGoalsLeg1},
                {"homeGoalsLeg2", m.homeGoalsLeg2},
                {"awayGoalsLeg2", m.awayGoalsLeg2},
                {"homePenalties", m.homePenalties},
                {"awayPenalties", m.awayPenalties},
                {"leg1Played", m.leg1Played},
                {"leg2Played", m.leg2Played},
                {"isFinal", m.isFinal},
                {"leg1Date", m.leg1Date},
                {"leg2Date", m.leg2Date},
                {"winner", m.winner ? m.winner->name : ""}
            });
        }
        jt["rounds"].push_back(jr);
    }
    return jt;
}

static void deserializeTournament(const json& jt, Tournament& t, Database* db) {
    t.name = jt.value("name", "");
    t.currentRoundIndex = jt.value("currentRoundIndex", 0);
    t.isFinished = jt.value("isFinished", false);
    std::string winnerName = jt.value("winner", "");
    t.winner = nullptr;
    if (!winnerName.empty() && db) {
        t.winner = db->getClub("", winnerName);
    }
    
    t.rounds.clear();
    if (jt.contains("rounds")) {
        for (const auto& jr : jt["rounds"]) {
            CupRound r;
            r.name = jr.value("name", "");
            r.isCompleted = jr.value("isCompleted", false);
            if (jr.contains("matches")) {
                for (const auto& jm : jr["matches"]) {
                    CupMatch m;
                    std::string hName = jm.value("home", "");
                    std::string aName = jm.value("away", "");
                    m.home = db ? db->getClub("", hName) : nullptr;
                    m.away = db ? db->getClub("", aName) : nullptr;
                    m.homeGoalsLeg1 = jm.value("homeGoalsLeg1", 0);
                    m.awayGoalsLeg1 = jm.value("awayGoalsLeg1", 0);
                    m.homeGoalsLeg2 = jm.value("homeGoalsLeg2", 0);
                    m.awayGoalsLeg2 = jm.value("awayGoalsLeg2", 0);
                    m.homePenalties = jm.value("homePenalties", 0);
                    m.awayPenalties = jm.value("awayPenalties", 0);
                    m.leg1Played = jm.value("leg1Played", false);
                    m.leg2Played = jm.value("leg2Played", false);
                    m.isFinal = jm.value("isFinal", false);
                    m.leg1Date = jm.value("leg1Date", "");
                    m.leg2Date = jm.value("leg2Date", "");
                    std::string wName = jm.value("winner", "");
                    m.winner = db ? db->getClub("", wName) : nullptr;
                    r.matches.push_back(m);
                }
            }
            t.rounds.push_back(r);
        }
    }
}

bool SaveManager::hasSaveGame(const std::string& filepath) {
    std::ifstream f(filepath);
    return f.good();
}

bool SaveManager::saveGame(const std::string& filepath, Player* p, CareerManager* cm, Database* db) {
    json j;
    
    // Save Player
    if (p) {
        j["player"] = {
            {"name", p->name},
            {"energy", p->energy},
            {"morale", p->morale},
            {"injuredDays", p->injuredDays},
            {"suspensionMatches", p->suspensionMatches},
            {"shooting", p->shooting},
            {"passing", p->passing},
            {"tackling", p->tackling},
            {"goalkeeping", p->goalkeeping},
            {"dribbling", p->dribbling},
            {"potential", p->potential},
            {"goals", p->goals},
            {"assists", p->assists},
            {"careerGoals", p->careerGoals},
            {"careerAssists", p->careerAssists},
            {"experience", p->experience},
            {"position", static_cast<int>(p->position)},
            {"salary", p->salary},
            {"money", p->money},
            {"age", p->age},
            {"weeksPlayed", p->weeksPlayed},
            {"nationality", p->nationality},
            {"isCalledUp", p->isCalledUp},
            {"totalSeasonRating", p->totalSeasonRating},
            {"matchesPlayedThisSeason", p->matchesPlayedThisSeason},
            {"coachTrust", p->coachTrust},
            {"isTransferListed", p->isTransferListed},
            {"contractYearsLeft", p->contractYearsLeft},
            {"currentClub", p->currentClub ? p->currentClub->name : ""},
            {"achievements", p->achievements}
        };
    }
    
    // Save CareerManager
    if (cm) {
        j["career"] = {
            {"day", cm->getCurrentDay()},
            {"summerDay", cm->getSummerDay()},
            {"year", cm->getYear()},
            {"isSummerBreak", cm->isSummerBreak()}
        };
    }
    
    // Save Database
    if (db) {
        j["database"]["leagues"] = json::array();
        for (const auto& l : db->m_leagues) {
            json jLeague = {{"name", l.name}, {"clubs", json::array()}};
            for (const auto& c : l.clubs) {
                jLeague["clubs"].push_back({
                    {"name", c.name},
                    {"strength", c.strength},
                    {"points", c.points},
                    {"wins", c.wins},
                    {"draws", c.draws},
                    {"losses", c.losses},
                    {"goalsFor", c.goalsFor},
                    {"goalsAgainst", c.goalsAgainst},
                    {"form", c.form}
                });
            }
            j["database"]["leagues"].push_back(jLeague);
        }
        
        j["database"]["players"] = json::array();
        for (const auto& player : db->m_players) {
            j["database"]["players"].push_back({
                {"name", player->name},
                {"nationality", player->nationality},
                {"position", static_cast<int>(player->position)},
                {"overall", player->overall},
                {"age", player->age},
                {"currentClub", player->currentClub ? player->currentClub->name : ""},
                {"goals", player->goals},
                {"assists", player->assists},
                {"matchesPlayed", player->matchesPlayed},
                {"avgRating", player->avgRating},
                {"careerGoals", player->careerGoals},
                {"careerAssists", player->careerAssists},
                {"careerMatches", player->careerMatches}
            });
        }
        
        j["database"]["history"] = json::object();
        for (const auto& kv : db->m_leagueHistory) {
            std::string yearStr = std::to_string(kv.first);
            j["database"]["history"][yearStr] = json::array();
            for (const auto& l : kv.second) {
                json jLeague = {{"name", l.name}, {"clubs", json::array()}};
                for (const auto& c : l.clubs) {
                    jLeague["clubs"].push_back({
                        {"name", c.name},
                        {"strength", c.strength},
                        {"points", c.points},
                        {"wins", c.wins},
                        {"draws", c.draws},
                        {"losses", c.losses},
                        {"goalsFor", c.goalsFor},
                        {"goalsAgainst", c.goalsAgainst}
                    });
                }
                j["database"]["history"][yearStr].push_back(jLeague);
            }
        }
        j["database"]["championsLeague"] = serializeTournament(db->m_championsLeague);
        j["database"]["europaLeague"] = serializeTournament(db->m_europaLeague);
        j["database"]["euroCup"] = serializeTournament(db->m_euroCup);
        j["database"]["worldCup"] = serializeTournament(db->m_worldCup);
        
        j["database"]["history_championsLeague"] = json::object();
        for (const auto& kv : db->m_championsLeagueHistory) {
            j["database"]["history_championsLeague"][std::to_string(kv.first)] = serializeTournament(kv.second);
        }
        j["database"]["history_europaLeague"] = json::object();
        for (const auto& kv : db->m_europaLeagueHistory) {
            j["database"]["history_europaLeague"][std::to_string(kv.first)] = serializeTournament(kv.second);
        }
        j["database"]["history_euroCup"] = json::object();
        for (const auto& kv : db->m_euroCupHistory) {
            j["database"]["history_euroCup"][std::to_string(kv.first)] = serializeTournament(kv.second);
        }
        j["database"]["history_worldCup"] = json::object();
        for (const auto& kv : db->m_worldCupHistory) {
            j["database"]["history_worldCup"][std::to_string(kv.first)] = serializeTournament(kv.second);
        }
    }
    
    // Sign the content, then embed the signature.
    j.erase("sig");
    j["sig"] = saveSig(j);

    std::ofstream o(filepath);
    if (o.is_open()) {
        o << j.dump(4);
        return true;
    }
    return false;
}

bool SaveManager::loadGame(const std::string& filepath, Player* p, CareerManager* cm, Database* db) {
    std::ifstream i(filepath);
    if (!i.is_open()) return false;
    
    json j;
    try {
        i >> j;
    } catch (...) {
        return false;
    }

    // Reject a tampered save (a signed save whose content was edited). Older saves without a
    // signature are still accepted; they get signed on the next save. (Stats are also clamped to
    // legitimate bounds below, so even an unsigned save can't grant impossible attributes.)
    if (j.contains("sig")) {
        std::string provided = j.value("sig", "");
        json tmp = j;
        tmp.erase("sig");
        if (saveSig(tmp) != provided) {
            LOG_WARN("Save signature mismatch - refusing tampered save: " << filepath);
            return false;
        }
    }

    if (p && j.contains("player")) {
        auto jp = j["player"];
        p->name = jp.value("name", "Player");
        p->energy = jp.value("energy", 100);
        p->morale = jp.value("morale", 100);
        p->injuredDays = jp.value("injuredDays", 0);
        p->suspensionMatches = jp.value("suspensionMatches", 0);
        p->shooting = jp.value("shooting", 50);
        p->passing = jp.value("passing", 50);
        p->tackling = jp.value("tackling", 50);
        p->goalkeeping = jp.value("goalkeeping", 50);
        p->dribbling = jp.value("dribbling", 50); // pre-dribbling saves default to average
        p->potential = jp.value("potential", 88); // pre-potential saves keep a fair ceiling
        p->goals = jp.value("goals", 0);
        p->assists = jp.value("assists", 0);
        p->careerGoals = jp.value("careerGoals", 0);
        p->careerAssists = jp.value("careerAssists", 0);
        p->experience = jp.value("experience", 0);
        p->position = static_cast<PlayerPosition>(jp.value("position", 0));
        p->salary = jp.value("salary", 0);
        p->money = jp.value("money", 0);
        p->age = jp.value("age", 18);
        p->weeksPlayed = jp.value("weeksPlayed", 0);
        p->nationality = jp.value("nationality", "Unknown");
        p->isCalledUp = jp.value("isCalledUp", false);
        p->totalSeasonRating = jp.value("totalSeasonRating", 0.0f);
        p->matchesPlayedThisSeason = jp.value("matchesPlayedThisSeason", 0);
        p->coachTrust = jp.value("coachTrust", 50.0f);
        p->isTransferListed = jp.value("isTransferListed", false);
        p->contractYearsLeft = jp.value("contractYearsLeft", 3);
        
        p->achievements.clear();
        if (jp.contains("achievements") && jp["achievements"].is_array()) {
            for (auto& ach : jp["achievements"]) {
                p->achievements.push_back(ach.get<std::string>());
            }
        }

        // Anti-cheat clamp: an attribute can never exceed the potential cap (or 99), and the cap
        // itself is bounded. Defeats hand-edited "5000 in every stat" saves outright.
        p->potential = std::clamp(p->potential, 1, 99);
        int cap = std::min(99, p->potential);
        p->shooting    = std::clamp(p->shooting, 1, cap);
        p->passing     = std::clamp(p->passing, 1, cap);
        p->tackling    = std::clamp(p->tackling, 1, cap);
        p->goalkeeping = std::clamp(p->goalkeeping, 1, cap);
        p->dribbling   = std::clamp(p->dribbling, 1, cap);
        if (p->experience < 0) p->experience = 0;

        // We will resolve currentClub after loading the database,
        // because the database will clear and rebuild its clubs, invalidating pointers.
    }
    
    std::string playerClubName = "";
    if (j.contains("player") && j["player"].contains("currentClub")) {
        playerClubName = j["player"].value("currentClub", "");
    }
    
    if (db && j.contains("database")) {
        auto jd = j["database"];
        
        if (jd.contains("leagues")) {
            std::map<std::string, Club> allClubs;
            for (auto& l : db->m_leagues) {
                for (auto& c : l.clubs) {
                    allClubs[c.name] = c;
                }
            }
            db->m_leagues.clear();
            for (const auto& jLeague : jd["leagues"]) {
                League newL;
                newL.name = jLeague.value("name", "");
                for (const auto& jClub : jLeague["clubs"]) {
                    std::string cName = jClub.value("name", "");
                    Club c = allClubs[cName];
                    c.name = cName;
                    c.strength = jClub.value("strength", 50);
                    c.points = jClub.value("points", 0);
                    c.wins = jClub.value("wins", 0);
                    c.draws = jClub.value("draws", 0);
                    c.losses = jClub.value("losses", 0);
                    c.goalsFor = jClub.value("goalsFor", 0);
                    c.goalsAgainst = jClub.value("goalsAgainst", 0);
                    c.form = jClub.value("form", std::string());
                    newL.clubs.push_back(c);
                }
                db->m_leagues.push_back(newL);
            }
        }
        
        if (jd.contains("players")) {
            db->m_players.clear();
            // Clear rosters
            for (auto& l : db->m_leagues) {
                for (auto& c : l.clubs) {
                    c.roster.clear();
                }
            }
            if (db->m_nationalTeams.clubs.size() > 0) {
                for (auto& c : db->m_nationalTeams.clubs) {
                    c.roster.clear();
                }
            }
            
            for (const auto& jp : jd["players"]) {
                auto player = std::make_unique<AIPlayer>();
                player->name = jp.value("name", "");
                player->nationality = jp.value("nationality", "");
                player->position = static_cast<PlayerPosition>(jp.value("position", 0));
                player->overall = jp.value("overall", 50);
                player->age = jp.value("age", 18);
                player->goals = jp.value("goals", 0);
                player->assists = jp.value("assists", 0);
                player->matchesPlayed = jp.value("matchesPlayed", 0);
                player->avgRating = jp.value("avgRating", 0.0f);
                player->careerGoals = jp.value("careerGoals", 0);
                player->careerAssists = jp.value("careerAssists", 0);
                player->careerMatches = jp.value("careerMatches", 0);
                
                std::string clubName = jp.value("currentClub", "");
                if (!clubName.empty()) {
                    player->currentClub = db->getClub("", clubName);
                    if (player->currentClub) {
                        player->currentClub->roster.push_back(player.get());
                    }
                }
                
                db->m_players.push_back(std::move(player));
            }
        } else {
            // Backward compatibility for old saves without players
            db->generatePlayers();
        }
        
        if (jd.contains("history")) {
            db->m_leagueHistory.clear();
            for (auto it = jd["history"].begin(); it != jd["history"].end(); ++it) {
                int year = std::stoi(it.key());
                std::vector<League> hLeagues;
                for (const auto& jLeague : it.value()) {
                    League l;
                    l.name = jLeague.value("name", "");
                    for (const auto& jClub : jLeague["clubs"]) {
                        Club c;
                        c.name = jClub.value("name", "");
                        c.strength = jClub.value("strength", 50);
                        c.points = jClub.value("points", 0);
                        c.wins = jClub.value("wins", 0);
                        c.draws = jClub.value("draws", 0);
                        c.losses = jClub.value("losses", 0);
                        c.goalsFor = jClub.value("goalsFor", 0);
                        c.goalsAgainst = jClub.value("goalsAgainst", 0);
                        l.clubs.push_back(c);
                    }
                    hLeagues.push_back(l);
                }
                db->m_leagueHistory[year] = hLeagues;
            }
        }
        
        if (jd.contains("championsLeague")) deserializeTournament(jd["championsLeague"], db->m_championsLeague, db);
        if (jd.contains("europaLeague")) deserializeTournament(jd["europaLeague"], db->m_europaLeague, db);
        if (jd.contains("euroCup")) deserializeTournament(jd["euroCup"], db->m_euroCup, db);
        if (jd.contains("worldCup")) deserializeTournament(jd["worldCup"], db->m_worldCup, db);
        
        db->m_championsLeagueHistory.clear();
        if (jd.contains("history_championsLeague")) {
            for (auto it = jd["history_championsLeague"].begin(); it != jd["history_championsLeague"].end(); ++it) {
                deserializeTournament(it.value(), db->m_championsLeagueHistory[std::stoi(it.key())], db);
            }
        }
        db->m_europaLeagueHistory.clear();
        if (jd.contains("history_europaLeague")) {
            for (auto it = jd["history_europaLeague"].begin(); it != jd["history_europaLeague"].end(); ++it) {
                deserializeTournament(it.value(), db->m_europaLeagueHistory[std::stoi(it.key())], db);
            }
        }
        db->m_euroCupHistory.clear();
        if (jd.contains("history_euroCup")) {
            for (auto it = jd["history_euroCup"].begin(); it != jd["history_euroCup"].end(); ++it) {
                deserializeTournament(it.value(), db->m_euroCupHistory[std::stoi(it.key())], db);
            }
        }
        db->m_worldCupHistory.clear();
        if (jd.contains("history_worldCup")) {
            for (auto it = jd["history_worldCup"].begin(); it != jd["history_worldCup"].end(); ++it) {
                deserializeTournament(it.value(), db->m_worldCupHistory[std::stoi(it.key())], db);
            }
        }
    }
    
    if (cm && j.contains("career")) {
        auto jc = j["career"];
        cm->setCurrentDay(jc.value("day", 1));
        cm->setSummerDay(jc.value("summerDay", 1));
        cm->setYear(jc.value("year", 2024));
        cm->setSummerBreak(jc.value("isSummerBreak", false));
    }
    
    if (p && !playerClubName.empty() && db) {
        p->currentClub = db->getClub("", playerClubName);
    }
    
    return true;
}
