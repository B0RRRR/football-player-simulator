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
        d.targetPos.x = std::clamp(d.targetPos.x, 50.f, 820.f);
        d.targetPos.y = std::clamp(d.targetPos.y, 140.f, 440.f);

        sf::Vector2f dir = d.targetPos - d.shape.getPosition();
        float len = std::hypot(dir.x, dir.y);
        if (len <= 0.f) continue;

        float currentSpeed = d.speed;
        if (!m_minigameActive && m_visualState == VisualState::Attacking) {
            currentSpeed = 150.f; // Sprinting during a scripted attack
        }

        float moveDist = currentSpeed * dt;
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

    for (size_t k = 0; k < byDist.size(); ++k) {
        int idx = byDist[k].second;
        if (userOnBall && k < 2) {
            m_dots[idx].targetPos = ballPos; // press the ball
            m_dots[idx].speed = chaseSpeed;
        } else {
            // Hold a loose shape, drifting goal-side of the ball. When the user is
            // defending, everyone holds - so the one man he's tackling is a stable target
            // and no one else swoops in on the loose ball.
            sf::Vector2f pos = m_dots[idx].shape.getPosition();
            m_dots[idx].targetPos = sf::Vector2f(pos.x - attackDir * 8.f, pos.y);
            m_dots[idx].speed = 60.f;
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

    for (size_t k = 0; k < mates.size(); ++k) {
        int idx = mates[k].second;
        sf::Vector2f pos = m_dots[idx].shape.getPosition();
        if (k < 2) {
            // Break forward into space, fanning slightly wide of the ball.
            float wide = (pos.y < ballPos.y) ? -35.f : 35.f;
            m_dots[idx].targetPos = sf::Vector2f(pos.x + attackDir * 90.f, pos.y + wide);
            m_dots[idx].speed = 110.f;
        } else {
            m_dots[idx].targetPos = sf::Vector2f(pos.x + attackDir * 15.f, pos.y);
            m_dots[idx].speed = 60.f;
        }
    }

    // --- Keepers: stay on the line, track the ball's height.
    for (int base : {0, 11}) {
        int gk = base;
        if (gk == m_userIdx) continue; // the user's own keeper minigame drives this dot
        float lineX = (base == 0) ? 70.f : 810.f;
        float y = std::clamp(ballPos.y, 240.f, 340.f);
        m_dots[gk].targetPos = sf::Vector2f(lineX, y);
        m_dots[gk].speed = 90.f;
    }

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

    m_qteAccuracy = 0.f;
    m_engine->processMinigameResult(buildMinigameResult(false, m_pendingKind, m_pendingVariant));
    endMinigame();
}

void MatchScreen::beginFoul(bool offenderIsHome) {
    // Tear down any live minigame first (a mistimed slide tackle gives a card from inside
    // an episode), so its physics/zoom don't run over the challenge.
    if (m_minigameActive) {
        endMinigame();
    }

    m_foulOffenderIsHome = offenderIsHome;

    sf::Vector2f spot = m_visualBall.getPosition();
    m_ballVelocity = sf::Vector2f(0.f, 0.f);
    m_ballFriction = 1.5f;
    m_ballCarrierIdx = -1;

    // Offender = nearest outfield player of the offending side; victim = nearest of the
    // fouled side. They're the two that act out the challenge over the ball.
    int offBase = offenderIsHome ? 0 : 11;
    int vicBase = offenderIsHome ? 11 : 0;
    auto nearest = [&](int base) {
        int best = -1; float bd = 1e9f;
        for (int i = base; i < base + 11; ++i) {
            if (i % 11 == 0) continue;      // not the keeper
            if (hasRedCard(i)) continue;
            sf::Vector2f d = m_dots[i].shape.getPosition() - spot;
            float dd = std::hypot(d.x, d.y);
            if (dd < bd) { bd = dd; best = i; }
        }
        return best;
    };
    m_foulOffenderIdx = nearest(offBase);
    m_foulVictimIdx = nearest(vicBase);

    m_visualState = VisualState::FoulChallenge;
    m_stateTimer = 0.f;
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

    m_foulPlayerIdx = taker;
    if (taker >= 0) {
        m_dots[taker].targetPos = spot;
        m_dots[taker].speed = 120.f;
    }

    m_visualState = VisualState::Foul;
    m_stateTimer = 0.f;
}

void MatchScreen::updateVisuals(sf::Time deltaTime) {
    float dt = deltaTime.asSeconds();
    
    float animSpeedMult = 1.0f;
    if (g_settings.matchSpeed == 2) animSpeedMult = 2.0f;
    if (g_settings.matchSpeed == 3) animSpeedMult = 8.0f;
    
    dt *= animSpeedMult;
    
    m_stateTimer += dt;
    float mom = 0.0f;
    if (!m_engine->getMomentumHistory().empty()) mom = m_engine->getMomentumHistory().back();
    
    float form[11][2] = {
        {0.02f, 0.5f}, {0.2f, 0.2f}, {0.15f, 0.4f}, {0.15f, 0.6f}, {0.2f, 0.8f},
        {0.45f, 0.2f}, {0.4f, 0.4f}, {0.4f, 0.6f}, {0.45f, 0.8f}, {0.7f, 0.35f}, {0.7f, 0.65f}
    };
    
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

        if (m_foulOffenderIdx >= 0 && m_foulVictimIdx >= 0) {
            sf::Vector2f vicPos = m_dots[m_foulVictimIdx].shape.getPosition();
            // Offender charges into the victim.
            m_dots[m_foulOffenderIdx].targetPos = vicPos;
            m_dots[m_foulOffenderIdx].speed = 260.f;
            // Victim is knocked back off the ball and staggers away from the offender.
            sf::Vector2f offPos = m_dots[m_foulOffenderIdx].shape.getPosition();
            sf::Vector2f away = vicPos - offPos;
            float len = std::hypot(away.x, away.y);
            if (len > 0.1f) { away.x /= len; away.y /= len; } else { away = sf::Vector2f(0.f, 1.f); }
            m_dots[m_foulVictimIdx].targetPos = vicPos + away * 28.f;
            m_dots[m_foulVictimIdx].speed = 90.f;
        }

        // Keep the ball dead on the spot through the challenge.
        m_visualBall.setPosition(spot);
        m_ballTarget = spot;
        m_ballVelocity = sf::Vector2f(0.f, 0.f);

        if (m_stateTimer > 1.3f) {
            setupFreeKick(m_foulOffenderIsHome); // whistle: settle into the dead ball
        }
    }
    else if (m_visualState == VisualState::Foul) {
        // Dead ball: it stays on the spot while the taker walks up to it, then he
        // knocks it back into play.
        m_ballCarrierIdx = -1;
        m_ballTarget = m_visualBall.getPosition();

        bool takerReady = false;
        if (m_foulPlayerIdx >= 0 && m_foulPlayerIdx < (int)m_dots.size()) {
            sf::Vector2f d = m_dots[m_foulPlayerIdx].shape.getPosition() - m_visualBall.getPosition();
            takerReady = std::hypot(d.x, d.y) < 16.f;
        }

        // Hold the dead ball for a beat so the foul actually reads as a foul: the whistle
        // goes, everything settles on the spot, the taker walks up, and only then is it
        // put back into play. The 6s arm is a safety net so a blocked taker can never
        // stall the match.
        const float DEAD_BALL_PAUSE = 3.0f;
        if ((takerReady && m_stateTimer > DEAD_BALL_PAUSE) || m_stateTimer > 6.0f) {
            if (m_foulPlayerIdx >= 0 && m_foulPlayerIdx < (int)m_dots.size()) {
                m_ballCarrierIdx = m_foulPlayerIdx;
                m_dots[m_foulPlayerIdx].speed = 100.f;
            }
            m_visualState = VisualState::NormalPlay;
            m_stateTimer = 0.f;
        }
    }

    updateDotMotion(dt);

    // Move ball globally
    if (m_ballCarrierIdx != -1) {
        m_ballTarget = m_dots[m_ballCarrierIdx].shape.getPosition();
    }
    
    float globalBDist = std::hypot(m_ballTarget.x - m_visualBall.getPosition().x, m_ballTarget.y - m_visualBall.getPosition().y);
    if (m_ballCarrierIdx != -1 && globalBDist < 10.f) {
        m_visualBall.setPosition(m_ballTarget); // Snap rigidly if close and held
    } else if (globalBDist > 0.f) {
        sf::Vector2f bdir = m_ballTarget - m_visualBall.getPosition();
        float bspeed = (m_visualState == VisualState::Attacking) ? 500.f : 400.f;
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

                    // Send the striker into the box *now*, while the winger is still carrying
                    // the ball. He used to only set off at the moment of the cross - but the
                    // ball travels at 500/s and he runs at 150/s, so it landed on the spot
                    // and hung there in mid-air waiting for him to catch up.
                    sf::Vector2f crossTarget(boxX, m_shotTargetY);
                    m_dots[m_attackFwdIdx].targetPos = crossTarget;

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
                    m_ballCarrierIdx = liveTeammate(ctx.attackerBase + 7);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(boxX + (m_pendingEvent.isHome ? 20.f : -20.f), m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 180.f; // Faster run!
                    if (m_stateTimer > 1.5f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                } else if (m_attackShape == AttackShape::Counter) {
                    // Fast direct break: the carrier starts deep and sprints at goal with
                    // no build-up. Fewer defenders get back (numDefendersToRun is trimmed
                    // in the dispatcher for this shape), so it's a genuine fast break.
                    m_ballCarrierIdx = liveTeammate(ctx.attackerBase + 9);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(boxX, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 230.f; // quicker than a solo run
                    if (m_stateTimer > 1.2f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                } else if (m_attackShape == AttackShape::ThroughBall) {
                    // A pass slid in behind the defense: a deep carrier holds it while the
                    // striker peels off, then the ball is released into space for him to run
                    // onto. Reuses the cross-flight beat to carry the ball and meet the man.
                    m_ballCarrierIdx = liveTeammate(ctx.attackerBase + 7);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(deepX, m_shotTargetY);

                    sf::Vector2f runTarget(boxX, m_shotTargetY);
                    m_dots[m_attackFwdIdx].targetPos = runTarget;
                    m_dots[m_attackFwdIdx].speed = 200.f; // making the run

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
                    m_ballCarrierIdx = liveTeammate(ctx.attackerBase + 7);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(deepX, m_shotTargetY);
                    m_dots[m_ballCarrierIdx].speed = 120.f;
                    if (m_stateTimer > 1.0f) { m_attackPhase = Beat::Shot; m_stateTimer = 0.f; }
                } else { // CenterAttack
                    m_ballCarrierIdx = m_attackFwdIdx;
                    float attackX = boxX + (rand()%20 - 10.f);
                    m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(attackX, m_shotTargetY);
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

                // Resolve as soon as the ball lands. This used to also demand the striker be
                // standing on the exact spot, which is what left the ball hanging motionless
                // in the middle - it had arrived and simply waited for him. He now gets a
                // head start back in phase 0, so he meets it.
                if (ctx.ballDist < 12.f || m_stateTimer > 2.0f) {
                    // If there are pending events (like the shot outcome), pop it to use for the shot visualization
                    if (m_engine->hasLogs()) {
                        m_pendingEvent = m_engine->popRecentLog();
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
                sf::Vector2f curPos = m_dots[m_ballCarrierIdx].shape.getPosition();
                float advanceX = m_engine->isHome() ? std::min(curPos.x + 150.f, 650.f) : std::max(curPos.x - 150.f, 230.f);
                m_dots[m_ballCarrierIdx].targetPos = sf::Vector2f(advanceX, curPos.y);
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
                // Goalkeeper Save: attacker winds up, then strikes toward goal.
                // Start zooming toward our own goal now, during the wind-up, so the camera
                // is already framed there once the shot is struck - instead of chasing the
                // ball's position from wherever it happens to start (which could be nowhere
                // near the keeper yet).
                bool userIsHome = m_engine->isHome();
                sf::Vector2f goalArea(userIsHome ? 90.f : 790.f, m_shotTargetY);

                float targetZoom = 0.5f;
                m_currentZoom += (targetZoom - m_currentZoom) * 3.0f * dt;
                float viewWidth = 1280.f * m_currentZoom;
                float viewHeight = 720.f * m_currentZoom;
                float minX = 40.f + viewWidth / 2.f;
                float maxX = 840.f - viewWidth / 2.f;
                float minY = 130.f + viewHeight / 2.f;
                float maxY = 450.f - viewHeight / 2.f;
                sf::Vector2f targetCenter = goalArea;
                targetCenter.x = (minX > maxX) ? 440.f : std::clamp(targetCenter.x, minX, maxX);
                targetCenter.y = (minY > maxY) ? 290.f : std::clamp(targetCenter.y, minY, maxY);
                sf::Vector2f currentCenter = m_camera.getCenter();
                currentCenter += (targetCenter - currentCenter) * 4.0f * dt;
                m_camera.setCenter(currentCenter);
                m_camera.setSize(1280.f * m_currentZoom, 720.f * m_currentZoom);

                if (m_stateTimer > 0.5f && !m_minigameActive) {
                    m_stateTimer = 0.f;
                    m_ballCarrierIdx = -1;
                    m_engine->triggerMinigame();
                }
    }
}
