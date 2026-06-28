import os, glob
for f in glob.glob('src/*.cpp') + glob.glob('src/*.h'):
    with open(f, 'r') as file:
        content = file.read()
    
    # Fix vector float casts
    content = content.replace('sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);', 'sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));')
    content = content.replace('sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);', 'sf::Vector2f mousePos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));')
    
    # Fix size_t to int conversions in CareerManager.cpp
    if "CareerManager.cpp" in f:
        content = content.replace('int n = lg->clubs.size();', 'int n = static_cast<int>(lg->clubs.size());')
        content = content.replace('int n = league->clubs.size();', 'int n = static_cast<int>(league->clubs.size());')
        content = content.replace('int n = l.clubs.size();', 'int n = static_cast<int>(l.clubs.size());')
        
    # Fix struct/class Club forward declarations
    if "TransferScreen.h" in f:
        content = content.replace('class Club;', 'struct Club;')
    if "Player.h" in f:
        content = content.replace('class Club;', 'struct Club;')
        
    # Fix size_t to int in CareerHubScreen.cpp
    if "CareerHubScreen.cpp" in f:
        content = content.replace('int n = lg->clubs.size();', 'int n = static_cast<int>(lg->clubs.size());')
    
    with open(f, 'w') as file:
        file.write(content)
print("Done fixing compiler warnings.")
