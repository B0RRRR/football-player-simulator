import os

with open("src/MatchScreen.h", "r") as f:
    header = f.read()
    
header = header.replace('std::vector<std::string> m_visibleLogs;', 'std::vector<MatchEvent> m_visibleLogs;')

# Also add a state for the 2D Engine
header = header.replace('sf::Vector2f m_ballTarget;', 'sf::Vector2f m_ballTarget;\n    EventType m_currentVisualEvent;\n    bool m_eventHomeAdvantage;\n    float m_visualEventTimer;')

with open("src/MatchScreen.h", "w") as f:
    f.write(header)

print("MatchScreen.h updated.")
