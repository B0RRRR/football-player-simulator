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
    Foul           // dead ball on the spot, taker walks up
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
    MatchScreen();
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
    int m_sendOffGraceIdx = -1;      // a red-carded offender kept on screen until play restarts
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
    
    // 2D Pitch Elements
    sf::RectangleShape m_pitchRect;
    sf::RectangleShape m_pitchLines;
    sf::CircleShape m_pitchCenter;
    sf::RectangleShape m_leftGoal;
    sf::RectangleShape m_rightGoal;
    
    std::vector<PlayerDot> m_dots;
    sf::CircleShape m_visualBall;
    sf::Vector2f m_ballTarget;
    int m_ballCarrierIdx;
    
    VisualState m_visualState;
    float m_stateTimer;
    MatchEvent m_pendingEvent;
    AttackShape m_attackShape = AttackShape::CenterAttack;
    bool m_passForward = true; // midfielder pass: forward (to a shot) vs sideways/back
    bool m_offsideRun = false; // this cross/through-ball episode: the striker mistimed his run
    bool m_offsidePassReleased = false; // the offside pass has been struck and is flying to him
    int m_attackWingerIdx;
    float m_shotTargetY;
    int m_attackFwdIdx;
    Beat m_attackPhase = Beat::Setup;
    
    float m_simTimer;

    // Scoreboard clock, in fractional minutes. PURELY COSMETIC - the engine's m_minute
    // still drives everything (chance rolls, full time, the scheduled injury/red-card
    // minutes), so this changes nothing about how often chances happen.
    // The engine's minute is frozen for the whole of a scripted episode or minigame, so
    // the board used to sit on a static "43'" for seconds at a time. This ticks in real
    // time in every state and is clamped to [m_minute, m_minute + 0.95], so it always
    // moves but can never run ahead of the engine or lie about full time.
    float m_displayTime = 0.f;

    // Minigame Elements (Tactical Episode)
    bool m_minigameActive;
    float m_minigameTimer;
    
    // Camera
    sf::View m_camera;
    sf::View m_uiView;
    float m_currentZoom;
    
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
    
    // Player Controls
    sf::Vector2f m_userMoveDir;
    float m_dashTimer;
    float m_dashSpeedBonus;
    int m_userIdx;
    ActionVariant m_pendingVariant = ActionVariant::Default;
    MinigameActionKind m_pendingKind = MinigameActionKind::Shot;
    float m_ballLoftTimer = 0.f;

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
    int m_matchSpeedMode; // 0=Slow, 1=Normal, 2=Fast
    bool m_isMinigameResultPending;
    float m_scriptTimer;
};
