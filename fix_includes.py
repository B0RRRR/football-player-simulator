import glob

for filepath in glob.glob('src/*Screen.cpp'):
    with open(filepath, 'r') as f:
        content = f.read()

    if 'UITheme::' in content and '#include "UITheme.h"' not in content:
        # Find the first #include and insert after it
        content = content.replace('#include', '#include "UITheme.h"\n#include', 1)
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Fixed {filepath}")
