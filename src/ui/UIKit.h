#pragma once
#include <SFML/Graphics.hpp>
#include "UITheme.h"
#include "Settings.h"   // g_renderScale
#include <string>
#include <cmath>
#include <algorithm>

// Shared "broadcast" UI kit - the menu/screen analogue of PitchRenderer. Every screen draws its
// chrome (background, titles, buttons, panels) through here, so the whole game shares one look
// and a restyle is a one-file change. Header-only, no external art: everything is procedural
// SFML shapes, styled after football TV graphics (dark navy panels, bright accent bars).
namespace UIKit {

    enum class BtnState { Normal, Hover, Pressed };

    // SFML has no rounded rectangle, so build one as a ConvexShape (four quarter-circle corners).
    inline sf::ConvexShape roundedRect(sf::Vector2f pos, sf::Vector2f size, float radius,
                                       sf::Color fill, float outline = 0.f,
                                       sf::Color outlineCol = sf::Color::Transparent) {
        radius = std::max(0.f, std::min(radius, std::min(size.x, size.y) * 0.5f));
        const int per = 6; // points per corner
        struct Corner { sf::Vector2f c; float a0; };
        Corner corners[4] = {
            {{size.x - radius, radius},          -90.f}, // top-right
            {{size.x - radius, size.y - radius},   0.f}, // bottom-right
            {{radius,          size.y - radius},  90.f}, // bottom-left
            {{radius,          radius},          180.f}, // top-left
        };
        sf::ConvexShape s;
        s.setPointCount(per * 4);
        int idx = 0;
        for (auto& cn : corners) {
            for (int i = 0; i < per; ++i) {
                float deg = cn.a0 + 90.f * (float)i / (per - 1);
                float rad = deg * 3.14159265f / 180.f;
                s.setPoint(idx++, cn.c + sf::Vector2f(std::cos(rad) * radius, std::sin(rad) * radius));
            }
        }
        s.setFillColor(fill);
        if (outline > 0.f) { s.setOutlineThickness(outline); s.setOutlineColor(outlineCol); }
        s.setPosition(pos);
        return s;
    }

    // Build a text laid out in DESIGN coordinates but rasterised at the REAL screen size, so it
    // stays crisp when the 1280x720 canvas is scaled up to fill the monitor. We ask the font for
    // glyphs at designSize x renderScale, then shrink the object by 1/renderScale - net on-screen
    // size is unchanged, but the glyph atlas now matches the pixels it's drawn on.
    inline sf::Text crispText(const sf::Font& font, const std::string& s, unsigned designSize) {
        float sc = (g_renderScale > 0.1f) ? g_renderScale : 1.f;
        sf::Text t;
        t.setFont(font);
        t.setString(UITheme::u8(s));
        t.setCharacterSize((unsigned)std::lround(designSize * sc));
        t.setScale(1.f / sc, 1.f / sc);
        return t;
    }

    // Convenience: a crisp label at a design-space position.
    inline void drawText(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f pos,
                         const std::string& s, unsigned designSize, sf::Color color,
                         float letterSpacing = 1.f, bool bold = false) {
        sf::Text t = crispText(font, s, designSize);
        t.setFillColor(color);
        t.setLetterSpacing(letterSpacing);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setPosition(pos);
        window.draw(t);
    }

    // Uppercase ASCII (titles read as broadcast lower-thirds); leaves any non-ASCII byte alone.
    inline std::string upper(const std::string& s) {
        std::string r = s;
        for (char& c : r) if (c >= 'a' && c <= 'z') c = char(c - 32);
        return r;
    }

    // Full-window background: deep gradient + a broadcast top bar + a faint corner graphic.
    inline void drawBackground(sf::RenderWindow& window) {
        UITheme::drawGradientBackground(window);
        sf::Vector2f sz = window.getView().getSize();

        // Faint diagonal accent slabs in the top-right corner - the "TV graphic" flourish.
        for (int i = 0; i < 3; ++i) {
            float x = sz.x - 260.f + i * 90.f;
            sf::ConvexShape slab;
            slab.setPointCount(4);
            slab.setPoint(0, {x, 0.f});
            slab.setPoint(1, {x + 55.f, 0.f});
            slab.setPoint(2, {x + 55.f - 150.f, 220.f});
            slab.setPoint(3, {x - 150.f, 220.f});
            sf::Uint8 a = (i == 1) ? 26 : 14;
            slab.setFillColor(sf::Color(UITheme::Accent.r, UITheme::Accent.g, UITheme::Accent.b, a));
            window.draw(slab);
        }

        // Top accent rail.
        sf::RectangleShape rail({sz.x, 5.f});
        rail.setPosition(0.f, 0.f);
        rail.setFillColor(UITheme::AccentDim);
        window.draw(rail);
        sf::RectangleShape railBright({220.f, 5.f});
        railBright.setPosition(0.f, 0.f);
        railBright.setFillColor(UITheme::Accent);
        window.draw(railBright);
    }

    // Big broadcast title: an accent block to the left, uppercase text, an accent underline.
    inline void drawTitle(sf::RenderWindow& window, sf::Font& font, sf::Vector2f pos,
                          const std::string& text, unsigned size = 46) {
        sf::RectangleShape bar({8.f, (float)size * 0.98f});
        bar.setPosition(pos.x - 24.f, pos.y + size * 0.16f);
        bar.setFillColor(UITheme::Accent);
        window.draw(bar);

        sf::Text t = crispText(font, upper(text), size);
        t.setFillColor(UITheme::TextWhite);
        t.setLetterSpacing(1.15f);
        t.setStyle(sf::Text::Bold);
        t.setPosition(pos);
        window.draw(t);

        sf::FloatRect b = t.getGlobalBounds(); // design-space width (scale already applied)
        sf::RectangleShape underline({b.width + 12.f, 3.f});
        underline.setPosition(pos.x, pos.y + size + 16.f);
        underline.setFillColor(UITheme::AccentDim);
        window.draw(underline);
    }

    // Shared body for the row-style widgets (button / option row): dark panel + a left accent
    // bar that grows and brightens on hover. Returns the bar width so callers can indent text.
    namespace detail {
        inline float rowBase(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size,
                             BtnState state) {
            sf::Color panel = (state == BtnState::Hover)   ? UITheme::PanelHover
                            : (state == BtnState::Pressed) ? UITheme::PanelPressed
                                                           : UITheme::PanelDark;
            window.draw(roundedRect(pos, size, 7.f, panel));

            float barW = (state == BtnState::Normal) ? 6.f : 12.f;
            sf::Color barCol = (state == BtnState::Normal) ? UITheme::AccentDim : UITheme::Accent;
            window.draw(roundedRect(pos, {barW + 6.f, size.y}, 7.f, barCol)); // rounded left cap
            sf::RectangleShape barFill({barW, size.y - 4.f});
            barFill.setPosition(pos.x + 2.f, pos.y + 2.f);
            barFill.setFillColor(barCol);
            window.draw(barFill);
            return barW;
        }
    }

    // A broadcast menu button: dark panel, a left accent bar that grows on hover, the label,
    // and a chevron highlight when active.
    inline void drawButton(sf::RenderWindow& window, sf::Font& font, sf::FloatRect bounds,
                           const std::string& label, BtnState state) {
        sf::Vector2f pos(bounds.left, bounds.top), size(bounds.width, bounds.height);
        float barW = detail::rowBase(window, pos, size, state);

        const unsigned labelSize = 23;
        sf::Text t = crispText(font, label, labelSize);
        t.setFillColor(state == BtnState::Normal ? UITheme::TextDim : UITheme::TextWhite);
        t.setLetterSpacing(1.05f);
        t.setPosition(pos.x + barW + 20.f, pos.y + size.y * 0.5f - labelSize * 0.72f);
        window.draw(t);

        if (state != BtnState::Normal) {
            sf::Text chev = crispText(font, ">", 24);
            chev.setStyle(sf::Text::Bold);
            chev.setFillColor(UITheme::Accent);
            chev.setPosition(pos.x + size.x - 34.f, pos.y + size.y * 0.5f - 24 * 0.72f);
            window.draw(chev);
        }
    }

    // A settings row: label on the left, the current value on the right in the accent colour.
    // Clicking it cycles/toggles the value (the screen decides what that means).
    inline void drawOptionRow(sf::RenderWindow& window, sf::Font& font, sf::FloatRect bounds,
                              const std::string& label, const std::string& value, BtnState state) {
        sf::Vector2f pos(bounds.left, bounds.top), size(bounds.width, bounds.height);
        float barW = detail::rowBase(window, pos, size, state);

        const unsigned labelSize = 21;
        sf::Text l = crispText(font, label, labelSize);
        l.setFillColor(state == BtnState::Normal ? UITheme::TextDim : UITheme::TextWhite);
        l.setLetterSpacing(1.05f);
        l.setPosition(pos.x + barW + 20.f, pos.y + size.y * 0.5f - labelSize * 0.72f);
        window.draw(l);

        sf::Text v = crispText(font, value, labelSize);
        v.setStyle(sf::Text::Bold);
        v.setFillColor(state == BtnState::Normal ? UITheme::TextWhite : UITheme::Accent);
        float vw = v.getGlobalBounds().width;
        v.setPosition(pos.x + size.x - 26.f - vw, pos.y + size.y * 0.5f - labelSize * 0.72f);
        window.draw(v);
    }

    // Forward declaration - drawIcon is defined lower down but used by the row helper here.
    inline void drawIcon(sf::RenderWindow& window, const std::string& kind, sf::Vector2f c,
                         float s, sf::Color col);

    // A row with a leading stat icon, a label, and (optionally) a right-aligned value in the
    // accent colour. Reusable for upgrade lists, status lines, anything "icon + text + value".
    inline void drawIconRow(sf::RenderWindow& window, sf::Font& font, sf::FloatRect bounds,
                            const std::string& iconKind, sf::Color iconCol,
                            const std::string& label, const std::string& value, BtnState state) {
        sf::Vector2f pos(bounds.left, bounds.top), size(bounds.width, bounds.height);
        float barW = detail::rowBase(window, pos, size, state);

        float ix = pos.x + barW + 22.f;
        if (!iconKind.empty())
            drawIcon(window, iconKind, {ix, pos.y + size.y * 0.5f}, 9.f, iconCol);

        const unsigned labelSize = 21;
        sf::Text l = crispText(font, label, labelSize);
        l.setFillColor(state == BtnState::Normal ? UITheme::TextDim : UITheme::TextWhite);
        l.setLetterSpacing(1.05f);
        l.setPosition(ix + 20.f, pos.y + size.y * 0.5f - labelSize * 0.72f);
        window.draw(l);

        if (!value.empty()) {
            sf::Text v = crispText(font, value, labelSize);
            v.setStyle(sf::Text::Bold);
            v.setFillColor(state == BtnState::Normal ? UITheme::TextWhite : UITheme::Accent);
            float vw = v.getGlobalBounds().width;
            v.setPosition(pos.x + size.x - 26.f - vw, pos.y + size.y * 0.5f - labelSize * 0.72f);
            window.draw(v);
        }
    }

    // A plain card/panel for grouping content.
    inline void drawPanel(sf::RenderWindow& window, sf::FloatRect bounds, bool accentEdge = true) {
        sf::Vector2f pos(bounds.left, bounds.top), size(bounds.width, bounds.height);
        window.draw(roundedRect(pos, size, 9.f, UITheme::PanelDark, 1.f,
                                sf::Color(255, 255, 255, 18)));
        if (accentEdge) {
            sf::RectangleShape edge({4.f, size.y - 20.f});
            edge.setPosition(pos.x, pos.y + 10.f);
            edge.setFillColor(UITheme::Accent);
            window.draw(edge);
        }
    }

    // A tiny procedurally-drawn stat icon, centred at c and fitting roughly a 2s box. No art
    // assets - just SFML primitives - so it scales and themes with everything else. `kind` is
    // one of: target, arrow, shield, weave, glove, smiley, bolt, star, coin, chart.
    inline void drawIcon(sf::RenderWindow& window, const std::string& kind, sf::Vector2f c,
                         float s, sf::Color col) {
        auto fillPoly = [&](std::initializer_list<sf::Vector2f> pts) {
            sf::ConvexShape sh; sh.setPointCount(pts.size());
            int i = 0; for (auto& p : pts) sh.setPoint(i++, c + sf::Vector2f(p.x * s, p.y * s));
            sh.setFillColor(col); window.draw(sh);
        };
        auto strip = [&](std::initializer_list<sf::Vector2f> pts) {
            sf::VertexArray v(sf::LineStrip, pts.size());
            int i = 0; for (auto& p : pts) { v[i].position = c + sf::Vector2f(p.x * s, p.y * s); v[i].color = col; ++i; }
            window.draw(v);
        };
        auto disc = [&](sf::Vector2f o, float r, sf::Color fill, float ol = 0.f, sf::Color olc = sf::Color::Transparent) {
            sf::CircleShape d(r * s); d.setOrigin(r * s, r * s); d.setPosition(c + sf::Vector2f(o.x * s, o.y * s));
            d.setFillColor(fill); if (ol > 0.f) { d.setOutlineThickness(ol * s); d.setOutlineColor(olc); }
            window.draw(d);
        };

        if (kind == "target") {
            disc({0, 0}, 0.9f, sf::Color::Transparent, 0.22f, col);
            disc({0, 0}, 0.28f, col);
        } else if (kind == "arrow") {
            fillPoly({{-0.75f,-0.2f},{0.15f,-0.2f},{0.15f,-0.5f},{0.8f,0.f},{0.15f,0.5f},{0.15f,0.2f},{-0.75f,0.2f}});
        } else if (kind == "shield") {
            fillPoly({{-0.62f,-0.6f},{0.62f,-0.6f},{0.62f,0.05f},{0.f,0.7f},{-0.62f,0.05f}});
        } else if (kind == "weave") {
            strip({{-0.8f,0.45f},{-0.4f,-0.45f},{0.f,0.45f},{0.4f,-0.45f},{0.8f,0.45f}});
            disc({0.8f, 0.45f}, 0.16f, col);
        } else if (kind == "glove") {
            fillPoly({{-0.45f,0.7f},{-0.45f,-0.2f},{-0.2f,-0.7f},{0.05f,-0.2f},{0.05f,-0.55f},
                      {0.3f,-0.55f},{0.3f,0.f},{0.55f,0.f},{0.55f,0.7f}});
        } else if (kind == "smiley") {
            disc({0, 0}, 0.9f, sf::Color::Transparent, 0.16f, col);
            disc({-0.32f,-0.2f}, 0.13f, col); disc({0.32f,-0.2f}, 0.13f, col);
            strip({{-0.38f,0.18f},{-0.12f,0.42f},{0.12f,0.42f},{0.38f,0.18f}});
        } else if (kind == "bolt") {
            fillPoly({{0.15f,-0.75f},{-0.4f,0.12f},{-0.05f,0.12f},{-0.18f,0.75f},{0.42f,-0.12f},{0.05f,-0.12f}});
        } else if (kind == "star") {
            sf::ConvexShape st; st.setPointCount(10);
            for (int i = 0; i < 10; ++i) {
                float rr = (i % 2 == 0) ? 0.95f : 0.42f;
                float a = (-90.f + i * 36.f) * 3.14159265f / 180.f;
                st.setPoint(i, c + sf::Vector2f(std::cos(a) * rr * s, std::sin(a) * rr * s));
            }
            st.setFillColor(col); window.draw(st);
        } else if (kind == "coin") {
            disc({0, 0}, 0.9f, col);
            sf::Color dk(col.r * 6 / 10, col.g * 6 / 10, col.b * 6 / 10);
            disc({0, 0}, 0.58f, sf::Color::Transparent, 0.14f, dk);
            sf::RectangleShape stem(sf::Vector2f(0.16f * s, 1.1f * s));
            stem.setOrigin(0.08f * s, 0.55f * s); stem.setPosition(c); stem.setFillColor(dk);
            window.draw(stem);
        } else if (kind == "chart") {
            for (int i = 0; i < 3; ++i) {
                float h = 0.5f + i * 0.35f;
                sf::RectangleShape bar(sf::Vector2f(0.34f * s, h * s));
                bar.setOrigin(0.17f * s, h * s);
                bar.setPosition(c + sf::Vector2f((-0.5f + i * 0.5f) * s, 0.6f * s));
                bar.setFillColor(col); window.draw(bar);
            }
        }
    }

    // Hit-test helper for a rect against a mapped mouse position.
    inline bool hit(sf::FloatRect bounds, sf::Vector2f mouse) {
        return bounds.contains(mouse);
    }

}
