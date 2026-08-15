#include "DrillArena.h"
#include "ShotPath.h"
#include "PitchRenderer.h"
#include "../core/AudioManager.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace {
    const float RIGHT_GOAL_X = 845.f, RIGHT_LINE = 835.f; // forward/midfield attack this
    const float LEFT_GOAL_X  = 35.f,  LEFT_LINE  = 45.f;  // defender/keeper defend this
    const float MAX_INK = 320.f;
    float frand(float a, float b) { return a + (b - a) * (rand() % 1000) / 1000.f; }
}

sf::Vector2f DrillArena::moveToward(sf::Vector2f from, sf::Vector2f to, float step) {
    sf::Vector2f d = to - from;
    float len = std::hypot(d.x, d.y);
    if (len <= step || len < 0.001f) return to;
    return from + sf::Vector2f(d.x / len, d.y / len) * step;
}

DrillArena::DrillArena(Kind kind, int primaryStat, int dribbling, int keeperStrength)
    : m_kind(kind), m_primaryStat(primaryStat), m_dribbling(dribbling), m_keeperStrength(keeperStrength) {
    PitchRenderer::resetAnim(); // fresh player animation state for this drill
    setupRep();
}

void DrillArena::setupRep() {
    m_result = Result::Pending;
    m_path.clear(); m_pathLen = 0.f; m_flightDist = 0.f;
    m_resultTimer = 0.f; m_keeperDiving = false; m_acted = false;
    m_liveT = 0.f; m_feintClock = 0.f; m_feintVec = m_feintTarget = sf::Vector2f(0, 0);
    m_shotReleased = false; m_shotDelay = 0.f;
    m_carrying = false;
    m_mates.clear(); m_markers.clear();

    if (m_kind == Kind::ForwardFinish) {
        float bx = frand(590.f, 705.f), by = frand(215.f, 365.f);
        m_ball = {bx, by};
        m_user = {bx - 16.f, by};
        m_keeper = {RIGHT_LINE, 290.f};
        // Defenders strung along the corridor from the ball to goal, so they sit near the
        // natural shooting line rather than off to the side.
        int nDef = 2 + rand() % 2;
        for (int i = 0; i < nDef; ++i) {
            float t = frand(0.35f, 0.85f);
            float x = bx + (818.f - bx) * t;
            float laneY = by + (290.f - by) * t;      // straight line toward goal centre
            m_markers.push_back({x, std::clamp(laneY + frand(-34.f, 34.f), 236.f, 344.f)});
        }
        m_phase = Phase::Ready;
    } else if (m_kind == Kind::DefenderDuel) {
        float cy = frand(240.f, 340.f);
        m_user = {470.f, cy};
        m_attacker = {520.f, cy};       // starts goal-side, drives at the left goal past you
        m_ball = m_attacker;
        m_phase = Phase::Live;
    } else if (m_kind == Kind::GoalkeeperSave) {
        m_user = {LEFT_LINE, 290.f};
        // 2-3 attackers knock the ball about; the shot comes at an unpredictable moment.
        int nAtt = 2 + rand() % 2;
        for (int i = 0; i < nAtt; ++i)
            m_markers.push_back({frand(230.f, 430.f), frand(200.f, 380.f)});
        m_carrierIdx = rand() % nAtt;
        m_ballInPass = false; m_passToIdx = -1;
        m_attacker = m_markers[m_carrierIdx];
        m_ball = m_attacker;
        m_shotDelay = frand(1.2f, 3.2f);        // no shot before this; keeps timing hidden
        m_feintClock = 0.f;
        m_endDir = {-1.f, 0.f};
        m_phase = Phase::Live;
    } else { // MidfielderPass
        m_user = {400.f, 290.f};
        m_ball = m_user;
        int nMates = 2 + rand() % 2;
        for (int i = 0; i < nMates; ++i)
            m_mates.push_back({frand(560.f, 730.f), frand(180.f, 400.f)});
        int nMk = 1 + rand() % 2;
        for (int i = 0; i < nMk; ++i)
            m_markers.push_back({frand(470.f, 620.f), frand(200.f, 380.f)});
        m_phase = Phase::Ready;
    }
    m_beaten.assign(m_markers.size(), false);
}

void DrillArena::nextRep() { setupRep(); }

std::string DrillArena::prompt() const {
    switch (m_kind) {
        case Kind::ForwardFinish:
            if (m_carrying)               return "Going past him...";
            if (m_phase == Phase::Ready)  return "LMB by the ball to draw a shot  |  Q to dribble past a defender";
            if (m_phase == Phase::Aiming) return "Trace the ball's path, then release";
            if (m_phase == Phase::Power)  return "Click to set the power";
            return "";
        case Kind::DefenderDuel:  return "Read the feint - CLICK THE BALL to nick it (one lunge!)";
        case Kind::GoalkeeperSave:
            return m_shotReleased ? "SHOT! - CLICK the height you dive to"
                                  : "Read the build-up - a shot can come any moment, then dive";
        case Kind::MidfielderPass:
            if (m_carrying)               return "Going past him...";
            if (m_phase == Phase::Ready)  return "LMB by the ball to draw a pass  |  Q to dribble past a marker";
            if (m_phase == Phase::Aiming) return "Trace the pass, then release";
            if (m_phase == Phase::Power)  return "Click to set the weight";
            return "";
    }
    return "";
}

void DrillArena::handleInput(sf::RenderWindow& window, const sf::Event& event, const sf::View& view) {
    sf::Vector2f m;
    if (event.type == sf::Event::MouseButtonPressed)
        m = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), view);
    else if (event.type == sf::Event::MouseMoved)
        m = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y), view);

    // Duel / keeper: one click resolves the attempt.
    if (m_kind == Kind::DefenderDuel && m_phase == Phase::Live && !m_acted
        && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        m_acted = true;
        float dClick = std::hypot(m.x - m_ball.x, m.y - m_ball.y);
        float dReach = std::hypot(m_user.x - m_ball.x, m_user.y - m_ball.y);
        resolveDuel(dClick < 14.f && dReach < 70.f);
        return;
    }
    if (m_kind == Kind::GoalkeeperSave && m_phase == Phase::Live && !m_acted && m_shotReleased
        && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        m_acted = true;
        resolveKeeper(m.y);
        return;
    }

    // Forward / midfielder: draw a path then a power bar.
    bool drawKind = (m_kind == Kind::ForwardFinish || m_kind == Kind::MidfielderPass);
    if (!drawKind) return;

    // Q takes on the nearest opponent (dribble). Scancode, so it fires regardless of layout
    // (the virtual-key path doesn't reach the drill in this environment).
    if (m_phase == Phase::Ready && !m_carrying
        && event.type == sf::Event::KeyPressed && event.key.scancode == sf::Keyboard::Scan::Q) {
        tryDribble();
        return;
    }

    if (m_phase == Phase::Ready && !m_carrying) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left
            && std::hypot(m.x - m_ball.x, m.y - m_ball.y) < 40.f) {
            m_phase = Phase::Aiming; m_from = m_ball; m_path.clear(); m_path.push_back(m_ball); m_pathLen = 0.f;
        }
    } else if (m_phase == Phase::Aiming) {
        if (event.type == sf::Event::MouseMoved && !m_path.empty()) {
            sf::Vector2f last = m_path.back();
            float seg = std::hypot(m.x - last.x, m.y - last.y);
            if (seg >= 4.f && m_pathLen < MAX_INK) {
                if (m_pathLen + seg > MAX_INK) {
                    sf::Vector2f d((m.x - last.x) / seg, (m.y - last.y) / seg);
                    m = last + d * (MAX_INK - m_pathLen); seg = MAX_INK - m_pathLen;
                }
                m_path.push_back(m); m_pathLen += seg;
            }
        } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            if (m_pathLen < 18.f) { m_phase = Phase::Ready; m_path.clear(); }
            else {
                ShotPath::constrainToArc(m_path); ShotPath::chaikin(m_path, 2);
                m_pathLen = ShotPath::length(m_path);
                m_phase = Phase::Power; m_powerT = 0.f; m_powerDir = 1.f;
            }
        }
    } else if (m_phase == Phase::Power) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (m_kind == Kind::ForwardFinish) launchShot(); else launchPass();
        }
    }
}

void DrillArena::launchShot() {
    m_endDir = {1.f, 0.f};
    if (m_path.size() >= 2) {
        sf::Vector2f d = m_path.back() - m_path.front();
        float len = std::hypot(d.x, d.y);
        if (len > 0.1f) m_endDir = {d.x / len, d.y / len};
    }
    m_flightSpeed = 520.f + m_powerT * 520.f;
    AudioManager::get().kick();
    sf::Vector2f last = m_path.back();
    float toLine = m_endDir.x != 0.f ? (RIGHT_GOAL_X - last.x) / m_endDir.x : 0.f;
    m_shotTargetY = (toLine > 0.f) ? last.y + m_endDir.y * toLine : last.y;
    m_flightDist = 0.f; m_phase = Phase::Flight;
}

void DrillArena::launchPass() {
    m_endDir = {1.f, 0.f};
    if (m_path.size() >= 2) {
        sf::Vector2f d = m_path.back() - m_path.front();
        float len = std::hypot(d.x, d.y);
        if (len > 0.1f) m_endDir = {d.x / len, d.y / len};
    }
    m_flightSpeed = 230.f + m_powerT * 340.f;
    AudioManager::get().kick();
    m_flightDist = 0.f; m_phase = Phase::Flight;
}

void DrillArena::resolveShotAtGoal(float crossY) {
    float dist = std::fabs(crossY - m_keeper.y);
    float reachFrac = std::clamp(dist / 55.f, 0.f, 1.f);
    float atKeeper = 56.f + m_keeperStrength * 0.34f;
    float farCorner = 32.f + m_keeperStrength * 0.24f;
    float saveChance = std::clamp(atKeeper - reachFrac * (atKeeper - farCorner), 10.f, 96.f);
    bool saved = (rand() % 100) < (int)saveChance;
    m_keeperDiving = true;
    m_keeperDiveTo = {RIGHT_LINE, std::clamp(crossY, 250.f, 330.f)};
    if (saved) { m_result = Result::Fail; m_ball = m_keeperDiveTo; }
    else { m_result = Result::Success; m_ball = {RIGHT_GOAL_X + 18.f, crossY};
           m_keeperDiveTo.y += (crossY > m_keeper.y) ? -26.f : 26.f; }
    m_phase = Phase::Done; m_resultTimer = 0.f;
}

void DrillArena::resolveDuel(bool won) {
    m_result = won ? Result::Success : Result::Fail;
    m_phase = Phase::Done; m_resultTimer = 0.f;
}

void DrillArena::resolveKeeper(float diveY) {
    // Only records the dive; the outcome is decided when the ball reaches the line so a
    // mistimed dive still lets the ball run into the net instead of freezing in mid-air.
    m_keeperDiving = true;
    m_keeperDiveTo = {LEFT_LINE, std::clamp(diveY, 250.f, 330.f)};
}

void DrillArena::resolvePass(bool completed) {
    m_result = completed ? Result::Success : Result::Fail;
    m_phase = Phase::Done; m_resultTimer = 0.f;
}

void DrillArena::tryDribble() {
    if (m_kind != Kind::ForwardFinish && m_kind != Kind::MidfielderPass) return;
    if (m_phase != Phase::Ready || m_carrying) return;

    // Nearest opponent you haven't beaten yet, and only if he's close enough to take on.
    int best = -1; float bestD = 1e9f;
    for (size_t i = 0; i < m_markers.size(); ++i) {
        if (i < m_beaten.size() && m_beaten[i]) continue;
        float d = std::hypot(m_user.x - m_markers[i].x, m_user.y - m_markers[i].y);
        if (d < bestD) { bestD = d; best = (int)i; }
    }
    if (best < 0 || bestD > 260.f) return;   // someone must actually be in front of you

    // Success scales with dribbling; a defender right on top of you is harder to beat.
    float chance = std::clamp(42.f + m_dribbling * 0.45f - std::max(0.f, 40.f - bestD) * 0.4f, 20.f, 88.f);
    if ((rand() % 100) < (int)chance) {
        m_beaten[best] = true;                       // knock it past and go
        float goalX = (m_kind == Kind::ForwardFinish) ? RIGHT_GOAL_X : 800.f;
        float dir = (goalX > m_user.x) ? 1.f : -1.f;
        m_carryTarget = {m_markers[best].x + dir * 44.f, m_markers[best].y};
        m_carrying = true;
    } else {
        m_ball = m_markers[best];                     // dispossessed
        m_result = Result::Fail; m_phase = Phase::Done; m_resultTimer = 0.f;
    }
}

void DrillArena::update(float dt) {
    // Gliding past a beaten opponent, ball at your feet; nothing else runs meanwhile.
    if (m_carrying) {
        m_user = moveToward(m_user, m_carryTarget, 340.f * dt);
        m_ball = m_user;
        if (std::hypot(m_user.x - m_carryTarget.x, m_user.y - m_carryTarget.y) < 2.f) m_carrying = false;
        return;
    }

    // Keeper eases to centre / into a dive (forward drill only uses m_keeper).
    if (m_kind == Kind::ForwardFinish) {
        sf::Vector2f tgt = m_keeperDiving ? m_keeperDiveTo : sf::Vector2f(RIGHT_LINE, 290.f);
        m_keeper = moveToward(m_keeper, tgt, (m_keeperDiving ? 900.f : 120.f) * dt);
    }

    // Midfielder drill: before the pass mates open up and markers sit in the lane; once the
    // ball is travelling, both mates and the nearest opponent race to it.
    if (m_kind == Kind::MidfielderPass && m_phase != Phase::Done) {
        m_liveT += dt;
        if (m_phase == Phase::Flight) {
            for (auto& mt : m_mates) mt = moveToward(mt, m_ball, 190.f * dt);      // team-mates chase
            // the closest opponent sprints in to intercept
            int c = -1; float best = 1e9f;
            for (size_t j = 0; j < m_markers.size(); ++j) {
                if (j < m_beaten.size() && m_beaten[j]) continue;
                float d = std::hypot(m_markers[j].x - m_ball.x, m_markers[j].y - m_ball.y);
                if (d < best) { best = d; c = (int)j; }
            }
            if (c >= 0) m_markers[c] = moveToward(m_markers[c], m_ball, 175.f * dt);
        } else {
            for (size_t i = 0; i < m_mates.size(); ++i) {
                m_mates[i].x = std::min(760.f, m_mates[i].x + 20.f * dt);          // drift forward
                m_mates[i].y += std::sin(m_liveT * 1.6f + i * 2.1f) * 30.f * dt;   // bob across
                m_mates[i].y = std::clamp(m_mates[i].y, 160.f, 420.f);
            }
            for (size_t k = 0; k < m_markers.size(); ++k) {
                if (k < m_beaten.size() && m_beaten[k]) continue;
                sf::Vector2f& mk = m_markers[k];
                sf::Vector2f near = m_mates.empty() ? m_ball : m_mates[0];
                float best = 1e9f;
                for (auto& mt : m_mates) {
                    float d = std::hypot(mk.x - mt.x, mk.y - mt.y);
                    if (d < best) { best = d; near = mt; }
                }
                sf::Vector2f tgt = m_ball * 0.4f + near * 0.6f;
                mk = moveToward(mk, tgt, 100.f * dt);
                mk.x = std::clamp(mk.x, 440.f, 780.f);
                mk.y = std::clamp(mk.y, 150.f, 430.f);
            }
        }
    }

    if (m_phase == Phase::Power) {
        m_powerT += m_powerDir * 1.25f * dt;
        if (m_powerT >= 1.f) { m_powerT = 1.f; m_powerDir = -1.f; }
        else if (m_powerT <= 0.f) { m_powerT = 0.f; m_powerDir = 1.f; }
        return;
    }

    if (m_phase == Phase::Flight) {
        m_flightSpeed -= m_flightSpeed * 0.85f * dt;
        if (m_flightSpeed < 160.f) m_flightSpeed = 160.f;
        m_flightDist += m_flightSpeed * dt;
        sf::Vector2f tp = ShotPath::pointAlong(m_path, m_from, m_endDir, m_flightDist);
        float k = 1.f - std::exp(-13.f * dt);
        m_ball += (tp - m_ball) * k;

        if (m_kind == Kind::ForwardFinish) {
            float dFromStart = std::hypot(m_ball.x - m_from.x, m_ball.y - m_from.y);
            if (dFromStart > 40.f)
                for (size_t i = 0; i < m_markers.size(); ++i) {
                    if (i < m_beaten.size() && m_beaten[i]) continue;   // already dribbled past
                    if (std::hypot(m_ball.x - m_markers[i].x, m_ball.y - m_markers[i].y) < 13.f) {
                        m_result = Result::Fail; m_phase = Phase::Done; return; // defender blocks it
                    }
                }
            if (m_ball.x > RIGHT_LINE - 2.f) {
                if (m_ball.y > 250.f && m_ball.y < 330.f) resolveShotAtGoal(m_ball.y);
                else { m_result = Result::Fail; m_phase = Phase::Done; }
            } else if (m_ball.y < 132.f || m_ball.y > 448.f || m_flightDist > m_pathLen + 1400.f) {
                m_result = Result::Fail; m_phase = Phase::Done;
            }
        } else { // MidfielderPass: reach a mate, or a marker cuts it out
            float dFromStart = std::hypot(m_ball.x - m_from.x, m_ball.y - m_from.y);
            if (dFromStart > 55.f) {
                for (size_t i = 0; i < m_markers.size(); ++i) {
                    if (i < m_beaten.size() && m_beaten[i]) continue;
                    if (std::hypot(m_ball.x - m_markers[i].x, m_ball.y - m_markers[i].y) < 16.f) { resolvePass(false); return; }
                }
                for (auto& mt : m_mates)
                    if (std::hypot(m_ball.x - mt.x, m_ball.y - mt.y) < 15.f) { resolvePass(true); return; }
            }
            if (m_ball.x < 42.f || m_ball.x > 838.f || m_ball.y < 132.f || m_ball.y > 448.f
                || m_flightDist > m_pathLen + 600.f) { resolvePass(false); }
        }
        return;
    }

    if (m_phase == Phase::Live) {
        m_liveT += dt;
        if (m_kind == Kind::DefenderDuel) {
            // Attacker grinds toward the left goal, jinking the ball around his feet.
            m_attacker.x -= 34.f * dt;
            m_feintClock += dt;
            if (m_feintClock > 0.22f) {
                m_feintClock = 0.f;
                float a = frand(0.f, 6.2831853f);
                m_feintTarget = sf::Vector2f(std::cos(a), std::sin(a)) * 16.f;
            }
            m_feintVec += (m_feintTarget - m_feintVec) * std::min(1.f, 14.f * dt);
            m_ball = m_attacker + m_feintVec;
            if (m_attacker.x < m_user.x - 14.f || m_liveT > 3.f) resolveDuel(false);
        } else if (m_kind == Kind::GoalkeeperSave) {
            if (!m_shotReleased) {
                // Attackers drift about, keeping the shape alive.
                for (auto& a : m_markers) {
                    a.y += std::sin(m_liveT * 1.9f + (&a - &m_markers[0]) * 1.7f) * 16.f * dt;
                    a.y = std::clamp(a.y, 196.f, 384.f);
                    a.x = std::clamp(a.x + std::cos(m_liveT * 1.3f) * 10.f * dt, 220.f, 440.f);
                }
                if (m_ballInPass) {
                    // Ball travelling from one attacker to another.
                    sf::Vector2f tgt = m_markers[m_passToIdx];
                    m_ball = moveToward(m_ball, tgt, 560.f * dt);
                    if (std::hypot(m_ball.x - tgt.x, m_ball.y - tgt.y) < 10.f) {
                        m_carrierIdx = m_passToIdx; m_ballInPass = false; m_feintClock = 0.f;
                    }
                } else {
                    m_ball = m_markers[m_carrierIdx];          // ball at the carrier's feet
                    m_feintClock += dt;
                    if (m_liveT >= m_shotDelay) {
                        // Release: struck flat straight off his feet at his height.
                        m_shotReleased = true;
                        m_attacker = m_markers[m_carrierIdx];
                        m_shotTargetY = std::clamp(m_attacker.y, 256.f, 324.f);
                        m_attacker.y = m_shotTargetY;
                        m_ball = m_attacker;
                        float len = std::hypot(LEFT_GOAL_X - m_ball.x, 0.f);
                        m_flightSpeed = frand(0.75f, 2.0f) * std::max(len * 1.5f, 280.f);
                        AudioManager::get().kick(); // the attacker strikes at your goal
                    } else if (m_feintClock > frand(0.45f, 0.85f) && m_markers.size() > 1) {
                        // Knock it to a different attacker.
                        int to = rand() % (int)m_markers.size();
                        if (to == m_carrierIdx) to = (to + 1) % (int)m_markers.size();
                        m_passToIdx = to; m_ballInPass = true; m_feintClock = 0.f;
                    }
                }
                m_attacker = m_markers[m_carrierIdx];
            } else {
                // Ball flies flat toward your goal; the outcome is settled at the line.
                m_ball.x -= m_flightSpeed * dt;
                m_flightSpeed = std::max(m_flightSpeed * (1.f - 0.15f * dt), 200.f);
                if (m_ball.x <= LEFT_LINE + 2.f) {
                    float reach = 30.f + m_primaryStat * 0.30f;   // YOUR goalkeeping
                    bool saved = m_acted && std::fabs(m_keeperDiveTo.y - m_shotTargetY) < reach;
                    m_result = saved ? Result::Success : Result::Fail;
                    m_phase = Phase::Done; m_resultTimer = 0.f;
                }
            }
        }
        return;
    }

    if (m_phase == Phase::Done) {
        m_resultTimer += dt;
        if (m_keeperDiving)
            m_keeper = moveToward(m_keeper, m_keeperDiveTo, 900.f * dt);
        if (m_kind == Kind::GoalkeeperSave) {
            if (m_keeperDiving)                       // your dive dot
                m_user = moveToward(m_user, m_keeperDiveTo, 900.f * dt);
            if (m_result == Result::Fail && m_ball.x > LEFT_GOAL_X - 12.f)
                m_ball.x -= 240.f * dt;               // beaten: ball rolls into the net
            else if (m_result == Result::Success)
                m_ball = moveToward(m_ball, m_user, 260.f * dt); // gathered by the keeper
        }
    }
}

void DrillArena::draw(sf::RenderWindow& window) {
    PitchRenderer::drawPitch(window);

    if ((m_phase == Phase::Aiming || m_phase == Phase::Power))
        PitchRenderer::drawPath(window, m_path);

    // Scene players per kind (stable ids: user=0, keeper=1, attacker=2, markers 10+, mates 30+).
    const sf::Color kMe(45, 95, 235), kOpp(232, 60, 60), kGk(245, 140, 20);
    if (m_kind == Kind::ForwardFinish) {
        for (size_t i = 0; i < m_markers.size(); ++i) PitchRenderer::drawPlayer(window, 10 + (int)i, m_markers[i], kOpp);
        PitchRenderer::drawPlayer(window, 1, m_keeper, kGk);
        PitchRenderer::drawPlayer(window, 0, m_user, kMe, true);
    } else if (m_kind == Kind::DefenderDuel) {
        PitchRenderer::drawPlayer(window, 2, m_attacker, kOpp);
        PitchRenderer::drawPlayer(window, 0, m_user, kMe, true);
    } else if (m_kind == Kind::GoalkeeperSave) {
        for (size_t i = 0; i < m_markers.size(); ++i) PitchRenderer::drawPlayer(window, 10 + (int)i, m_markers[i], kOpp);
        PitchRenderer::drawPlayer(window, 0, m_user, kMe, true);
    } else { // MidfielderPass
        for (size_t i = 0; i < m_markers.size(); ++i) PitchRenderer::drawPlayer(window, 10 + (int)i, m_markers[i], kOpp);
        for (size_t i = 0; i < m_mates.size(); ++i)   PitchRenderer::drawPlayer(window, 30 + (int)i, m_mates[i], kMe);
        PitchRenderer::drawPlayer(window, 0, m_user, kMe, true);
    }
    PitchRenderer::drawBall(window, m_ball);

    if (m_phase == Phase::Power) {
        float W = 320.f, H = 20.f, left = 440.f - W / 2.f, top = 620.f;
        sf::RectangleShape bg(sf::Vector2f(W, H)); bg.setPosition(left, top);
        bg.setFillColor(sf::Color(30, 34, 48, 230)); window.draw(bg);
        sf::RectangleShape fl(sf::Vector2f(W * m_powerT, H)); fl.setPosition(left, top);
        fl.setFillColor(sf::Color(60, 200, 90, 235)); window.draw(fl);
    }

    // Outcome flash over the pitch so duels/saves read clearly.
    if (m_phase == Phase::Done && m_result != Result::Pending) {
        sf::RectangleShape flash(sf::Vector2f(800.f, 320.f));
        flash.setPosition(40.f, 130.f);
        sf::Uint8 a = (sf::Uint8)std::clamp(70.f - m_resultTimer * 45.f, 0.f, 70.f);
        flash.setFillColor(m_result == Result::Success ? sf::Color(60, 200, 90, a)
                                                       : sf::Color(210, 70, 60, a));
        window.draw(flash);
    }
}
