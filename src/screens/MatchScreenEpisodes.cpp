#include "MatchScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Settings.h"
#include "UITheme.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>

// Episode choreography and match AI.
//
// Split out of MatchScreen.cpp, which had grown past 1800 lines and mixed screen
// lifecycle, input, rendering, physics and the episode scripts together. Same class,
// separate translation unit - the scripts and the AI that drives the 22 dots live here.

void MatchScreen::resetToKickoff() {
    m_visualState = VisualState::Kickoff;
    m_stateTimer = 0.f;
    m_currentZoom = 1.0f;
    m_camera = m_uiView;
    m_sendOffGraceIdx = -1;
    m_deadBallTakerIdx = -1;
    
    int kickerIdx = m_pendingEvent.isHome ? 11 + 9 : 9; // Away team kicks off if home scored
    if (m_engine->getMinute() == 0) kickerIdx = 9; // At 0' Home kicks off
    
    m_ballTarget = sf::Vector2f(440.f, 290.f);
    m_visualBall.setPosition(m_ballTarget);
    m_ballCarrierIdx = kickerIdx;
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    for(int i=0; i<11; ++i) {
        float tx = 50.f + form[i][0] * 780.f;
        if (tx > 435.f) tx = 435.f;
        m_dots[i].targetPos = sf::Vector2f(tx, 140.f + form[i][1] * 300.f);
        m_dots[i].shape.setPosition(m_dots[i].targetPos);
    }
    for(int i=0; i<11; ++i) {
        float tx = 830.f - form[i][0] * 780.f;
        if (tx < 445.f) tx = 445.f;
        m_dots[i+11].targetPos = sf::Vector2f(tx, 140.f + form[i][1] * 300.f);
        m_dots[i+11].shape.setPosition(m_dots[i+11].targetPos);
    }
    m_dots[kickerIdx].targetPos = sf::Vector2f(440.f, 290.f);
    m_dots[kickerIdx].shape.setPosition(m_dots[kickerIdx].targetPos);
}

void MatchScreen::updateAmbientShape() {
    // Everyone not involved in whatever is happening used to just stand: the scripts only
    // ever assign targets to their own participants, and the minigame AI parked the rest
    // on the spot, so 16-odd players stood like statues through every episode.
    //
    // This gives every player a baseline target - his formation slot, slid along with the
    // ball - so the shape breathes with the play. Targets are ABSOLUTE (formation slot +
    // a shift derived from the ball's position), never "current position + delta", which
    // is what previously turned into a treadmill that marched the whole side off the pitch.
    //
    // Callers run this FIRST; the scripts and minigame AI then overwrite the players they
    // actually control.
    static const float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };

    // During a minigame the ball is glued to the user's feet, so following it here would
    // make the whole formation mirror his dribble. Anchor to the frozen episode position
    // instead; in open play, follow the live ball.
    sf::Vector2f ball = m_minigameActive ? m_ambientAnchor : m_visualBall.getPosition();
    // The whole shape shuffles toward whichever end the ball is in.
    float shift = (ball.x - 440.f) * 0.35f;

    // A slow, per-player drift so players jockey independently instead of standing dead
    // still once they reach their slot. Each dot has its own phase (via its index), so
    // nobody moves in lockstep - and it's tiny, so the shape still reads as a formation.
    static float s_ambientClock = 0.f;
    s_ambientClock += 1.f / 60.f;

    for (int team = 0; team < 2; ++team) {
        int base = team * 11;
        for (int i = 0; i < 11; ++i) {
            int idx = base + i;
            if (hasRedCard(idx)) continue;
            // Never drag whoever has the ball back to his slot - he's doing something.
            // (Notably the kick-off taker, whose target is set once and would otherwise be
            // clobbered here, pulling him off the centre spot.)
            if (idx == m_ballCarrierIdx) continue;
            // The user drives his own dot; don't fight him for it.
            if (m_minigameActive && idx == m_userIdx) continue;

            if (i == 0) { // keeper: hold the line, track the ball's height
                m_dots[idx].targetPos = sf::Vector2f(team == 0 ? 70.f : 810.f,
                                                     std::clamp(ball.y, 240.f, 340.f));
                m_dots[idx].speed = 80.f;
                continue;
            }

            float slotX = (team == 0) ? std::min(50.f + form[i][0] * 780.f, 435.f)
                                      : std::max(830.f - form[i][0] * 780.f, 445.f);
            float slotY = 140.f + form[i][1] * 300.f;

            float driftX = std::sin(s_ambientClock * 0.9f + idx * 1.3f) * 9.f;
            float driftY = std::cos(s_ambientClock * 0.7f + idx * 2.1f) * 9.f;

            // Slide with the ball, and lean gently into the ball's channel so the side
            // shuffles across rather than holding rigid lanes.
            m_dots[idx].targetPos = sf::Vector2f(std::clamp(slotX + shift + driftX, 55.f, 815.f),
                                                 slotY + (ball.y - slotY) * 0.15f + driftY);
            m_dots[idx].speed = 55.f;
        }
    }
}

void MatchScreen::updateDotMotion(float dt) {
    // Steers every dot toward its targetPos. This used to sit at the tail of
    // updateVisuals(), which update() skips entirely while a minigame is running - so
    // for the whole duration of an episode every player except the user stood frozen.
    // It now runs in both paths; updateMinigameAI() supplies fresh targets during a
    // minigame, since the scripts aren't ticking then.
    for (size_t i = 0; i < m_dots.size(); ++i) {
        // The user drives their own dot directly (WASD in updateMinigame); pulling it
        // toward a scripted target here as well would fight the player for control.
        if (m_minigameActive && (int)i == m_userIdx) continue;

        auto& d = m_dots[i];
        // The dead-ball taker is exempt: a thrower stands ON (or just behind) the touchline
        // and a corner taker stands at the flag. The general clamp keeps everyone else a
        // comfortable margin inside the pitch, which used to strand the thrower 10px infield.
        if ((int)i != m_deadBallTakerIdx) {
            d.targetPos.x = std::clamp(d.targetPos.x, 50.f, 820.f);
            d.targetPos.y = std::clamp(d.targetPos.y, 140.f, 440.f);
        } else {
            d.targetPos.x = std::clamp(d.targetPos.x, 26.f, 846.f);
            d.targetPos.y = std::clamp(d.targetPos.y, 116.f, 464.f);
        }

        sf::Vector2f dir = d.targetPos - d.shape.getPosition();
        float len = std::hypot(dir.x, dir.y);
        if (len <= 0.f) continue;

        // Each dot moves at its own speed. This used to force EVERY dot to 150 for the
        // whole of the Attacking state, so the instant a script began all 22 players
        // jumped from their ambient stroll (55) to a sprint - a very visible gear change
        // on every transition. The scripts set their own participants' speeds; everyone
        // else keeps the ambient pace, so the switch is invisible.
        float moveDist = d.speed * dt;
        if (moveDist >= len) {
            d.shape.setPosition(d.targetPos);
        } else {
            d.shape.move((dir.x / len) * moveDist, (dir.y / len) * moveDist);
        }
    }
}

void MatchScreen::updateMinigameAI(float dt) {
    (void)dt;
    Player* p = m_gameManager->getPlayer();
    sf::Vector2f ballPos = m_visualBall.getPosition();

    // Baseline shape for all 22 first; the pressers/runners/keepers below overwrite the
    // few who are actually involved. Without this everyone else stands frozen.
    updateAmbientShape();

    bool userIsHome = m_engine->isHome();
    int ownBase = userIsHome ? 0 : 11;
    int oppBase = userIsHome ? 11 : 0;

    // Which way our team attacks.
    float attackDir = userIsHome ? 1.f : -1.f;

    // Opponent pace scales with their club, so a weak side doesn't press like a top one.
    int oppStrength = m_engine->getOpponentClub() ? m_engine->getOpponentClub()->strength : 70;
    float chaseSpeed = 90.f + (oppStrength / 100.f) * 60.f; // ~90..150

    // Are WE the ones on the ball? Only then do opponents swarm it. When the user is
    // defending (a tackle/save), the ball is the opponent's to lose - having every nearby
    // opponent pile onto it meant a second opponent would run in and boot the loose ball
    // away before the user could get his tackle in. Computed up front so the press logic
    // below can gate on it.
    bool userOnBall = (m_pendingKind == MinigameActionKind::Shot || m_pendingKind == MinigameActionKind::Pass);

    // --- Opponents: the two nearest close down the ball (only when we have it), the rest
    // hold shape.
    std::vector<std::pair<float, int>> byDist;
    for (int i = oppBase; i < oppBase + 11; ++i) {
        if (i % 11 == 0) continue;      // their keeper handled below
        if (hasRedCard(i)) continue;
        sf::Vector2f d = m_dots[i].shape.getPosition() - ballPos;
        byDist.push_back({std::hypot(d.x, d.y), i});
    }
    std::sort(byDist.begin(), byDist.end());

    // Only the two nearest actually close the ball down, and only while we have it. The
    // rest keep the ambient shape set above (they used to be parked on the spot here).
    if (userOnBall) {
        for (size_t k = 0; k < byDist.size() && k < 2; ++k) {
            int idx = byDist[k].second;
            m_dots[idx].targetPos = ballPos; // press the ball
            m_dots[idx].speed = chaseSpeed;
        }
    }

    // --- Team-mates: a couple push forward so a pass has somewhere to go. Without
    // this pickPassTarget() only ever sees statues standing in their kick-off spots.
    std::vector<std::pair<float, int>> mates;
    for (int i = ownBase; i < ownBase + 11; ++i) {
        if (i == m_userIdx) continue;
        if (i % 11 == 0) continue;      // our keeper stays home
        if (hasRedCard(i)) continue;
        sf::Vector2f d = m_dots[i].shape.getPosition() - ballPos;
        mates.push_back({std::hypot(d.x, d.y), i});
    }
    std::sort(mates.begin(), mates.end());

    // Two men break into space to give the pass somewhere to go; the rest keep the ambient
    // shape. Anchor the run to the FROZEN episode position (m_ambientAnchor), not the live
    // ball. The live ball is glued to the user's dribble, so keying the run off it made the
    // supporting men shadow his every step - "everyone moves exactly like me". Off a fixed
    // point they make one honest run into space and hold, independent of the user.
    sf::Vector2f anchor = m_minigameActive ? m_ambientAnchor : ballPos;
    for (size_t k = 0; k < mates.size() && k < 2; ++k) {
        int idx = mates[k].second;
        sf::Vector2f pos = m_dots[idx].shape.getPosition();
        float wide = (pos.y < anchor.y) ? -45.f : 45.f;
        m_dots[idx].targetPos = sf::Vector2f(anchor.x + attackDir * (100.f + k * 40.f), anchor.y + wide);
        m_dots[idx].speed = 110.f;
    }

    // (Keepers are handled by updateAmbientShape above - line-holding, tracking the ball.)

    // --- Pressure: an opponent who reaches the ball while WE have it wins it back
    // (userOnBall computed above). When he's defending (Tackle/Save) the opponent already
    // has it, so there's nothing to lose here.

    // A committed action is untouchable: the player has already planted his foot, and
    // sniping the ball mid-QTE would punish someone who did everything right.
    if (!userOnBall || m_qte.isActive() || byDist.empty()) return;

    // Grace period. The attack scripts hand over control with a defender already right on
    // top of the carrier (~15px), so the player needs a real window to shoot or pass
    // before anyone can rob him - otherwise it feels like the ball is taken the instant
    // control arrives. Long enough to comfortably start an action.
    const float GRACE = 3.5f;
    if (m_minigameTimer < GRACE) return;

    // Once the ball has been struck it's gone - nobody "tackles" a shot that's already
    // on its way. Without this an opponent could still rob us mid-flight, after the QTE
    // had already decided the outcome.
    float ballSpeed = std::hypot(m_ballVelocity.x, m_ballVelocity.y);
    if (ballSpeed > 120.f) return;

    m_tackleAttemptTimer -= dt;
    if (m_tackleAttemptTimer > 0.f) return;

    float gap = byDist[0].first;
    if (gap > 14.f) return;

    // One attempt roughly every second, not one per frame.
    m_tackleAttemptTimer = 1.0f;

    // Contact isn't an automatic steal - weigh their tackling against our ball control,
    // and keep the ceiling low enough that dwelling on the ball is risky but not a
    // guaranteed loss.
    int duel = 28 + (oppStrength - (p->passing + p->shooting) / 2) / 2;
    duel = std::clamp(duel, 8, 55);
    if ((rand() % 100) >= duel) return;

    // Robbed by a closing opponent while dwelling on the ball - a turnover, so the log
    // reads "robbed of possession" instead of "missed a golden opportunity". Hand the ball
    // to the man who won it and snap it to his feet, so it doesn't fly across the pitch to
    // reach him once open play resumes. byDist excludes keepers, so this is an outfielder.
    int winner = byDist[0].second;
    m_qteAccuracy = 0.f;
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballCarrierIdx = winner;
    m_visualBall.setPosition(m_dots[winner].shape.getPosition());
    m_ballTarget = m_dots[winner].shape.getPosition();
    m_engine->processMinigameResult(buildMinigameResult(false, m_pendingKind, ActionVariant::Dispossessed));
    endMinigame();
}

bool MatchScreen::offsideBuildup(int strikerIdx, bool attackingHome, int holderIdx) {
    // Keep the ball with the passer and let the striker run in. Whistle only once he is
    // CLEARLY past the offside line (real positions, not just his target) and the run has
    // played out for a beat - so on screen you actually see him ahead of the last defender
    // before the flag. If the defence tracks him and he never gets clear, drop the offside
    // intent and let the move continue onside rather than flagging a phantom.
    float dir = attackingHome ? 1.f : -1.f;
    float lineFwd = (offsideLineX(attackingHome) - 440.f) * dir;
    float fwdNow = (m_dots[strikerIdx].shape.getPosition().x - 440.f) * dir;
    bool clearlyBeyond = fwdNow > lineFwd + 10.f;

    if (m_offsidePassReleased) {
        // The pass is on its way to the offside runner. Keep the ball loose (the run
        // scripts re-grab it as carrier every frame, so force it back to -1) and steer it
        // onto him; raise the flag only once it reaches his feet, so the through-ball is
        // seen being played rather than the ball teleporting to him.
        m_ballCarrierIdx = -1;
        m_ballTarget = m_dots[strikerIdx].shape.getPosition();
        float d = std::hypot(m_visualBall.getPosition().x - m_ballTarget.x,
                             m_visualBall.getPosition().y - m_ballTarget.y);
        if (d < 14.f || m_stateTimer > 5.0f) resolveOffside(attackingHome);
        return true;
    }

    // Hold the ball with the passer while the striker breaks beyond the last defender.
    if (holderIdx >= 0 && holderIdx < (int)m_dots.size()) m_ballCarrierIdx = holderIdx;

    if (clearlyBeyond && m_stateTimer > 1.5f) {
        // Clearly offside and the run has read on screen: slide the pass into him now. The
        // whistle waits for the ball to arrive (handled in the branch above).
        m_offsidePassReleased = true;
        m_ballCarrierIdx = -1;
        m_ballTarget = m_dots[strikerIdx].shape.getPosition();
    } else if (m_stateTimer > 3.5f) {
        m_offsideRun = false; m_stateTimer = 0.f; // defence tracked him - no offside after all
    }
    return true;
}

void MatchScreen::resolveOffside(bool attackingHome) {
    // Put the ball where the offside runner strayed, add a visual-only log (the engine
    // has no offside event and it doesn't affect stats), then restart to the defending
    // side. setupFreeKick hands the ball to the OTHER team and gives a clean dead-ball
    // restart with no challenge - exactly an offside decision.
    if (m_attackFwdIdx >= 0 && m_attackFwdIdx < (int)m_dots.size()) {
        m_visualBall.setPosition(m_dots[m_attackFwdIdx].shape.getPosition());
    }
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballCarrierIdx = -1;
    m_offsidePassReleased = false;

    MatchEvent e;
    e.text = "[" + std::to_string(m_engine->getMinute()) + "'] Offside! The flag is up, the run was mistimed.";
    e.type = EventType::Normal;
    e.isHome = !attackingHome; // free kick goes to the defending side
    e.outcome = EventOutcome::None;
    m_visibleLogs.push_back(e);
    if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());

    setupFreeKick(attackingHome); // attacking side gave it away -> defenders restart
}

bool MatchScreen::handleFoulIfCard() {
    if (m_pendingEvent.type != EventType::Card && m_pendingEvent.type != EventType::Foul) {
        return false;
    }
    m_engine->commitEvent(m_pendingEvent);
    m_visibleLogs.push_back(m_pendingEvent);
    if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
    beginFoul(m_pendingEvent.isHome);
    return true;
}

void MatchScreen::beginFoul(bool offenderIsHome) {
    // Tear down any live minigame first (a mistimed slide tackle gives a card from inside
    // an episode), so its physics/zoom don't run over the challenge.
    if (m_minigameActive) {
        endMinigame();
    }

    m_foulOffenderIsHome = offenderIsHome;

    // Capture the ball carrier BEFORE clearing it - the challenge should centre on the man
    // who actually had the ball, not just whoever happens to be nearest the loose ball.
    int carrier = m_ballCarrierIdx;

    sf::Vector2f spot = m_visualBall.getPosition();
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballFriction = 1.5f;
    m_ballCarrierIdx = -1;

    int offBase = offenderIsHome ? 0 : 11;
    int vicBase = offenderIsHome ? 11 : 0;

    // Nearest outfield player of a team to a point.
    auto nearestTo = [&](int base, sf::Vector2f from) {
        int best = -1; float bd = 1e9f;
        for (int i = base; i < base + 11; ++i) {
            if (i % 11 == 0) continue;      // not the keeper
            if (hasRedCard(i)) continue;
            sf::Vector2f d = m_dots[i].shape.getPosition() - from;
            float dd = std::hypot(d.x, d.y);
            if (dd < bd) { bd = dd; best = i; }
        }
        return best;
    };

    // Build the challenge around whoever had the ball. If the carrier is on the fouled
    // side he's the victim (a defender lunges in); if he's on the offending side he IS the
    // fouler (he barged someone). Falls back to nearest-to-ball when the ball was loose.
    bool carrierValid = (carrier >= 0 && carrier % 11 != 0 && !hasRedCard(carrier));
    bool carrierIsOffender = carrierValid && ((carrier < 11) == offenderIsHome);
    bool carrierIsVictim   = carrierValid && ((carrier < 11) != offenderIsHome);

    if (carrierIsVictim) {
        m_foulVictimIdx = carrier;
        m_foulOffenderIdx = nearestTo(offBase, m_dots[carrier].shape.getPosition());
    } else if (carrierIsOffender) {
        m_foulOffenderIdx = carrier;
        m_foulVictimIdx = nearestTo(vicBase, m_dots[carrier].shape.getPosition());
    } else {
        m_foulOffenderIdx = nearestTo(offBase, spot);
        m_foulVictimIdx = nearestTo(vicBase, spot);
    }

    // Work out the challenge geometry ONCE, here. Doing it per frame in the
    // FoulChallenge branch meant the victim's "stagger away" target was recomputed from
    // his own latest position every frame, so it kept retreating and he fled forever with
    // the offender chasing him - both ended up miles from the ball.
    if (m_foulOffenderIdx >= 0 && m_foulVictimIdx >= 0) {
        sf::Vector2f vicPos = m_dots[m_foulVictimIdx].shape.getPosition();
        sf::Vector2f offPos = m_dots[m_foulOffenderIdx].shape.getPosition();

        // The foul happened at the victim's feet - put the dead ball there, so the
        // challenge and the free kick both centre on him rather than on wherever the ball
        // had drifted.
        m_visualBall.setPosition(vicPos);
        m_ballTarget = vicPos;

        sf::Vector2f away = vicPos - offPos;
        float len = std::hypot(away.x, away.y);
        if (len > 0.1f) { away.x /= len; away.y /= len; } else { away = sf::Vector2f(0.f, 1.f); }

        // Offender lunges slightly PAST the victim (a real challenge, not a gentle touch)
        // and the victim is knocked well clear, so the contact reads on screen.
        m_foulLungeTarget = vicPos + (vicPos - offPos) * 0.15f;
        m_foulStaggerTarget = vicPos + away * 45.f;
    }

    // If this challenge is a sending-off, make sure the player who leaves the pitch is the
    // one we just showed lunging in. commitEvent (already run) had picked a random outfield
    // shirt, so the red used to fall on someone nowhere near the incident.
    if (m_pendingEvent.outcome == EventOutcome::RedCard && m_foulOffenderIdx >= 0) {
        m_engine->setLastRedCardPlayer(m_foulOffenderIsHome, m_foulOffenderIdx % 11);
        // Setting the red would make hasRedCard() true at once and stop him being drawn -
        // he'd vanish mid-lunge. Keep him on the pitch through the challenge and the dead
        // ball; he leaves only when play restarts (cleared in the Foul->NormalPlay hop).
        m_sendOffGraceIdx = m_foulOffenderIdx;
    }

    m_visualState = VisualState::FoulChallenge;
    m_stateTimer = 0.f;
    m_foulClock = 0.f;
}

void MatchScreen::setupFreeKick(bool offenderIsHome) {
    // A foul can be given from inside an episode (a mistimed slide tackle). Tear the
    // minigame down first, or its physics and zoomed camera would keep running over the
    // top of the dead ball.
    if (m_minigameActive) {
        endMinigame();
    }

    // The ball is dead at the spot of the foul.
    sf::Vector2f spot = m_visualBall.getPosition();
    spot.x = std::clamp(spot.x, 60.f, 820.f);
    spot.y = std::clamp(spot.y, 150.f, 430.f);

    m_visualBall.setPosition(spot);
    m_visualBall.setScale(1.f, 1.f);
    m_ballTarget = spot;
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballFriction = 1.5f;
    m_ballLoftTimer = 0.f;
    m_ballCarrierIdx = -1; // nobody has it until the taker walks up to it

    // The side that was fouled restarts. Pick their nearest outfield player.
    int base = offenderIsHome ? 11 : 0;
    int taker = -1;
    float bestDist = 1e9f;
    for (int i = base; i < base + 11; ++i) {
        if (i % 11 == 0) continue; // leave the keeper on his line
        if (hasRedCard(i)) continue;
        sf::Vector2f d = m_dots[i].shape.getPosition() - spot;
        float dist = std::hypot(d.x, d.y);
        if (dist < bestDist) {
            bestDist = dist;
            taker = i;
        }
    }

    // Realistic restart: a foul within shooting range of the fouled side's goal is a
    // DIRECT free kick - the offending side throws up a wall and a shot follows. A foul
    // out in midfield is just knocked back into play (the simple restart below).
    bool fouledIsHome = !offenderIsHome;              // the side that WON the free kick
    float attackGoalX = fouledIsHome ? 845.f : 35.f;  // the goal they're shooting at
    float distToGoal = std::abs(attackGoalX - spot.x);
    m_fkDirect = (distToGoal < 250.f && std::abs(spot.y - 290.f) < 150.f);
    m_fkAttackHome = fouledIsHome;
    m_fkStruck = false;
    m_fkResolved = false;
    m_fkWindup = 0.f;
    m_fkWallCount = 0;
    m_fkKeeperIdx = -1;
    m_fkUserTaker = false;

    if (m_fkDirect) {
        // Let the user take his side's dangerous free kicks - he's the one you want on the
        // ball, and it's the whole point of the wall+shot minigame.
        Player* up = m_gameManager->getPlayer();
        if (up && up->position != PlayerPosition::Goalkeeper && fouledIsHome == m_engine->isHome()) {
            int upi = (up->position == PlayerPosition::Defender) ? 3
                    : (up->position == PlayerPosition::Midfielder) ? 7
                    : (up->position == PlayerPosition::Forward) ? 10 : 0;
            int userDot = m_engine->isHome() ? upi : 11 + upi;
            if (!hasRedCard(userDot)) { taker = userDot; m_fkUserTaker = true; }
        }

        // Direction from ball to the goal being attacked.
        sf::Vector2f toGoal(attackGoalX - spot.x, 290.f - spot.y);
        float gl = std::hypot(toGoal.x, toGoal.y);
        if (gl > 0.1f) { toGoal.x /= gl; toGoal.y /= gl; }
        sf::Vector2f perp(-toGoal.y, toGoal.x); // across the wall

        // The defending (offending) side forms the wall ~90px in front of the ball. Take
        // the three nearest outfielders and line them up shoulder to shoulder.
        int dBase = offenderIsHome ? 0 : 11;
        std::vector<std::pair<float,int>> cand;
        for (int i = dBase; i < dBase + 11; ++i) {
            if (i % 11 == 0) continue;      // the keeper stays on his line
            if (i == taker) continue;
            if (hasRedCard(i)) continue;
            sf::Vector2f d = m_dots[i].shape.getPosition() - spot;
            cand.push_back({std::hypot(d.x, d.y), i});
        }
        std::sort(cand.begin(), cand.end());
        sf::Vector2f wallCenter = spot + toGoal * 90.f;
        int n = std::min(3, (int)cand.size());
        for (int k = 0; k < n; ++k) {
            int idx = cand[k].second;
            // Shoulder to shoulder: the dots are radius 6, so a 12px pitch leaves them
            // touching with no gap for the ball to squeeze through.
            float off = (k - (n - 1) / 2.f) * 12.f;
            sf::Vector2f pos = wallCenter + perp * off;
            m_dots[idx].targetPos = pos;
            m_dots[idx].speed = 150.f;
            m_fkWallPos[m_fkWallCount] = pos;
            m_fkWall[m_fkWallCount++] = idx;
        }
        m_fkKeeperIdx = dBase; // local 0 = keeper
    }

    m_foulPlayerIdx = taker;
    if (taker >= 0) {
        m_dots[taker].targetPos = spot;
        m_dots[taker].speed = 120.f;
    }

    m_visualState = VisualState::Foul;
    m_stateTimer = 0.f;
}

bool MatchScreen::beginThrowInIfOut() {
    sf::Vector2f b = m_visualBall.getPosition();
    if (b.y >= 130.f && b.y <= 450.f) return false; // still between the touchlines

    // The side that did NOT touch it last gets the throw, so a shot deflected out off a
    // defender correctly goes to the attacking side rather than being guessed from roles.
    bool throwerIsHome = true;
    if (m_lastToucherIdx >= 0 && m_lastToucherIdx < (int)m_dots.size())
        throwerIsHome = !(m_lastToucherIdx < 11);

    m_throwInSpot = sf::Vector2f(std::clamp(b.x, 60.f, 820.f), (b.y < 130.f) ? 130.f : 450.f);

    m_visualBall.setPosition(m_throwInSpot);
    m_visualBall.setScale(1.f, 1.f);
    m_ballTarget = m_throwInSpot;
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballFriction = 1.5f;
    m_ballLoftTimer = 0.f;
    m_ballCarrierIdx = -1;

    int base = throwerIsHome ? 0 : 11;
    int best = -1; float bd = 1e9f;
    for (int i = base; i < base + 11; ++i) {
        if (i % 11 == 0) continue;   // the keeper doesn't take throw-ins
        if (hasRedCard(i)) continue;
        sf::Vector2f d = m_dots[i].shape.getPosition() - m_throwInSpot;
        float dd = std::hypot(d.x, d.y);
        if (dd < bd) { bd = dd; best = i; }
    }
    m_throwInTaker = best;
    m_deadBallTakerIdx = best; // let him reach the line instead of stopping short of it

    m_currentZoom = 1.0f;
    m_camera = m_uiView;
    m_visualState = VisualState::ThrowIn;
    m_stateTimer = 0.f;
    m_foulClock = 0.f; // real-time dead-ball pause, so it's 3s at any match speed
    return true;
}

void MatchScreen::beginCorner(bool attackingHome, float outY) {
    if (m_minigameActive) endMinigame();

    m_cornerAttackHome = attackingHome;
    m_cornerStruck = false;
    m_cornerResolved = false;
    m_cornerSuccess = false;
    m_cornerGoodDelivery = false;
    m_cornerHeaderPending = false;
    m_cornerHeaded = false;
    m_cornerAimOffset = sf::Vector2f(0.f, 0.f);
    m_cornerUserTakes = false;
    m_cornerUserHead = false;
    m_cornerWindup = 0.f;
    m_cornerTargetIdx = -1;
    m_cornerCrowdCount = 0;
    m_qte.cancel();

    float goalX = attackingHome ? 845.f : 35.f;
    m_cornerSpot = sf::Vector2f(attackingHome ? 838.f : 42.f, (outY < 290.f) ? 134.f : 446.f);

    // Don't snap the ball to the flag - let it visibly run out over the byline from wherever
    // it is (off the keeper's parry, most often). It used to teleport there, so the save and
    // the corner read as two unrelated events.
    m_cornerDeflecting = true;
    m_cornerDeflectTarget = sf::Vector2f(attackingHome ? 858.f : 22.f, m_cornerSpot.y);
    m_visualBall.setScale(1.f, 1.f);
    m_ballTarget = m_cornerDeflectTarget;
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballFriction = 1.5f;
    m_ballCarrierIdx = -1;

    int atkBase = attackingHome ? 0 : 11;
    int defBase = attackingHome ? 11 : 0;

    Player* up = m_gameManager->getPlayer();
    int userDot = -1;
    if (up) {
        int upi = (up->position == PlayerPosition::Defender) ? 3
                : (up->position == PlayerPosition::Midfielder) ? 7
                : (up->position == PlayerPosition::Forward) ? 10 : 0;
        userDot = m_engine->isHome() ? upi : 11 + upi;
    }
    bool userAttacking = up && (attackingHome == m_engine->isHome()) && userDot >= 0 && !hasRedCard(userDot);

    // A midfielder or defender whips it in himself; a forward stays in the box to attack
    // the cross and lets a team-mate deliver it.
    int taker = -1;
    if (userAttacking && up->position != PlayerPosition::Goalkeeper) {
        if (up->position == PlayerPosition::Midfielder || up->position == PlayerPosition::Defender) {
            taker = userDot; m_cornerUserTakes = true;
        } else if (up->position == PlayerPosition::Forward) {
            m_cornerUserHead = true;
        }
    }
    if (taker < 0) {
        float bd = 1e9f;
        for (int i = atkBase; i < atkBase + 11; ++i) {
            if (i % 11 == 0 || hasRedCard(i)) continue;
            if (i == userDot && m_cornerUserHead) continue; // he's waiting in the box
            sf::Vector2f d = m_dots[i].shape.getPosition() - m_cornerSpot;
            float dd = std::hypot(d.x, d.y);
            if (dd < bd) { bd = dd; taker = i; }
        }
    }
    m_cornerTaker = taker;
    m_deadBallTakerIdx = taker; // he stands at the flag, outside the normal clamp

    // Pack the box. Every attacker gets an anchor and a defender marking him, so a corner
    // looks like a corner rather than two men and a keeper.
    float dir = attackingHome ? -1.f : 1.f; // inward from the goal being attacked
    sf::Vector2f anchors[6] = {
        sf::Vector2f(goalX + dir * 45.f,  252.f),
        sf::Vector2f(goalX + dir * 45.f,  328.f),
        sf::Vector2f(goalX + dir * 82.f,  224.f),
        sf::Vector2f(goalX + dir * 82.f,  290.f),
        sf::Vector2f(goalX + dir * 82.f,  356.f),
        sf::Vector2f(goalX + dir * 118.f, 290.f)
    };

    auto place = [&](int idx, sf::Vector2f pos) {
        if (idx < 0 || m_cornerCrowdCount >= 12) return;
        m_dots[idx].targetPos = pos;
        m_dots[idx].speed = 150.f;
        m_cornerCrowdPos[m_cornerCrowdCount] = pos;
        m_cornerCrowd[m_cornerCrowdCount++] = idx;
    };

    std::vector<int> atk;
    for (int i = atkBase; i < atkBase + 11; ++i) {
        if (i % 11 == 0 || i == m_cornerTaker || hasRedCard(i)) continue;
        atk.push_back(i);
    }
    if (m_cornerUserHead && userDot >= 0) {
        atk.erase(std::remove(atk.begin(), atk.end(), userDot), atk.end());
        atk.insert(atk.begin(), userDot); // the user gets the prime spot to attack
    }
    int nAtk = std::min((int)atk.size(), 5);
    for (int k = 0; k < nAtk; ++k) place(atk[k], anchors[k]);

    std::vector<int> def;
    for (int i = defBase; i < defBase + 11; ++i) {
        if (i % 11 == 0 || hasRedCard(i)) continue; // the keeper holds his line
        def.push_back(i);
    }
    int nDef = std::min((int)def.size(), 5);
    for (int k = 0; k < nDef; ++k) {
        place(def[k], anchors[k] + sf::Vector2f(dir * 13.f, (k % 2 == 0) ? -10.f : 10.f));
    }

    m_cornerTargetIdx = (m_cornerUserHead && userDot >= 0) ? userDot : (nAtk > 0 ? atk[0] : -1);

    m_currentZoom = 1.0f;
    m_camera = m_uiView;
    m_visualState = VisualState::Corner;
    m_stateTimer = 0.f;
    m_foulClock = 0.f; // real-time beat while the box fills up
}

void MatchScreen::deliverCorner(bool good) {
    m_cornerStruck = true;
    m_cornerGoodDelivery = good;
    m_cornerHeaded = false;
    m_stateTimer = 0.f;
    m_ballCarrierIdx = -1;

    if (good) {
        m_cornerAimOffset = sf::Vector2f(0.f, 0.f);
        if (m_cornerUserHead) {
            // It's on his head - he has the flight of the ball to time the header.
            startQTE(MinigameActionKind::Shot, ActionVariant::Default, false, 1.0f);
            m_cornerHeaderPending = true;
        }
    } else {
        // Overhit, or straight onto a defender: land it clear of the intended man. Kept as
        // an OFFSET so the ball still tracks him while he moves, instead of being aimed at
        // the patch of grass he happened to be standing on when it was struck.
        m_cornerAimOffset = sf::Vector2f((m_cornerAttackHome ? -1.f : 1.f) * (25.f + rand() % 25),
                                         ((rand() % 2 == 0) ? -1.f : 1.f) * (45.f + rand() % 30));
    }

    m_ballTarget = cornerAimPoint();
}

sf::Vector2f MatchScreen::cornerAimPoint() const {
    sf::Vector2f base = (m_cornerTargetIdx >= 0 && m_cornerTargetIdx < (int)m_dots.size())
        ? m_dots[m_cornerTargetIdx].shape.getPosition()
        : sf::Vector2f(m_cornerAttackHome ? 760.f : 120.f, 290.f);
    return base + m_cornerAimOffset;
}

void MatchScreen::registerCornerOutcome() {
    if (m_cornerUserHead) {
        // He attacked the cross himself, so it counts exactly like any other shot of his.
        MinigameResult r = buildMinigameResult(m_cornerSuccess, MinigameActionKind::Shot, ActionVariant::Default);
        m_engine->processMinigameResult(r);
        while (m_engine->hasLogs()) {
            MatchEvent e = m_engine->popRecentLog();
            m_engine->commitEvent(e);
            m_visibleLogs.push_back(e);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
        }
        return;
    }

    MatchEvent e;
    e.isHome = m_cornerAttackHome;
    Club* atk = (m_cornerAttackHome == m_engine->isHome()) ? m_engine->getPlayerClub() : m_engine->getOpponentClub();
    std::string name = atk ? atk->name : std::string("The attacking side");
    std::string tag = "[" + std::to_string(m_engine->getMinute()) + "'] ";

    if (m_cornerSuccess) {
        e.type = EventType::Goal; e.outcome = EventOutcome::Goal;
        Player* p = m_gameManager->getPlayer();
        if (m_cornerUserTakes && p) {
            p->assists++;   // his delivery, someone else's head
            e.text = tag + "GOAL! Headed in from " + p->name + "'s corner!";
        } else {
            e.text = tag + "GOAL! " + name + " score from the corner!";
        }
    } else {
        e.type = EventType::Chance; e.outcome = EventOutcome::Saved;
        e.text = tag + name + "'s corner is cleared.";
    }
    m_engine->commitEvent(e);
    m_visibleLogs.push_back(e);
    if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
}

void MatchScreen::strikeFreeKick(bool success, ActionVariant variant) {
    m_fkSuccess = success;
    m_fkVariant = variant;
    m_fkStruck = true;
    m_stateTimer = 0.f;

    m_ballCarrierIdx = -1;
    m_fkHitWall = false;
    float goalX = m_fkAttackHome ? 845.f : 35.f;

    if (success) {
        // Bent over the wall into a corner of the goal mouth (y 250..330), carried a touch
        // past the line so it visibly ends up in the net.
        float side = (rand() % 2 == 0) ? -1.f : 1.f;
        m_ballTarget = sf::Vector2f(m_fkAttackHome ? goalX + 22.f : goalX - 22.f, 290.f + side * 26.f);
        return;
    }

    // A failed kick has to LOOK failed. It used to be aimed inside the goal mouth, so a
    // missed timing bar still sent the ball into the net and only the scoreboard disagreed.
    // Now it either cannons into the wall or clears the frame entirely.
    if (m_fkWallCount > 0 && rand() % 2 == 0) {
        m_fkHitWall = true;
        m_ballTarget = m_fkWallPos[rand() % m_fkWallCount];
    } else {
        // The goal spans y 250..330 - put it comfortably outside a post (or high over).
        float side = (rand() % 2 == 0) ? -1.f : 1.f;
        m_ballTarget = sf::Vector2f(goalX, 290.f + side * (78.f + rand() % 40));
    }
}

void MatchScreen::registerFreeKickOutcome() {
    if (m_fkUserTaker) {
        // The user struck it: credit it through the normal result path so his rating, goal
        // tally and the score update exactly as an open-play shot would. Then consume the
        // log it enqueues, so the score updates and it isn't re-staged as an open-play move.
        MinigameResult r = buildMinigameResult(m_fkSuccess, MinigameActionKind::Shot, m_fkVariant);
        m_engine->processMinigameResult(r);
        while (m_engine->hasLogs()) {
            MatchEvent e = m_engine->popRecentLog();
            m_engine->commitEvent(e);
            m_visibleLogs.push_back(e);
            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
        }
    } else {
        // An AI free kick: build the outcome for the attacking side directly.
        MatchEvent e;
        e.isHome = m_fkAttackHome;
        Club* atk = (m_fkAttackHome == m_engine->isHome()) ? m_engine->getPlayerClub() : m_engine->getOpponentClub();
        std::string name = atk ? atk->name : std::string("The attacker");
        std::string tag = "[" + std::to_string(m_engine->getMinute()) + "'] ";
        if (m_fkSuccess) {
            e.type = EventType::Goal; e.outcome = EventOutcome::Goal;
            e.text = tag + "GOAL! " + name + " scores direct from the free kick!";
        } else if (m_fkHitWall) {
            e.type = EventType::Chance; e.outcome = EventOutcome::Saved;
            e.text = tag + name + "'s free kick smashes straight into the wall.";
        } else {
            e.type = EventType::Chance; e.outcome = EventOutcome::Miss;
            e.text = tag + name + "'s free kick flies wide of the post.";
        }
        m_engine->commitEvent(e);
        m_visibleLogs.push_back(e);
        if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
    }
}

void MatchScreen::updateVisuals(sf::Time deltaTime) {
    float dt = deltaTime.asSeconds();
    m_foulClock += dt; // real time, before the speed multiplier below

    // Scripts/animation run at the chosen multiplier (0.5x..2.5x). Previously only Fast
    // (2x) and Instant (8x!) were handled - the latter is why scripts were a blur.
    dt *= matchSpeedMult(g_settings.matchSpeed);

    m_stateTimer += dt;
    float mom = 0.0f;
    if (!m_engine->getMomentumHistory().empty()) mom = m_engine->getMomentumHistory().back();

    // Give everyone a living baseline before the scripts run - the scripts below then
    // overwrite their own participants. Skipped at kick-off, where the sides are already
    // placed on their marks and shouldn't wander. Without this, every player not named by
    // the current script simply stood still for the whole episode.
    if (m_visualState != VisualState::Kickoff) {
        updateAmbientShape();
    }

    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };

    // Ball off the side of the pitch during live play - award a throw-in rather than
    // leaving it out of play for NormalPlay to drag back across the field.
    if (m_visualState == VisualState::NormalPlay || m_visualState == VisualState::Attacking) {
        beginThrowInIfOut();
    }

    if (m_visualState == VisualState::Kickoff) {
        if (m_stateTimer > 2.0f) {
            m_visualState = VisualState::NormalPlay;
            m_stateTimer = 0.f;
            int kickerIdx = m_pendingEvent.isHome ? 11 + 9 : 9;
            if (m_engine->getMinute() <= 1) kickerIdx = 9;
            m_ballCarrierIdx = -1;
            m_ballTarget = m_dots[kickerIdx + 1].shape.getPosition(); // Pass back
            
            // Pop the Kick-Off event if it's the very first minute so it doesn't stay in queue
            if (m_engine->getMinute() == 0 && m_engine->hasLogs()) {
                m_pendingEvent = m_engine->popRecentLog();
                m_visibleLogs.push_back(m_pendingEvent);
            }
        }
    }
    else if (m_visualState == VisualState::GoalKick) {
        int gkIdx = m_pendingEvent.isHome ? 11 : 0; // Defending GK takes the kick
        m_ballTarget = m_dots[gkIdx].shape.getPosition();
        float distToGk = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
        if (distToGk < 10.f) {
            m_ballCarrierIdx = gkIdx;
            if (m_stateTimer > 1.5f) {
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
                int teamBase = (gkIdx == 0) ? 0 : 11;
                m_ballCarrierIdx = teamBase + 1 + (rand() % 10);
            }
        } else {
            m_stateTimer = 0.f; // Freeze timer while ball travels
        }
    }
    else if (m_visualState == VisualState::NormalPlay) {
        // Old random minigame trigger deleted
        static float s_idleTime = 0.f;
        s_idleTime += dt;
        
        float shift = mom * 1.5f;
        for (size_t i = 0; i < 22; ++i) {
            int idx = i % 11;
            float tx = (i < 11) ? 50.f + form[idx][0] * 780.f : 830.f - form[idx][0] * 780.f;
            if (idx != 0) tx += (i < 11) ? shift : shift;
            float ty = 140.f + form[idx][1] * 300.f;
            
            float swayX = std::sin(s_idleTime * 2.0f + i) * 6.0f;
            float swayY = std::cos(s_idleTime * 1.5f + i) * 6.0f;

            m_dots[i].targetPos = sf::Vector2f(tx + swayX, ty + swayY);
            // Everyone strolls at the same pace here. Without this they kept whatever
            // speed the last script gave them (a Counter carrier still on 230), so play
            // resumed with players zipping back to their slots at sprint pace.
            m_dots[i].speed = 55.f;
        }
        
        if (m_ballCarrierIdx == -1 || (rand() % 100 < 2 && std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y) < 10.f)) {
            bool pickHome = (mom > 0); if (mom == 0) pickHome = (rand()%2 == 0);
            do {
                m_ballCarrierIdx = (pickHome ? 0 : 11) + 1 + (rand() % 8);
            } while(hasRedCard(m_ballCarrierIdx));
        }
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
        
    } 
    else if (m_visualState == VisualState::Attacking) {
        updateAttackEpisode(dt);
    }
    else if (m_visualState == VisualState::GoalCelebration) {
        if (m_stateTimer > 3.0f) {
            resetToKickoff();
        }
    }
    else if (m_visualState == VisualState::FoulChallenge) {
        // Show the foul being committed: the offender lunges into the victim over the
        // ball, the victim stumbles, then the whistle goes and we settle into the dead
        // ball. Without this the ball just stopped for no visible reason.
        m_ballCarrierIdx = -1;
        sf::Vector2f spot = m_visualBall.getPosition();

        // Fixed targets from beginFoul - both men converge and stop, rather than the
        // victim's target fleeing ahead of him every frame.
        if (m_foulOffenderIdx >= 0 && m_foulVictimIdx >= 0) {
            m_dots[m_foulOffenderIdx].targetPos = m_foulLungeTarget;
            m_dots[m_foulOffenderIdx].speed = 260.f; // charges in
            m_dots[m_foulVictimIdx].targetPos = m_foulStaggerTarget;
            m_dots[m_foulVictimIdx].speed = 130.f;   // knocked back
        }

        // Zoom in on the incident so the challenge is actually visible - on the full-pitch
        // view a 40px lunge among 22 dots was impossible to spot, which is why "the
        // challenge isn't visible". Frame the offender/victim, or the ball if unknown.
        sf::Vector2f focus = spot;
        if (m_foulOffenderIdx >= 0 && m_foulVictimIdx >= 0) {
            focus = (m_dots[m_foulOffenderIdx].shape.getPosition()
                   + m_dots[m_foulVictimIdx].shape.getPosition()) * 0.5f;
        }
        m_currentZoom += (0.5f - m_currentZoom) * 3.0f * dt;
        float vw = 1280.f * m_currentZoom, vh = 720.f * m_currentZoom;
        float minX = 40.f + vw / 2.f, maxX = 840.f - vw / 2.f;
        float minY = 130.f + vh / 2.f, maxY = 450.f - vh / 2.f;
        sf::Vector2f tc = focus;
        tc.x = (minX > maxX) ? 440.f : std::clamp(tc.x, minX, maxX);
        tc.y = (minY > maxY) ? 290.f : std::clamp(tc.y, minY, maxY);
        sf::Vector2f cc = m_camera.getCenter();
        cc += (tc - cc) * 4.0f * dt;
        m_camera.setCenter(cc);
        m_camera.setSize(vw, vh);

        // Keep the ball dead on the spot through the challenge.
        m_visualBall.setPosition(spot);
        m_ballTarget = spot;
        m_ballVelocity = sf::Vector2f(0.f, 0.f);

        // Real-time beat (m_foulClock, not the speed-scaled m_stateTimer) so the challenge
        // reads the same at 0.5x or 2.5x - long enough to see the contact and a beat to
        // register it. Then the whistle settles into the dead ball (which pulls the camera
        // back out).
        if (m_foulClock > 1.6f) {
            setupFreeKick(m_foulOffenderIsHome);
        }
    }
    else if (m_visualState == VisualState::Foul) {
        // Dead ball: it stays on the spot while the taker walks up to it, then he
        // knocks it back into play.
        m_ballCarrierIdx = -1;
        m_ballTarget = m_visualBall.getPosition();

        // Pull the camera back out from the challenge zoom to the wide match view before
        // play restarts.
        m_currentZoom += (1.0f - m_currentZoom) * 3.0f * dt;
        if (m_currentZoom > 0.98f) { m_currentZoom = 1.0f; m_camera = m_uiView; }
        else {
            sf::Vector2f cc = m_camera.getCenter();
            cc += (sf::Vector2f(640.f, 360.f) - cc) * 3.0f * dt;
            m_camera.setCenter(cc);
            m_camera.setSize(1280.f * m_currentZoom, 720.f * m_currentZoom);
        }

        bool takerReady = false;
        if (m_foulPlayerIdx >= 0 && m_foulPlayerIdx < (int)m_dots.size()) {
            // Re-assert the taker's walk-up every frame. setupFreeKick sets it once, but
            // updateAmbientShape now runs first and would drag him back to his formation
            // slot - he'd never reach the ball, takerReady would never fire, and the 6s
            // safety net would restart the ball with nobody near it.
            m_dots[m_foulPlayerIdx].targetPos = m_visualBall.getPosition();
            m_dots[m_foulPlayerIdx].speed = 120.f;

            sf::Vector2f d = m_dots[m_foulPlayerIdx].shape.getPosition() - m_visualBall.getPosition();
            takerReady = std::hypot(d.x, d.y) < 16.f;
        }

        // Hold the wall in place while it forms - ambient would otherwise drag the
        // defenders back to their formation slots the moment they arrive.
        for (int k = 0; k < m_fkWallCount; ++k) {
            int idx = m_fkWall[k];
            if (idx >= 0 && idx < (int)m_dots.size()) {
                m_dots[idx].targetPos = m_fkWallPos[k];
                m_dots[idx].speed = 150.f;
            }
        }

        // Hold the dead ball for a beat so the foul actually reads as a foul: the whistle
        // goes, everything settles on the spot, the taker walks up, and only then is it
        // put back into play. The 6s arm is a safety net so a blocked taker can never
        // stall the match.
        const float DEAD_BALL_PAUSE = 3.0f;
        if ((takerReady && m_stateTimer > DEAD_BALL_PAUSE) || m_stateTimer > 6.0f) {
            m_sendOffGraceIdx = -1; // the sent-off man now leaves the pitch as play resumes
            if (m_fkDirect) {
                // A direct free kick: line the shot up rather than knocking it back into
                // play. The user times a shot on the timing bar; an AI taker's outcome is
                // pre-rolled here and played out as an animated strike.
                m_visualState = VisualState::FreeKickShot;
                m_stateTimer = 0.f;
                m_fkWindup = 0.f;
                m_fkStruck = false;
                m_fkResolved = false;
                if (m_fkUserTaker) {
                    startQTE(MinigameActionKind::Shot, ActionVariant::Default, false, 1.0f);
                } else {
                    // Attacking club strength vs the keeper decides it; direct free kicks
                    // are low percentage, so keep the base modest.
                    Club* atk = (m_fkAttackHome == m_engine->isHome()) ? m_engine->getPlayerClub() : m_engine->getOpponentClub();
                    int str = atk ? atk->strength : 60;
                    int chance = std::clamp(6 + (str - 55) / 3, 4, 45);
                    m_fkSuccess = (rand() % 100) < chance;
                }
            } else {
                if (m_foulPlayerIdx >= 0 && m_foulPlayerIdx < (int)m_dots.size()) {
                    m_ballCarrierIdx = m_foulPlayerIdx;
                    m_dots[m_foulPlayerIdx].speed = 100.f;
                }
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
        }
    }
    else if (m_visualState == VisualState::Corner) {
        // Hold the packed box and the taker in place - ambient would scatter them back to
        // their formation slots within a frame.
        for (int k = 0; k < m_cornerCrowdCount; ++k) {
            int idx = m_cornerCrowd[k];
            if (idx >= 0 && idx < (int)m_dots.size()) {
                m_dots[idx].targetPos = m_cornerCrowdPos[k];
                m_dots[idx].speed = 150.f;
            }
        }
        if (m_cornerTaker >= 0 && m_cornerTaker < (int)m_dots.size() && !m_cornerStruck) {
            m_dots[m_cornerTaker].targetPos = m_cornerSpot;
            m_dots[m_cornerTaker].speed = 150.f;
        }

        if (m_cornerDeflecting) {
            // The ball is still running out of play. Nobody chases it; the box fills up
            // behind it (the crowd targets above are already set), then it's placed at the
            // flag once it has clearly crossed the line.
            m_ballCarrierIdx = -1;
            m_ballTarget = m_cornerDeflectTarget;
            sf::Vector2f bp = m_visualBall.getPosition();
            float d = std::hypot(bp.x - m_cornerDeflectTarget.x, bp.y - m_cornerDeflectTarget.y);
            if (d < 12.f || m_foulClock > 2.0f) {
                m_cornerDeflecting = false;
                m_visualBall.setPosition(m_cornerSpot);
                m_ballTarget = m_cornerSpot;
                m_foulClock = 0.f; // now start the beat that lets the box settle
            }
        } else if (!m_cornerStruck) {
            m_ballCarrierIdx = -1;
            m_visualBall.setPosition(m_cornerSpot);
            m_ballTarget = m_cornerSpot;

            // Give the box time to fill before it's whipped in.
            if (m_foulClock > 2.5f) {
                if (m_cornerUserTakes) {
                    if (!m_qte.isActive()) startQTE(MinigameActionKind::Pass, ActionVariant::Lofted, false, 1.0f);
                    m_qte.update(deltaTime.asSeconds()); // real time, fair at any match speed
                    if (m_qte.isExpired()) { m_qte.cancel(); deliverCorner(false); }
                } else {
                    m_cornerWindup += deltaTime.asSeconds();
                    if (m_cornerWindup > 0.8f) deliverCorner(rand() % 100 < 55);
                }
            }
        } else if (!m_cornerHeaded) {
            // The cross is in the air. Keep steering it onto the man - aiming at the spot he
            // stood on when it was struck left the ball hanging over empty grass while he
            // ran on. A user forward times his header against the flight.
            m_ballCarrierIdx = -1;
            m_ballTarget = cornerAimPoint();

            if (m_cornerHeaderPending) {
                m_qte.update(deltaTime.asSeconds());
                if (m_qte.isExpired()) { m_qte.cancel(); m_cornerHeaderPending = false; m_cornerSuccess = false; }
            }

            sf::Vector2f bp = m_visualBall.getPosition();
            float d = std::hypot(bp.x - m_ballTarget.x, bp.y - m_ballTarget.y);
            if (d < 14.f || m_stateTimer > 3.0f) {
                if (m_cornerHeaderPending) { m_qte.cancel(); m_cornerHeaderPending = false; m_cornerSuccess = false; }
                if (!m_cornerUserHead) {
                    // A good delivery still has to be finished off.
                    m_cornerSuccess = m_cornerGoodDelivery && (rand() % 100 < 40);
                }

                // The ball has been met - now play the finish out instead of freezing it in
                // mid-air for a beat and then teleporting it to the keeper.
                m_cornerHeaded = true;
                m_stateTimer = 0.f;
                float goalX = m_cornerAttackHome ? 845.f : 35.f;
                if (m_cornerSuccess) {
                    // Headed past the keeper and over the line.
                    m_ballTarget = sf::Vector2f(m_cornerAttackHome ? goalX + 22.f : goalX - 22.f,
                                                290.f + ((rand() % 2 == 0) ? -24.f : 24.f));
                } else {
                    // Headed away / gathered: it ends up with the defending keeper.
                    int gk = m_cornerAttackHome ? 11 : 0;
                    m_ballTarget = m_dots[gk].shape.getPosition();
                }
            }
        } else if (!m_cornerResolved) {
            // The finish is in flight; resolve when it gets there.
            m_ballCarrierIdx = -1;
            sf::Vector2f bp = m_visualBall.getPosition();
            float d = std::hypot(bp.x - m_ballTarget.x, bp.y - m_ballTarget.y);
            if (d < 14.f || m_stateTimer > 2.0f) {
                registerCornerOutcome();
                m_cornerResolved = true;
                m_stateTimer = 0.f;
            }
        } else if (m_stateTimer > 1.0f) {
            m_deadBallTakerIdx = -1;
            if (m_cornerSuccess) {
                m_pendingEvent.isHome = m_cornerAttackHome; // so the right side kicks off
                resetToKickoff();
            } else {
                int gk = m_cornerAttackHome ? 11 : 0; // cleared by the defending keeper
                m_ballCarrierIdx = gk;
                m_lastToucherIdx = gk;
                m_visualState = VisualState::NormalPlay;
                m_stateTimer = 0.f;
            }
        }
    }
    else if (m_visualState == VisualState::ThrowIn) {
        // Dead ball on the touchline: the taker walks over, and after a real-time beat he
        // throws it back to the nearest team-mate.
        m_ballCarrierIdx = -1;
        m_visualBall.setPosition(m_throwInSpot);
        m_ballTarget = m_throwInSpot;
        m_ballVelocity = sf::Vector2f(0.f, 0.f);

        if (m_throwInTaker >= 0 && m_throwInTaker < (int)m_dots.size()) {
            // Re-asserted every frame: updateAmbientShape runs first and would otherwise
            // pull him back to his formation slot before he ever reaches the ball. He stands
            // just BEHIND the line, as a thrower does, rather than inside the field.
            sf::Vector2f stand = m_throwInSpot;
            stand.y += (m_throwInSpot.y < 290.f) ? -9.f : 9.f;
            m_dots[m_throwInTaker].targetPos = stand;
            m_dots[m_throwInTaker].speed = 140.f;
        }

        if (m_foulClock > 3.0f) {
            int mate = -1; float bd = 1e9f;
            if (m_throwInTaker >= 0) {
                int base = (m_throwInTaker < 11) ? 0 : 11;
                for (int i = base; i < base + 11; ++i) {
                    if (i == m_throwInTaker || i % 11 == 0) continue;
                    if (hasRedCard(i)) continue;
                    sf::Vector2f d = m_dots[i].shape.getPosition() - m_throwInSpot;
                    float dd = std::hypot(d.x, d.y);
                    if (dd < bd) { bd = dd; mate = i; }
                }
            }
            m_ballCarrierIdx = (mate >= 0) ? mate : m_throwInTaker;
            m_lastToucherIdx = m_ballCarrierIdx;
            m_deadBallTakerIdx = -1; // back under the normal keep-on-the-pitch clamp
            m_visualState = VisualState::NormalPlay;
            m_stateTimer = 0.f;
        }
    }
    else if (m_visualState == VisualState::FreeKickShot) {
        float goalX = m_fkAttackHome ? 845.f : 35.f;
        sf::Vector2f ballPos = m_visualBall.getPosition();

        // Frame the ball and the goal it's aimed at.
        sf::Vector2f focus((ballPos.x + goalX) * 0.5f, 290.f);
        m_currentZoom += (0.62f - m_currentZoom) * 3.0f * dt;
        float vw = 1280.f * m_currentZoom, vh = 720.f * m_currentZoom;
        float minX = 40.f + vw / 2.f, maxX = 840.f - vw / 2.f;
        float minY = 130.f + vh / 2.f, maxY = 450.f - vh / 2.f;
        sf::Vector2f tc = focus;
        tc.x = (minX > maxX) ? 440.f : std::clamp(tc.x, minX, maxX);
        tc.y = (minY > maxY) ? 290.f : std::clamp(tc.y, minY, maxY);
        sf::Vector2f cc = m_camera.getCenter();
        cc += (tc - cc) * 4.0f * dt;
        m_camera.setCenter(cc);
        m_camera.setSize(vw, vh);

        // Keep the wall standing and the keeper on his line through the kick.
        for (int k = 0; k < m_fkWallCount; ++k) {
            int idx = m_fkWall[k];
            if (idx >= 0 && idx < (int)m_dots.size()) {
                m_dots[idx].targetPos = m_fkWallPos[k];
                m_dots[idx].speed = 60.f;
            }
        }

        if (!m_fkStruck) {
            // Hold the ball on the spot; the taker stands over it.
            m_ballCarrierIdx = -1;
            if (m_foulPlayerIdx >= 0 && m_foulPlayerIdx < (int)m_dots.size())
                m_dots[m_foulPlayerIdx].targetPos = m_visualBall.getPosition();

            if (m_fkUserTaker) {
                // Real-time so the bar plays fair at any match speed. Space/click locks it
                // (handleInput); if it expires it's a wild effort.
                m_qte.update(deltaTime.asSeconds());
                if (m_qte.isExpired()) { m_qte.cancel(); strikeFreeKick(false, ActionVariant::Default); }
            } else {
                m_fkWindup += deltaTime.asSeconds();
                if (m_fkWindup > 0.9f) strikeFreeKick(m_fkSuccess, ActionVariant::Default);
            }
        } else if (!m_fkResolved) {
            // The ball is on its way (m_ballTarget lerp carries it). The keeper springs
            // toward its line; register the outcome once it arrives.
            if (m_fkKeeperIdx >= 0 && m_fkKeeperIdx < (int)m_dots.size()) {
                // He covers his goal: on a scoring kick he dives the wrong way, otherwise he
                // tracks the flight but stays inside the frame rather than wandering off it.
                float coverY = m_fkSuccess ? (290.f + (m_ballTarget.y > 290.f ? -30.f : 30.f))
                                           : std::clamp(m_ballTarget.y, 250.f, 330.f);
                m_dots[m_fkKeeperIdx].targetPos = sf::Vector2f(goalX, coverY);
                m_dots[m_fkKeeperIdx].speed = 240.f;
            }
            // A kick blocked by the wall never reaches the goal line, so resolve on arrival
            // at whatever it was aimed at as well as on crossing.
            float distToTarget = std::hypot(ballPos.x - m_ballTarget.x, ballPos.y - m_ballTarget.y);
            bool crossed = m_fkAttackHome ? (ballPos.x > goalX - 12.f) : (ballPos.x < goalX + 12.f);
            if (distToTarget < 12.f || crossed || m_stateTimer > 3.0f) {
                registerFreeKickOutcome();
                m_fkResolved = true;
                m_stateTimer = 0.f;
            }
        } else {
            // Brief beat on the outcome, then restart appropriately.
            if (m_stateTimer > 1.2f) {
                m_fkDirect = false;
                if (m_fkSuccess) {
                    m_pendingEvent.isHome = m_fkAttackHome; // the scorer, so the right side kicks off
                    resetToKickoff();
                } else {
                    m_currentZoom = 1.0f; m_camera = m_uiView;
                    m_visualState = VisualState::NormalPlay;
                    m_stateTimer = 0.f;
                    if (m_fkKeeperIdx >= 0) { m_ballCarrierIdx = m_fkKeeperIdx; } // keeper restarts
                }
            }
        }
    }

    updateDotMotion(dt);

    // Move ball globally
    if (m_ballCarrierIdx != -1) {
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
        m_lastToucherIdx = m_ballCarrierIdx; // whoever is on the ball touched it last
    }
    
    float globalBDist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
    if (m_ballCarrierIdx != -1 && globalBDist < 10.f) {
        m_visualBall.setPosition(m_ballTarget); // Snap rigidly if close and held
    } else if (globalBDist > 0.f) {
        sf::Vector2f bdir = m_ballTarget - m_visualBall.getPosition();
        float bspeed = (m_visualState == VisualState::Attacking) ? 500.f : 400.f;
        if (m_visualState == VisualState::Corner) {
            // A parried ball trickles out of play - at full pace it crossed the byline in a
            // couple of frames and the corner still looked like a teleport. The cross itself
            // is floated, so it hangs rather than being fired in like a shot.
            if (m_cornerDeflecting) bspeed = 190.f;
            else if (m_cornerStruck && !m_cornerHeaded) bspeed = 300.f;
        }
        float moveDist = bspeed * dt;
        if (moveDist >= globalBDist) {
            m_visualBall.setPosition(m_ballTarget);
        } else {
            m_visualBall.move((bdir.x / globalBDist) * moveDist, (bdir.y / globalBDist) * moveDist);
        }
    }
}


void MatchScreen::updateAttackEpisode(float dt) {
    EpisodeCtx ctx;
    ctx.attackerBase = m_pendingEvent.isHome ? 0 : 11;
    ctx.defenderBase = m_pendingEvent.isHome ? 11 : 0;
    ctx.isGoal = (m_pendingEvent.outcome == EventOutcome::Goal);
    ctx.isSave = (m_pendingEvent.outcome == EventOutcome::Saved);
    ctx.isMiss = (!ctx.isGoal && !ctx.isSave);
    ctx.ballDist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x,
                              m_ballTarget.y - m_visualBall.getPosition().y);

    switch (m_attackPhase) {
        case Beat::Setup: runEpisodeSetup(dt, ctx); break;
        case Beat::CrossInFlight: runWingCross(dt, ctx); break;
        case Beat::DefTacklePrep: runDefenderTackle(dt, ctx); break;
        case Beat::DefTackleClose: runDefenderTackle(dt, ctx); break;
        case Beat::DefTacklePause: runDefenderTackle(dt, ctx); break;
        case Beat::DefTackleResolve: runDefenderTackle(dt, ctx); break;
        case Beat::Shot: runShotResolution(dt, ctx); break;
        case Beat::Resolve: runShotResolution(dt, ctx); break;
        case Beat::MidPassHold: runMidfielderPass(dt, ctx); break;
        case Beat::MidPassResolve: runMidfielderPass(dt, ctx); break;
        case Beat::PassInFlight: runMidfielderPass(dt, ctx); break;
        case Beat::PassIntercepted: runMidfielderPass(dt, ctx); break;
        case Beat::PassReceived: runMidfielderPass(dt, ctx); break;
        case Beat::MidTackleChase: runMidfielderTackle(dt, ctx); break;
        case Beat::MidTackleResolve: runMidfielderTackle(dt, ctx); break;
        case Beat::MidTackleWon: runMidfielderTackle(dt, ctx); break;
        case Beat::SoloRun: runSoloRun(dt, ctx); break;
        case Beat::SoloRunResolve: runSoloRun(dt, ctx); break;
        case Beat::GkShotWindup: runGoalkeeperSave(dt, ctx); break;
    }

    // Defenders track whoever is carrying the ball during the build-up beats.
    bool ballIsCarried = (m_attackPhase == Beat::Setup
                       || m_attackPhase == Beat::CrossInFlight
                       || m_attackPhase == Beat::DefTacklePrep
                       || m_attackPhase == Beat::MidPassHold
                       || m_attackPhase == Beat::PassReceived
                       || m_attackPhase == Beat::MidTackleChase
                       || m_attackPhase == Beat::GkShotWindup);
    if (ballIsCarried && m_ballCarrierIdx != -1) {
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();

        int numDefendersToRun = 4;
        if (m_attackPhase == Beat::MidPassHold || m_attackPhase == Beat::PassReceived) {
            numDefendersToRun = m_attackWingerIdx; // the value we stored (1 or 2)
        } else if (m_attackShape == AttackShape::Counter) {
            numDefendersToRun = 1; // caught out - only one defender recovers, so it's a real break
        }

        for (int i = 1; i <= numDefendersToRun; ++i) {
            sf::Vector2f carrierPos = m_dots[m_ballCarrierIdx].shape.getPosition();
            float offsetX = m_pendingEvent.isHome ? (i * 15.f) : -(i * 15.f);
            float offsetY = (i%2==0) ? (i * 20.f) : -(i * 20.f);
            m_dots[ctx.defenderBase + i].targetPos = sf::Vector2f(carrierPos.x + offsetX, carrierPos.y + offsetY);
            m_dots[ctx.defenderBase + i].speed = 150.f; // tracking runners
        }
    }
}

AttackShape MatchScreen::pickAttackShape(bool attackingHome) const {
    // Weighted pick, not hard rules: every shape keeps a floor weight so anything can
    // still turn up. The situation only tilts the odds, so attacks stay unpredictable
    // but stop feeling like a flat dice roll.
    float w[6];
    w[(int)AttackShape::WingCross]    = 10.f;
    w[(int)AttackShape::SoloRun]      = 8.f;
    w[(int)AttackShape::CenterAttack] = 10.f;
    w[(int)AttackShape::Counter]      = 5.f;
    w[(int)AttackShape::ThroughBall]  = 6.f;
    w[(int)AttackShape::LongShot]     = 4.f;

    // Momentum: +100 = home dominating, -100 = away. A sharp swing toward the side now
    // attacking reads as a turnover won high up -> favour the fast break.
    float momentum = m_engine->getMomentumHistory().empty() ? 0.f : m_engine->getMomentumHistory().back();
    float towardUs = attackingHome ? momentum : -momentum; // >0 = this side is on top
    if (towardUs > 40.f) w[(int)AttackShape::Counter] += 8.f;

    // Score: a side that's behind gambles more (long shots, balls in behind); a side in
    // front keeps it patient on the flanks/through the middle.
    int goalDiff = m_engine->getHomeScore() - m_engine->getAwayScore();
    int ourLead = attackingHome ? goalDiff : -goalDiff;
    if (ourLead < 0) {
        w[(int)AttackShape::LongShot]    += 6.f;
        w[(int)AttackShape::ThroughBall] += 5.f;
        w[(int)AttackShape::Counter]     += 3.f;
    } else if (ourLead > 0) {
        w[(int)AttackShape::WingCross]    += 5.f;
        w[(int)AttackShape::CenterAttack] += 4.f;
    }

    // Relative strength: the stronger side builds patiently; the weaker side has to be
    // direct to hurt them.
    int myStr  = attackingHome ? (m_engine->isHome() ? m_engine->getPlayerClub()->strength : m_engine->getOpponentClub()->strength)
                               : (m_engine->isHome() ? m_engine->getOpponentClub()->strength : m_engine->getPlayerClub()->strength);
    int oppStr = attackingHome ? (m_engine->isHome() ? m_engine->getOpponentClub()->strength : m_engine->getPlayerClub()->strength)
                               : (m_engine->isHome() ? m_engine->getPlayerClub()->strength : m_engine->getOpponentClub()->strength);
    int edge = myStr - oppStr;
    if (edge > 8) {
        w[(int)AttackShape::WingCross]    += 5.f;
        w[(int)AttackShape::CenterAttack] += 4.f;
    } else if (edge < -8) {
        w[(int)AttackShape::Counter]  += 5.f;
        w[(int)AttackShape::LongShot] += 4.f;
    }

    float total = 0.f;
    for (float x : w) total += x;
    float r = (rand() % 1000) / 1000.f * total;
    for (int i = 0; i < 6; ++i) {
        if (r < w[i]) return (AttackShape)i;
        r -= w[i];
    }
    return AttackShape::CenterAttack;
}

void MatchScreen::runEpisodeSetup(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::Setup) {
                // Phase 0: Setup and build up
                Player* p = m_gameManager->getPlayer();

                bool isDefenderTackleMinigame = p && p->position == PlayerPosition::Defender && (m_pendingEvent.isHome != m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;
                bool isMidfielderTackleMinigame = p && p->position == PlayerPosition::Midfielder && (m_pendingEvent.isHome != m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;
                bool isMidfielderPassMinigame = p && p->position == PlayerPosition::Midfielder && (m_pendingEvent.isHome == m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;
                bool isGoalkeeperSaveMinigame = p && p->position == PlayerPosition::Goalkeeper && (m_pendingEvent.isHome != m_engine->isHome()) && m_pendingEvent.type == EventType::PendingMinigame;

                if (isGoalkeeperSaveMinigame) {
                    // Goalkeeper: an attacker winds up and strikes toward goal, instead of
                    // falling through to the generic "freeze the ball, then trigger" fallback
                    // in Phase 2 (which handed the GK a stationary ball to walk up to).
                    m_attackPhase = Beat::GkShotWindup;
                    m_stateTimer = 0.f;
                    do {
                        m_attackFwdIdx = ctx.attackerBase + 9 + (rand() % 2);
                    } while (hasRedCard(m_attackFwdIdx));
                    m_shotTargetY = 290.f + (rand() % 60 - 30.f);
                    m_ballCarrierIdx = m_attackFwdIdx;
                    return;
                }

                if (isDefenderTackleMinigame) {
                    m_attackPhase = Beat::DefTacklePrep;
                    m_stateTimer = 0.f;
                    int options[] = {9, 10};
                    m_attackFwdIdx = liveTeammate(ctx.attackerBase + options[rand() % 2]);

                    m_attackWingerIdx = liveTeammate(ctx.attackerBase + 5); // Use winger index as passer
                    m_ballCarrierIdx = m_attackWingerIdx;

                    int userPosIdx = 3;
                    int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;

                    sf::Vector2f defPos = m_dots[myDefenderIdx].shape.getPosition();
                    sf::Vector2f attPos = m_dots[m_attackFwdIdx].shape.getPosition();

                    // Pass target is the midpoint
                    m_ballTarget = sf::Vector2f((defPos.x + attPos.x) / 2.f, (defPos.y + attPos.y) / 2.f);
                    return;
                } else if (isMidfielderTackleMinigame) {
                    m_attackPhase = Beat::MidTackleChase;
                    m_stateTimer = 0.f;
                    int options[] = {7, 8, 9, 10};
                    m_ballCarrierIdx = liveTeammate(ctx.attackerBase + options[rand() % 4]);
                    m_attackFwdIdx = liveTeammate(ctx.attackerBase + 10);
                    m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition(); // user will sprint here
                    return;
                } else if (isMidfielderPassMinigame) {
                    if (rand() % 100 < 20) {
                        m_attackPhase = Beat::SoloRun; // Solo Run
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = (m_engine->isHome() ? 0 : 11) + 7;
                        m_dots[m_ballCarrierIdx].targetPos = m_engine->isHome() ? sf::Vector2f(720.f, m_shotTargetY) : sf::Vector2f(160.f, m_shotTargetY);
                        m_dots[m_ballCarrierIdx].speed = 180.f;
                    } else {
                        m_attackPhase = Beat::MidPassHold; // Pass
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = (m_engine->isHome() ? 0 : 11) + 7; // User midfielder

                        int myBase = m_engine->isHome() ? 0 : 11;
                        if (rand() % 100 < 50) {
                            m_attackFwdIdx = liveTeammate(myBase + 9 + (rand() % 2)); // Forward
                            m_passForward = true;
                        } else {
                            int t = myBase + 2 + (rand() % 6); // Defs/Mids
                            if (t == myBase + 7) t = myBase + 8; // Avoid self
                            m_attackFwdIdx = liveTeammate(t);
                            m_passForward = false; // sideways/back
                        }

                        m_attackWingerIdx = 1 + (rand() % 2); // Store number of defenders to run (1 or 2)
                    }
                    return;
                }

                // Attacking third X for whichever side is going forward.
                float boxX = m_pendingEvent.isHome ? 700.f : 180.f;
                float deepX = m_pendingEvent.isHome ? 470.f : 410.f; // just past halfway

                if (m_attackShape == AttackShape::WingCross) {
                    m_ballCarrierIdx = m_attackWingerIdx;
                    m_dots[m_attackWingerIdx].targetPos = sf::Vector2f(boxX, m_attackWingerIdx%11==5 ? 160.f : 420.f);
                    m_dots[m_attackWingerIdx].speed = 150.f;

                    // Striker attacks the box, but held to the offside line rather than the
                    // old fixed boxX that parked him miles beyond the last defender. On an
                    // m_offsideRun episode he deliberately steps beyond it and gets flagged.
                    bool aHome = m_pendingEvent.isHome;
                    float dir = aHome ? 1.f : -1.f;
                    float lineFwd = (offsideLineX(aHome) - 440.f) * dir;
                    float wantFwd = (boxX - 440.f) * dir;
                    float tgtFwd = m_offsideRun ? (lineFwd + 40.f) : std::min(wantFwd, lineFwd - 12.f);
                    sf::Vector2f crossTarget(440.f + tgtFwd * dir, m_shotTargetY);
                    m_dots[m_attackFwdIdx].targetPos = crossTarget;
                    m_dots[m_attackFwdIdx].speed = 150.f;

                    if (m_offsideRun && offsideBuildup(m_attackFwdIdx, aHome, m_attackWingerIdx)) return;

                    sf::Vector2f fwdPos = m_dots[m_attackFwdIdx].shape.getPosition();
                    float fwdToTarget = std::hypot(fwdPos.x - crossTarget.x, fwdPos.y - crossTarget.y);

                    // Hold the cross until he's closing in, so ball and man arrive together.
                    // The timer is a safety net if he can't get there.
                    if (m_stateTimer > 1.0f && (fwdToTarget < 70.f || m_stateTimer > 3.0f)) {
                        m_attackPhase = Beat::CrossInFlight;
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = -1; // Release ball for cross
                        m_ballTarget = crossTarget;
                    }
                } else if (m_attackShape == AttackShape::SoloRun) {
                    // Start from whoever is already closest to the ball rather than a
                    // fixed shirt number - otherwise the ball visibly flies across the
                    // pitch to him and the episode reads as a hard cut from normal play.
                    m_ballCarrierIdx = nearestToBall(ctx.attackerBase);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(boxX + (m_pendingEvent.isHome ? 20.f : -20.f), m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 180.f; // Faster run!
                    if (m_stateTimer > 1.5f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                } else if (m_attackShape == AttackShape::Counter) {
                    // Fast direct break: the carrier starts deep and sprints at goal with
                    // no build-up. Fewer defenders get back (numDefendersToRun is trimmed
                    // in the dispatcher for this shape), so it's a genuine fast break.
                    m_ballCarrierIdx = nearestToBall(ctx.attackerBase);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(boxX, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 230.f; // quicker than a solo run
                    if (m_stateTimer > 1.2f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                } else if (m_attackShape == AttackShape::ThroughBall) {
                    // A pass slid in behind the defense: a deep carrier holds it while the
                    // striker peels off, then the ball is released into space for him to run
                    // onto. Reuses the cross-flight beat to carry the ball and meet the man.
                    m_ballCarrierIdx = nearestToBall(ctx.attackerBase);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(deepX, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 140.f;

                    // The run is timed to the offside line - onside he peels off level with
                    // the last defender; on an m_offsideRun episode he breaks too early and
                    // gets flagged when the ball is slid through.
                    bool aHome = m_pendingEvent.isHome;
                    float dir = aHome ? 1.f : -1.f;
                    float lineFwd = (offsideLineX(aHome) - 440.f) * dir;
                    float wantFwd = (boxX - 440.f) * dir;
                    float tgtFwd = m_offsideRun ? (lineFwd + 40.f) : std::min(wantFwd, lineFwd - 12.f);
                    sf::Vector2f runTarget(440.f + tgtFwd * dir, m_shotTargetY);
                    m_dots[m_attackFwdIdx].targetPos = runTarget;
                    m_dots[m_attackFwdIdx].speed = 200.f; // making the run

                    if (m_offsideRun && offsideBuildup(m_attackFwdIdx, aHome, m_ballCarrierIdx)) return;

                    sf::Vector2f fwdPos = m_dots[m_attackFwdIdx].shape.getPosition();
                    float fwdToTarget = std::hypot(fwdPos.x - runTarget.x, fwdPos.y - runTarget.y);
                    if (m_stateTimer > 0.8f && (fwdToTarget < 90.f || m_stateTimer > 3.0f)) {

                        m_attackPhase = Beat::CrossInFlight;
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = -1; // slide it into space
                        m_ballTarget = runTarget;
                    }
                } else if (m_attackShape == AttackShape::LongShot) {
                    // A crack from distance: the carrier steadies himself in a deep position
                    // and lets fly without approaching the box. m_shotTargetY was set at the
                    // goal mouth; keep the carrier deep so runShotResolution fires from range
                    // (where the keeper-save and distance-scatter make it a real gamble).
                    m_ballCarrierIdx = nearestToBall(ctx.attackerBase);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(deepX, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 120.f;
                    if (m_stateTimer > 1.0f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                } else { // CenterAttack
                    m_ballCarrierIdx = m_attackFwdIdx;
                    float attackX = boxX + (rand()%20 - 10.f);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(attackX, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 150.f;
                    if (m_stateTimer > 1.5f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                }
    }
}

void MatchScreen::runWingCross(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::CrossInFlight) {
                // Phase 1: the cross is in the air.
                m_ballCarrierIdx = -1;
                m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
                m_dots[m_attackFwdIdx].speed = 160.f; // attacking the cross

                // Resolve as soon as the ball lands. This used to also demand the striker be
                // standing on the exact spot, which is what left the ball hanging motionless
                // in the middle - it had arrived and simply waited for him. He now gets a
                // head start back in phase 0, so he meets it.
                if (ctx.ballDist < 12.f || m_stateTimer > 2.0f) {
                    // If there are pending events (like the shot outcome), pop it to use for the shot visualization
                    if (m_engine->hasLogs()) {
                        m_pendingEvent = m_engine->popRecentLog();
                        if (handleFoulIfCard()) return; // a card here becomes a visible foul
                    }
                    m_attackPhase = Beat::Shot;
                    m_stateTimer = 0.f;
                }
    }
}

void MatchScreen::runDefenderTackle(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::DefTacklePrep) {
                // Wait for ball to arrive to the passer smoothly before starting the tackle minigame sequence
                m_ballCarrierIdx = m_attackWingerIdx;
                if (m_stateTimer > 1.0f) {
                    m_attackPhase = Beat::DefTackleClose;
                    m_stateTimer = 0.f;

                    Player* p = m_gameManager->getPlayer();
                    int userPosIdx = 0;
                    if (p->position == PlayerPosition::Defender) userPosIdx = 3;
                    else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
                    else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
                    int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;

                    sf::Vector2f defPos = m_dots[myDefenderIdx].shape.getPosition();
                    sf::Vector2f attPos = m_dots[m_attackFwdIdx].shape.getPosition();

                    // Pass target is the midpoint
                    m_ballTarget = sf::Vector2f((defPos.x + attPos.x) / 2.f, (defPos.y + attPos.y) / 2.f);
                }
    } else if (m_attackPhase == Beat::DefTackleClose) {
                // Defender Minigame Phase 1: Ball is passed to forward, defender runs to intercept!
                Player* p = m_gameManager->getPlayer();
                int userPosIdx = 0;
                if (p->position == PlayerPosition::Defender) userPosIdx = 3;
                else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
                else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
                int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;

                m_ballCarrierIdx = -1;

                // Attacker runs to the pass target
                m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
                m_dots[m_attackFwdIdx].speed = 100.f; 

                // Defender target is slightly offset to not overlap perfectly
                float offsetX = m_pendingEvent.isHome ? 12.f : -12.f; 
                sf::Vector2f defTarget(m_ballTarget.x + offsetX, m_ballTarget.y + 8.f);
                m_dots[myDefenderIdx].targetPos = defTarget;

                // Defender reaction time: wait slightly before sprinting
                if (m_stateTimer < 0.4f) {
                    m_dots[myDefenderIdx].speed = 0.f;
                } else {
                    m_dots[myDefenderIdx].speed = 190.f; // Sprint faster to catch up!
                }

                float defDist = std::hypot(m_dots[myDefenderIdx].shape.getPosition().x - defTarget.x, m_dots[myDefenderIdx].shape.getPosition().y - defTarget.y);
                float bDist = std::hypot(m_visualBall.getPosition().x - m_ballTarget.x, m_visualBall.getPosition().y - m_ballTarget.y);

                // When defender and ball reach their targets, trigger the minigame!
                if ((defDist < 15.f && bDist < 15.f) || m_stateTimer > 3.0f) {
                    m_engine->triggerMinigame();
                    m_attackPhase = Beat::DefTacklePause;
                    m_stateTimer = 0.f;
                }
    } else if (m_attackPhase == Beat::DefTacklePause) {
                // Defender Minigame Phase 2: Pause while minigame is active
                Player* p = m_gameManager->getPlayer();
                int userPosIdx = 0;
                if (p->position == PlayerPosition::Defender) userPosIdx = 3;
                else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
                else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
                int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;

                m_dots[m_attackFwdIdx].speed = 0.f;
                m_dots[myDefenderIdx].speed = 0.f;
    } else if (m_attackPhase == Beat::DefTackleResolve) {
                // Defender Minigame Phase 3: Resolution
                bool isTackle = (m_pendingEvent.outcome == EventOutcome::TackleWon || m_pendingEvent.outcome == EventOutcome::Intercepted);
                Player* p = m_gameManager->getPlayer();
                int userPosIdx = 0;
                if (p->position == PlayerPosition::Defender) userPosIdx = 3;
                else if (p->position == PlayerPosition::Midfielder) userPosIdx = 7;
                else if (p->position == PlayerPosition::Forward) userPosIdx = 10;
                int myDefenderIdx = (m_engine->isHome() ? 0 : 11) + userPosIdx;

                if (isTackle) {
                    // Success! Defender takes the ball
                    m_ballCarrierIdx = myDefenderIdx;
                    m_dots[myDefenderIdx].speed = 100.f;
                    m_dots[m_attackFwdIdx].speed = 100.f;

                    m_visualState = VisualState::NormalPlay;
                    m_stateTimer = 0.f;
                    m_isMinigameResultPending = false;
                    m_engine->commitEvent(m_pendingEvent);
                    m_visibleLogs.push_back(m_pendingEvent);
                    if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
                } else {
                    // Failure! Attacker shoots immediately!
                    m_attackPhase = Beat::Shot;
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = -1;
                    m_shotTargetY = 290.f + (rand()%60 - 30.f);
                    m_dots[myDefenderIdx].speed = 100.f;
                    m_dots[m_attackFwdIdx].speed = 100.f;
                }
    }
}

void MatchScreen::runShotResolution(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::Shot) {
                // A defender sometimes hauls the attacker down on the edge of the box - a
                // foul in a dangerous area, which becomes a direct free kick (wall + shot).
                // Rolled once per attack, and not while the user is lining up his own shot,
                // so it doesn't interrupt his minigame. This is the main way dangerous free
                // kicks arise, so it's what puts the wall on screen with any regularity.
                if (!m_boxFoulRolled && m_pendingEvent.type != EventType::PendingMinigame) {
                    m_boxFoulRolled = true;
                    if (rand() % 100 < 22) {
                        if (m_attackFwdIdx >= 0 && m_attackFwdIdx < (int)m_dots.size())
                            m_ballCarrierIdx = m_attackFwdIdx; // centre the challenge on the attacker
                        beginFoul(!m_pendingEvent.isHome); // the defending side commits the foul
                        return;
                    }
                }

                // Phase 2: The Shot
                m_ballCarrierIdx = -1;
                float targetY = m_shotTargetY;

                if (m_pendingEvent.type == EventType::PendingMinigame) {
                    Player* p = m_gameManager->getPlayer();
                    bool isUserForward = p && p->position == PlayerPosition::Forward && m_pendingEvent.isHome == m_engine->isHome();

                    if (isUserForward) {
                        m_engine->triggerMinigame();
                        m_stateTimer = 0.f;
                        return; 
                    } else {
                        // Freeze ball mid-air so animation can finish AFTER minigame
                        m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(750.f, targetY) : sf::Vector2f(130.f, targetY);
                        if (m_stateTimer > 0.3f) {
                            m_engine->triggerMinigame();
                            m_stateTimer = 0.f;
                            return; 
                        }
                    }
                } else {
                    if (ctx.isGoal) {
                        m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(860.f, targetY) : sf::Vector2f(20.f, targetY);
                        m_dots[ctx.defenderBase].targetPos = m_pendingEvent.isHome ? sf::Vector2f(810.f, targetY > 290.f ? 250.f : 330.f) : sf::Vector2f(70.f, targetY > 290.f ? 250.f : 330.f);
                    } else if (ctx.isSave) {
                        m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(810.f, targetY) : sf::Vector2f(70.f, targetY);
                        m_dots[ctx.defenderBase].targetPos = m_ballTarget;
                    } else {
                        m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(860.f, targetY > 290.f ? 360.f : 220.f) : sf::Vector2f(20.f, targetY > 290.f ? 360.f : 220.f);
                        m_dots[ctx.defenderBase].targetPos = m_pendingEvent.isHome ? sf::Vector2f(810.f, 290.f) : sf::Vector2f(70.f, 290.f);
                    }
                    m_dots[ctx.defenderBase].speed = 190.f; // keeper reacting to the strike

                    if (ctx.isSave) {
                        float distToTarget = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
                        if (distToTarget < 15.f) {
                            int gkIdx = m_pendingEvent.isHome ? 11 : 0;
                            m_ballCarrierIdx = gkIdx;
                        }
                        if (distToTarget < 10.f) {
                            m_attackPhase = Beat::Resolve;
                            m_stateTimer = 0.f;
                        }
                    }

                    if (m_stateTimer > 1.2f) { // slightly longer to ensure trajectory is seen
                        m_attackPhase = Beat::Resolve;
                        m_stateTimer = 0.f;
                    }
                }
    } else if (m_attackPhase == Beat::Resolve) {
                // Resolution
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());

                if (ctx.isGoal) {
                    m_visualState = VisualState::GoalCelebration;
                    m_stateTimer = 0.f;
                    float targetY = m_shotTargetY;
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(860.f, targetY > 290.f ? 330.f : 250.f) : sf::Vector2f(20.f, targetY > 290.f ? 330.f : 250.f);
                } else if (ctx.isSave && rand() % 100 < 45) {
                    // The keeper turned it behind - corner rather than a goal kick. This is
                    // where most corners come from, so it's what makes them show up at all.
                    beginCorner(m_pendingEvent.isHome, m_shotTargetY);
                } else {
                    m_visualState = VisualState::GoalKick;
                    m_stateTimer = 0.f;
                    int gkIdx = m_pendingEvent.isHome ? 11 : 0;
                    m_visualBall.setPosition(m_dots[gkIdx].shape.getPosition()); // Snap ball to GK feet
                }
    }
}

void MatchScreen::runMidfielderPass(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::MidPassHold) {
                // Midfielder Attacking: carry the ball into space before the pass/shot decision.
                // (Standing still here used to leave the interactive minigame starting at
                // midfield with every other dot frozen - an open, undefended dribble to goal.)
                // Absolute destination, not "current + 150" recomputed per frame: the
                // clamp made that terminate at the same spot anyway, but as written it was
                // another creeping target.
                sf::Vector2f curPos = m_dots[m_ballCarrierIdx].shape.getPosition();
                float advanceX = m_engine->isHome() ? 650.f : 230.f;
                m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(advanceX, curPos.y);
                m_dots[m_ballCarrierIdx].speed = 140.f;
                if (m_stateTimer > 1.0f && !m_minigameActive) {
                    m_stateTimer = 0.f;
                    m_engine->triggerMinigame();
                }
    } else if (m_attackPhase == Beat::MidPassResolve) {
                // Minigame resolved, process the pass
                m_isMinigameResultPending = false;
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());

                bool isSuccess = (m_pendingEvent.outcome == EventOutcome::PassGood ||
                                  m_pendingEvent.outcome == EventOutcome::Goal);

                if (isSuccess) {
                    // Pass to forward!
                    m_ballCarrierIdx = -1;
                    m_ballTarget = m_dots[m_attackFwdIdx].shape.getPosition();
                    m_attackPhase = Beat::PassInFlight;
                    m_stateTimer = 0.f;
                } else {
                    // Bad pass, intercepted by opponent
                    m_ballCarrierIdx = -1;
                    m_attackPhase = Beat::PassIntercepted; // Fail resolution
                    m_stateTimer = 0.f;
                    int oppDefenderIdx = (m_engine->isHome() ? 11 : 0) + 3;
                    m_ballTarget = m_dots[oppDefenderIdx].shape.getPosition();
                }
    } else if (m_attackPhase == Beat::PassInFlight) {
                // Ball traveling to Forward
                float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
                if (dist < 10.f) {
                    if (!m_passForward) {
                        m_attackPhase = Beat::PassReceived;
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = m_attackFwdIdx; // Receiver gets the ball
                    } else {
                        // Forward shoots!
                        if (m_engine->hasLogs()) {
                            m_pendingEvent = m_engine->popRecentLog();
                            if (handleFoulIfCard()) return; // a card here becomes a visible foul
                        }
                        m_attackPhase = Beat::Shot;
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = -1;
                        m_shotTargetY = 290.f + (rand()%60 - 30.f);
                    }
                }
    } else if (m_attackPhase == Beat::PassIntercepted) {
                // Ball intercepted by opponent
                float dist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
                if (dist < 10.f) {
                    m_visualState = VisualState::NormalPlay;
                    m_stateTimer = 0.f;
                }
    } else if (m_attackPhase == Beat::PassReceived) {
                // Receiver gets the ball
                if (m_stateTimer > 0.5f) {
                    if (!m_passForward) {
                        // Backward/Sideways pass: no shot, just commit and normal play
                        m_engine->commitEvent(m_pendingEvent);
                        m_visibleLogs.push_back(m_pendingEvent);
                        if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());

                        m_pendingEvent = MatchEvent{"", EventType::Normal, true};
                        m_visualState = VisualState::NormalPlay;
                        m_stateTimer = 0.f;
                    } else {
                        // Forward pass: go to shot
                        m_attackPhase = Beat::Shot; // Go to shot
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = -1;
                    }
                }
    }
}

void MatchScreen::runMidfielderTackle(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::MidTackleChase) {
                // Midfielder Defending: Sprint to opponent
                int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
                m_dots[myMidIdx].targetPos = m_dots[m_ballCarrierIdx].shape.getPosition();
                m_dots[myMidIdx].speed = 160.f; // Realistic sprint

                float dist = std::hypot(m_dots[myMidIdx].shape.getPosition().x - m_dots[m_ballCarrierIdx].shape.getPosition().x, 
                                        m_dots[myMidIdx].shape.getPosition().y - m_dots[m_ballCarrierIdx].shape.getPosition().y);

                if (dist < 15.f && !m_minigameActive) {
                    m_stateTimer = 0.f;
                    m_engine->triggerMinigame();
                }
    } else if (m_attackPhase == Beat::MidTackleResolve) {
                // Minigame resolved, process the tackle
                m_isMinigameResultPending = false;
                m_engine->commitEvent(m_pendingEvent);
                m_visibleLogs.push_back(m_pendingEvent);
                if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());

                bool isSuccess = (m_pendingEvent.outcome == EventOutcome::TackleWon || m_pendingEvent.outcome == EventOutcome::Intercepted);
                int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
                m_dots[myMidIdx].speed = 100.f; // Reset speed

                if (isSuccess) {
                    m_ballCarrierIdx = myMidIdx;
                    m_attackPhase = Beat::MidTackleWon;
                    m_stateTimer = 0.f;
                } else {
                    // Failed tackle, opponent passes to forward
                    m_attackPhase = Beat::CrossInFlight;
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = -1;
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(700.f, m_shotTargetY) : sf::Vector2f(180.f, m_shotTargetY);
                    do {
                        m_attackFwdIdx = ctx.attackerBase + 9 + (rand()%2);
                    } while(hasRedCard(m_attackFwdIdx));
                    m_dots[m_attackFwdIdx].targetPos = m_ballTarget;
                    m_dots[m_attackFwdIdx].speed = 100.f;
                }
    } else if (m_attackPhase == Beat::MidTackleWon) {
                // Succcessful tackle wait a bit
                if (m_stateTimer > 1.0f) {
                    m_visualState = VisualState::NormalPlay;
                    m_stateTimer = 0.f;
                }
    }
}

void MatchScreen::runSoloRun(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::SoloRun) {
                // Midfielder Solo Run
                if (m_stateTimer > 1.5f && !m_minigameActive) {
                    m_stateTimer = 0.f;
                    m_engine->triggerMinigame();
                }
    } else if (m_attackPhase == Beat::SoloRunResolve) {
                // Minigame resolved, process the solo run
                m_isMinigameResultPending = false;
                bool isSuccess = (m_pendingEvent.outcome == EventOutcome::Goal);
                int myMidIdx = (m_engine->isHome() ? 0 : 11) + 7;
                m_dots[myMidIdx].speed = 100.f; // Reset speed

                // Go to Phase 2 for shot trajectory
                m_attackPhase = Beat::Shot;
                m_stateTimer = 0.f;
                m_ballCarrierIdx = -1;
                m_shotTargetY = 290.f + (rand()%60 - 30.f);
    }
}

void MatchScreen::runGoalkeeperSave(float dt, const EpisodeCtx& ctx) {
    (void)dt; (void)ctx;
    if (m_attackPhase == Beat::GkShotWindup) {
                // Goalkeeper Save: show the striker actually bearing down on goal and
                // shooting, instead of the camera snapping to the goal while the ball
                // teleported to the edge of the box. The attacker keeps the ball at his
                // feet here and drives toward a shooting spot; when he arrives (or the beat
                // ends) the shot is struck from HIS position in initMinigame.
                bool userIsHome = m_engine->isHome();
                float shootX = userIsHome ? 250.f : 630.f; // just outside the box we defend
                sf::Vector2f shootPos(shootX, m_shotTargetY);

                if (m_attackFwdIdx >= 0) {
                    m_dots[m_attackFwdIdx].targetPos = shootPos;
                    m_dots[m_attackFwdIdx].speed = 170.f; // driving at goal
                    m_ballCarrierIdx = m_attackFwdIdx;     // ball at his feet through the run
                }

                // Camera follows the attacker as he comes in, so the build-up is visible.
                sf::Vector2f focus = (m_attackFwdIdx >= 0) ? m_dots[m_attackFwdIdx].shape.getPosition()
                                                           : shootPos;
                float targetZoom = 0.6f;
                m_currentZoom += (targetZoom - m_currentZoom) * 3.0f * dt;
                float viewWidth = 1280.f * m_currentZoom;
                float viewHeight = 720.f * m_currentZoom;
                float minX = 40.f + viewWidth / 2.f;
                float maxX = 840.f - viewWidth / 2.f;
                float minY = 130.f + viewHeight / 2.f;
                float maxY = 450.f - viewHeight / 2.f;
                sf::Vector2f targetCenter = focus;
                targetCenter.x = (minX > maxX) ? 440.f : std::clamp(targetCenter.x, minX, maxX);
                targetCenter.y = (minY > maxY) ? 290.f : std::clamp(targetCenter.y, minY, maxY);
                sf::Vector2f currentCenter = m_camera.getCenter();
                currentCenter += (targetCenter - currentCenter) * 4.0f * dt;
                m_camera.setCenter(currentCenter);
                m_camera.setSize(1280.f * m_currentZoom, 720.f * m_currentZoom);

                // Strike once he's arrived at the shooting spot, or after a short beat as a
                // safety net. Long enough that the run reads as a run.
                float distToShoot = (m_attackFwdIdx >= 0)
                    ? std::hypot(m_dots[m_attackFwdIdx].shape.getPosition().x - shootPos.x,
                                 m_dots[m_attackFwdIdx].shape.getPosition().y - shootPos.y)
                    : 0.f;
                if ((distToShoot < 25.f && m_stateTimer > 0.6f) || m_stateTimer > 1.8f) {
                    if (!m_minigameActive) {
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = -1;
                        m_engine->triggerMinigame();
                    }
                }
    }
}
