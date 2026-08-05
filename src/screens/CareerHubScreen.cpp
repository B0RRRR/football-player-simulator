#include "UITheme.h"
#include "UIKit.h"
#include "CareerHubScreen.h"
#include "MatchScreen.h"
#include "MenuScreen.h"
#include "UpgradeScreen.h"
#include "LeagueTableScreen.h"
#include "TrainingScreen.h"
#include "EventScreen.h"
#include "TransferScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "MatchEngine.h"
#include "MatchStatsScreen.h"
#include "InterviewScreen.h"
#include "EuropeanCupScreen.h"
#include "SettingsScreen.h"
#include "MyStatusScreen.h"
#include "SquadScreen.h"
#include <algorithm>

CareerHubScreen::CareerHubScreen() {
}

void CareerHubScreen::init() {
    m_buttons.clear();
    m_hoverAction = m_pressedAction = "";

    struct Def { std::string label, action; bool primary; };
    std::vector<Def> defs = {
        {"Advance Day",   "Advance Day",   true},
        {"My Status",     "My Status",     false},
        {"My Squad",      "My Squad",      false},
        {"Upgrades",      "Upgrades",      false},
        {"League Table",  "League Table",  false},
        {"Tournaments",   "Tournaments",   false},
        {"Settings",      "Settings",      false},
        {"Quit to Menu",  "Quit to Menu",  false},
    };
    const float x = 560.f, w = 430.f, h = 48.f, gap = 54.f, y0 = 150.f;
    for (size_t i = 0; i < defs.size(); ++i) {
        Button b;
        b.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
        b.label = defs[i].label; b.action = defs[i].action; b.primary = defs[i].primary;
        m_buttons.push_back(b);
    }

    // Transfer Center: shown only during a window (added visually in draw), fixed slot.
    m_transfer.bounds = sf::FloatRect(x, y0 + defs.size() * gap, w, h);
    m_transfer.label = "Transfer Center";
    m_transfer.action = "Transfer Center";
    m_transfer.primary = true;

    // Debug shortcuts, a small row along the bottom.
    struct Dbg { std::string label, action; };
    std::vector<Dbg> dbg = {
        {"Skip Match",  "Debug: Skip Match"},
        {"Skip Train",  "Debug: Skip Training"},
        {"Skip Season", "Debug: Skip Season"},
    };
    const float dx = 560.f, dw = 138.f, dgap = 8.f, dy = 648.f, dh = 32.f;
    for (size_t i = 0; i < dbg.size(); ++i) {
        Button b;
        b.bounds = sf::FloatRect(dx + i * (dw + dgap), dy, dw, dh);
        b.label = dbg[i].label; b.action = dbg[i].action;
        m_buttons.push_back(b);
    }
}

void CareerHubScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    auto actionAt = [&](sf::Vector2f m) -> std::string {
        if (m_showTransfer && m_transfer.bounds.contains(m)) return m_transfer.action;
        for (auto& b : m_buttons) if (b.bounds.contains(m)) return b.action;
        return "";
    };

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverAction = actionAt(m);
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedAction = actionAt(m);
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        std::string released = actionAt(m);
        if (!released.empty() && released == m_pressedAction) dispatch(window, released);
        m_pressedAction = "";
    }
}

void CareerHubScreen::dispatch(sf::RenderWindow& window, const std::string& action) {
    (void)window;
    if (action == "Settings") {
        m_gameManager->changeScreen(std::make_shared<SettingsScreen>()); return;
    }
    if (action == "Tournaments" || action == "European Cups") {
        m_gameManager->changeScreen(std::make_shared<EuropeanCupScreen>()); return;
    }
    if (action == "Quit to Menu") {
        m_gameManager->changeScreen(std::make_shared<MenuScreen>()); return;
    }
    if (action == "Transfer Center") {
        m_gameManager->changeScreen(std::make_shared<TransferScreen>()); return;
    }
    if (action == "My Status") {
        m_gameManager->changeScreen(std::make_shared<MyStatusScreen>()); return;
    }
    if (action == "My Squad") {
        m_gameManager->changeScreen(std::make_shared<SquadScreen>()); return;
    }
    if (action == "Upgrades") {
        m_gameManager->changeScreen(std::make_shared<UpgradeScreen>()); return;
    }
    if (action == "League Table") {
        m_gameManager->changeScreen(std::make_shared<LeagueTableScreen>()); return;
    }
    if (action == "Advance Day" || action == "Recovery") {
        Player* p = m_gameManager->getPlayer();
        CareerManager* cm = m_gameManager->getCareerManager();
        if (p->injuredDays > 0) {
            p->energy += 20; if (p->energy > 100) p->energy = 100;
            cm->advanceDay(true);
            return;
        }
        if (cm->getDayType() == CalendarDayType::Rest && (rand() % 100 < 15)) {
            m_gameManager->changeScreen(std::make_shared<EventScreen>()); return;
        }
        if (cm->isSummerBreak()) {
            if (cm->hasInternationalMatchToday())
                m_gameManager->changeScreen(std::make_shared<MatchScreen>());
            else cm->advanceDay();
        } else if (cm->hasEuropeanMatchToday()) {
            m_gameManager->changeScreen(std::make_shared<MatchScreen>());
        } else if (cm->getDayType() == CalendarDayType::Match) {
            m_gameManager->changeScreen(std::make_shared<MatchScreen>());
        } else if (cm->getDayType() == CalendarDayType::Training) {
            m_gameManager->changeScreen(std::make_shared<TrainingScreen>());
        } else {
            cm->advanceDay();
        }
        return;
    }
    if (action == "Skip Summer") {
        CareerManager* cm = m_gameManager->getCareerManager();
        while (cm->isSummerBreak()) cm->advanceDay();
        return;
    }
    if (action == "Debug: Skip Training") {
        if (m_gameManager->getCareerManager()->getDayType() == CalendarDayType::Training) {
            m_gameManager->getPlayer()->experience += 8;
            m_gameManager->getCareerManager()->advanceDay();
        }
        return;
    }
    if (action == "Debug: Skip Season") {
        m_gameManager->getCareerManager()->skipSeason();
        return;
    }
    if (action == "Debug: Skip Match") {
        CareerManager* cm = m_gameManager->getCareerManager();
        if (!(cm->getDayType() == CalendarDayType::Match || cm->hasInternationalMatchToday())) return;
        Player* p = m_gameManager->getPlayer();
        if (p->suspensionMatches > 0) p->suspensionMatches--;
        Club* opp = nullptr;
        Club* playerClub = p->currentClub;
        bool isHomeMatch = true;

        if (cm->hasInternationalMatchToday()) {
            opp = cm->getInternationalOpponent();
            isHomeMatch = cm->isHomeInternationalMatch();
            const League* nats = m_gameManager->getDatabase().getNationalTeams();
            if (nats)
                for (auto& c : nats->clubs)
                    if (c.name == p->nationality) playerClub = const_cast<Club*>(&c);
        } else if (cm->hasEuropeanMatchToday()) {
            opp = cm->getTodayOpponent();
            isHomeMatch = cm->isHomeMatchToday();
        } else {
            const League* lg = nullptr;
            for (const auto& l : m_gameManager->getDatabase().getLeagues())
                for (const auto& c : l.clubs)
                    if (c.name == p->currentClub->name) { lg = &l; break; }
            if (lg) {
                int n = (int)lg->clubs.size();
                int r = p->weeksPlayed % (n - 1);
                int pIndex = -1;
                for (int i = 0; i < n; ++i)
                    if (lg->clubs[i].name == p->currentClub->name) { pIndex = i; break; }
                auto rotate = [n, r](int x) { return x == 0 ? 0 : 1 + (x - 1 + r) % (n - 1); };
                for (int i = 0; i < n / 2; ++i) {
                    int t1 = (i == 0) ? 0 : rotate(i);
                    int t2 = rotate(n - 1 - i);
                    if (t1 == pIndex) { opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t2].name); break; }
                    else if (t2 == pIndex) { opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t1].name); isHomeMatch = false; break; }
                }
            }
        }

        std::shared_ptr<MatchEngine> engine = std::make_shared<MatchEngine>(playerClub, opp, isHomeMatch, p);
        int skipGuard = 0;
        while (engine->getState() != MatchState::Finished && ++skipGuard < 100000) {
            if (engine->getState() == MatchState::Simulating) engine->updateMinute();
            else if (engine->getState() == MatchState::MinigameTriggered) {
                MinigameResult a; a.success = rand() % 2 == 0; a.kind = MinigameActionKind::Shot;
                a.power = (rand() % 100) / 100.f; a.accuracy = (rand() % 100) / 100.f;
                engine->processMinigameResult(a);
            }
            while (engine->hasLogs()) {
                MatchEvent ev = engine->popRecentLog();
                if (ev.type == EventType::PendingMinigame) {
                    bool attacking = (ev.isHome == engine->isHome());
                    MinigameResult r; r.success = (rand() % 100) < 50;
                    r.power = (rand() % 100) / 100.f; r.accuracy = (rand() % 100) / 100.f;
                    if (attacking)
                        r.kind = (p->position == PlayerPosition::Forward) ? MinigameActionKind::Shot
                               : ((rand() % 2 == 0) ? MinigameActionKind::Shot : MinigameActionKind::Pass);
                    else
                        r.kind = (p->position == PlayerPosition::Goalkeeper) ? MinigameActionKind::Save
                                                                             : MinigameActionKind::Tackle;
                    engine->processMinigameResult(r);
                } else engine->commitEvent(ev);
            }
        }
        m_gameManager->changeScreen(std::make_shared<MatchStatsScreen>(engine));
        return;
    }
}

void CareerHubScreen::update(sf::Time deltaTime) {
    (void)deltaTime;
    Player* p = m_gameManager->getPlayer();
    CareerManager* cm = m_gameManager->getCareerManager();

    if (p && cm && p->weeksPlayed >= cm->getSeasonLength()) {
        m_gameManager->changeScreen(std::make_shared<InterviewScreen>());
        return;
    }
    if (!p || !p->currentClub || !cm) return;

    m_clubTitle = p->currentClub->name + " - Hub";

    // Dynamic first button.
    if (cm->isSummerBreak()) {
        if (cm->hasInternationalMatchToday()) {
            m_buttons[0].label = "Play Int. Match"; m_buttons[0].action = "Advance Day"; m_buttons[0].primary = true;
        } else if (!cm->hasRemainingInternationalMatches()) {
            m_buttons[0].label = "Proceed to Club Season"; m_buttons[0].action = "Skip Summer"; m_buttons[0].primary = true;
        } else {
            m_buttons[0].label = "Simulate Day"; m_buttons[0].action = "Advance Day"; m_buttons[0].primary = true;
        }
    } else if (p->injuredDays > 0) {
        m_buttons[0].label = "Recovery (Rest)"; m_buttons[0].action = "Recovery"; m_buttons[0].primary = true;
    } else {
        m_buttons[0].label = "Advance Day"; m_buttons[0].action = "Advance Day"; m_buttons[0].primary = true;
    }

    // Transfer window visibility.
    int currentDay = cm->getCurrentDay();
    bool isTransferWindow = cm->isSummerBreak() || (currentDay >= 154 && currentDay <= 184);
    m_showTransfer = (isTransferWindow && p->weeksPlayed > 0);

    // Date / schedule header lines.
    int currentYear = cm->getYear();
    int totalDaysOffset = currentDay - 1;
    if (cm->isSummerBreak()) totalDaysOffset = 304 + cm->getSummerDay() - 1;
    int daysInMonth[] = {31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31};
    std::string monthNames[] = {"August", "September", "October", "November", "December", "January",
                                "February", "March", "April", "May", "June", "July"};
    int monthIndex = 0;
    while (totalDaysOffset >= daysInMonth[monthIndex]) {
        totalDaysOffset -= daysInMonth[monthIndex];
        if (++monthIndex >= 12) { monthIndex = 11; totalDaysOffset = daysInMonth[11] - 1; }
    }
    int displayYear = cm->isSummerBreak() ? currentYear : (monthIndex >= 5 ? currentYear + 1 : currentYear);
    std::string dateStr = monthNames[monthIndex] + " " + std::to_string(totalDaysOffset + 1) + ", " + std::to_string(displayYear);

    m_notice = "";
    if (cm->isSummerBreak()) {
        m_line1 = "SUMMER BREAK  -  " + dateStr;
        m_line2 = p->isCalledUp ? "Called up for the National Team" : "Resting this summer";
        m_noticeColor = sf::Color(255, 165, 0);
        if (isTransferWindow) m_notice = "TRANSFER WINDOW OPEN";
    } else {
        m_line1 = dateStr + "   -   Week " + std::to_string(p->weeksPlayed);
        m_line2 = "Schedule: " + cm->getDayTypeString();
        if (p->injuredDays > 0) {
            m_notice = "INJURED - " + std::to_string(p->injuredDays) + " days left";
            m_noticeColor = sf::Color(230, 80, 80);
        } else if (isTransferWindow && p->weeksPlayed > 0) {
            m_notice = "TRANSFER WINDOW OPEN";
            m_noticeColor = UITheme::Highlight;
        }
    }
}

void CareerHubScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();

    UIKit::drawBackground(window);

    // Club crest next to the title. The box is centred on the title's vertical mid-line so the
    // two line up regardless of the crest's own aspect ratio.
    const float titleY = 40.f, titleSize = 32.f;
    float titleX = 40.f;
    if (p && p->currentClub) {
        sf::Texture& logo = AssetManager::get().getTexture(p->currentClub->name, false);
        sf::Vector2u ts = logo.getSize();
        float box = 60.f;
        float boxX = 34.f, boxY = titleY + titleSize * 0.5f - box * 0.5f; // vertically centred on title
        float sc = (ts.x > 0 && ts.y > 0) ? box / (float)std::max(ts.x, ts.y) : 1.f;
        sf::Sprite crest(logo);
        crest.setScale(sc, sc);
        crest.setPosition(boxX + (box - ts.x * sc) * 0.5f, boxY + (box - ts.y * sc) * 0.5f);
        window.draw(crest);
        titleX = boxX + box + 24.f;
    }
    UIKit::drawTitle(window, font, {titleX, titleY}, m_clubTitle.empty() ? "Career Hub" : m_clubTitle, (unsigned)titleSize);

    // Date / schedule header.
    UIKit::drawText(window, font, {560.f, 96.f}, m_line1, 17, UITheme::TextWhite, 1.0f, true);
    UIKit::drawText(window, font, {560.f, 120.f}, m_line2, 15, UITheme::TextDim, 1.0f);
    if (!m_notice.empty())
        UIKit::drawText(window, font, {880.f, 120.f}, m_notice, 15, m_noticeColor, 1.2f, true);

    auto stateOf = [&](const std::string& action) {
        if (action == m_pressedAction && action == m_hoverAction) return UIKit::BtnState::Pressed;
        if (action == m_hoverAction) return UIKit::BtnState::Hover;
        return UIKit::BtnState::Normal;
    };

    // Player panel (left).
    UIKit::drawPanel(window, {50.f, 140.f, 470.f, 520.f});
    if (p) {
        float px = 74.f, py = 158.f;
        // Flag + name.
        if (!p->nationality.empty()) {
            sf::Texture& tex = AssetManager::get().getTexture(p->nationality, true);
            sf::Vector2u ts = tex.getSize();
            float fh = 34.f, fw = (ts.y > 0) ? fh * (float)ts.x / (float)ts.y : 50.f;
            sf::RectangleShape frame({fw + 2.f, fh + 2.f});
            frame.setPosition(px - 1.f, py - 1.f); frame.setFillColor(sf::Color(0, 0, 0, 120));
            window.draw(frame);
            sf::Sprite flag(tex);
            flag.setScale(fw / std::max(1u, ts.x), fh / std::max(1u, ts.y));
            flag.setPosition(px, py);
            window.draw(flag);
        }
        UIKit::drawText(window, font, {px + 62.f, py - 2.f}, p->name, 26, UITheme::TextWhite, 1.0f, true);

        std::string posStr = "Unknown";
        if (p->position == PlayerPosition::Forward) posStr = "Forward";
        else if (p->position == PlayerPosition::Midfielder) posStr = "Midfielder";
        else if (p->position == PlayerPosition::Defender) posStr = "Defender";
        else if (p->position == PlayerPosition::Goalkeeper) posStr = "Goalkeeper";
        UIKit::drawText(window, font, {px + 62.f, py + 30.f}, posStr + "  ·  " + p->nationality, 15, UITheme::Accent, 1.0f);

        float y = 226.f;
        auto row = [&](const std::string& icon, sf::Color ic, const std::string& s,
                       sf::Color tc = UITheme::TextWhite, unsigned sz = 18) {
            UIKit::drawIcon(window, icon, {82.f, y + sz * 0.55f}, 8.5f, ic);
            UIKit::drawText(window, font, {104.f, y}, s, sz, tc, 1.0f);
            y += (float)sz + 11.f;
        };
        row("star", UITheme::Highlight,
            "Overall " + std::to_string(p->overall()) + "    Rating " + std::to_string(p->positionalRating())
            + "    Potential " + std::to_string(p->potential), UITheme::TextWhite);
        y += 6.f;
        if (p->usesShooting())    row("target", UITheme::Accent, "Shooting     " + std::to_string(p->shooting), UITheme::TextDim);
        if (p->usesPassing())     row("arrow",  UITheme::Accent, "Passing      " + std::to_string(p->passing), UITheme::TextDim);
        if (p->usesTackling())    row("shield", UITheme::Accent, "Tackling     " + std::to_string(p->tackling), UITheme::TextDim);
        if (p->usesDribbling())   row("weave",  UITheme::Accent, "Dribbling    " + std::to_string(p->dribbling), UITheme::TextDim);
        if (p->usesGoalkeeping()) row("glove",  UITheme::Accent, "Goalkeeping  " + std::to_string(p->goalkeeping), UITheme::TextDim);
        y += 8.f;
        row("smiley", UITheme::Accent,    "Morale   " + std::to_string(p->morale) + "%");
        row("bolt",   UITheme::Accent,    "Energy   " + std::to_string(p->energy) + "%");
        row("chart",  UITheme::Accent,    "XP   " + std::to_string(p->experience));
        row("coin",   UITheme::Highlight, "Money   $" + std::to_string(p->money));
        row("coin",   UITheme::Highlight, "Salary   $" + std::to_string(p->salary) + "/w");
    }

    // Action buttons.
    for (const auto& b : m_buttons)
        UIKit::drawButton(window, font, b.bounds, b.label, stateOf(b.action));
    if (m_showTransfer)
        UIKit::drawButton(window, font, m_transfer.bounds, m_transfer.label, stateOf(m_transfer.action));
}
