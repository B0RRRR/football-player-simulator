import os

with open("src/MatchEngine.h", "r") as f:
    header = f.read()
    
# We will add PendingMinigame to EventType
header = header.replace('Card\n};', 'Card,\n    PendingMinigame\n};')

# We add commitEvent and triggerMinigame to public methods
header = header.replace('void updateMinute();', 'void updateMinute();\n    void commitEvent(const MatchEvent& event);\n    void triggerMinigame();')

with open("src/MatchEngine.h", "w") as f:
    f.write(header)

print("MatchEngine.h updated.")
