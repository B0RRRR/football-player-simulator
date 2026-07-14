#include "UITheme.h"
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
#include <iostream>

InterviewScreen::InterviewScreen() {
}

void InterviewScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::Yellow);
    m_titleText.setPosition(100.f, 50.f);
    m_titleText.setString("End of Season Interview");
    
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

    // 1. Champions League Winner
    if (db.getChampionsLeague().winner) {
        Question q;
        q.desc = "Journalist: 'Who won the Champions League this season?'";
        std::string correct = db.getChampionsLeague().winner->name;
        
        // Find 2 random wrong clubs
        std::vector<std::string> wrong;
        for (const auto& l : db.getLeagues()) {
            for (const auto& c : l.clubs) {
                if (c.name != correct && c.strength > 75) wrong.push_back(c.name);
            }
        }
        auto rng = std::default_random_engine(rand());
        std::shuffle(wrong.begin(), wrong.end(), rng);
        
        q.options.push_back({correct, true});
        q.options.push_back({wrong[0], false});
        q.options.push_back({wrong[1], false});
        std::shuffle(q.options.begin(), q.options.end(), rng);
        m_questions.push_back(q);
    }

    // 2. Europa League Winner
    if (db.getEuropaLeague().winner) {
        Question q;
        q.desc = "Journalist: 'Who won the Europa League this season?'";
        std::string correct = db.getEuropaLeague().winner->name;
        
        std::vector<std::string> wrong;
        for (const auto& l : db.getLeagues()) {
            for (const auto& c : l.clubs) {
                if (c.name != correct && c.strength > 65) wrong.push_back(c.name);
            }
        }
        auto rng = std::default_random_engine(rand());
        std::shuffle(wrong.begin(), wrong.end(), rng);
        
        q.options.push_back({correct, true});
        q.options.push_back({wrong[0], false});
        q.options.push_back({wrong[1], false});
        std::shuffle(q.options.begin(), q.options.end(), rng);
        m_questions.push_back(q);
    }

    // 3. Player's Goals
    {
        Question q;
        q.desc = "Journalist: 'How many goals did you score this season?'";
        int correct = p->goals;
        int w1 = correct + 1 + rand() % 3;
        int w2 = correct - 1 - rand() % 3;
        
        // If w2 goes below 0, make it a larger number instead
        if (w2 < 0) {
            w2 = correct + 4 + rand() % 3;
        }
        
        // Ensure w1 and w2 are strictly unique
        if (w1 == w2) w2++;
        
        q.options.push_back({std::to_string(correct), true});
        q.options.push_back({std::to_string(w1), false});
        q.options.push_back({std::to_string(w2), false});
        auto rng = std::default_random_engine(rand());
        std::shuffle(q.options.begin(), q.options.end(), rng);
        m_questions.push_back(q);
    }

    // 4. League Winner
    if (p->currentClub) {
        const League* myLg = nullptr;
        for (const auto& l : db.getLeagues()) {
            for (const auto& c : l.clubs) {
                if (c.name == p->currentClub->name) {
                    myLg = &l; break;
                }
            }
        }
        if (myLg) {
            std::vector<Club> sorted = myLg->clubs;
            std::sort(sorted.begin(), sorted.end(), [](const Club& a, const Club& b) {
                if (a.points != b.points) return a.points > b.points;
                int gdA = a.goalsFor - a.goalsAgainst;
                int gdB = b.goalsFor - b.goalsAgainst;
                if (gdA != gdB) return gdA > gdB;
                return a.goalsFor > b.goalsFor;
            });
            
            Question q;
            q.desc = "Journalist: 'Which team won our league (" + myLg->name + ") this season?'";
            std::string correct = sorted[0].name;
            std::string w1 = sorted[1].name;
            std::string w2 = sorted[2].name;
            
            q.options.push_back({correct, true});
            q.options.push_back({w1, false});
            q.options.push_back({w2, false});
            auto rng = std::default_random_engine(rand());
            std::shuffle(q.options.begin(), q.options.end(), rng);
            m_questions.push_back(q);
        }
    }
}

void InterviewScreen::startNextQuestion() {
    if (m_questionIndex >= m_questions.size()) {
        finishInterview();
        return;
    }

    auto& q = m_questions[m_questionIndex];
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

void InterviewScreen::applyOption(const EventOption& opt) {
    if (opt.isCorrect) m_correctAnswers++;
    
    m_questionIndex++;
    startNextQuestion();
}

void InterviewScreen::finishInterview() {
    m_isFinished = true;
    m_buttons.clear();

    Player* p = m_gameManager->getPlayer();
    
    int total = m_questions.size();
    float pct = total > 0 ? (float)m_correctAnswers / total : 0.0f;
    
    int xpBonus = 0;
    int moraleBonus = 0;
    
    std::string res = "Interview Finished!\n\nYou got " + std::to_string(m_correctAnswers) + " out of " + std::to_string(total) + " right.\n\n";
    
    if (pct == 1.0f) {
        xpBonus = 500;
        moraleBonus = 20;
        res += "The press loved you! You know your football.\n(+500 XP, +20 Morale)";
    } else if (pct >= 0.5f) {
        xpBonus = 200;
        moraleBonus = 10;
        res += "A solid interview.\n(+200 XP, +10 Morale)";
    } else {
        moraleBonus = -10;
        res += "The fans are questioning your focus...\n(-10 Morale)";
    }
    
    p->experience += xpBonus;
    p->morale += moraleBonus;
    if (p->morale > 100) p->morale = 100;
    if (p->morale < 0) p->morale = 0;

    m_descriptionText.setString(res);

    auto& font = AssetManager::get().getFont("MainFont");
    Button btn;
    btn.rect.setSize(sf::Vector2f(400.f, 60.f));
    btn.rect.setPosition(100.f, 500.f);
    btn.rect.setFillColor(UITheme::ButtonNormal);
    btn.text.setFont(font);
    btn.text.setString("Proceed to Season Summary");
    btn.text.setCharacterSize(24);
    btn.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = btn.text.getLocalBounds();
    btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                         btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
    m_buttons.push_back(btn);
}

void InterviewScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (m_isFinished) {
                    m_gameManager->changeScreen(std::make_shared<AwardsScreen>());
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

void InterviewScreen::update(sf::Time deltaTime) {
    if (m_isFinished) return;

    m_timeRemaining -= deltaTime.asSeconds();
    if (m_timeRemaining <= 0.0f) {
        m_timeRemaining = 0.0f;
        EventOption failOpt = {"Time Out", false};
        applyOption(failOpt);
    }

    float ratio = m_timeRemaining / m_maxTime;
    if (ratio < 0.0f) ratio = 0.0f;
    m_timerBar.setSize(sf::Vector2f(1080.f * ratio, 20.f));
    
    if (ratio > 0.5f) m_timerBar.setFillColor(sf::Color::Green);
    else if (ratio > 0.2f) m_timerBar.setFillColor(sf::Color::Yellow);
    else m_timerBar.setFillColor(sf::Color::Red);
}

void InterviewScreen::draw(sf::RenderWindow& window) {
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
