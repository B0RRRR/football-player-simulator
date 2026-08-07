#include "UITheme.h"
#include "UIKit.h"
#include "EventScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <cstdlib>
#include <algorithm>

EventScreen::EventScreen() {
}

void EventScreen::init() {
    m_currentQuestions.clear();

    int r = rand() % 3;
    if (r == 0) {
        m_title = "Flash Interview";
        m_currentQuestions.push_back({
            "Journalist: 'Tough game today. Who is to blame?'",
            {{"'I take full responsibility.'", 10, -5, 0, 5.0f, 0},
             {"'The whole team underperformed.'", 0, -10, 0, -5.0f, 0},
             {"'No comment.'", 0, -5, 0, 0.0f, 0}}});
        m_currentQuestions.push_back({
            "Journalist: 'Are you satisfied with your playing time?'",
            {{"'I'm working hard for my spot.'", 20, 5, 0, 10.0f, 0},
             {"'I deserve to play more.'", 0, -5, 0, -15.0f, 0},
             {"'Coach knows best.'", 0, 0, 0, 5.0f, 0}}});
        m_maxTime = 7.0f;
    } else if (r == 1) {
        m_title = "Sponsorship Deal";
        m_currentQuestions.push_back({
            "Sponsor: 'We want you for our new commercial. Are you in?'",
            {{"'Let's do it! (Costs Energy)'", 0, 10, -30, 0.0f, 50000},
             {"'I need to focus on football.'", 30, 0, 20, 5.0f, 0}}});
        if (rand() % 2 == 0) {
            m_currentQuestions.push_back({
                "Director: 'Okay, action! Read the line with passion!'",
                {{"(Read with low energy)", 0, -10, -10, 0.0f, 10000},
                 {"(Give it your all!)", 20, 10, -20, 0.0f, 30000},
                 {"(Mess up the lines)", 0, -20, -10, 0.0f, 0}}});
        }
        m_maxTime = 5.0f;
    } else {
        m_title = "Locker Room Drama";
        m_currentQuestions.push_back({
            "Captain: 'You've been slacking in training. Step it up!'",
            {{"'You're right, my bad.'", 40, -10, -20, 10.0f, 0},
             {"'Mind your own business.'", 0, -20, 0, -20.0f, 0},
             {"'I'll show you on the pitch.'", 0, 15, 0, 0.0f, 0}}});
        m_maxTime = 6.0f;
    }

    m_questionIndex = 0;
    m_isFinished = false;
    m_accXp = 0; m_accMorale = 0; m_accEnergy = 0; m_accTrust = 0.0f; m_accMoney = 0;
    startNextQuestion();
}

void EventScreen::startNextQuestion() {
    if (m_questionIndex >= (int)m_currentQuestions.size()) { finishEvent(); return; }
    auto& q = m_currentQuestions[m_questionIndex];
    m_desc = q.desc;
    m_timeRemaining = m_maxTime;
    m_hoverIdx = m_pressedIdx = -1;

    m_buttons.clear();
    const float x = 240.f, w = 800.f, h = 62.f, gap = 78.f, startY = 320.f;
    for (size_t i = 0; i < q.options.size(); ++i) {
        Button b;
        b.bounds = sf::FloatRect(x, startY + i * gap, w, h);
        b.label = q.options[i].text;
        b.option = q.options[i];
        m_buttons.push_back(b);
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
    m_hoverIdx = m_pressedIdx = -1;

    Player* p = m_gameManager->getPlayer();
    p->experience += m_accXp;
    p->morale = std::clamp(p->morale + m_accMorale, 0, 100);
    p->energy = std::clamp(p->energy + m_accEnergy, 0, 100);
    p->coachTrust = std::clamp(p->coachTrust + m_accTrust, 0.f, 100.f);
    p->money += m_accMoney;

    std::string res;
    if (m_accXp != 0)     res += "XP: " + std::to_string(m_accXp) + "\n";
    if (m_accMorale != 0) res += "Morale: " + std::to_string(m_accMorale) + "\n";
    if (m_accEnergy != 0) res += "Energy: " + std::to_string(m_accEnergy) + "\n";
    if (m_accTrust != 0.f) res += "Coach Trust: " + std::to_string((int)m_accTrust) + "\n";
    if (m_accMoney != 0)  res += "Money: +$" + std::to_string(m_accMoney) + "\n";
    if (res.empty()) res = "No significant changes.";
    m_desc = res;

    Button b;
    b.bounds = sf::FloatRect(440.f, 480.f, 400.f, 60.f);
    b.label = "Continue to Hub";
    m_buttons.push_back(b);
}

void EventScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
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
            if (m_isFinished) m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            else applyOption(m_buttons[rel].option);
        }
        m_pressedIdx = -1;
    }
}

void EventScreen::update(sf::Time deltaTime) {
    if (m_isFinished) return;
    m_timeRemaining -= deltaTime.asSeconds();
    if (m_timeRemaining <= 0.0f) {
        m_timeRemaining = 0.0f;
        applyOption({"Time Out", 0, -10, 0, -10.0f, 0});
    }
}

void EventScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 44.f}, m_title, 40);

    if (!m_isFinished) {
        float ratio = std::clamp(m_timeRemaining / m_maxTime, 0.f, 1.f);
        window.draw(UIKit::roundedRect({240.f, 150.f}, {800.f, 12.f}, 6.f, UITheme::PanelPressed));
        sf::Color tc = ratio > 0.5f ? sf::Color(90, 210, 120) : ratio > 0.2f ? UITheme::Highlight : sf::Color(230, 90, 90);
        if (ratio > 0.01f) window.draw(UIKit::roundedRect({240.f, 150.f}, {800.f * ratio, 12.f}, 6.f, tc));

        UIKit::drawPanel(window, {240.f, 190.f, 800.f, 96.f});
        UIKit::drawText(window, font, {268.f, 224.f}, m_desc, 20, UITheme::TextWhite, 1.0f);
    } else {
        UIKit::drawPanel(window, {240.f, 200.f, 800.f, 220.f});
        UIKit::drawText(window, font, {272.f, 232.f}, "OUTCOME", 18, UITheme::Accent, 2.0f, true);
        UIKit::drawText(window, font, {272.f, 280.f}, m_desc, 20, UITheme::TextWhite, 1.0f);
    }

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, m_buttons[i].bounds, m_buttons[i].label, st);
    }
}
