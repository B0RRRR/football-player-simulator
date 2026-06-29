import glob
import re

for filepath in glob.glob('src/*Screen.cpp'):
    with open(filepath, 'r') as f:
        content = f.read()

    # Add include if missing
    if '#include "UITheme.h"' not in content:
        content = content.replace('#include "Screen.h"', '#include "Screen.h"\n#include "UITheme.h"')
    
    # 1. Replace all window.clear(...) variants
    # Match window.clear( anything inside balanced parens );
    # A simple regex for window.clear( ... ) that might contain nested parens:
    content = re.sub(r'window\.clear\([^;]+\);', 'UITheme::drawGradientBackground(window);', content)

    # 2. Replace button colors
    # btn.rect.setFillColor(sf::Color(100, 100, 100)); -> btn.rect.setFillColor(UITheme::ButtonNormal);
    content = re.sub(r'btn\.rect\.setFillColor\(sf::Color\([^\)]+\)\);', 'btn.rect.setFillColor(UITheme::ButtonNormal);', content)
    
    # btn.rect.setFillColor(sf::Color(150, 150, 150)); -> btn.rect.setFillColor(UITheme::ButtonHover);
    # Actually some are already using ButtonNormal/Hover from previous attempt, but some might have been missed if I didn't replace them everywhere.
    content = content.replace('sf::Color(150, 150, 150)', 'UITheme::ButtonHover')
    content = content.replace('sf::Color(100, 100, 100)', 'UITheme::ButtonNormal')
    
    # Also menu screen uses sf::Color(100, 100, 200) for hover sometimes, let's catch standard colors:
    # We will just manually fix MenuScreen if needed, but let's check basic ones.
    
    with open(filepath, 'w') as f:
        f.write(content)

print("UI Polish complete.")
