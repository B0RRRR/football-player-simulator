#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class SettingsScreen : public Screen {
public:
    SettingsScreen();

    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class RowKind { Option, Action };
    struct Row {
        sf::FloatRect bounds;
        std::string action;  // identifier
        std::string label;   // shown on the left
        RowKind kind;
    };

    std::vector<Row> m_rows;
    int m_hoverIdx = -1;
    int m_pressedIdx = -1;
    float m_saveFlash = 0.f;   // brief "Saved" confirmation timer

    std::string currentValue(const std::string& action) const; // right-hand value for a row
    void activate(const std::string& action);
    void cycleResolution();
};
