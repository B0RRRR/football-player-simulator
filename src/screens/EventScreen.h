#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class EventScreen : public Screen {
public:
    EventScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    std::string m_title = "Event";
    std::string m_desc;

    struct EventOption {
        std::string text;
        int xpChange = 0;
        int moraleChange = 0;
        int energyChange = 0;
        float trustChange = 0.0f;
        int moneyChange = 0;
    };

    struct EventQuestion {
        std::string desc;
        std::vector<EventOption> options;
    };

    struct Button {
        sf::FloatRect bounds;
        std::string label;
        EventOption option;
    };

    std::vector<Button> m_buttons;
    int m_hoverIdx = -1;
    int m_pressedIdx = -1;
    std::vector<EventQuestion> m_currentQuestions;
    int m_questionIndex = 0;
    
    float m_timeRemaining = 0.0f;
    float m_maxTime = 0.0f;

    // Accumulated rewards
    int m_accXp = 0;
    int m_accMorale = 0;
    int m_accEnergy = 0;
    float m_accTrust = 0.0f;
    int m_accMoney = 0;

    bool m_isFinished = false;

    void startNextQuestion();
    void applyOption(const EventOption& opt);
    void finishEvent();
};
