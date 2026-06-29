import os

with open("src/MatchScreen.h", "r") as f:
    header = f.read()
    
# We will add GoalKick to VisualState
header = header.replace('WaitingForMinigame\n};', 'WaitingForMinigame,\n    GoalKick\n};')

with open("src/MatchScreen.h", "w") as f:
    f.write(header)

print("MatchScreen.h updated.")
