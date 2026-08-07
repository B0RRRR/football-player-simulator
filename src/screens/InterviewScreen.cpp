#include "UITheme.h"
#include "UIKit.h"
#include "InterviewScreen.h"
#include "AwardsScreen.h"
#include "SeasonEndScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Database.h"
#include "AssetManager.h"
#include "Player.h"
#include <cstdlib>
#include <algorithm>
#include <random>

InterviewScreen::InterviewScreen() {
}

void InterviewScreen::init() {
    generateQuestions();
    m_questionIndex = 0;
    m_correctAnswers = 0;
    m_isFinished = false;
    startNextQuestion();
}

void InterviewScreen::generateQuestions() {
    m_questions.clear();
    Database& db = m_gameManager->getDatabase();
    Player* p = m_gameManager->getPlayer();

    if (db.getChampionsLeague().winner) {
        Question q;
        q.desc = "Journalist: 'Who won the Champions League this season?'";
        std::string correct = db.getChampionsLeague().winner->name;
        std::vector<std::string> wrong;
        for (const auto& l : db.getLeagues())
            for (const auto& c : l.clubs)
                if (c.name != correct && c.strength > 75) wrong.push_back(c.name);
        auto rng = std::default_random_engine(rand());
        std::shuffle(wrong.begin(), wrong.end(), rng);
        q.options.push_back({correct, true});
        q.options.push_back({wrong[0], false});
        q.options.push_back({wrong[1], false});
        std::shuffle(q.options.begin(), q.options.end(), rng);
        m_questions.push_back(q);
    }

    if (db.getEuropaLeague().winner) {
        Question q;
        q.desc = "Journalist: 'Who won the Europa League this season?'";
        std::string correct = db.getEuropaLeague().winner->name;
        std::vector<std::string> wrong;
        for (const auto& l : db.getLeagues())
            for (const auto& c : l.clubs)
                if (c.name != correct && c.strength > 65) wrong.push_back(c.name);
        auto rng = std::default_random_engine(rand());
        std::shuffle(wrong.begin(), wrong.end(), rng);
        q.options.push_back({correct, true});
        q.options.push_back({wrong[0], false});
        q.options.push_back({wrong[1], false});
        std::shuffle(q.options.begin(), q.options.end(), rng);
        m_questions.push_back(q);
    }

    {
        Question q;
        q.desc = "Journalist: 'How many goals did you score this season?'";
        int correct = p->goals;
        int w1 = correct + 1 + rand() % 3;
        int w2 = correct - 1 - rand() % 3;
        if (w2 < 0) w2 = correct + 4 + rand() % 3;
        if (w1 == w2) w2++;
        q.options.push_back({std::to_string(correct), true});
        q.options.push_back({std::to_string(w1), false});
        q.options.push_back({std::to_string(w2), false});
        auto rng = std::default_random_engine(rand());
        std::shuffle(q.options.begin(), q.options.end(), rng);
        m_questions.push_back(q);
    }

    if (p->currentClub) {
        const League* myLg = nullptr;
        for (const auto& l : db.getLeagues())
            for (const auto& c : l.clubs)
                if (c.name == p->currentClub->name) { myLg = &l; break; }
        if (myLg) {
            std::vector<Club> sorted = myLg->clubs;
            std::sort(sorted.begin(), sorted.end(), [](const Club& a, const Club& b) {
                if (a.points != b.points) return a.points > b.points;
                int gdA = a.goalsFor - a.goalsAgainst, gdB = b.goalsFor - b.goalsAgainst;
                if (gdA != gdB) return gdA > gdB;
                return a.goalsFor > b.goalsFor;
            });
            Question q;
            q.desc = "Journalist: 'Which team won our league (" + myLg->name + ") this season?'";
            q.options.push_back({sorted[0].name, true});
            q.options.push_back({sorted[1].name, false});
            q.options.push_back({sorted[2].name, false});
            auto rng = std::default_random_engine(rand());
            std::shuffle(q.options.begin(), q.options.end(), rng);
            m_questions.push_back(q);
        }
    }
}

void InterviewScreen::startNextQuestion() {
    if (m_questionIndex >= (int)m_questions.size()) { finishInterview(); return; }
    auto& q = m_questions[m_questionIndex];
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

void InterviewScreen::applyOption(const EventOption& opt) {
    if (opt.isCorrect) m_correctAnswers++;
    m_questionIndex++;
    startNextQuestion();
}

void InterviewScreen::finishInterview() {
    m_isFinished = true;
    m_buttons.clear();
    m_hoverIdx = m_pressedIdx = -1;

    Player* p = m_gameManager->getPlayer();
    int total = (int)m_questions.size();
    float pct = total > 0 ? (float)m_correctAnswers / total : 0.0f;

    int xpBonus = 0, moraleBonus = 0;
    std::string res = "You got " + std::to_string(m_correctAnswers) + " of " + std::to_string(total) + " right.\n\n";
    if (pct == 1.0f)      { xpBonus = 500; moraleBonus = 20; res += "The press loved you! You know your football.\n(+500 XP, +20 Morale)"; }
    else if (pct >= 0.5f) { xpBonus = 200; moraleBonus = 10; res += "A solid interview.\n(+200 XP, +10 Morale)"; }
    else                  { moraleBonus = -10; res += "The fans are questioning your focus...\n(-10 Morale)"; }

    p->experience += xpBonus;
    p->morale = std::clamp(p->morale + moraleBonus, 0, 100);
    m_desc = res;

    Button b;
    b.bounds = sf::FloatRect(440.f, 500.f, 400.f, 60.f);
    b.label = "Proceed to Season Summary";
    m_buttons.push_back(b);
}

void InterviewScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
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
            if (m_isFinished) m_gameManager->changeScreen(std::make_shared<AwardsScreen>());
            else applyOption(m_buttons[rel].option);
        }
        m_pressedIdx = -1;
    }
}

void InterviewScreen::update(sf::Time deltaTime) {
    if (m_isFinished) return;
    m_timeRemaining -= deltaTime.asSeconds();
    if (m_timeRemaining <= 0.0f) {
        m_timeRemaining = 0.0f;
        applyOption({"Time Out", false});
    }
}

void InterviewScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 44.f}, "Season Interview", 40);

    if (!m_isFinished) {
        // Countdown bar.
        float ratio = std::clamp(m_timeRemaining / m_maxTime, 0.f, 1.f);
        window.draw(UIKit::roundedRect({240.f, 150.f}, {800.f, 12.f}, 6.f, UITheme::PanelPressed));
        sf::Color tc = ratio > 0.5f ? sf::Color(90, 210, 120) : ratio > 0.2f ? UITheme::Highlight : sf::Color(230, 90, 90);
        if (ratio > 0.01f) window.draw(UIKit::roundedRect({240.f, 150.f}, {800.f * ratio, 12.f}, 6.f, tc));

        // Question.
        UIKit::drawPanel(window, {240.f, 190.f, 800.f, 96.f});
        UIKit::drawText(window, font, {268.f, 224.f}, m_desc, 20, UITheme::TextWhite, 1.0f);
    } else {
        UIKit::drawPanel(window, {240.f, 200.f, 800.f, 240.f});
        UIKit::drawText(window, font, {272.f, 232.f}, "INTERVIEW COMPLETE", 18, UITheme::Accent, 2.0f, true);
        UIKit::drawText(window, font, {272.f, 280.f}, m_desc, 20, UITheme::TextWhite, 1.0f);
    }

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, m_buttons[i].bounds, m_buttons[i].label, st);
    }
}
