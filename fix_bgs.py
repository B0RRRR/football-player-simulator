import os, glob, re

for f in glob.glob('src/*Screen.cpp'):
    if "SettingsScreen.cpp" in f: continue
    
    with open(f, 'r') as file:
        content = file.read()
    
    # Replace window.clear(...) with UITheme::drawGradientBackground(window);
    content = re.sub(r'window\.clear\([^)]*\);', 'UITheme::drawGradientBackground(window);', content)
    
    with open(f, 'w') as file:
        file.write(content)
print("Done fixing backgrounds.")
