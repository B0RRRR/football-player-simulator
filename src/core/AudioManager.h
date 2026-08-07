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
    void playMusic(const std::string& name, bool loop = true); // stream one track (music volume)
    // Play every track in a folder on a loop (rotates to the next when one ends). Returns true
    // if at least one track was found. Drop as many files as you like into the folder.
    bool musicPlaylist(const std::string& folder);
    void stopMusic();
    void setMusicEnabled(bool on);                    // pause/resume music (e.g. off during a match)
    void update();                                    // advances the playlist when a track ends
    void applyVolumes();                              // re-apply settings volumes to what's playing

private:
    AudioManager() = default;
    std::size_t pickNextTrack();                      // random, but no repeat within m_gap tracks

    std::map<std::string, sf::SoundBuffer> m_buffers;
    std::vector<sf::Sound> m_voices;
    std::size_t m_cursor = 0;
    sf::Music m_music;
    bool m_musicPlaying = false;
    bool m_musicEnabled = true;                       // desired on/off (false silences during matches)
    bool m_hasSource = false;                         // a track/playlist has been opened at least once

    std::vector<std::string> m_playlist;              // resolved file paths
    std::size_t m_playlistIdx = 0;
    bool m_usingPlaylist = false;
    std::size_t m_gap = 0;                            // min tracks before one may repeat
    std::vector<std::size_t> m_recent;                // rolling window of the last m_gap indices
};
