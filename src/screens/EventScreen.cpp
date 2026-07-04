#include "UITheme.h"
#include "EventScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <cstdlib>

EventScreen::EventScreen() {
}

void EventScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::Cyan);
    m_titleText.setPosition(100.f, 50.f);
    m_titleText.setString("Random Event!");
    
    m_descriptionText.setFont(font);
    m_descriptionText.setCharacterSize(24);
    m_descriptionText.setFillColor(sf::Color::White);
    m_descriptionText.setPosition(100.f, 150.f);

    generateRandomEvent();
}

void EventScreen::generateRandomEvent() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    struct EventData {
        std::string desc;
        EventOption opt1;
        EventOption opt2;
    };

    std::vector<EventData> events = {
        {
            "Press asks: 'Why is the team struggling lately?'",
            {"'It's my fault, I need to train harder.'\n(-10 Morale, +20 XP)", 20, -10, 0},
            {"'My teammates need to step up.'\n(+10 Morale, -20 Energy)", 0, 10, -20}
        },
        {
            "Fans want your autograph after a tough match.",
            {"Spend an hour with them.\n(+15 Morale, -15 Energy)", 0, 15, -15},
            {"Politely decline and go home to rest.\n(-10 Morale, +15 Energy)", 0, -10, 15}
        },
        {
            "Coach offers an extra tactical session.",
            {"Stay and learn.\n(+30 XP, -20 Energy)", 30, 0, -20},
            {"Skip it, you need rest.\n(+0 XP, +20 Energy)", 0, 0, 20}
        },
        {
            "You got invited to a late-night party by teammates.",
            {"Go and bond with the team.\n(+20 Morale, -30 Energy)", 0, 20, -30},
            {"Stay home and sleep.\n(-5 Morale, +20 Energy)", 0, -5, 20}
        }
    };

    int r = rand() % events.size();
    EventData ev = events[r];

    m_descriptionText.setString(ev.desc);

    // Create buttons for options
    float startY = 300.f;
    EventOption opts[2] = {ev.opt1, ev.opt2};

    for (int i = 0; i < 2; ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(600.f, 80.f));
        btn.rect.setPosition(100.f, startY + i * 100.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        
        btn.text.setFont(font);
        btn.text.setString(opts[i].text);
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        
        btn.option = opts[i];
        m_buttons.push_back(btn);
    }
}

void EventScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                Player* p = m_gameManager->getPlayer();
                p->experience += btn.option.xpChange;
                p->morale += btn.option.moraleChange;
                if (p->morale > 100) p->morale = 100;
                if (p->morale < 0) p->morale = 0;
                
                p->energy += btn.option.energyChange;
                if (p->energy > 100) p->energy = 100;
                if (p->energy < 0) p->energy = 0;
                
                // Advance day and return to hub
                m_gameManager->getCareerManager()->advanceDay();
                m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            } else {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            }
        }
    }
}

void EventScreen::update(sf::Time deltaTime) {
}

void EventScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    window.draw(m_titleText);
    window.draw(m_descriptionText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
