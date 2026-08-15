#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <vector>
#include <string>

// Single place all sound goes through (the audio analogue of PitchRenderer / UIKit). One-shot
// SFX play through a small pool of voices so overlapping sounds work; background music streams
// on one channel. Volumes come live from g_settings, so the Settings sliders take effect at once.
// The .wav files in assets/sounds are procedural placeholders - drop real files in with the same
// names to upgrade them, no code change needed.
class AudioManager {
public:
    static AudioManager& get();

    void loadAll();                                   // load every SFX buffer once at startup
    void sfx(const std::string& name);                // fire a one-shot effect at the sound volume
    void kick();                                      // a random ball-strike sound (shot_1..N)
    void goalNet();                                   // random net sound (goal_1..N), on a goal
    void reaction(bool homeScored);                   // crowd celebration / whistle, faded in & out
    void setAmbienceEnabled(bool on);                 // stadium ambience during matches (faded loop)
    void playMusic(const std::string& name, bool loop = true); // stream one track (music volume)
    // Play every track in a folder on a loop (rotates to the next when one ends). Returns true
    // if at least one track was found. Drop as many files as you like into the folder.
    bool musicPlaylist(const std::string& folder);
    void stopMusic();
    void setMusicEnabled(bool on);                    // pause/resume music (e.g. off during a match)
    void startCrowd();                                // looping stadium ambience (match only)
    void stopCrowd();
    void update(float dt);                            // advances playlists + drives ambience/reaction fades
    void applyVolumes();                              // re-apply settings volumes to what's playing

private:
    AudioManager() = default;
    std::size_t pickNextTrack();                      // random, but no repeat within m_gap tracks

    std::map<std::string, sf::SoundBuffer> m_buffers;
    std::vector<sf::Sound> m_voices;
    std::size_t m_cursor = 0;
    int m_shotCount = 0;                               // how many shot_N ball-strike sounds loaded
    sf::Sound m_crowd;          // dedicated looping ambience voice
    sf::Music m_music;
    bool m_musicPlaying = false;
    bool m_musicEnabled = true;                       // desired on/off (false silences during matches)
    bool m_hasSource = false;                         // a track/playlist has been opened at least once
    float m_musicFade = 1.f, m_musicFadeTarget = 1.f; // 0..1 envelope so music fades smoothly on/off
    float musicVolume() const;                        // slider level * fade envelope

    std::vector<std::string> m_playlist;              // resolved file paths
    std::size_t m_playlistIdx = 0;
    bool m_usingPlaylist = false;
    std::size_t m_gap = 0;                            // min tracks before one may repeat
    std::vector<std::size_t> m_recent;                // rolling window of the last m_gap indices

    int m_goalCount = 0;

    // Stadium ambience: a faded loop that rotates intershum clips (each fades in/out so clip
    // boundaries don't click), plus the overall fade in at kick-off / out at the final whistle.
    sf::Music m_amb;
    std::vector<std::string> m_ambList;               // resolved intershum file paths
    std::size_t m_ambIdx = 0;
    float m_ambVol = 0.f, m_ambTarget = 0.f;          // 0..1 fade
    bool m_ambOn = false;
    void ambienceNextClip();

    // One-off crowd reaction to a goal (celebration / whistle), on its own faded channel.
    sf::Music m_react;
    float m_reactVol = 0.f, m_reactTarget = 0.f;
};
