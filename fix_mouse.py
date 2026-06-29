import os, glob

for f in glob.glob('src/*Screen.cpp'):
    if "SettingsScreen.cpp" in f: continue
    
    with open(f, 'r') as file:
        content = file.read()
    
    content = content.replace(
        'sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));',
        'sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);'
    )
    content = content.replace(
        'sf::Vector2f mousePos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));',
        'sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);'
    )
    
    # Also add UITheme includes if not present
    if "UITheme.h" not in content:
        content = content.replace('#include <SFML/Graphics.hpp>', '#include <SFML/Graphics.hpp>\n#include "UITheme.h"')
        content = content.replace('#include "Screen.h"', '#include "Screen.h"\n#include "UITheme.h"')
    
    with open(f, 'w') as file:
        file.write(content)
print("Done fixing mouse coordinates.")
