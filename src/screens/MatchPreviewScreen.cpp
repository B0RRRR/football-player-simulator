#include "MatchPreviewScreen.h"
#include "MatchScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Database.h"
#include "Player.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "UITheme.h"
#include "UIKit.h"
#include "Kits.h"
#include <algorithm>
#include <memory>

// --- fixture resolution -----------------------------------------------------------------------
// Mirrors MatchScreen::init: the opponent/venue are derived deterministically from the calendar,
// so re-resolving here and again in the match itself yields the same fixture.
void MatchPreviewScreen::resolveFixture() {
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;
    CareerManager* cm = m_gameManager->getCareerManager();
    Database& db = m_gameManager->getDatabase();

    Club* opp = nullptr;
    bool isHomeMatch = true;
    Club* playerClub = p->currentClub;

    if (cm->hasInternationalMatchToday()) {
        opp = cm->getInternationalOpponent();
        isHomeMatch = cm->isHomeInternationalMatch();
        m_isNat = true;
        const League* nats = db.getNationalTeams();
        if (nats)
            for (const auto& c : nats->clubs)
                if (c.name == p->nationality) { playerClub = const_cast<Club*>(&c); break; }
    } else if (cm->hasEuropeanMatchToday()) {
        opp = cm->getTodayOpponent();
        isHomeMatch = cm->isHomeMatchToday();
    } else {
        const League* lg = nullptr;
        for (const auto& l : db.getLeagues())
            for (const auto& c : l.clubs)
                if (c.name == p->currentClub->name) { lg = &l; break; }
        if (lg) {
            int n = static_cast<int>(lg->clubs.size());
            int r = p->weeksPlayed % (n - 1);
            int pIndex = -1;
            for (int i = 0; i < n; ++i)
                if (lg->clubs[i].name == p->currentClub->name) { pIndex = i; break; }
            auto rotate = [n, r](int x) { return x == 0 ? 0 : 1 + (x - 1 + r) % (n - 1); };
            for (int i = 0; i < n / 2; ++i) {
                int t1 = (i == 0) ? 0 : rotate(i);
                int t2 = rotate(n - 1 - i);
                if (t1 == pIndex) { opp = db.getClub(lg->name, lg->clubs[t2].name); break; }
                else if (t2 == pIndex) { opp = db.getClub(lg->name, lg->clubs[t1].name); isHomeMatch = false; break; }
            }
        }
    }

    // Fallback identical to the match screen's, so the two never disagree about who plays.
    if (!opp && playerClub)
        for (const auto& l : db.getLeagues()) {
            for (const auto& c : l.clubs)
                if (c.name != playerClub->name) { opp = db.getClub(l.name, c.name); break; }
            if (opp) break;
        }

    if (!playerClub || !opp) return;

    m_userClub = playerClub;
    m_home = isHomeMatch ? playerClub : opp;
    m_away = isHomeMatch ? opp : playerClub;
    m_valid = true;
}

int MatchPreviewScreen::leaguePosition(const Club* c) const {
    if (!c) return 0;
    Database& db = m_gameManager->getDatabase();
    for (const auto& l : db.getLeagues()) {
        bool here = false;
        for (const auto& cl : l.clubs) if (cl.name == c->name) { here = true; break; }
        if (!here) continue;
        std::vector<const Club*> table;
        for (const auto& cl : l.clubs) table.push_back(&cl);
        std::sort(table.begin(), table.end(), [](const Club* a, const Club* b) {
            if (a->points != b->points) return a->points > b->points;
            int gda = a->goalsFor - a->goalsAgainst, gdb = b->goalsFor - b->goalsAgainst;
            if (gda != gdb) return gda > gdb;
            return a->goalsFor > b->goalsFor;
        });
        for (size_t i = 0; i < table.size(); ++i)
            if (table[i]->name == c->name) return static_cast<int>(i) + 1;
    }
    return 0;
}

void MatchPreviewScreen::init() {
    m_valid = false;
    m_home = m_away = m_userClub = nullptr;
    m_isNat = false;
    m_pressed = m_hover = "";
    resolveFixture();

    if (!m_valid) {
        // Nothing to play (mirrors MatchScreen's safety net): skip the day and go back.
        m_gameManager->getCareerManager()->advanceDay();
        m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        return;
    }

    m_startBtn = sf::FloatRect(520.f, 654.f, 240.f, 52.f);
    m_backBtn  = sf::FloatRect(60.f, 654.f, 150.f, 52.f);
}

// --- input ------------------------------------------------------------------------------------
void MatchPreviewScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    auto at = [&](sf::Vector2f m) -> std::string {
        if (m_startBtn.contains(m)) return "start";
        if (m_backBtn.contains(m))  return "back";
        return "";
    };
    if (event.type == sf::Event::MouseMoved)
        m_hover = at(window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y}));
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        m_pressed = at(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}));
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        std::string rel = at(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}));
        if (!rel.empty() && rel == m_pressed) {
            if (rel == "start") {
                AudioManager::get().sfx("confirm");
                m_gameManager->changeScreen(std::make_shared<MatchScreen>());
            } else {
                m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            }
        }
        m_pressed = "";
    }
}

void MatchPreviewScreen::update(sf::Time) {}

// --- drawing ----------------------------------------------------------------------------------
namespace {
    // Centre a string horizontally on x and draw it. Measures with the SAME styling used to draw
    // (letter spacing + bold) and subtracts the glyph's left bearing, so it lands truly centred.
    void centered(sf::RenderWindow& w, sf::Font& f, float cx, float y,
                  const std::string& s, unsigned size, sf::Color col, float spacing = 1.f,
                  bool bold = false) {
        sf::Text t = UIKit::crispText(f, s, size);
        t.setLetterSpacing(spacing);
        if (bold) t.setStyle(sf::Text::Bold);
        sf::FloatRect b = t.getGlobalBounds();
        UIKit::drawText(w, f, {cx - b.left - b.width / 2.f, y}, s, size, col, spacing, bold);
    }
}

void MatchPreviewScreen::drawTeam(sf::RenderWindow& window, sf::Font& font, float cx,
                                  const Club* c, bool isUser, sf::Color kit) {
    if (!c) return;

    // Crest, centred at the top of the half.
    sf::Texture& logo = AssetManager::get().getTexture(c->name, m_isNat);
    sf::Vector2u ts = logo.getSize();
    const float box = 104.f, topY = 150.f;
    if (ts.x > 0 && ts.y > 0) {
        float sc = box / (float)std::max(ts.x, ts.y);
        sf::Sprite crest(logo);
        crest.setScale(sc, sc);
        crest.setPosition(cx - ts.x * sc / 2.f, topY - ts.y * sc / 2.f + box / 2.f);
        window.draw(crest);
    }

    // Name (+ "YOUR CLUB" tag), sat a little below the crest.
    centered(window, font, cx, topY + box + 16.f, c->name, 26, UITheme::TextWhite, 1.0f, true);
    if (isUser)
        centered(window, font, cx, topY + box + 48.f, "YOUR CLUB", 13, UITheme::Accent, 3.0f, true);

    // League position.
    int pos = leaguePosition(c);
    centered(window, font, cx, topY + box + 82.f, "LEAGUE POSITION", 12, UITheme::TextDim, 3.0f);
    centered(window, font, cx, topY + box + 100.f, pos > 0 ? ("#" + std::to_string(pos)) : "-",
             30, UITheme::Highlight, 1.0f, true);

    // Recent form: five dots (oldest left), W green / D amber / L red; empty rings pad a short run.
    {
        const float r = 11.f, gap = 30.f, fy = topY + box + 168.f;
        float startX = cx - (5 - 1) * gap / 2.f;
        const std::string& form = c->form;
        int have = static_cast<int>(form.size());
        for (int i = 0; i < 5; ++i) {
            // Right-align the results so the newest sits at the far right slot.
            int idx = i - (5 - have);
            sf::CircleShape dot(r, 48);
            dot.setOrigin(r, r);
            dot.setPosition(startX + i * gap, fy);
            if (idx < 0) {
                dot.setFillColor(sf::Color(255, 255, 255, 16));
                dot.setOutlineThickness(1.5f);
                dot.setOutlineColor(sf::Color(255, 255, 255, 40));
            } else {
                char res = form[idx];
                sf::Color col = res == 'W' ? sf::Color(60, 200, 90)
                              : res == 'D' ? sf::Color(235, 190, 40)
                                           : sf::Color(220, 70, 70);
                dot.setFillColor(col);
            }
            window.draw(dot);
        }
        centered(window, font, cx, fy + 22.f, "RECENT FORM", 11, UITheme::TextDim, 3.0f);
    }

    // Kit token: a big "player" disc in the club's kit colour. The colour is resolved exactly like
    // the match itself (home kit; away switches on a clash), so it matches the players on the pitch.
    {
        const float R = 46.f, ky = topY + box + 268.f;
        sf::CircleShape shadow(R + 4.f, 72); shadow.setOrigin(R + 4.f, R + 4.f);
        shadow.setPosition(cx, ky + 4.f); shadow.setFillColor(sf::Color(0, 0, 0, 70));
        window.draw(shadow);
        sf::CircleShape disc(R, 72); disc.setOrigin(R, R); disc.setPosition(cx, ky);
        disc.setFillColor(kit);
        disc.setOutlineThickness(3.f);
        disc.setOutlineColor(sf::Color(255, 255, 255, 120));
        window.draw(disc);
        // A small inner highlight so it reads as a rounded token, not a flat circle.
        sf::CircleShape gloss(R * 0.4f, 48); gloss.setOrigin(R * 0.4f, R * 0.4f);
        gloss.setPosition(cx - R * 0.28f, ky - R * 0.30f);
        gloss.setFillColor(sf::Color(255, 255, 255, 45));
        window.draw(gloss);
    }
}

void MatchPreviewScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    UIKit::drawBackground(window);

    // Header strip.
    centered(window, font, 640.f, 44.f, "MATCHDAY", 34, UITheme::TextWhite, 4.0f, true);
    {
        sf::RectangleShape rule({220.f, 3.f});
        rule.setPosition(640.f - 110.f, 92.f);
        rule.setFillColor(UITheme::AccentDim);
        window.draw(rule);
    }

    // Two team panels either side of a central divider.
    UIKit::drawPanel(window, {60.f, 118.f, 500.f, 512.f});
    UIKit::drawPanel(window, {720.f, 118.f, 500.f, 512.f});

    // Resolve the two shirts exactly as the match does (home kit; away switches on a clash) so the
    // preview's kit tokens match the figures rendered on the pitch.
    sf::Color homeShirt, awayShirt;
    Kits::resolve(m_home->name, m_away->name, homeShirt, awayShirt);
    drawTeam(window, font, 310.f, m_home, m_home == m_userClub, homeShirt);
    drawTeam(window, font, 970.f, m_away, m_away == m_userClub, awayShirt);

    // Central "VS", centred in the gap between the two panels (their inner edges are 560 and 720).
    centered(window, font, 640.f, 300.f, "VS", 40, UITheme::Accent, 1.0f, true);

    // Buttons.
    auto st = [&](const std::string& id) {
        if (id == m_pressed && id == m_hover) return UIKit::BtnState::Pressed;
        if (id == m_hover) return UIKit::BtnState::Hover;
        return UIKit::BtnState::Normal;
    };
    UIKit::drawButton(window, font, m_startBtn, "Start Match", st("start"));
    UIKit::drawButton(window, font, m_backBtn, "Back", st("back"));
}
