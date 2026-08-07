#include "UITheme.h"
#include "UIKit.h"
#include "TransferScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "Database.h"
#include "Player.h"
#include "AssetManager.h"
#include <algorithm>
#include <cstdlib>
#include <random>

TransferScreen::TransferScreen() {
}

void TransferScreen::init() {
    generateOffersIfNeeded();
    refreshTab();
}

void TransferScreen::generateOffersIfNeeded() {
    if (!m_offers.empty()) return;
    Player* p = m_gameManager->getPlayer();
    Database& db = m_gameManager->getDatabase();

    int baseChance = p->isTransferListed ? 60 : 15;
    if (rand() % 100 > baseChance) return;

    int playerOverall = p->positionalRating();
    int targetStrength = std::min(90, playerOverall + ((p->goals + p->assists) / 5));

    std::vector<Club*> suitable;
    for (const auto& l : db.getLeagues())
        for (const auto& c : l.clubs) {
            if (p->currentClub && c.name == p->currentClub->name) continue;
            int margin = p->isTransferListed ? 12 : 6;
            if (abs(c.strength - targetStrength) <= margin) suitable.push_back(db.getClub(l.name, c.name));
        }
    if (suitable.size() > 3) {
        std::random_device rd; std::mt19937 g(rd());
        std::shuffle(suitable.begin(), suitable.end(), g);
        suitable.resize(3);
    }
    for (auto* c : suitable)
        m_offers.push_back({c, c->strength * 50 + (rand() % 1000)});
}

void TransferScreen::refreshTab() {
    m_click.clear();
    m_hoverIdx = m_pressedIdx = -1;

    // Tabs + back (always present).
    m_click.push_back({sf::FloatRect(60.f, 110.f, 220.f, 46.f), "Inbox / Offers", "TAB_INBOX"});
    m_click.push_back({sf::FloatRect(296.f, 110.f, 200.f, 46.f), "Search Clubs", "TAB_SEARCH"});
    m_click.push_back({sf::FloatRect(1050.f, 42.f, 180.f, 46.f), "Back to Hub", "BACK"});

    if (m_currentTab == Tab::Inbox) buildInboxTab();
    else buildSearchTab();
}

void TransferScreen::buildInboxTab() {
    if (m_offers.empty()) {
        m_infoStr = "No pending offers right now. Wait, or scout clubs in the Search tab.";
        return;
    }
    m_infoStr = "You have " + std::to_string(m_offers.size()) + " pending offer(s). Click one to accept.";
    const float x = 60.f, w = 780.f, h = 66.f, gap = 78.f, y0 = 220.f;
    for (size_t i = 0; i < m_offers.size(); ++i) {
        Click c;
        c.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
        c.action = "ACCEPT"; c.offer = m_offers[i]; c.isCard = true;
        c.logo = m_offers[i].club->name;
        c.label = m_offers[i].club->name + "  (STR " + std::to_string(m_offers[i].club->strength) + ")";
        c.rightText = "$" + std::to_string(m_offers[i].offeredSalary) + "/w";
        m_click.push_back(c);
    }
}

void TransferScreen::buildSearchTab() {
    Database& db = m_gameManager->getDatabase();
    const auto& leagues = db.getLeagues();
    m_infoStr = "Browse clubs and declare your interest.";
    if (leagues.empty()) return;

    if (m_searchLeagueIdx < 0) m_searchLeagueIdx = (int)leagues.size() - 1;
    if (m_searchLeagueIdx >= (int)leagues.size()) m_searchLeagueIdx = 0;
    const League& lg = leagues[m_searchLeagueIdx];
    m_searchLeagueName = lg.name;

    m_click.push_back({sf::FloatRect(60.f, 196.f, 54.f, 40.f), "<", "PREV_LEAGUE"});
    m_click.push_back({sf::FloatRect(120.f, 196.f, 54.f, 40.f), ">", "NEXT_LEAGUE"});

    const float x = 60.f, w = 780.f, h = 50.f, gap = 58.f, y0 = 256.f;
    int perPage = 7, start = m_searchPage * perPage;
    for (int i = 0; i < perPage && (start + i) < (int)lg.clubs.size(); ++i) {
        const Club& c = lg.clubs[start + i];
        Click cl;
        cl.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
        cl.action = "APPLY"; cl.isCard = true;
        cl.targetClub = db.getClub(lg.name, c.name);
        cl.logo = c.name;
        cl.label = c.name + "  (STR " + std::to_string(c.strength) + ")";
        cl.rightText = "Apply";
        m_click.push_back(cl);
    }
    if (start + perPage < (int)lg.clubs.size())
        m_click.push_back({sf::FloatRect(700.f, 666.f, 140.f, 40.f), "Next Page", "NEXT_PAGE"});
    if (m_searchPage > 0)
        m_click.push_back({sf::FloatRect(540.f, 666.f, 140.f, 40.f), "Prev Page", "PREV_PAGE"});
}

void TransferScreen::attemptTransfer(Club* targetClub) {
    if (!targetClub) return;
    Player* p = m_gameManager->getPlayer();
    if (p->currentClub && targetClub->name == p->currentClub->name) {
        m_messageStr = "You already play for " + targetClub->name + "!"; m_messageTimer = 3.0f; return;
    }
    int targetStrength = p->positionalRating() + ((p->goals + p->assists) / 5);
    int diff = targetStrength - targetClub->strength;
    if (diff < -8) {
        m_messageStr = "Refused! " + targetClub->name + " thinks you're not good enough."; m_messageTimer = 4.0f;
    } else if (diff > 20) {
        m_messageStr = "Refused! " + targetClub->name + " cannot afford your wages."; m_messageTimer = 4.0f;
    } else {
        for (const auto& off : m_offers)
            if (off.club->name == targetClub->name) {
                m_messageStr = "You already have an offer from " + targetClub->name + "!"; m_messageTimer = 3.0f; return;
            }
        int salary = targetClub->strength * 50 + (rand() % 1000);
        if (p->isTransferListed) salary = (int)(salary * 0.9f);
        m_offers.push_back({targetClub, salary});
        m_messageStr = "Success! " + targetClub->name + " sent an offer to your inbox."; m_messageTimer = 4.0f;
    }
}

void TransferScreen::dispatch(const Click& c) {
    if (c.action == "BACK") { m_gameManager->changeScreen(std::make_shared<CareerHubScreen>()); return; }
    if (c.action == "TAB_INBOX")  { m_currentTab = Tab::Inbox;  refreshTab(); return; }
    if (c.action == "TAB_SEARCH") { m_currentTab = Tab::Search; refreshTab(); return; }
    if (c.action == "PREV_LEAGUE") { m_searchLeagueIdx--; m_searchPage = 0; refreshTab(); return; }
    if (c.action == "NEXT_LEAGUE") { m_searchLeagueIdx++; m_searchPage = 0; refreshTab(); return; }
    if (c.action == "NEXT_PAGE") { m_searchPage++; refreshTab(); return; }
    if (c.action == "PREV_PAGE") { m_searchPage--; refreshTab(); return; }
    if (c.action == "APPLY") { attemptTransfer(c.targetClub); refreshTab(); return; }
    if (c.action == "ACCEPT") {
        Player* p = m_gameManager->getPlayer();
        p->currentClub = m_gameManager->getDatabase().getClub("", c.offer.club->name);
        if (!p->currentClub) p->currentClub = c.offer.club;
        p->salary = c.offer.offeredSalary;
        p->coachTrust = 50.0f;
        p->isTransferListed = false;
        m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
    }
}

void TransferScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_click.size(); ++i) if (m_click[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_click.size(); ++i) if (m_click[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int rel = -1;
        for (size_t i = 0; i < m_click.size(); ++i) if (m_click[i].bounds.contains(m)) rel = (int)i;
        if (rel >= 0 && rel == m_pressedIdx) dispatch(m_click[rel]); // may rebuild m_click
        m_pressedIdx = -1;
    }
}

void TransferScreen::update(sf::Time deltaTime) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= deltaTime.asSeconds();
        if (m_messageTimer <= 0.0f) m_messageStr.clear();
    }
}

void TransferScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {60.f, 40.f}, "Transfer Center", 36);

    UIKit::drawText(window, font, {60.f, 172.f}, m_infoStr, 16, UITheme::TextDim, 1.0f);
    if (!m_messageStr.empty())
        UIKit::drawText(window, font, {540.f, 122.f}, m_messageStr, 16, UITheme::Highlight, 1.0f, true);
    if (m_currentTab == Tab::Search && !m_searchLeagueName.empty())
        UIKit::drawText(window, font, {190.f, 204.f}, m_searchLeagueName, 22, UITheme::TextWhite, 1.0f, true);

    // A club/offer card: crest + name + a right-hand value.
    auto card = [&](const Click& c, UIKit::BtnState st) {
        sf::Vector2f pos(c.bounds.left, c.bounds.top), size(c.bounds.width, c.bounds.height);
        UIKit::detail::rowBase(window, pos, size, st);
        float box = size.y - 16.f;
        sf::Texture& tex = AssetManager::get().getTexture(c.logo, false);
        sf::Vector2u ts = tex.getSize();
        if (ts.x > 0 && ts.y > 0) {
            float sc = box / (float)std::max(ts.x, ts.y);
            sf::Sprite s(tex); s.setScale(sc, sc);
            s.setPosition(pos.x + 16.f + (box - ts.x * sc) * 0.5f, pos.y + 8.f + (box - ts.y * sc) * 0.5f);
            window.draw(s);
        }
        UIKit::drawTextCenteredY(window, font, pos.x + 16.f + box + 16.f, pos.y + size.y * 0.5f, c.label, 19,
                                 st == UIKit::BtnState::Normal ? UITheme::TextDim : UITheme::TextWhite, 1.0f, true);
        float rw = UIKit::crispText(font, c.rightText, 19).getGlobalBounds().width;
        UIKit::drawTextCenteredY(window, font, pos.x + size.x - 24.f - rw, pos.y + size.y * 0.5f, c.rightText, 19,
                                 UITheme::Accent, 1.0f, true);
    };

    for (size_t i = 0; i < m_click.size(); ++i) {
        const Click& c = m_click[i];
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;

        bool isTab = (c.action == "TAB_INBOX" || c.action == "TAB_SEARCH");
        if (isTab) {
            bool activeTab = (c.action == "TAB_INBOX") == (m_currentTab == Tab::Inbox);
            if (activeTab && st == UIKit::BtnState::Normal) st = UIKit::BtnState::Hover;
            UIKit::drawButton(window, font, c.bounds, c.label, st, false);
        } else if (c.isCard) {
            card(c, st);
        } else {
            bool chev = (c.action == "BACK");
            UIKit::drawButton(window, font, c.bounds, c.label, st, chev);
        }
    }
}
