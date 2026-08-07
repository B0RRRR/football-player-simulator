#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class InterviewScreen : public Screen {
public:
    InterviewScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    std::string m_desc; // current question, or the result summary when finished

    struct EventOption {
        std::string text;
        bool isCorrect = false;
    };

    struct Question {
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
    std::vector<Question> m_questions;
    int m_questionIndex = 0;
    
    float m_timeRemaining = 0.0f;
    float m_maxTime = 10.0f;

    int m_correctAnswers = 0;
    bool m_isFinished = false;

    void generateQuestions();
    void startNextQuestion();
    void applyOption(const EventOption& opt);
    void finishInterview();
};
