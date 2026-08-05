#pragma once
#include "Screen.h"
#include "MatchEngine.h"
#include "TimingBar.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

enum class VisualState {
    Kickoff,
    NormalPlay,
    Attacking,
    GoalCelebration,
    WaitingForMinigame,
    GoalKick,
    PassingScript,
    PressingScript,
    FoulChallenge, // the foul being committed - offender lunges into the victim
    Foul,          // dead ball on the spot, taker walks up
    FreeKickShot,  // a direct free kick near goal: wall is set, taker strikes it
    ThrowIn,       // ball out over a touchline: taker walks over and throws it back in
    Corner         // ball out over a goal line off a defender: crowded box, then the cross
};

// Beats of a scripted episode. These were bare ints (0,1,2,3,9..12,40..44,50..52,
// 60,61,70) scattered through one giant if/else - the numbering has gaps because
// scripts were bolted on over time, so the values are pinned to the old ones to keep
// the change mechanical.
enum class Beat {
    Setup            = 0,  // pick the script, position everyone
    CrossInFlight    = 1,  // wing cross is in the air
    Shot             = 2,  // the strike itself
    Resolve          = 3,  // goal / save / miss aftermath

    DefTacklePrep    = 9,  // defender episode: ball travels to the passer
    DefTackleClose   = 10, // defender sprints to intercept
    DefTacklePause   = 11, // frozen while the QTE runs
    DefTackleResolve = 12,

    MidPassHold      = 40, // midfielder carries the ball, then the pass QTE arms
    MidPassResolve   = 41,
    PassInFlight     = 42,
    PassIntercepted  = 43,
    PassReceived     = 44,

    MidTackleChase   = 50, // midfielder defending: sprint at the carrier
    MidTackleResolve = 51,
    MidTackleWon     = 52,

    SoloRun          = 60,
    SoloRunResolve   = 61,

    GkShotWindup     = 70  // attacker winds up; the save QTE arms on release
};

// The build-up shape of an attack. This used to be an int (m_attackType) that meant
// Wing/Solo/Center (0/1/2) AND, in the midfielder pass path, pass direction (2/3) -
// two unrelated axes on one variable. Pass direction now lives in m_passForward.
enum class AttackShape {
    WingCross,    // was m_attackType 0
    SoloRun,      // was 1
    CenterAttack, // was 2
    Counter,
    ThroughBall,
    LongShot
};

struct PlayerDot {
    sf::CircleShape shape;
    sf::Vector2f targetPos;
    float speed;
    bool isHome;
};

class MatchScreen : public Screen {
public:
    MatchScreen() = default;
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    void initMinigame();
    void updateMinigame(sf::Time deltaTime);
    bool hasRedCard(int globalIdx) const;
    // Returns idx if that player is still on the pitch, otherwise the nearest live
    // team-mate. Scripts target players by fixed index; a sent-off player is invisible
    // but his dot still exists, so a cross/pass aimed at him sent the ball drifting to
    // an empty spot ("bouncing off the air").
    int liveTeammate(int idx) const;
    // The live outfield player of `base`'s team standing closest to the ball right now.
    // Used to start an episode from wherever the ball already is, instead of handing it
    // to a fixed shirt number on the far side of the pitch.
    int nearestToBall(int base) const;
    void updateVisuals(sf::Time deltaTime);
    void resetToKickoff();
    MinigameResult buildMinigameResult(bool success, MinigameActionKind kind, ActionVariant variant = ActionVariant::Default) const;

    // QTE flow: startQTE arms the timing bar for an action, resolveQTE applies the
    // grade once the player locks it (or it expires).
    int statForAction(MinigameActionKind kind) const;
    void startQTE(MinigameActionKind kind, ActionVariant variant, bool hardMode, float sweeps = 2.0f);
    void resolveQTE(const QTEResult& result);
    // Ends the interactive episode and restores the wide match camera. Must be used
    // everywhere a minigame finishes: a QTE locked by the player resolves inside
    // handleInput, after which updateMinigame is no longer called and could not undo
    // the zoom itself.
    void endMinigame();
    // Best team-mate to aim a pass at, or -1 if none. Index into m_dots.
    int pickPassTarget() const;
    // Dead-ball restart after a foul/card. offenderIsHome names the team that gave it
    // away; the other side takes the kick.
    void setupFreeKick(bool offenderIsHome);
    // Plays a brief visible challenge (offender lunges into victim) before the dead ball,
    // so a card/foul reads as an actual foul rather than the ball just stopping.
    void beginFoul(bool offenderIsHome);
    // If m_pendingEvent (just popped) is a card/foul, commit it and start the challenge,
    // returning true so the caller aborts whatever it was doing. Episode scripts pop the
    // "shot outcome" from the log queue and could otherwise swallow a card, treating it as
    // a miss and skipping the visible foul.
    bool handleFoulIfCard();
    // Forward-coordinate (X) of the offside line for a team attacking: the second-rearmost
    // defender of the side they're attacking. A runner beyond it is offside.
    float offsideLineX(bool attackingHome) const;
    void holdOffsideLine(int defenderBase, bool attackingHome); // back four holds one flat line
    // Whistle for offside and restart to the defending side. offenderIsHome names the
    // ATTACKING team that strayed offside.
    void resolveOffside(bool attackingHome);
    // Holds the ball with the passer while the striker breaks clear, and whistles offside
    // only once he is genuinely past the line (so it's visible on screen). If he can't get
    // clear, drops the offside intent and the move plays on. Returns true while it owns the
    // frame (caller should return).
    bool offsideBuildup(int strikerIdx, bool attackingHome, int holderIdx);
    // Shared state every episode beat needs, computed once per frame.
    struct EpisodeCtx {
        int attackerBase;
        int defenderBase;
        bool isGoal;
        bool isSave;
        bool isMiss;
        float ballDist; // ball's distance from its current target
    };

    // The Attacking state, split by script. Each run* method owns one scripted
    // scenario; adding a new attack is adding a method, not another arm of a
    // 500-line if/else.
    void updateAttackEpisode(float dt);
    void runEpisodeSetup(float dt, const EpisodeCtx& ctx);
    void runWingCross(float dt, const EpisodeCtx& ctx);
    void runSoloRun(float dt, const EpisodeCtx& ctx);
    void runMidfielderPass(float dt, const EpisodeCtx& ctx);
    void runMidfielderTackle(float dt, const EpisodeCtx& ctx);
    void runDefenderTackle(float dt, const EpisodeCtx& ctx);
    void runGoalkeeperSave(float dt, const EpisodeCtx& ctx);
    void runShotResolution(float dt, const EpisodeCtx& ctx);
    // Weighted pick of the build-up shape from the match situation (momentum, score,
    // relative strength), replacing a flat rand()%3.
    AttackShape pickAttackShape(bool attackingHome) const;

    // Steers all dots toward their targets. Runs in normal play AND during a minigame.
    void updateDotMotion(float dt);
    // Baseline "living shape" for everyone: each player's formation slot, slid with the
    // ball. Runs before the scripts/minigame AI, which then overwrite whoever is actually
    // involved - so uninvolved players drift with the play instead of standing frozen.
    void updateAmbientShape();
    // Gives the other 21 players something to do while a minigame runs: opponents press
    // the ball, team-mates make runs, keepers hold their line. Without this they stand
    // still, because the episode scripts don't tick during a minigame.
    void updateMinigameAI(float dt);
    void resolveShotQTE(const QTEResult& result);
    void resolvePassQTE(const QTEResult& result);
    void resolveTackleQTE(const QTEResult& result);
    void resolveSaveQTE(const QTEResult& result);

    std::shared_ptr<MatchEngine> m_engine;
    
    // UI Elements
    sf::Sprite m_homeLogo;
    sf::Sprite m_awayLogo;
    sf::Text m_homeName;
    sf::Text m_awayName;
    sf::Text m_scoreText;
    sf::Text m_timeText;
    
    sf::Text m_logText;
    sf::Text m_statusText;
    sf::Text m_statsTitle;
    sf::Text m_homeStatsText;
    sf::Text m_awayStatsText;
    sf::RectangleShape m_btnSkipRect;
    sf::Text m_btnSkipText;
    
    int m_foulPlayerIdx = -1; // who restarts play after a foul
    int m_foulVictimIdx = -1;
    int m_foulOffenderIdx = -1;      // who committed the foul (shown lunging in)
    bool m_foulOffenderIsHome = true; // remembered across the challenge -> free kick
    bool m_foulContact = false;      // the two men have actually met; the stumble is now playing
    int m_sendOffGraceIdx = -1;      // a red-carded offender kept on screen until play restarts

    // Direct free kick (wall + shot). Set up by setupFreeKick when the foul is within
    // shooting range; played out by the FreeKickShot state.
    bool m_fkDirect = false;         // this dead ball is a direct free kick, not a simple restart
    bool m_fkUserTaker = false;      // the user is taking it (interactive timing bar)
    bool m_fkPenalty = false;        // the foul was inside the box: penalty, no wall
    bool m_fkAttackHome = true;      // the fouled (attacking) side is home
    bool m_fkStruck = false;         // the ball has been struck at goal
    bool m_fkResolved = false;       // the outcome has been registered with the engine
    bool m_fkSuccess = false;        // the struck shot beats the keeper
    bool m_fkHitWall = false;        // a failed kick that cannoned into the wall (vs flying wide)
    float m_fkWindup = 0.f;          // AI taker's run-up timer before he strikes
    int m_fkKeeperIdx = -1;          // the defending keeper (dot index)
    int m_fkWall[4] = {-1,-1,-1,-1}; // defenders forming the wall
    sf::Vector2f m_fkWallPos[4];     // where each wall defender must stand (re-asserted vs ambient)
    int m_fkWallCount = 0;
    ActionVariant m_fkVariant = ActionVariant::Default;

    void strikeFreeKick(bool success, ActionVariant variant);
    void registerFreeKickOutcome();

    // Throw-ins. The ball leaving over a touchline used to just sit out of play until
    // NormalPlay dragged it across the pitch to whichever carrier it picked.
    int m_lastToucherIdx = -1;       // who touched the ball last (decides who concedes)
    int m_throwInTaker = -1;
    sf::Vector2f m_throwInSpot;
    bool beginThrowInIfOut();

    // The man taking a dead ball is exempt from updateDotMotion's keep-on-the-pitch clamp,
    // so he can stand right on (or just behind) the line like a real thrower/corner taker.
    int m_deadBallTakerIdx = -1;

    // Corners: ball out over a goal line off a defender. Both boxes crowd, then the cross.
    bool m_cornerAttackHome = true;  // the side attacking (taking the corner) is home
    int m_cornerTaker = -1;
    sf::Vector2f m_cornerSpot;
    bool m_cornerUserTakes = false;  // user delivers it (midfielder / defender)
    bool m_cornerUserHead = false;   // user attacks the cross in the box (forward)
    bool m_cornerStruck = false;
    bool m_cornerResolved = false;
    bool m_cornerSuccess = false;        // it ends in a goal
    bool m_cornerGoodDelivery = false;   // the cross actually found its man
    bool m_cornerHeaderPending = false;  // user forward is timing his header on the incoming ball
    bool m_cornerDeflecting = false;     // ball still rolling out over the byline off the keeper
    sf::Vector2f m_cornerDeflectTarget;
    bool m_cornerHeaded = false;         // the cross has been met; the finish is now in flight
    sf::Vector2f m_cornerAimOffset;      // how far off the man a poor delivery lands
    float m_cornerWindup = 0.f;
    int m_cornerTargetIdx = -1;      // who the delivery is aimed at
    sf::Vector2f m_cornerAimBase;    // his box ANCHOR (aim here, not his live spot mid-run)
    int m_cornerCrowd[12] = {0};
    sf::Vector2f m_cornerCrowdPos[12];
    int m_cornerCrowdCount = 0;

    void beginCorner(bool attackingHome, float outY);
    void deliverCorner(bool good);
    sf::Vector2f cornerAimPoint() const;
    void registerCornerOutcome();
    // Fixed once when the foul is given. Recomputing these per frame made the victim's
    // target run 28px further away every frame - a treadmill that sent both players
    // sprinting off across the pitch.
    sf::Vector2f m_foulLungeTarget;   // where the offender lunges to
    sf::Vector2f m_foulStaggerTarget; // where the victim is knocked to
    // Real-time clock (NOT scaled by match speed) for the foul beats, so the challenge and
    // dead-ball pause read the same however fast the match is being run.
    float m_foulClock = 0.f;
    
    std::vector<MatchEvent> m_visibleLogs;
    std::vector<sf::RectangleShape> m_momentumBars;
    
    // 2D Pitch Elements. The pitch geometry itself is drawn by PitchRenderer (shared with the
    // training drills); only the moving pieces (dots, ball) are held here as position state.
    std::vector<PlayerDot> m_dots;
    sf::CircleShape m_visualBall;
    sf::Vector2f m_ballTarget;
    int m_ballCarrierIdx = -1;

    VisualState m_visualState = VisualState::Kickoff;
    float m_stateTimer = 0.f;
    MatchEvent m_pendingEvent;
    AttackShape m_attackShape = AttackShape::CenterAttack;
    bool m_passForward = true; // midfielder pass: forward (to a shot) vs sideways/back
    bool m_offsideRun = false; // this cross/through-ball episode: the striker mistimed his run
    bool m_offsidePassReleased = false; // the offside pass has been struck and is flying to him
    bool m_boxFoulRolled = false;       // this attack has already rolled for a foul near the box
    int m_attackWingerIdx = -1;
    float m_shotTargetY = 290.f;

    // "Draw & strike" shot minigame. Phase 1: hold LMB by the ball and trace the ball's path
    // freehand with the cursor - a polyline of finite length. Phase 2: a power bar. The ball
    // then follows the drawn path and, once it runs out, carries straight on along the last
    // heading until it crosses the goal line (existing detection/keeper save resolve it).
    enum class ShotStage { None, Aiming, Power };
    ShotStage m_shotStage = ShotStage::None;
    MinigameActionKind m_drawKind = MinigameActionKind::Shot; // Shot or Pass shares the draw UI
    int m_drawButton = 0;                // mouse button that started the draw (0=L shot, 1=R pass)
    ActionVariant m_drawVariant = ActionVariant::Default;     // captured at begin (e.g. lofted pass)
    // A saved shot holds for a real-time beat so the stop is actually seen (the ball at the
    // keeper), then either he gathers it or it's pushed out for a corner.
    bool m_saveHold = false;
    float m_saveHoldT = 0.f;
    bool m_saveParried = false;
    sf::Vector2f m_saveContact;
    int m_saveGkIdx = -1;
    MinigameActionKind m_saveKind = MinigameActionKind::Shot;

    // Defender 1v1 duel: the attacker dribbles at you and jinks the ball; you time a click on
    // the ball to nick it. A read-and-react tackle instead of a plain timing bar.
    bool m_defDuel = false;
    float m_defDuelT = 0.f;
    float m_defFeintClock = 0.f;
    sf::Vector2f m_defFeintVec;      // smoothed ball offset from the attacker's feet
    sf::Vector2f m_defFeintTarget;   // where it's jinking toward (changes fast, any direction)
    bool m_defDuelActed = false;
    int m_defDuelAttacker = -1;
    void resolveDefenderDuel(bool won);

    // A dispossession in progress: an opponent (or the rushing keeper) runs onto the ball and
    // takes it, rather than the ball teleporting to him.
    bool m_stealHold = false;
    float m_stealHoldT = 0.f;
    int m_stealThief = -1;

    sf::Vector2f m_shotFrom;              // the ball, first point of the path
    sf::Vector2f m_shotGoalAxis{1.f, 0.f};// unit vector from the ball toward the target goal
    float m_shotCurlSign = 0.f;          // locked bend direction (+/-1); a real curl never S-bends
    std::vector<sf::Vector2f> m_shotPath; // traced path points (world coords)
    float m_shotPathLen = 0.f;           // accumulated length, capped so it can't reach forever
    float m_shotPowerT = 0.f;            // 0..1 power marker
    float m_shotPowerDir = 1.f;

    bool  m_shotCurveActive = false;     // the struck ball is travelling the path
    float m_shotFlightDist = 0.f;        // distance covered along the path so far
    float m_shotFlightSpeed = 600.f;     // px/s, from the power bar
    sf::Vector2f m_shotEndDir{1.f, 0.f}; // heading of the final segment (straight continuation)

    void beginDrawAction(MinigameActionKind kind, int button, ActionVariant variant);
    void addShotPathPoint(sf::Vector2f p); // append the raw cursor point (jitter + length cap)
    void constrainShotPath();              // on release: resample + limit curvature to a plausible arc
    void smoothShotPath();                 // Chaikin pass so the traced line reads as a curve
    void launchDrawnAction();              // dispatches to the shot / pass launch by m_drawKind
    void launchDrawnShot();
    void launchDrawnPass();
    // Resolve a drawn pass. receiver >=0 = the team-mate it reached (delivered by pass stat);
    // intercepted = an opponent got in the lane; receiver <0 & !intercepted = played into space.
    void resolveDrawnPass(int receiver, bool intercepted);
    sf::Vector2f shotPointAt(float dist) const; // position `dist` along path, then straight on

    // Goalkeeper "read & dive" save: instead of a timing bar, the keeper reads the flight
    // and commits a dive to a corner. He saves it if he picks the right side (the reach he
    // covers scales with his goalkeeping), not by pressing at the right instant.
    bool m_gkDiveMode = false;   // this save is the directional minigame, not the timing bar
    bool m_gkDived = false;      // he has committed his dive
    bool m_gkCommittedRush = false; // he rushed off his line and the striker shot anyway (out of position)
    float m_stuckTimer = 0.f;    // watchdog: real time the engine minute has been frozen
    int m_lastWatchMin = -1;     // last minute the watchdog saw advance
    bool m_gkDiveSaved = false;  // the committed dive is a save (drives the parry visual)
    float m_gkShotClock = 0.f;   // time since the shot was struck
    float m_gkFlightTime = 1.4f; // how long the ball takes to reach the line
    float m_gkDiveY = 290.f;     // the height he dived to
    float m_gkGoalLineX = 45.f;  // his goal line, where he ends the dive
    void resolveGkDive(float diveY);
    int m_attackFwdIdx = -1;
    Beat m_attackPhase = Beat::Setup;

    float m_simTimer = 0.f;

    // Scoreboard clock, in fractional minutes. PURELY COSMETIC - the engine's m_minute
    // still drives everything (chance rolls, full time, the scheduled injury/red-card
    // minutes), so this changes nothing about how often chances happen.
    // The engine's minute is frozen for the whole of a scripted episode or minigame, so
    // the board used to sit on a static "43'" for seconds at a time. This ticks in real
    // time in every state and is clamped to [m_minute, m_minute + 0.95], so it always
    // moves but can never run ahead of the engine or lie about full time.
    float m_displayTime = 0.f;

    // Minigame Elements (Tactical Episode)
    bool m_minigameActive = false;
    float m_minigameTimer = 0.f;

    // Camera
    sf::View m_camera;
    sf::View m_uiView;
    float m_currentZoom = 1.f;
    
    // Physics
    sf::Vector2f m_ballVelocity;
    // Drag coefficient. Normally 1.5 (a loose ball rolls to a stop quickly), but a
    // struck shot at the keeper uses a much lower value so it flies straight and true
    // instead of decaying to a crawl before it reaches the line.
    float m_ballFriction = 1.5f;
    // Where the ambient formation anchors during a minigame. Frozen at the episode's
    // start so the shape doesn't slide around with the user-controlled ball - otherwise
    // every player mirrors the user's dribble.
    sf::Vector2f m_ambientAnchor;
    // A low-pass-filtered ball position that the off-ball shape follows. The raw ball jumps
    // between episodes/turnovers, and keying the whole formation off it made the entire side
    // lurch across the pitch every time. Eased slowly, players jog into shape instead.
    float m_ambientBallX = 440.f;
    float m_ambientBallY = 290.f;
    
    // Player Controls. These are (re)set in initMinigame, but they're read from draw code
    // and QTE handlers too, so give them sane defaults rather than leaving them garbage.
    sf::Vector2f m_userMoveDir{1.f, 0.f};
    // Mouse aiming: a click sets the exact point the action is played at, instead of the
    // eight coarse directions WASD can express. Consumed by the shot/pass resolvers.
    sf::Vector2f m_mouseAimWorld;
    bool m_mouseAimValid = false;
    float m_dashTimer = 0.f;
    float m_dashSpeedBonus = 0.f;
    int m_userIdx = 0;
    ActionVariant m_pendingVariant = ActionVariant::Default;
    MinigameActionKind m_pendingKind = MinigameActionKind::Shot;
    float m_ballLoftTimer = 0.f;

    // Dribble (take on the nearest opponent) while carrying the ball, on the Q key. Polled
    // in updateMinigame (edge-triggered) rather than off key events, which don't reliably
    // reach the screen. m_dribbleCd rate-limits taps so it isn't a per-press machine gun.
    bool  m_qPrevMatch = false;
    float m_dribbleCd = 0.f;
    // A successful take-on drives you forward for a beat: the player auto-runs along
    // m_userMoveDir (toward goal) while this is >0, so it reads as accelerating past your
    // man rather than teleporting behind him.
    float m_dribbleBurst = 0.f;
    void tryMatchDribble();

    // Rate-limits opponent tackle attempts. Without it the duel roll runs every frame,
    // so at 60fps even a 40% chance lands within a couple of frames and the ball is
    // gone before the player can react - the odds may as well not exist.
    float m_tackleAttemptTimer = 0.f;

    // QTE (timing minigame) - decides the outcome of the committed action
    TimingBar m_qte;
    MinigameActionKind m_qteKind = MinigameActionKind::Shot;
    ActionVariant m_qteVariant = ActionVariant::Default;
    // Quality of the last locked QTE (0..1). Feeds MinigameResult::accuracy, so the
    // engine's rating/log logic keys off the player's timing instead of ball geometry.
    float m_qteAccuracy = 0.f;
    // True once the player has actually struck the ball (shot or pass resolved). Until
    // then an attacking player is dribbling: the ball is glued to his feet rather than
    // lying around as a loose physics object he can accidentally boot away.
    bool m_ballStruck = false;

    // Key states for robust input handling
    bool m_keyUp = false;
    bool m_keyDown = false;
    bool m_keyLeft = false;
    bool m_keyRight = false;
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
        sf::Color baseColor = sf::Color(100, 100, 100);
        bool isHovered = false;
    };
    std::vector<Button> m_speedButtons;
    bool m_isMinigameResultPending = false;
    float m_scriptTimer = 0.f;
};
