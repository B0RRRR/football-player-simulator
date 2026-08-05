#include "UITheme.h"
#include "UIKit.h"
#include "NewCareerScreen.h"
#include "CareerHubScreen.h"
#include "CareerManager.h"
#include "MatchScreen.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include <iostream>
#include <random>

NewCareerScreen::NewCareerScreen() : m_state(SetupState::InputName), m_playerName(""), m_selectedPosition(3) {
}

void NewCareerScreen::init() {
    // Wipe the previous career before anything reads the world: reset the player and rebuild
    // the database/calendar to the opening season. Done here (not at finalize) so the club
    // list below is built from the fresh leagues and the chosen club pointer stays valid.
    m_gameManager->getPlayer()->reset();
    m_gameManager->getCareerManager()->resetCareer();
    rebuildButtons();
}

void NewCareerScreen::rebuildButtons() {
    m_buttons.clear();
    m_hoverIdx = m_pressedIdx = -1;

    std::vector<std::string> labels, actions, flags, logos;

    if (m_state == SetupState::InputName) {
        m_title = "New Career";
        m_info = "STEP 1 / 4   -   ENTER YOUR NAME";
    } else if (m_state == SetupState::SelectNationality) {
        m_title = "Choose Nationality";
        m_info = "STEP 2 / 4   -   PICK YOUR COUNTRY";
        std::vector<std::string> euro = {
            "France", "Spain", "England", "Netherlands", "Portugal", "Belgium",
            "Germany", "Croatia", "Italy", "Switzerland", "Denmark", "Austria",
            "Norway", "Turkey", "Russia", "Sweden"
        };
        for (const auto& n : euro) { labels.push_back(n); actions.push_back("NAT_" + n); flags.push_back(n); }
    } else if (m_state == SetupState::SelectPosition) {
        m_title = "Choose Position";
        m_info = "STEP 3 / 4   -   YOUR ROLE ON THE PITCH";
        labels = {"Goalkeeper", "Defender", "Midfielder", "Forward"};
        actions = {"POS_0", "POS_1", "POS_2", "POS_3"};
    } else if (m_state == SetupState::SelectClub) {
        m_title = "Choose Club";
        m_info = "STEP 4 / 4   -   SIGN YOUR FIRST CONTRACT";
        auto leagues = m_gameManager->getDatabase().getLeagues();
        std::vector<const Club*> weak;
        for (const auto& lg : leagues)
            for (const auto& c : lg.clubs)
                if (c.strength <= 75) weak.push_back(&c);
        if (weak.size() > 6) {
            std::random_device rd; std::mt19937 g(rd());
            std::shuffle(weak.begin(), weak.end(), g);
            weak.resize(6);
        }
        for (const auto* c : weak) {
            labels.push_back(c->name + "   (STR " + std::to_string(c->strength) + ")");
            actions.push_back("CLUB_" + c->name);
            logos.push_back(c->name);
        }
    }

    // Lay out the option widgets per state.
    if (m_state == SetupState::SelectNationality) {
        const float x0 = 90.f, y0 = 250.f, cw = 250.f, ch = 60.f, gx = 22.f, gy = 16.f;
        const int cols = 4;
        for (size_t i = 0; i < labels.size(); ++i) {
            int col = (int)i % cols, row = (int)i / cols;
            Button b;
            b.bounds = sf::FloatRect(x0 + col * (cw + gx), y0 + row * (ch + gy), cw, ch);
            b.label = labels[i]; b.action = actions[i]; b.flag = flags[i];
            m_buttons.push_back(b);
        }
    } else if (m_state == SetupState::SelectClub) {
        // Logo cards, tighter and higher so the last one clears the Cancel button.
        const float x = 100.f, w = 580.f, h = 54.f, gap = 62.f, y0 = 230.f;
        for (size_t i = 0; i < labels.size(); ++i) {
            Button b;
            b.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
            b.label = labels[i]; b.action = actions[i]; b.logo = logos[i];
            m_buttons.push_back(b);
        }
    } else if (m_state == SetupState::SelectPosition) {
        const float x = 120.f, w = 460.f, h = 58.f, gap = 68.f, y0 = 250.f;
        for (size_t i = 0; i < labels.size(); ++i) {
            Button b;
            b.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
            b.label = labels[i]; b.action = actions[i];
            m_buttons.push_back(b);
        }
    }

    // Name step: an Accept button so Enter isn't required.
    if (m_state == SetupState::InputName) {
        Button accept;
        accept.bounds = sf::FloatRect(120.f, 348.f, 220.f, 56.f);
        accept.label = "Accept";
        accept.action = "AcceptName";
        m_buttons.push_back(accept);
    }

    // Cancel/back, always bottom-left.
    Button back;
    back.bounds = sf::FloatRect(90.f, 650.f, 190.f, 50.f);
    back.label = "Cancel";
    back.action = "Cancel";
    m_buttons.push_back(back);
}

void NewCareerScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (m_state == SetupState::InputName) {
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b') {
                while (!m_playerName.empty() && (m_playerName.back() & 0xC0) == 0x80) m_playerName.pop_back();
                if (!m_playerName.empty()) m_playerName.pop_back();
            } else if (event.text.unicode >= 32 && event.text.unicode != 127
                       && UITheme::utf8Length(m_playerName) < 20) {
                UITheme::utf8Append(m_playerName, static_cast<sf::Uint32>(event.text.unicode));
            }
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter && !m_playerName.empty()) {
            m_state = SetupState::SelectNationality;
            rebuildButtons();
        }
    }

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i)
            if (m_buttons[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i)
            if (m_buttons[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int released = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i)
            if (m_buttons[i].bounds.contains(m)) released = (int)i;
        if (released < 0 || released != m_pressedIdx) { m_pressedIdx = -1; return; }

        const std::string action = m_buttons[released].action;
        m_pressedIdx = -1;

        if (action == "Cancel") {
            m_gameManager->changeScreen(std::make_shared<MenuScreen>());
            return;
        }
        if (action == "AcceptName") {
            if (!m_playerName.empty()) { m_state = SetupState::SelectNationality; rebuildButtons(); }
            return;
        }
        if (m_state == SetupState::SelectNationality && action.rfind("NAT_", 0) == 0) {
            m_selectedNationality = action.substr(4);
            m_state = SetupState::SelectPosition;
            rebuildButtons();
        } else if (m_state == SetupState::SelectPosition && action.rfind("POS_", 0) == 0) {
            m_selectedPosition = std::stoi(action.substr(4));
            m_state = SetupState::SelectClub;
            rebuildButtons();
        } else if (m_state == SetupState::SelectClub && action.rfind("CLUB_", 0) == 0) {
            m_selectedClub = action.substr(5);

            Player* p = m_gameManager->getPlayer();
            p->name = m_playerName;
            p->nationality = m_selectedNationality;
            p->position = static_cast<PlayerPosition>(m_selectedPosition);

            // A raw prospect: decent at his job, weak elsewhere; one shared potential ceiling.
            int primary = 56 + (rand() % 7);   // 56-62
            auto sec = [&]() { return 42 + (rand() % 9); }; // 42-50
            p->shooting = sec(); p->passing = sec(); p->tackling = sec(); p->goalkeeping = sec(); p->dribbling = sec();
            switch (p->position) {
                case PlayerPosition::Forward:    p->shooting = primary; break;
                case PlayerPosition::Midfielder: p->passing = primary; break;
                case PlayerPosition::Defender:   p->tackling = primary; break;
                case PlayerPosition::Goalkeeper: p->goalkeeping = primary; break;
            }
            p->potential = 78 + (rand() % 15); // 78-92

            const Club* clubObj = nullptr;
            for (const auto& l : m_gameManager->getDatabase().getLeagues())
                for (const auto& c : l.clubs)
                    if (c.name == m_selectedClub) clubObj = &c;
            p->currentClub = const_cast<Club*>(clubObj);

            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            return;
        }
    }
}

void NewCareerScreen::update(sf::Time deltaTime) {
    m_caret += deltaTime.asSeconds();
}

// Draw a country card: rounded panel + the flag + the country name, hover-aware.
static void drawFlagCard(sf::RenderWindow& window, sf::Font& font, sf::FloatRect b,
                         const std::string& country, UIKit::BtnState state) {
    sf::Vector2f pos(b.left, b.top), size(b.width, b.height);
    sf::Color panel = (state == UIKit::BtnState::Hover)   ? UITheme::PanelHover
                    : (state == UIKit::BtnState::Pressed) ? UITheme::PanelPressed
                                                          : UITheme::PanelDark;
    window.draw(UIKit::roundedRect(pos, size, 7.f, panel,
                                   state == UIKit::BtnState::Normal ? 0.f : 2.f, UITheme::Accent));

    // Flag, scaled to a fixed height, left-aligned.
    sf::Texture& tex = AssetManager::get().getTexture(country, true);
    sf::Vector2u ts = tex.getSize();
    float fh = 34.f, fw = 50.f;
    if (ts.y > 0) fw = fh * (float)ts.x / (float)ts.y;
    sf::Sprite flag(tex);
    flag.setScale(fw / std::max(1u, ts.x), fh / std::max(1u, ts.y));
    flag.setPosition(pos.x + 14.f, pos.y + (size.y - fh) * 0.5f);
    // Subtle frame behind the flag so light flags don't bleed into the panel.
    sf::RectangleShape frame({fw + 2.f, fh + 2.f});
    frame.setPosition(pos.x + 13.f, pos.y + (size.y - fh) * 0.5f - 1.f);
    frame.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(frame);
    window.draw(flag);

    UIKit::drawText(window, font, {pos.x + 14.f + fw + 16.f, pos.y + size.y * 0.5f - 20.f * 0.72f},
                    country, 20, state == UIKit::BtnState::Normal ? UITheme::TextDim : UITheme::TextWhite,
                    1.05f);
}

// Draw a club card: rounded panel + the crest + the club name/strength, hover-aware.
static void drawLogoCard(sf::RenderWindow& window, sf::Font& font, sf::FloatRect b,
                         const std::string& clubName, const std::string& label, UIKit::BtnState state) {
    sf::Vector2f pos(b.left, b.top), size(b.width, b.height);
    sf::Color panel = (state == UIKit::BtnState::Hover)   ? UITheme::PanelHover
                    : (state == UIKit::BtnState::Pressed) ? UITheme::PanelPressed
                                                          : UITheme::PanelDark;
    window.draw(UIKit::roundedRect(pos, size, 7.f, panel,
                                   state == UIKit::BtnState::Normal ? 0.f : 2.f, UITheme::Accent));

    // Crest, fit inside a square box preserving aspect (missing logos fall back to a blank
    // texture, so this is always safe).
    sf::Texture& tex = AssetManager::get().getTexture(clubName, false);
    sf::Vector2u ts = tex.getSize();
    float box = size.y - 14.f;
    float sc = (ts.x > 0 && ts.y > 0) ? box / (float)std::max(ts.x, ts.y) : 1.f;
    sf::Sprite crest(tex);
    crest.setScale(sc, sc);
    crest.setPosition(pos.x + 12.f + (box - ts.x * sc) * 0.5f, pos.y + 7.f + (box - ts.y * sc) * 0.5f);
    window.draw(crest);

    UIKit::drawText(window, font, {pos.x + 12.f + box + 16.f, pos.y + size.y * 0.5f - 20.f * 0.72f},
                    label, 20, state == UIKit::BtnState::Normal ? UITheme::TextDim : UITheme::TextWhite,
                    1.05f);
}

void NewCareerScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {120.f, 120.f}, m_title, 46);
    UIKit::drawText(window, font, {122.f, 194.f}, m_info, 15, UITheme::Accent, 2.2f);

    if (m_state == SetupState::InputName) {
        // Name input field.
        sf::FloatRect box(120.f, 260.f, 620.f, 66.f);
        bool caretOn = std::fmod(m_caret, 1.0f) < 0.5f;
        window.draw(UIKit::roundedRect({box.left, box.top}, {box.width, box.height}, 8.f,
                                       UITheme::PanelDark, 2.f, UITheme::Accent));
        sf::RectangleShape bar({5.f, box.height - 16.f});
        bar.setPosition(box.left + 8.f, box.top + 8.f);
        bar.setFillColor(UITheme::Accent);
        window.draw(bar);
        std::string shown = m_playerName + (caretOn ? "|" : " ");
        UIKit::drawText(window, font, {box.left + 26.f, box.top + box.height * 0.5f - 28 * 0.72f},
                        shown.empty() ? "|" : shown, 28, UITheme::TextWhite, 1.0f);
        UIKit::drawText(window, font, {120.f, 428.f},
                        "Any language works - type your name, then Accept (or press Enter).",
                        16, UITheme::TextDim, 1.0f);
    }

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;

        const Button& b = m_buttons[i];
        if (!b.flag.empty())
            drawFlagCard(window, font, b.bounds, b.flag, st);
        else if (!b.logo.empty())
            drawLogoCard(window, font, b.bounds, b.logo, b.label, st);
        else
            UIKit::drawButton(window, font, b.bounds, b.label, st);
    }
}
