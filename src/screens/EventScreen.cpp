#include "UITheme.h"
#include "EventScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <cstdlib>
#include <iostream>

EventScreen::EventScreen() {
}

void EventScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::Cyan);
    m_titleText.setPosition(100.f, 50.f);
    m_titleText.setString("Event!");
    
    m_descriptionText.setFont(font);
    m_descriptionText.setCharacterSize(24);
    m_descriptionText.setFillColor(sf::Color::White);
    m_descriptionText.setPosition(100.f, 150.f);

    m_timerBg.setSize(sf::Vector2f(1080.f, 20.f));
    m_timerBg.setPosition(100.f, 100.f);
    m_timerBg.setFillColor(sf::Color(50, 50, 50));

    m_timerBar.setSize(sf::Vector2f(1080.f, 20.f));
    m_timerBar.setPosition(100.f, 100.f);
    m_timerBar.setFillColor(sf::Color::Green);

    // Setup an event
    m_currentQuestions.clear();
    
    int r = rand() % 3;
    if (r == 0) {
        m_titleText.setString("Flash Interview!");
        m_currentQuestions.push_back({
            "Journalist: 'Tough game today. Who is to blame?'",
            {
                {"'I take full responsibility.'", 10, -5, 0, 5.0f, 0},
                {"'The whole team underperformed.'", 0, -10, 0, -5.0f, 0},
                {"'No comment.'", 0, -5, 0, 0.0f, 0}
            }
        });
        m_currentQuestions.push_back({
            "Journalist: 'Are you satisfied with your playing time?'",
            {
                {"'I'm working hard for my spot.'", 20, 5, 0, 10.0f, 0},
                {"'I deserve to play more.'", 0, -5, 0, -15.0f, 0},
                {"'Coach knows best.'", 0, 0, 0, 5.0f, 0}
            }
        });
        m_maxTime = 7.0f; // 7 seconds per question
    } else if (r == 1) {
        m_titleText.setString("Sponsorship Deal");
        m_currentQuestions.push_back({
            "Sponsor: 'We want you for our new commercial. Are you in?'",
            {
                {"'Let's do it! (Costs Energy)'", 0, 10, -30, 0.0f, 50000},
                {"'I need to focus on football.'", 30, 0, 20, 5.0f, 0}
            }
        });
        if (rand() % 2 == 0) {
            m_currentQuestions.push_back({
                "Director: 'Okay, action! Read the line with passion!'",
                {
                    {"(Read with low energy)", 0, -10, -10, 0.0f, 10000},
                    {"(Give it your all!)", 20, 10, -20, 0.0f, 30000},
                    {"(Mess up the lines)", 0, -20, -10, 0.0f, 0}
                }
            });
        }
        m_maxTime = 5.0f; // fast reaction
    } else {
        m_titleText.setString("Locker Room Drama");
        m_currentQuestions.push_back({
            "Captain: 'You've been slacking in training. Step it up!'",
            {
                {"'You're right, my bad.'", 40, -10, -20, 10.0f, 0},
                {"'Mind your own business.'", 0, -20, 0, -20.0f, 0},
                {"'I'll show you on the pitch.'", 0, 15, 0, 0.0f, 0}
            }
        });
        m_maxTime = 6.0f;
    }

    m_questionIndex = 0;
    m_isFinished = false;
    m_accXp = 0; m_accMorale = 0; m_accEnergy = 0; m_accTrust = 0.0f; m_accMoney = 0;

    startNextQuestion();
}

void EventScreen::startNextQuestion() {
    if (m_questionIndex >= m_currentQuestions.size()) {
        finishEvent();
        return;
    }

    auto& q = m_currentQuestions[m_questionIndex];
    m_descriptionText.setString(q.desc);
    
    m_timeRemaining = m_maxTime;

    m_buttons.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    float startY = 300.f;
    for (size_t i = 0; i < q.options.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(800.f, 60.f));
        btn.rect.setPosition(100.f, startY + i * 80.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        
        btn.text.setFont(font);
        btn.text.setString(q.options[i].text);
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        
        btn.option = q.options[i];
        m_buttons.push_back(btn);
    }
}

void EventScreen::applyOption(const EventOption& opt) {
    m_accXp += opt.xpChange;
    m_accMorale += opt.moraleChange;
    m_accEnergy += opt.energyChange;
    m_accTrust += opt.trustChange;
    m_accMoney += opt.moneyChange;
    
    m_questionIndex++;
    startNextQuestion();
}

void EventScreen::finishEvent() {
    m_isFinished = true;
    m_buttons.clear();

    Player* p = m_gameManager->getPlayer();
    p->experience += m_accXp;
    p->morale += m_accMorale;
    p->energy += m_accEnergy;
    p->coachTrust += m_accTrust;
    p->money += m_accMoney;
    
    // Bounds checking
    if (p->morale > 100) p->morale = 100;
    if (p->morale < 0) p->morale = 0;
    if (p->energy > 100) p->energy = 100;
    if (p->energy < 0) p->energy = 0;
    if (p->coachTrust > 100.0f) p->coachTrust = 100.0f;
    if (p->coachTrust < 0.0f) p->coachTrust = 0.0f;

    std::string res = "Event Finished!\n\n";
    if (m_accXp != 0) res += "XP: " + std::to_string(m_accXp) + "\n";
    if (m_accMorale != 0) res += "Morale: " + std::to_string(m_accMorale) + "\n";
    if (m_accEnergy != 0) res += "Energy: " + std::to_string(m_accEnergy) + "\n";
    if (m_accTrust != 0.0f) res += "Coach Trust: " + std::to_string((int)m_accTrust) + "\n";
    if (m_accMoney != 0) res += "Money: +$" + std::to_string(m_accMoney) + "\n";
    
    if (m_accXp == 0 && m_accMorale == 0 && m_accEnergy == 0 && m_accTrust == 0.0f && m_accMoney == 0) {
        res += "No significant changes.";
    }

    m_descriptionText.setString(res);

    auto& font = AssetManager::get().getFont("MainFont");
    Button btn;
    btn.rect.setSize(sf::Vector2f(400.f, 60.f));
    btn.rect.setPosition(100.f, 400.f);
    btn.rect.setFillColor(UITheme::ButtonNormal);
    btn.text.setFont(font);
    btn.text.setString("Continue to Hub");
    btn.text.setCharacterSize(24);
    btn.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = btn.text.getLocalBounds();
    btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                         btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
    m_buttons.push_back(btn);
}

void EventScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (m_isFinished) {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                } else {
                    applyOption(btn.option);
                }
                return;
            }
        }
    } else if (event.type == sf::Event::MouseMoved) {
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

void EventScreen::update(sf::Time deltaTime) {
    if (m_isFinished) return;

    m_timeRemaining -= deltaTime.asSeconds();
    if (m_timeRemaining <= 0.0f) {
        m_timeRemaining = 0.0f;
        // Time ran out! Apply penalties.
        EventOption failOpt = {"Time Out", 0, -10, 0, -10.0f, 0};
        applyOption(failOpt);
    }

    float ratio = m_timeRemaining / m_maxTime;
    if (ratio < 0.0f) ratio = 0.0f;
    m_timerBar.setSize(sf::Vector2f(1080.f * ratio, 20.f));
    
    if (ratio > 0.5f) m_timerBar.setFillColor(sf::Color::Green);
    else if (ratio > 0.2f) m_timerBar.setFillColor(sf::Color::Yellow);
    else m_timerBar.setFillColor(sf::Color::Red);
}

void EventScreen::draw(sf::RenderWindow& window) {
    window.draw(m_titleText);
    
    if (!m_isFinished) {
        window.draw(m_timerBg);
        window.draw(m_timerBar);
    }

    window.draw(m_descriptionText);
    
    for (auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
