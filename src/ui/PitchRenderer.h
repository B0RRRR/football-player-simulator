#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

// Shared drawing for anything that happens ON the pitch - used by the match and by training
// drills so both look identical. THIS is the single place field art plugs into: players are
// drawn as small animated figures (facing + run cycle derived from their movement), the ball has
// a shadow and rolls. Change it here once and every screen that draws through PitchRenderer gets
// it, with no external art assets.
//
// World coordinates match the match: pitch rect 40,130 .. 840,450; goals at x~35 / ~845;
// goal mouth y 250..330. Everything is drawn in the caller's current view.
namespace PitchRenderer {

    inline void drawPitch(sf::RenderWindow& window) {
        const float L = 40.f, T = 130.f, R = 840.f, B = 450.f;
        const float W = R - L, H = B - T, cy = (T + B) * 0.5f, cx = (L + R) * 0.5f;
        const sf::Color lineCol(228, 236, 228, 190);

        // Base grass + mown vertical stripes.
        sf::RectangleShape base({W, H});
        base.setPosition(L, T);
        base.setFillColor(sf::Color(38, 124, 54));
        window.draw(base);
        const int stripes = 10;
        float sw = W / stripes;
        for (int i = 0; i < stripes; ++i) {
            if (i % 2) continue;
            sf::RectangleShape s({sw, H});
            s.setPosition(L + i * sw, T);
            s.setFillColor(sf::Color(46, 140, 64));
            window.draw(s);
        }

        auto outline = [&](float x, float y, float w, float h, float th = 2.f) {
            sf::RectangleShape r({w, h});
            r.setPosition(x, y);
            r.setFillColor(sf::Color::Transparent);
            r.setOutlineThickness(th);
            r.setOutlineColor(lineCol);
            window.draw(r);
        };
        auto spot = [&](float x, float y) {
            sf::CircleShape d(2.6f);
            d.setOrigin(2.6f, 2.6f);
            d.setPosition(x, y);
            d.setFillColor(lineCol);
            window.draw(d);
        };
        auto arc = [&](float ax, float ay, float r, float a0deg, float a1deg) {
            const int n = 18;
            sf::VertexArray v(sf::LineStrip, n);
            for (int i = 0; i < n; ++i) {
                float a = (a0deg + (a1deg - a0deg) * i / (n - 1)) * 3.14159265f / 180.f;
                v[i].position = {ax + std::cos(a) * r, ay + std::sin(a) * r};
                v[i].color = lineCol;
            }
            window.draw(v);
        };

        outline(L, T, W, H, 3.f);                 // boundary
        // Halfway line + centre circle + spot.
        sf::RectangleShape half({2.5f, H});
        half.setPosition(cx - 1.25f, T);
        half.setFillColor(lineCol);
        window.draw(half);
        sf::CircleShape cc(46.f);
        cc.setOrigin(46.f, 46.f);
        cc.setPosition(cx, cy);
        cc.setFillColor(sf::Color::Transparent);
        cc.setOutlineThickness(2.f);
        cc.setOutlineColor(lineCol);
        window.draw(cc);
        spot(cx, cy);

        // Penalty + six-yard boxes, penalty spots and arcs (both ends).
        const float paW = 110.f, paH = 180.f, gaW = 40.f, gaH = 80.f, penX = 84.f; // spot 11m out, matches the ball
        outline(L, cy - paH * 0.5f, paW, paH);
        outline(L, cy - gaH * 0.5f, gaW, gaH);
        spot(L + penX, cy);
        arc(L + penX, cy, 50.f, -38.f, 38.f);
        outline(R - paW, cy - paH * 0.5f, paW, paH);
        outline(R - gaW, cy - gaH * 0.5f, gaW, gaH);
        spot(R - penX, cy);
        arc(R - penX, cy, 50.f, 142.f, 218.f);

        // Goals (just outside each goal line) with a net grid.
        for (bool leftGoal : {true, false}) {
            float gx = leftGoal ? 10.f : R;        // left net 10..40, right net 840..870
            float gw = 30.f, gy = 250.f, gh = 80.f;
            sf::RectangleShape net({gw, gh});
            net.setPosition(gx, gy);
            net.setFillColor(sf::Color(255, 255, 255, 22));
            window.draw(net);
            for (float x = gx; x <= gx + gw + 0.5f; x += 6.f) {
                sf::RectangleShape v({1.f, gh});
                v.setPosition(x, gy);
                v.setFillColor(sf::Color(255, 255, 255, 55));
                window.draw(v);
            }
            for (float y = gy; y <= gy + gh + 0.5f; y += 10.f) {
                sf::RectangleShape hl({gw, 1.f});
                hl.setPosition(gx, y);
                hl.setFillColor(sf::Color(255, 255, 255, 55));
                window.draw(hl);
            }
            outline(gx, gy, gw, gh, 3.f);
        }
    }

    // --- Per-entity animation state (facing + run phase), keyed by a stable caller id. -------
    // `render` is the eased on-screen position (smoothing #6); it trails the engine's target so a
    // low-frequency tick can't teleport the figure.
    namespace detail {
        struct Anim { sf::Vector2f last, render; sf::Vector2f facing{1.f, 0.f}; float phase = 0.f; bool init = false; };
        inline std::map<int, Anim>& anim() { static std::map<int, Anim> m; return m; }
        // The single ball: eased position + a roll angle that advances only with travel.
        struct Ball { sf::Vector2f last, render; float roll = 0.f; bool init = false; };
        inline Ball& ball() { static Ball b; return b; }
    }

    // Forget all animation state - call when switching screens so identities don't carry over.
    inline void resetAnim() { detail::anim().clear(); detail::ball() = detail::Ball{}; }

    // An animated player figure at `pos` (its CENTRE). `id` must be stable per player across
    // frames so facing/run-cycle/smoothing can be tracked. `shirt` is the kit colour. When a
    // `number` (>=0) and `font` are given, the shirt number is drawn on the body.
    inline void drawPlayer(sf::RenderWindow& window, int id, sf::Vector2f pos, sf::Color shirt,
                           bool highlight = false, int number = -1, const sf::Font* font = nullptr) {
        detail::Anim& st = detail::anim()[id];
        if (!st.init) { st.render = pos; st.last = pos; st.init = true; }
        st.render += (pos - st.render) * 0.45f;                  // #6 ease toward the target
        sf::Vector2f vel = st.render - st.last;
        float speed = std::hypot(vel.x, vel.y);
        st.last = st.render;
        sf::Vector2f d = st.render;                              // draw at the smoothed position

        bool moving = speed > 0.35f;
        if (moving) {
            sf::Vector2f dir(vel.x / speed, vel.y / speed);
            st.facing += (dir - st.facing) * 0.35f;              // smooth turn toward heading
            float fl = std::hypot(st.facing.x, st.facing.y);
            if (fl > 0.01f) { st.facing.x /= fl; st.facing.y /= fl; }
            st.phase += std::min(speed, 6.f) * 0.5f;             // advance stride by distance moved
        } else {
            st.phase += 0.06f;                                   // gentle idle sway
        }
        sf::Vector2f f = st.facing;
        sf::Vector2f perp(-f.y, f.x);

        // #8 depth: figures lower on the pitch are "closer", so a touch larger (subtle perspective).
        float sc = 1.f + std::clamp((d.y - 290.f) / 160.f, -1.f, 1.f) * 0.14f;
        float bob = (moving ? 1.4f : 0.4f) * std::sin(st.phase) * sc;
        float stride = (moving ? std::sin(st.phase) * 3.6f : 0.f) * sc;

        // Shadow.
        sf::CircleShape shadow(7.f * sc);
        shadow.setScale(1.15f, 0.55f);
        shadow.setOrigin(7.f * sc, 7.f * sc);
        shadow.setPosition(d.x + 2.f, d.y + 6.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 70));
        window.draw(shadow);

        // Legs (swing fore/aft along the facing axis).
        for (int s = -1; s <= 1; s += 2) {
            sf::Vector2f lp = d + perp * (3.2f * sc * (float)s) + f * (stride * (float)s);
            sf::CircleShape leg(2.3f * sc);
            leg.setOrigin(2.3f * sc, 2.3f * sc);
            leg.setPosition(lp.x, lp.y + 3.f * sc);
            leg.setFillColor(sf::Color(18, 22, 36));
            window.draw(leg);
        }

        // Highlight ring (drawn under the body so the figure sits on top).
        if (highlight) {
            sf::CircleShape hl(11.5f * sc);
            hl.setOrigin(11.5f * sc, 11.5f * sc);
            hl.setPosition(d.x, d.y - bob);
            hl.setFillColor(sf::Color::Transparent);
            hl.setOutlineThickness(2.5f);
            hl.setOutlineColor(sf::Color::Yellow);
            window.draw(hl);
        }

        // Body.
        sf::CircleShape body(7.f * sc);
        body.setOrigin(7.f * sc, 7.f * sc);
        body.setPosition(d.x, d.y - bob);
        body.setFillColor(shirt);
        body.setOutlineThickness(1.5f);
        body.setOutlineColor(sf::Color(0, 0, 0, 90));
        window.draw(body);

        // Head, nudged toward the facing direction.
        sf::CircleShape head(3.5f * sc);
        head.setOrigin(3.5f * sc, 3.5f * sc);
        head.setPosition(d.x + f.x * 3.6f * sc, d.y - bob + f.y * 3.6f * sc);
        head.setFillColor(sf::Color(240, 205, 175));
        window.draw(head);

        // #2 shirt number, in a colour that contrasts the kit, on top of the figure.
        if (number >= 0 && font) {
            sf::Text t(std::to_string(number), *font, 14);
            float lum = 0.299f * shirt.r + 0.587f * shirt.g + 0.114f * shirt.b;
            t.setFillColor(lum > 140.f ? sf::Color(20, 22, 30) : sf::Color(245, 246, 250));
            sf::FloatRect lb = t.getLocalBounds();
            t.setOrigin(lb.left + lb.width * 0.5f, lb.top + lb.height * 0.5f);
            t.setScale(0.62f * sc, 0.62f * sc);
            t.setPosition(d.x, d.y - bob);
            window.draw(t);
        }
    }

    // The ball at ground position `pos`; `height` (px) lifts it above its shadow for aerial balls
    // (#5). The roll spot (#4) advances only when the ball actually travels, so a still ball doesn't
    // spin.
    inline void drawBall(sf::RenderWindow& window, sf::Vector2f pos, float height = 0.f) {
        detail::Ball& b = detail::ball();
        if (!b.init) { b.render = pos; b.last = pos; b.init = true; }
        b.render += (pos - b.render) * 0.5f;                     // #6 smoothing (matches the players)
        sf::Vector2f vel = b.render - b.last;
        float dist = std::hypot(vel.x, vel.y);
        b.last = b.render;
        b.roll += dist * 0.18f;                                  // #4 spin tied to distance travelled

        if (height < 0.f) height = 0.f;
        sf::Vector2f ground = b.render;
        sf::Vector2f centre(ground.x, ground.y - height);
        float hf = std::min(height / 40.f, 1.f);

        // Shadow stays on the grass and shrinks/fades as the ball climbs, selling the height.
        sf::CircleShape shadow(4.f);
        shadow.setScale(1.25f * (1.f - 0.35f * hf), 0.6f * (1.f - 0.35f * hf));
        shadow.setOrigin(4.f, 4.f);
        shadow.setPosition(ground.x + 1.f + height * 0.12f, ground.y + 4.f);
        shadow.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)(70.f * (1.f - 0.5f * hf))));
        window.draw(shadow);

        float r = 4.5f + hf * 1.6f;                              // a touch larger when lofted (closer)
        sf::CircleShape ball(r);
        ball.setOrigin(r, r);
        ball.setPosition(centre);
        ball.setFillColor(sf::Color::White);
        ball.setOutlineThickness(1.f);
        ball.setOutlineColor(sf::Color(40, 40, 50));
        window.draw(ball);

        sf::CircleShape spot(1.4f);
        spot.setOrigin(1.4f, 1.4f);
        spot.setPosition(centre.x + std::cos(b.roll) * 1.7f, centre.y + std::sin(b.roll) * 1.7f);
        spot.setFillColor(sf::Color(30, 30, 45));
        window.draw(spot);
    }

    // The drawn shot/pass path (a coloured trace). Default is the yellow shot colour; the
    // match passes cyan for a pass and a dimmer alpha once the ball is in flight.
    inline void drawPath(sf::RenderWindow& window, const std::vector<sf::Vector2f>& path,
                         sf::Color color = sf::Color(255, 230, 90, 210)) {
        if (path.size() < 2) return;
        sf::VertexArray line(sf::LineStrip, path.size());
        for (std::size_t s = 0; s < path.size(); ++s) {
            line[s].position = path[s];
            line[s].color = color;
        }
        window.draw(line);
    }

}
