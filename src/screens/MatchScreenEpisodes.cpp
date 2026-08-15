#include "MatchScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "AudioManager.h"
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

    m_possessionLock = 0.f; m_possessionTeam = -1; // a goal / kick-off clears any turnover spell
}

// A turnover with consequence: the side that won the ball keeps it and drives forward for a spell,
// instead of open play flipping it straight back on the next momentum roll. Called when you're
// robbed or a pass is cut out.
void MatchScreen::beginTurnoverPossession(bool winnerIsHome) {
    m_possessionTeam = winnerIsHome ? 0 : 1;
    m_possessionLock = 5.0f;
    m_engine->applyMomentum(winnerIsHome ? 16.f : -16.f); // swing the bar to the side that won it

    // Start the spell with a real outfielder of the winning side who is actually near the ball,
    // so it isn't handed to a formation-slot repick that could drift possession straight back.
    int base = winnerIsHome ? 0 : 11;
    sf::Vector2f bp = m_visualBall.getPosition();
    int nearest = -1; float best = 1e9f;
    for (int i = base + 1; i < base + 11; ++i) { // outfielders only (keeper is base + 0)
        if (hasRedCard(i)) continue;
        sf::Vector2f d = m_dots[i].shape.getPosition() - bp;
        float dd = std::hypot(d.x, d.y);
        if (dd < best) { best = dd; nearest = i; }
    }
    if (nearest >= 0) m_ballCarrierIdx = nearest;
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
    sf::Vector2f rawBall = m_minigameActive ? m_ambientAnchor : m_visualBall.getPosition();
    // Ease a filtered reference toward it so a ball that teleports between episodes doesn't
    // yank the whole side across the pitch in one frame. ~6%/frame = players jog over ~1-2s.
    m_ambientBallX += (rawBall.x - m_ambientBallX) * 0.06f;
    m_ambientBallY += (rawBall.y - m_ambientBallY) * 0.06f;
    float ballX = m_ambientBallX, ballY = m_ambientBallY;

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

            if (i == 0) { // keeper: hold the centre of his line, only leaning toward the ball
                // Was tracking the ball's height 1:1, so while you dribbled wide he drifted to
                // the very corner of his goal and left it gaping. Now he stays near centre and
                // only shades a little toward the ball.
                float ky = std::clamp(290.f + (ballY - 290.f) * 0.35f, 262.f, 318.f);
                m_dots[idx].targetPos = sf::Vector2f(team == 0 ? 70.f : 810.f, ky);
                m_dots[idx].speed = 80.f;
                continue;
            }

            // Movement is by role, not one blanket slide. Defenders hold a shallow back
            // line and barely follow the ball's channel; forwards push up and drift across
            // freely. A uniform shift is what made centre-backs charge downfield in a clump
            // and wingers bolt to the far flank.
            float shiftScale, leanScale;
            if (i <= 4)      { shiftScale = 0.15f; leanScale = 0.06f; } // defenders
            else if (i <= 8) { shiftScale = 0.30f; leanScale = 0.14f; } // midfield
            else             { shiftScale = 0.45f; leanScale = 0.22f; } // forwards
            float shift = (ballX - 440.f) * shiftScale;

            float slotX = (team == 0) ? std::min(50.f + form[i][0] * 780.f, 435.f)
                                      : std::max(830.f - form[i][0] * 780.f, 445.f);
            float slotY = 140.f + form[i][1] * 300.f;

            float driftX = std::sin(s_ambientClock * 0.9f + idx * 1.3f) * 8.f;
            float driftY = std::cos(s_ambientClock * 0.7f + idx * 2.1f) * 8.f;

            m_dots[idx].targetPos = sf::Vector2f(std::clamp(slotX + shift + driftX, 55.f, 815.f),
                                                 slotY + (ballY - slotY) * leanScale + driftY);
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

    // A rushing AI keeper must not leave his own penalty box. Only the user's OWN keeper is
    // exempt (a human keeper may roam); the opponent's AI keeper stays boxed even then.
    Player* up = m_gameManager->getPlayer();
    int userGkIdx = (up && up->position == PlayerPosition::Goalkeeper)
                    ? (m_engine->isHome() ? 0 : 11) : -1;

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

        // Keep an AI keeper inside his own penalty area (home defends the left box, away the
        // right). The user's keeper is exempt. Boxes: x within 110px of the goal line, y 200..380.
        if ((int)i % 11 == 0 && (int)i != userGkIdx) {
            bool homeGk = ((int)i == 0);
            d.targetPos.x = std::clamp(d.targetPos.x, homeGk ? 36.f : 730.f, homeGk ? 150.f : 844.f);
            d.targetPos.y = std::clamp(d.targetPos.y, 200.f, 380.f);
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

    // While you're drawing/aiming a shot or pass, the play holds: opponents don't close in
    // and your team-mates KEEP the positions they had broken into, so you still have someone
    // to pick out. This is handled BEFORE the ambient shape on purpose - running the ambient
    // here would drag every team-mate back toward his formation slot the instant you started
    // drawing a pass, which is exactly the "why are my runners retreating?" bug. Freezing
    // each dot on its current spot holds the runs in place instead.
    if (m_shotStage != ShotStage::None) {
        for (size_t i = 0; i < m_dots.size(); ++i) {
            if ((int)i == m_userIdx) continue;
            m_dots[i].targetPos = m_dots[i].shape.getPosition();
        }
        return;
    }

    // Baseline shape for all 22 first; the pressers/runners/keepers below overwrite the
    // few who are actually involved. Without this everyone else stands frozen.
    updateAmbientShape();

    // Once the shot is away the ball is a projectile - nobody presses or tackles it. A DRAWN
    // shot is flown by hand with zero velocity, so the ballSpeed guard further down doesn't
    // catch it; without this an opponent "robbed" the flying shot and it teleported to a free
    // kick. Let the flight reach the goal line / touchline and resolve there.
    if (m_ballStruck) return;

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
        // Jockey rather than climb into the carrier. Standing ON the ball (targetPos = ballPos)
        // meant two opponents piled onto the player's body and just sat there doing nothing,
        // because a real tackle is gated (grace period, cooldown, duel roll). Instead they take
        // up a position goal-SIDE of the ball - between the carrier and the goal he's attacking
        // - and hold a step off him: it reads as pressing/blocking the way forward, and the
        // nearest man is still close enough to lunge in when the tackle logic below fires.
        float goalX = userIsHome ? 845.f : 35.f;
        sf::Vector2f toGoal(goalX - ballPos.x, 290.f - ballPos.y);
        float gl = std::hypot(toGoal.x, toGoal.y);
        if (gl > 1.f) { toGoal.x /= gl; toGoal.y /= gl; } else { toGoal = sf::Vector2f(attackDir, 0.f); }
        sf::Vector2f perp(-toGoal.y, toGoal.x);
        for (size_t k = 0; k < byDist.size() && k < 2; ++k) {
            int idx = byDist[k].second;
            if (k == 0) {
                // The nearest man goes STRAIGHT for the ball (an interception run), sprinting
                // faster than a walking carrier so a straight stroll is actually closed down and
                // the tackle below can commit. Still slower than a Q dash (210), so beating your
                // man gets you clear - that's the counter to the press.
                m_dots[idx].targetPos = ballPos;
                m_dots[idx].speed = std::max(chaseSpeed, 178.f);
            } else {
                // Second man covers goal-side and off to one flank, in case the first is beaten.
                m_dots[idx].targetPos = ballPos + toGoal * 40.f + perp * 30.f;
                m_dots[idx].speed = chaseSpeed;
            }
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

    // Team-mates OPEN UP for the pass: they spread into lanes ahead of the ball and keep
    // moving (checking toward/away, drifting across) so there's always a live option, instead
    // of making one run and standing still. One man peels in behind the defence (a through-
    // ball run - onside if you feed him early, offside if you dwell). Anchored to the frozen
    // episode position so they don't shadow the user's dribble step for step.
    // Only open up when WE have the ball to give. Positions are keyed to the goal we're
    // attacking (fixed lanes), not to a frozen central anchor - so they actually get forward
    // into space instead of hovering by the centre circle, and advance as the ball does. Y is
    // a fixed lane (not the ball's Y) so they don't just mirror the dribble sideways.
    float t = m_minigameTimer;
    float goalX = userIsHome ? 845.f : 35.f;
    int nMates = userOnBall ? std::min((int)mates.size(), 4) : 0;
    for (int k = 0; k < nMates; ++k) {
        int idx = mates[k].second;
        float lane = (k - (nMates - 1) / 2.0f) * 74.f;
        sf::Vector2f target;
        if (k == 0) {
            // A short give-and-go option, just ahead of the ball, checking in and out.
            float adv = 78.f + std::sin(t * 1.7f) * 30.f;
            target = sf::Vector2f(ballPos.x + attackDir * adv, ballPos.y + lane * 0.5f + std::cos(t * 1.3f) * 22.f);
        } else if (k == nMates - 1 && nMates >= 3) {
            // Runner in behind, right on the shoulder of the last man toward goal (offside risk).
            target = sf::Vector2f(goalX - attackDir * (58.f + std::sin(t * 1.2f) * 22.f),
                                  290.f + lane * 0.55f + std::sin(t * 1.5f) * 16.f);
        } else {
            // Push up into the attacking third, spread across a lane, constantly adjusting.
            float frac = 0.55f + 0.12f * k;
            float tx = ballPos.x + (goalX - ballPos.x) * frac + std::sin(t * 1.5f + k) * 26.f;
            target = sf::Vector2f(tx, 290.f + lane + std::cos(t * 1.3f + k * 2.f) * 24.f);
        }
        target.x = std::clamp(target.x, 55.f, 815.f);
        target.y = std::clamp(target.y, 150.f, 430.f);
        m_dots[idx].targetPos = target;
        m_dots[idx].speed = 130.f;
    }

    // (Keepers are handled by updateAmbientShape above - line-holding, tracking the ball.)

    // --- Pressure: an opponent who reaches the ball while WE have it wins it back
    // (userOnBall computed above). When he's defending (Tackle/Save) the opponent already
    // has it, so there's nothing to lose here.

    // A committed action is untouchable: the player has already planted his foot, and sniping the
    // ball mid-QTE - or while he's drawing/powering a shot or pass (m_shotStage) - would punish
    // someone who did everything right, and would turn his shot into a "robbed of possession"
    // turnover that never counts as a shot. So once he commits to an action, let it play out.
    if (!userOnBall || m_qte.isActive() || m_shotStage != ShotStage::None || byDist.empty()) return;

    // Grace period. The attack scripts hand over control with a defender already right on
    // top of the carrier (~15px), so the player needs a real window to shoot or pass
    // before anyone can rob him - otherwise it feels like the ball is taken the instant
    // control arrives. Long enough to comfortably start an action.
    const float GRACE = 1.5f;
    if (m_minigameTimer < GRACE) return;

    // Once the ball has been struck it's gone - nobody "tackles" a shot that's already
    // on its way. Without this an opponent could still rob us mid-flight, after the QTE
    // had already decided the outcome.
    float ballSpeed = std::hypot(m_ballVelocity.x, m_ballVelocity.y);
    if (ballSpeed > 120.f) return;

    m_tackleAttemptTimer -= dt;
    if (m_tackleAttemptTimer > 0.f) return;

    // The nearest presser jockeys a step off the ball (~20px) rather than climbing onto it,
    // so allow a lunge from that range - otherwise he'd hover forever and never commit.
    float gap = byDist[0].first;
    if (gap > 30.f) return;

    // One attempt roughly every second, not one per frame.
    m_tackleAttemptTimer = 1.0f;

    // Contact isn't an automatic steal - weigh their tackling against how well you SHIELD the
    // ball, which is a dribbling skill (a striker shouldn't stroll through midfield just because
    // he finishes well). Crucially, the pressure RAMPS the longer you dwell on the ball without
    // acting: walking in a straight line to the box is no longer safe - hold it too long and
    // you get robbed, so you have to actually pass, shoot, or beat your man (Q dash).
    int resist = p->dribbling;
    int duel = 30 + (oppStrength - resist) / 2;
    duel += (g_settings.difficulty - 1) * 6; // Hard: opponents rob you more; Easy: less
    duel += (int)std::clamp((m_minigameTimer - GRACE) * 10.f, 0.f, 35.f); // dwell pressure
    duel = std::clamp(duel, 10, 82);
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
    AudioManager::get().sfx("whistle_short_" + std::to_string(1 + rand() % 4)); // ref blows for the foul

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
    // ...and only if he is actually ON the ball. The ball no longer gets dragged to the
    // participants, so a "carrier" standing away from it would put the whole incident
    // somewhere the ball isn't.
    bool carrierNearBall = false;
    if (carrier >= 0 && carrier < (int)m_dots.size()) {
        sf::Vector2f d = m_dots[carrier].shape.getPosition() - spot;
        carrierNearBall = std::hypot(d.x, d.y) < 45.f;
    }
    bool carrierValid = (carrier >= 0 && carrier % 11 != 0 && !hasRedCard(carrier) && carrierNearBall);
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

    // The two men have to be close enough for this to read as a duel. "Nearest opponent"
    // alone could be most of a pitch away, which is how you got a challenge between players
    // who were never near each other and a free kick awarded behind the play. If they are
    // far apart, pick the tightest opposing pair that is also near the ball instead.
    auto separation = [&](int a, int b) {
        sf::Vector2f d = m_dots[a].shape.getPosition() - m_dots[b].shape.getPosition();
        return std::hypot(d.x, d.y);
    };
    auto distToBall = [&](int a) {
        sf::Vector2f d = m_dots[a].shape.getPosition() - spot;
        return std::hypot(d.x, d.y);
    };
    // A duel is only believable if the two men are on each other AND on the ball.
    bool pairOk = (m_foulOffenderIdx >= 0 && m_foulVictimIdx >= 0
                   && separation(m_foulOffenderIdx, m_foulVictimIdx) < 85.f
                   && std::min(distToBall(m_foulOffenderIdx), distToBall(m_foulVictimIdx)) < 60.f);
    if (!pairOk) {
        int bo = -1, bv = -1; float bestScore = 1e9f;
        for (int o = offBase; o < offBase + 11; ++o) {
            if (o % 11 == 0 || hasRedCard(o)) continue;
            for (int v = vicBase; v < vicBase + 11; ++v) {
                if (v % 11 == 0 || hasRedCard(v)) continue;
                sf::Vector2f op = m_dots[o].shape.getPosition();
                sf::Vector2f vp = m_dots[v].shape.getPosition();
                sf::Vector2f mid((op.x + vp.x) * 0.5f, (op.y + vp.y) * 0.5f);
                // Tight duel first, near the ball second.
                float score = std::hypot(op.x - vp.x, op.y - vp.y)
                            + std::hypot(mid.x - spot.x, mid.y - spot.y) * 0.6f;
                if (score < bestScore) { bestScore = score; bo = o; bv = v; }
            }
        }
        if (bo >= 0 && bv >= 0) { m_foulOffenderIdx = bo; m_foulVictimIdx = bv; }
    }

    // The challenge is played out in two phases (see the FoulChallenge state): the offender
    // closes the man down, and only when they actually MEET does the stumble play and the
    // whistle go. The geometry is worked out at the moment of contact, not here, because
    // targets fixed up front went stale as soon as either man moved.
    // The whistle stops play where the ball IS. It used to be snapped onto the victim, and
    // since he is often not standing on it (the duel pair is chosen separately), the ball
    // visibly jumped across the pitch just before every foul. Leave it on the spot - the
    // FoulChallenge state pins it there for the rest of the stoppage.
    m_foulContact = false;
    m_ballTarget = spot;

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
    float goalLineX = fouledIsHome ? 840.f : 40.f;    // the pitch edge in front of that goal
    float distToGoal = std::abs(attackGoalX - spot.x);

    // Inside the box it's a PENALTY. Scaled off the real thing: the pitch is 800px for
    // ~105m, so the 16.5m area is ~126px deep and the 40.3m width is ~190px (y 195..385),
    // with the spot 11m (~84px) out. This also removes the case where a foul almost on the
    // goal line put the "wall" 90px further on - i.e. inside the net.
    bool inBox = (std::abs(goalLineX - spot.x) < 126.f) && (std::abs(spot.y - 290.f) < 95.f);
    m_fkPenalty = inBox;
    m_fkDirect = inBox || (distToGoal < 250.f && std::abs(spot.y - 290.f) < 150.f);
    m_fkAttackHome = fouledIsHome;
    m_fkStruck = false;
    m_fkResolved = false;
    m_fkWindup = 0.f;
    m_fkWallCount = 0;
    m_fkKeeperIdx = -1;
    m_fkUserTaker = false;
    m_penKeepOut.clear();

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

        int dBase = offenderIsHome ? 0 : 11;
        m_fkKeeperIdx = dBase; // local 0 = keeper

        if (m_fkPenalty) {
            // Ball on the spot (11m ~ 84px out), no wall.
            spot = sf::Vector2f(fouledIsHome ? (goalLineX - 84.f) : (goalLineX + 84.f), 290.f);
            m_visualBall.setPosition(spot);
            m_ballTarget = spot;

            // Everyone except the taker and the keepers must stand OUTSIDE the box, behind the
            // ball. Line them up just beyond the box edge, spread across the pitch (two depths).
            float dir = fouledIsHome ? -1.f : 1.f;                 // toward midfield = "behind" the ball
            float boxEdge = fouledIsHome ? (goalLineX - 126.f) : (goalLineX + 126.f);
            std::vector<int> outs;
            for (int i = 0; i < (int)m_dots.size(); ++i) {
                if (i % 11 == 0 || i == taker || hasRedCard(i)) continue; // keepers + taker stay put
                outs.push_back(i);
            }
            for (size_t k = 0; k < outs.size(); ++k) {
                float t = outs.size() > 1 ? (float)k / (outs.size() - 1) : 0.5f;
                sf::Vector2f pos(boxEdge + dir * (16.f + (k % 2) * 24.f), 178.f + t * 224.f);
                m_dots[outs[k]].targetPos = pos;
                m_dots[outs[k]].speed = 150.f;
                m_penKeepOut.push_back({outs[k], pos});
            }
        } else {
            // Direction from ball to the goal being attacked.
            sf::Vector2f toGoal(attackGoalX - spot.x, 290.f - spot.y);
            float gl = std::hypot(toGoal.x, toGoal.y);
            if (gl > 0.1f) { toGoal.x /= gl; toGoal.y /= gl; }
            sf::Vector2f perp(-toGoal.y, toGoal.x); // across the wall

            // The defending side forms the wall ~90px in front of the ball, but never past
            // the goal line - close-in free kicks used to shove all three men into the net.
            std::vector<std::pair<float,int>> cand;
            for (int i = dBase; i < dBase + 11; ++i) {
                if (i % 11 == 0) continue;      // the keeper stays on his line
                if (i == taker) continue;
                if (hasRedCard(i)) continue;
                sf::Vector2f d = m_dots[i].shape.getPosition() - spot;
                cand.push_back({std::hypot(d.x, d.y), i});
            }
            std::sort(cand.begin(), cand.end());

            float room = std::abs(goalLineX - spot.x) - 25.f; // keep them off the line
            sf::Vector2f wallCenter = spot + toGoal * std::clamp(90.f, 0.f, std::max(0.f, room));
            int n = std::min(3, (int)cand.size());
            for (int k = 0; k < n; ++k) {
                int idx = cand[k].second;
                // Shoulder to shoulder: the dots are radius 6, so a 12px pitch leaves them
                // touching with no gap for the ball to squeeze through.
                float off = (k - (n - 1) / 2.f) * 12.f;
                sf::Vector2f pos = wallCenter + perp * off;
                pos.y = std::clamp(pos.y, 140.f, 440.f);
                m_dots[idx].targetPos = pos;
                m_dots[idx].speed = 150.f;
                m_fkWallPos[m_fkWallCount] = pos;
                m_fkWall[m_fkWallCount++] = idx;
            }
        }
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
    // Aim the cross at his BOX anchor, not wherever he is mid-run - a deep player still
    // jogging up from his own half was dragging the delivery out toward midfield.
    m_cornerAimBase = anchors[0];

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
    // The fixed box anchor the target is running to - so the cross always drops into the box
    // and the man meets it there, rather than being aimed at his mid-run position.
    return m_cornerAimBase + m_cornerAimOffset;
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
    AudioManager::get().kick();

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
        if (m_fkPenalty) {
            if (m_fkSuccess) {
                e.type = EventType::Goal; e.outcome = EventOutcome::Goal;
                e.text = tag + "GOAL! " + name + " makes no mistake from the penalty spot!";
            } else {
                e.type = EventType::Chance; e.outcome = EventOutcome::Saved;
                e.text = tag + name + " misses the penalty!";
            }
        } else if (m_fkSuccess) {
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
    if (m_possessionLock > 0.f) m_possessionLock -= dt; // turnover possession spell counts down
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
        
        // While a turnover possession spell is live, the side that won the ball keeps it - open
        // play doesn't get to flip it back on the momentum roll below.
        bool lockActive = (m_possessionLock > 0.f && m_possessionTeam >= 0);

        if (m_ballCarrierIdx == -1 || (rand() % 100 < 2 && std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y) < 10.f)) {
            bool pickHome;
            if (lockActive) pickHome = (m_possessionTeam == 0);
            else { pickHome = (mom > 0); if (mom == 0) pickHome = (rand()%2 == 0); }
            // Bounded retry: an unbounded do/while here hard-freezes the app (window won't
            // even close) if every candidate happens to be sent off.
            for (int tries = 0; tries < 24; ++tries) {
                m_ballCarrierIdx = (pickHome ? 0 : 11) + 1 + (rand() % 8);
                if (!hasRedCard(m_ballCarrierIdx)) break;
            }
        }
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();

        // The winner drives the ball forward toward the goal he's attacking - a real counter,
        // not a stroll back into shape - so a turnover actually turns into pressure.
        if (lockActive) {
            float goalX = (m_possessionTeam == 0) ? 845.f : 35.f; // home attacks right, away left
            sf::Vector2f cur = m_dots[m_ballCarrierIdx].shape.getPosition();
            float dirX = (goalX > cur.x) ? 1.f : -1.f;
            m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(
                std::clamp(cur.x + dirX * 140.f, 60.f, 820.f), cur.y);
            m_dots[m_ballCarrierIdx].speed = 130.f;
        }
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

        // Two phases. The offender first CLOSES HIM DOWN, and the whistle waits until they
        // actually meet; a fixed timer used to blow it while they were still yards apart,
        // which is why no contact was ever visible. On contact the ball is moved to where
        // they met, so the free kick is given at the incident and not behind the play.
        bool haveBoth = (m_foulOffenderIdx >= 0 && m_foulVictimIdx >= 0);
        if (haveBoth && !m_foulContact) {
            sf::Vector2f offPos = m_dots[m_foulOffenderIdx].shape.getPosition();
            sf::Vector2f vicPos = m_dots[m_foulVictimIdx].shape.getPosition();

            m_dots[m_foulOffenderIdx].targetPos = vicPos;  // charges the man
            m_dots[m_foulOffenderIdx].speed = 300.f;
            m_dots[m_foulVictimIdx].targetPos = vicPos;    // stands his ground
            m_dots[m_foulVictimIdx].speed = 30.f;

            float sep = std::hypot(offPos.x - vicPos.x, offPos.y - vicPos.y);
            if (sep < 15.f || m_foulClock > 1.5f) {
                m_foulContact = true;
                m_foulClock = 0.f;

                sf::Vector2f contact((offPos.x + vicPos.x) * 0.5f, (offPos.y + vicPos.y) * 0.5f);
                // The free kick must be given where the ball actually was, not wherever the duel
                // pair happened to be. If the chosen offender/victim are far from the ball, their
                // midpoint would fling it across the pitch - so keep the incident within reach of
                // the real spot (m_ballTarget, which beginFoul left on the ball).
                sf::Vector2f spot = m_ballTarget;
                sf::Vector2f delta = contact - spot;
                float dlen = std::hypot(delta.x, delta.y);
                if (dlen > 55.f) contact = spot + delta * (55.f / dlen);
                m_visualBall.setPosition(contact);
                m_ballTarget = contact;

                sf::Vector2f away = vicPos - offPos;
                float len = std::hypot(away.x, away.y);
                if (len > 0.1f) { away.x /= len; away.y /= len; } else { away = sf::Vector2f(0.f, 1.f); }
                m_foulLungeTarget = contact + away * 10.f;   // follows through over the ball
                m_foulStaggerTarget = contact + away * 42.f; // victim knocked clear
            }
        } else if (haveBoth) {
            m_dots[m_foulOffenderIdx].targetPos = m_foulLungeTarget;
            m_dots[m_foulOffenderIdx].speed = 150.f;
            m_dots[m_foulVictimIdx].targetPos = m_foulStaggerTarget;
            m_dots[m_foulVictimIdx].speed = 130.f;   // knocked back
        }

        sf::Vector2f spot = m_ballTarget;

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
        // reads the same at 0.5x or 2.5x: a beat AFTER the contact to let the stumble
        // register, then the whistle settles into the dead ball (which pulls the camera
        // back out). The fallback covers a challenge with no valid pair.
        if (m_foulContact ? (m_foulClock > 0.9f) : (!haveBoth && m_foulClock > 1.6f)) {
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
        // Keep everyone out of the box for a penalty (ambient would pull them back in).
        for (auto& kp : m_penKeepOut)
            if (kp.first >= 0 && kp.first < (int)m_dots.size()) {
                m_dots[kp.first].targetPos = kp.second;
                m_dots[kp.first].speed = 150.f;
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
                    // Penalties are converted around three quarters of the time; a direct
                    // free kick is a long shot by comparison.
                    int chance = m_fkPenalty ? std::clamp(68 + (str - 55) / 4, 60, 88)
                                             : std::clamp(6 + (str - 55) / 3, 4, 45);
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
                // Sprint into the box first, THEN jostle around the spot. Applying the slow
                // jostle speed from the start left everyone crawling in and the cross came
                // before anyone arrived.
                float ph = m_stateTimer * 2.3f + k * 1.7f;
                sf::Vector2f drift(std::sin(ph) * 8.f, std::cos(ph * 0.8f) * 8.f);
                m_dots[idx].targetPos = m_cornerCrowdPos[k] + drift;
                sf::Vector2f cur = m_dots[idx].shape.getPosition();
                float distToSpot = std::hypot(cur.x - m_cornerCrowdPos[k].x, cur.y - m_cornerCrowdPos[k].y);
                m_dots[idx].speed = (distToSpot > 28.f) ? 215.f : 70.f;
            }
        }
        // The defending keeper shuffles across his line, anticipating the cross.
        {
            int gk = m_cornerAttackHome ? 11 : 0;
            if (gk >= 0 && gk < (int)m_dots.size() && !hasRedCard(gk)) {
                float gLineX = m_cornerAttackHome ? 808.f : 72.f;
                float gy = 290.f + std::sin(m_stateTimer * 1.9f) * 24.f;
                m_dots[gk].targetPos = sf::Vector2f(gLineX, gy);
                m_dots[gk].speed = 95.f;
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

            // Don't whip it in until the taker has actually reached the flag - otherwise a
            // taker who started far away (you, as a defender) is still jogging over when the
            // delivery fires. The box also needs a beat to fill. A 6s safety net covers a
            // taker who somehow can't get there.
            float takerDist = 1e9f;
            if (m_cornerTaker >= 0 && m_cornerTaker < (int)m_dots.size()) {
                sf::Vector2f td = m_dots[m_cornerTaker].shape.getPosition() - m_cornerSpot;
                takerDist = std::hypot(td.x, td.y);
            }
            // ...and the target man has actually arrived in the box (or a 6s safety net).
            float targetDist = 1e9f;
            if (m_cornerTargetIdx >= 0 && m_cornerTargetIdx < (int)m_dots.size()) {
                sf::Vector2f td = m_dots[m_cornerTargetIdx].shape.getPosition() - m_cornerAimBase;
                targetDist = std::hypot(td.x, td.y);
            }
            bool ready = (takerDist < 18.f && targetDist < 40.f && m_foulClock > 2.5f) || m_foulClock > 6.0f;
            if (ready) {
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
                    // A good delivery to a free man at the back post is a big chance.
                    m_cornerSuccess = m_cornerGoodDelivery && (rand() % 100 < 55);
                }

                // The ball has been met - now play the finish out instead of freezing it in
                // mid-air for a beat and then teleporting it to the keeper.
                m_cornerHeaded = true;
                m_stateTimer = 0.f;
                float goalX = m_cornerAttackHome ? 845.f : 35.f;
                float metY = m_visualBall.getPosition().y; // where the header was met
                if (m_cornerSuccess) {
                    // Headed into the net, roughly where he met it (keeps a far-post header at
                    // the far post instead of always snapping to the centre).
                    float aimY = std::clamp(metY, 255.f, 325.f);
                    m_ballTarget = sf::Vector2f(m_cornerAttackHome ? goalX + 22.f : goalX - 22.f, aimY);
                } else {
                    // Missed header flies WIDE or OVER near where he met it - it does not sail
                    // back across the goal to the keeper (which read as a miss at an empty net).
                    float side = (metY < 290.f) ? -1.f : 1.f;
                    m_ballTarget = sf::Vector2f(goalX, 290.f + side * (72.f + rand() % 34));
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
        // Penalty: keep everyone else out of the box through the kick too.
        for (auto& kp : m_penKeepOut)
            if (kp.first >= 0 && kp.first < (int)m_dots.size()) {
                m_dots[kp.first].targetPos = kp.second;
                m_dots[kp.first].speed = 90.f;
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

    // Track an aerial flight for long deliveries with nobody carrying the ball. The curve-shot and
    // loft-timer paths own the ball when active, so defer to them.
    if (m_ballCarrierIdx != -1 || m_shotCurveActive || m_ballLoftTimer > 0.f) {
        m_ballAirborne = false;
    } else if (!m_ballAirborne && globalBDist > 140.f) {
        m_ballAirborne = true;
        m_ballAirFrom = m_visualBall.getPosition();
        m_ballAirLen = globalBDist;
    } else if (m_ballAirborne && globalBDist < 12.f) {
        m_ballAirborne = false;
    }

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
        // A delivery in flight (nobody carrying it - a pass/cross/through-ball is released with
        // carrier == -1 until it arrives) eases into its target over the last stretch instead of
        // arriving at full pace and dead-stopping in the air. A CARRIED ball must keep tight to its
        // carrier, so it is never eased - otherwise it visibly trails a step behind the dribbler.
        if (m_ballCarrierIdx == -1) {
            float ease = std::clamp(globalBDist / 55.f, 0.30f, 1.f);
            moveDist *= ease;
        }
        if (moveDist >= globalBDist) {
            m_visualBall.setPosition(m_ballTarget);
        } else {
            m_visualBall.move((bdir.x / globalBDist) * moveDist, (bdir.y / globalBDist) * moveDist);
        }
    }
}


void MatchScreen::holdOffsideLine(int defenderBase, bool attackingHome) {
    // The defenders who aren't actively challenging the ball hold ONE flat line, so a stray
    // man doesn't sit deep by his own goal and drag the offside line (and the striker) back
    // unrealistically. The line's depth tracks the ball but never collapses onto the goal.
    float goalX = attackingHome ? 845.f : 35.f;
    float goalDir = attackingHome ? 1.f : -1.f;
    float lineX = m_visualBall.getPosition().x + goalDir * 130.f;
    lineX = attackingHome ? std::clamp(lineX, 470.f, goalX - 75.f)
                          : std::clamp(lineX, goalX + 75.f, 410.f);

    std::vector<int> backs;
    for (int i = 1; i <= 4; ++i) {
        int idx = defenderBase + i;
        if (idx < 0 || idx >= (int)m_dots.size() || hasRedCard(idx)) continue;
        backs.push_back(idx);
    }
    std::sort(backs.begin(), backs.end(), [&](int a, int b) {
        return m_dots[a].shape.getPosition().y < m_dots[b].shape.getPosition().y;
    });
    for (size_t k = 0; k < backs.size(); ++k) {
        float ly = 195.f + (backs.size() > 1 ? (float)k / (backs.size() - 1) : 0.5f) * 190.f;
        m_dots[backs[k]].targetPos = sf::Vector2f(lineX, ly);
        m_dots[backs[k]].speed = 95.f; // shuffle as one unit
    }
}

void MatchScreen::updateAttackEpisode(float dt) {
    EpisodeCtx ctx;
    ctx.attackerBase = m_pendingEvent.isHome ? 0 : 11;
    ctx.defenderBase = m_pendingEvent.isHome ? 11 : 0;

    // Hold a coherent back line for the whole attack (including when the ball is in flight
    // for a cross, which is exactly the offside moment). Challenging defenders override their
    // own slot below; the rest keep the line.
    holdOffsideLine(ctx.defenderBase, m_pendingEvent.isHome);
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

        int numDefendersToRun = 3;
        if (m_attackPhase == Beat::MidPassHold || m_attackPhase == Beat::PassReceived) {
            numDefendersToRun = m_attackWingerIdx; // the value we stored (1 or 2)
        } else if (m_attackShape == AttackShape::Counter) {
            numDefendersToRun = 1; // caught out - only one defender recovers, so it's a real break
        }

        sf::Vector2f carrierPos = m_dots[m_ballCarrierIdx].shape.getPosition();

        // The N defenders NEAREST the ball step out to challenge; the rest hold their line
        // (the ambient shape). They used to be assigned slots by shirt number, so on a run
        // through the middle the left-back was sent to a right-side slot and vice versa -
        // the back four crossed over each other.
        std::vector<std::pair<float,int>> line;
        for (int i = 1; i <= 4; ++i) {
            int idx = ctx.defenderBase + i;
            if (hasRedCard(idx)) continue;
            sf::Vector2f d = m_dots[idx].shape.getPosition() - carrierPos;
            line.push_back({std::hypot(d.x, d.y), idx});
        }
        std::sort(line.begin(), line.end()); // by distance to the ball
        int n = std::min(numDefendersToRun, (int)line.size());
        std::vector<int> closers;
        for (int k = 0; k < n; ++k) closers.push_back(line[k].second);

        // Match the challenging slots to each defender's CURRENT vertical order, so the
        // higher man takes the higher slot and nobody swaps sides.
        std::sort(closers.begin(), closers.end(), [&](int a, int b) {
            return m_dots[a].shape.getPosition().y < m_dots[b].shape.getPosition().y;
        });
        float goalDir = m_pendingEvent.isHome ? 1.f : -1.f; // toward the goal being attacked
        for (int k = 0; k < n; ++k) {
            int idx = closers[k];
            float slotY = carrierPos.y + (k - (n - 1) / 2.f) * 28.f;      // spread top->bottom
            float slotX = carrierPos.x + goalDir * (10.f + k * 10.f);     // goal-side, staggered
            m_dots[idx].targetPos = sf::Vector2f(slotX, slotY);
            m_dots[idx].speed = 150.f; // tracking runners
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
                    m_gkCommittedRush = false; // fresh each 1v1 (reset here, NOT in initMinigame,
                                               // which runs after the windup has set it)
                    m_stateTimer = 0.f;
                    // Bounded retry: only two candidates (9,10), so if both are sent off an
                    // unbounded do/while spins forever and hangs the whole app.
                    for (int tries = 0; tries < 8; ++tries) {
                        m_attackFwdIdx = ctx.attackerBase + 9 + (rand() % 2);
                        if (!hasRedCard(m_attackFwdIdx)) break;
                    }
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
                    // Never let him receive on top of the keeper: keep clear of the goal line even
                    // when the defence sits deep (which drags the offside line near the goal).
                    float goalX = aHome ? 845.f : 35.f;
                    crossTarget.x = aHome ? std::min(crossTarget.x, goalX - 62.f)
                                          : std::max(crossTarget.x, goalX + 62.f);
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
                    float goalX = aHome ? 845.f : 35.f;
                    runTarget.x = aHome ? std::min(runTarget.x, goalX - 62.f)
                                        : std::max(runTarget.x, goalX + 62.f);
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
    auto userDotIdx = [&]() {
        Player* p = m_gameManager->getPlayer();
        int upi = (p->position == PlayerPosition::Defender) ? 3
                : (p->position == PlayerPosition::Midfielder) ? 7
                : (p->position == PlayerPosition::Forward) ? 10 : 0;
        return (m_engine->isHome() ? 0 : 11) + upi;
    };

    if (m_attackPhase == Beat::DefTacklePrep) {
                // The attacker carries the ball himself and drives AT you - no long pass and no
                // sprint across the pitch. You hold your ground and the attack comes to you.
                m_ballCarrierIdx = m_attackFwdIdx;
                if (m_stateTimer > 0.5f) {
                    m_attackPhase = Beat::DefTackleClose;
                    m_stateTimer = 0.f;
                }
    } else if (m_attackPhase == Beat::DefTackleClose) {
                int myDefenderIdx = userDotIdx();
                m_ballCarrierIdx = m_attackFwdIdx; // ball stays at the attacker's feet

                sf::Vector2f dPos = m_dots[myDefenderIdx].shape.getPosition();
                sf::Vector2f aPos = m_dots[m_attackFwdIdx].shape.getPosition();
                sf::Vector2f delta = dPos - aPos;
                float dist = std::hypot(delta.x, delta.y);
                sf::Vector2f u = (dist > 1.f) ? sf::Vector2f(delta.x / dist, delta.y / dist)
                                              : sf::Vector2f(m_pendingEvent.isHome ? 1.f : -1.f, 0.f);

                // Attacker dribbles right up to you; you hold, stepping only a touch to meet him.
                m_dots[m_attackFwdIdx].targetPos = dPos;
                m_dots[m_attackFwdIdx].speed = 95.f;
                m_dots[myDefenderIdx].targetPos = dPos - u * 8.f; // small step toward the attacker
                m_dots[myDefenderIdx].speed = 55.f;

                // Only start the duel once he's actually ON you - so the minigame never opens
                // with the ball half a pitch away. The long timeout is just a safety net.
                if (dist < 30.f || m_stateTimer > 6.0f) {
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
                        // Don't reassign the carrier here: forcing the nominal striker to be
                        // the fouled man dragged the incident away from wherever the ball
                        // actually was. beginFoul centres it on the real carrier / the ball.
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
                        // Count the goal the instant the ball crosses the line, not on a fixed timer.
                        float bx = m_visualBall.getPosition().x;
                        if (m_pendingEvent.isHome ? (bx >= 840.f) : (bx <= 40.f)) {
                            m_attackPhase = Beat::Resolve;
                            m_stateTimer = 0.f;
                        }
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
                    // Let the ball carry into the net at the height it crossed, decelerating there,
                    // instead of veering to a corner - reads as physics, not a magnet to the post.
                    float ny = std::clamp(m_shotTargetY, 256.f, 324.f);
                    m_ballTarget = m_pendingEvent.isHome ? sf::Vector2f(858.f, ny) : sf::Vector2f(22.f, ny);
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

                // Keep the scene alive while you carry it into space: the nearest opponent closes
                // you down (jockeying goal-side, not standing on you), so it reads as a contest
                // rather than everyone frozen until the minigame arms - and the press is already
                // set when it does.
                {
                    sf::Vector2f bp = curPos;
                    int oppBase = m_engine->isHome() ? 11 : 0;
                    int nearest = -1; float best = 1e9f;
                    for (int i = oppBase; i < oppBase + 11; ++i) {
                        if (i % 11 == 0 || hasRedCard(i)) continue;
                        sf::Vector2f d = m_dots[i].shape.getPosition() - bp;
                        float dd = std::hypot(d.x, d.y);
                        if (dd < best) { best = dd; nearest = i; }
                    }
                    if (nearest >= 0) {
                        float goalX = m_engine->isHome() ? 845.f : 35.f;
                        sf::Vector2f toGoal(goalX - bp.x, 290.f - bp.y);
                        float gl = std::hypot(toGoal.x, toGoal.y);
                        if (gl > 1.f) { toGoal.x /= gl; toGoal.y /= gl; }
                        m_dots[nearest].targetPos = bp + toGoal * 22.f;
                        m_dots[nearest].speed = 135.f;
                    }
                }

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
                    // Bounded retry (only two candidates): an unbounded do/while hangs the app
                    // if both are sent off.
                    for (int tries = 0; tries < 8; ++tries) {
                        m_attackFwdIdx = ctx.attackerBase + 9 + (rand()%2);
                        if (!hasRedCard(m_attackFwdIdx)) break;
                    }
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

                // MANUAL RUSH: hold LMB to charge off your line at the striker. Reach him and
                // you smother it (a save); come out and fail to reach him and you're stranded
                // when he shoots. Let go and you hold your line for the dive instead.
                int gkIdx = userIsHome ? 0 : 11;
                if (m_attackFwdIdx >= 0 && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    sf::Vector2f gp = m_dots[gkIdx].shape.getPosition();
                    sf::Vector2f ap = m_dots[m_attackFwdIdx].shape.getPosition();
                    sf::Vector2f d = ap - gp;
                    float dd = std::hypot(d.x, d.y);
                    if (dd > 1.f) { m_dots[gkIdx].targetPos = ap; m_dots[gkIdx].speed = 175.f; }

                    if (dd < 30.f) {
                        // Smothered at his feet - clean save, keeper collects. Drain the log so
                        // it shows and play resumes with the keeper on the ball.
                        m_ballCarrierIdx = gkIdx;
                        m_visualBall.setPosition(m_dots[gkIdx].shape.getPosition());
                        m_engine->processMinigameResult(buildMinigameResult(true, MinigameActionKind::Save, ActionVariant::Dive));
                        while (m_engine->hasLogs()) {
                            MatchEvent e = m_engine->popRecentLog();
                            m_engine->commitEvent(e);
                            m_visibleLogs.push_back(e);
                            if (m_visibleLogs.size() > 5) m_visibleLogs.erase(m_visibleLogs.begin());
                        }
                        m_currentZoom = 1.0f; m_camera = m_uiView;
                        m_visualState = VisualState::NormalPlay;
                        m_stateTimer = 0.f;
                        return;
                    }
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
                        // Did he rush and get caught off his line? If so the dive is a
                        // scramble back and much harder (handled in resolveGkDive).
                        // Committed only if you actually RUSHED out from your resting spot on
                        // the line (~70/810) - measured from THERE, not the goal line itself,
                        // or a keeper standing still counted as "out" and could never save.
                        float gkRestX = userIsHome ? 70.f : 810.f;
                        m_gkCommittedRush = std::fabs(m_dots[gkIdx].shape.getPosition().x - gkRestX) > 35.f;
                        m_stateTimer = 0.f;
                        m_ballCarrierIdx = -1;
                        m_engine->triggerMinigame();
                    }
                }
    }
}
