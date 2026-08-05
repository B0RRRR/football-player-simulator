#include "UITheme.h"
#include "UIKit.h"
#include "UpgradeScreen.h"
#include "CareerHubScreen.h"
#include "AssetManager.h"
#include "GameManager.h"

UpgradeScreen::UpgradeScreen() {
}

void UpgradeScreen::init() {
    m_rows.clear();
    m_hoverIdx = m_pressedIdx = -1;

    Player* pl = m_gameManager->getPlayer();
    struct Def { std::string action, icon; };
    std::vector<Def> defs;
    if (!pl || pl->usesShooting())    defs.push_back({"Shooting", "target"});
    if (!pl || pl->usesPassing())     defs.push_back({"Passing", "arrow"});
    if (!pl || pl->usesTackling())    defs.push_back({"Tackling", "shield"});
    if (!pl || pl->usesDribbling())   defs.push_back({"Dribbling", "weave"});
    if (!pl || pl->usesGoalkeeping()) defs.push_back({"Goalkeeping", "glove"});
    defs.push_back({"Coach", "chart"});
    defs.push_back({"Car", "smiley"});

    const float x = 500.f, w = 620.f, h = 54.f, gap = 62.f, y0 = 210.f;
    for (size_t i = 0; i < defs.size(); ++i) {
        Row r;
        r.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
        r.action = defs[i].action; r.icon = defs[i].icon;
        m_rows.push_back(r);
    }
    // Back button.
    Row back; back.bounds = sf::FloatRect(x, y0 + defs.size() * gap + 8.f, 260.f, 50.f); back.action = "Back";
    m_rows.push_back(back);
}

void UpgradeScreen::dispatch(const std::string& action) {
    Player* p = m_gameManager->getPlayer();
    if (!p) return;
    auto buy = [&](int& stat) {
        int cost = stat * 5;
        if (p->experience >= cost && stat < p->potential) { p->experience -= cost; stat++; }
    };
    if (action == "Back")            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
    else if (action == "Shooting")    buy(p->shooting);
    else if (action == "Passing")     buy(p->passing);
    else if (action == "Tackling")    buy(p->tackling);
    else if (action == "Dribbling")   buy(p->dribbling);
    else if (action == "Goalkeeping") buy(p->goalkeeping);
    else if (action == "Coach") {
        if (p->money >= 25000) {
            p->money -= 25000;
            if (p->usesShooting()    && p->shooting    < p->potential) p->shooting++;
            if (p->usesPassing()     && p->passing     < p->potential) p->passing++;
            if (p->usesTackling()    && p->tackling    < p->potential) p->tackling++;
            if (p->usesDribbling()   && p->dribbling   < p->potential) p->dribbling++;
            if (p->usesGoalkeeping() && p->goalkeeping < p->potential) p->goalkeeping++;
        }
    } else if (action == "Car") {
        if (p->money >= 20000) { p->money -= 20000; p->morale = std::min(100, p->morale + 50); }
    }
}

void UpgradeScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_rows.size(); ++i) if (m_rows[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_rows.size(); ++i) if (m_rows[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int rel = -1;
        for (size_t i = 0; i < m_rows.size(); ++i) if (m_rows[i].bounds.contains(m)) rel = (int)i;
        if (rel >= 0 && rel == m_pressedIdx) dispatch(m_rows[rel].action);
        m_pressedIdx = -1;
    }
}

void UpgradeScreen::update(sf::Time) {}

void UpgradeScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 110.f}, "Training & Upgrades", 42);

    // Left resource / summary panel.
    UIKit::drawPanel(window, {90.f, 200.f, 380.f, 380.f});
    if (p) {
        float y = 224.f;
        auto row = [&](const std::string& ic, sf::Color icc, const std::string& s, sf::Color tc) {
            UIKit::drawIcon(window, ic, {118.f, y + 18 * 0.55f}, 9.f, icc);
            UIKit::drawText(window, font, {140.f, y}, s, 18, tc, 1.0f); y += 30.f;
        };
        row("chart", UITheme::Accent,    "XP     " + std::to_string(p->experience), UITheme::TextWhite);
        row("coin",  UITheme::Highlight, "Money  $" + std::to_string(p->money), UITheme::TextWhite);
        y += 10.f;
        row("star",   UITheme::Highlight, "Overall    " + std::to_string(p->overall()), UITheme::TextWhite);
        row("shield", UITheme::Accent,    "Potential  " + std::to_string(p->potential), UITheme::TextDim);
        row("smiley", UITheme::Accent,    "Morale     " + std::to_string(p->morale) + "%", UITheme::TextDim);
        y += 10.f;
        UIKit::drawText(window, font, {118.f, y}, "Spend XP to raise a stat (cost rises with the", 14, UITheme::TextDim, 1.0f);
        UIKit::drawText(window, font, {118.f, y + 20.f}, "stat). A stat cannot pass your Potential cap.", 14, UITheme::TextDim, 1.0f);
    }

    // Upgrade rows.
    for (size_t i = 0; i < m_rows.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;

        const Row& r = m_rows[i];
        if (r.action == "Back") { UIKit::drawButton(window, font, r.bounds, "Back to Hub", st); continue; }
        if (!p) continue;

        std::string label, value;
        sf::Color icc = UITheme::Accent;
        auto stat = [&](const std::string& name, int v) {
            bool max = v >= p->potential;
            label = name + "    " + std::to_string(v);
            value = max ? "MAX" : (std::to_string(v * 5) + " XP");
        };
        if (r.action == "Shooting")         stat("Shooting", p->shooting);
        else if (r.action == "Passing")     stat("Passing", p->passing);
        else if (r.action == "Tackling")    stat("Tackling", p->tackling);
        else if (r.action == "Dribbling")   stat("Dribbling", p->dribbling);
        else if (r.action == "Goalkeeping") stat("Goalkeeping", p->goalkeeping);
        else if (r.action == "Coach")  { label = "Personal Coach   +1 all stats"; value = "$25000"; icc = UITheme::Highlight; }
        else if (r.action == "Car")    { label = "Sports Car   +50 morale";       value = "$20000"; icc = UITheme::Highlight; }

        UIKit::drawIconRow(window, font, r.bounds, r.icon, icc, label, value, st);
    }
}
