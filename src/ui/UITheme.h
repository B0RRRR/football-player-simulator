#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace UITheme {
    // Render a std::string as UTF-8, so a Cyrillic (or any non-ASCII) player name shows
    // correctly. sf::Text::setString(std::string) treats bytes as Latin-1, which mangles
    // multi-byte UTF-8. ASCII is unchanged, so this is safe to use everywhere.
    inline sf::String u8(const std::string& s) {
        return sf::String::fromUtf8(s.begin(), s.end());
    }

    // Number of UTF-8 code points (letters), not bytes - a Cyrillic letter is two bytes.
    inline size_t utf8Length(const std::string& s) {
        size_t n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    }

    // Append one Unicode code point to a UTF-8 std::string.
    inline void utf8Append(std::string& s, sf::Uint32 codepoint) {
        sf::String one(codepoint);
        std::basic_string<sf::Uint8> bytes = one.toUtf8();
        s.append(bytes.begin(), bytes.end());
    }

    // Color Palette
    const sf::Color BackgroundDark(15, 20, 35);
    const sf::Color BackgroundLight(35, 45, 75);
    const sf::Color ButtonNormal(50, 70, 110);
    const sf::Color ButtonHover(70, 100, 150);
    const sf::Color ButtonActive(100, 150, 200);
    const sf::Color TextWhite(240, 240, 245);
    const sf::Color TextDim(180, 190, 210);
    const sf::Color Highlight(255, 215, 0); // Gold for active/selection

    // Broadcast accent (the bright "TV graphics" colour) used by UIKit for accent bars,
    // hover states and title marks. Cyan reads as sporty/technical against the navy base.
    const sf::Color Accent(0, 190, 255);
    const sf::Color AccentDim(0, 110, 165);
    const sf::Color PanelDark(28, 38, 62);      // button/panel base
    const sf::Color PanelHover(48, 68, 108);    // hovered panel
    const sf::Color PanelPressed(20, 28, 46);   // pressed panel

    // Draw a vertical gradient background filling the window
    inline void drawGradientBackground(sf::RenderWindow& window) {
        sf::VertexArray bg(sf::Quads, 4);
        sf::Vector2f size(window.getView().getSize());
        
        bg[0].position = sf::Vector2f(0, 0);
        bg[1].position = sf::Vector2f(size.x, 0);
        bg[2].position = sf::Vector2f(size.x, size.y);
        bg[3].position = sf::Vector2f(0, size.y);

        bg[0].color = BackgroundDark;
        bg[1].color = BackgroundDark;
        bg[2].color = BackgroundLight;
        bg[3].color = BackgroundLight;

        window.draw(bg);
    }
}
