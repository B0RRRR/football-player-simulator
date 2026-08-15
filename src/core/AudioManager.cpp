#include "AudioManager.h"
#include "Settings.h"
#include "Logger.h"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>

AudioManager& AudioManager::get() {
    static AudioManager instance;
    return instance;
}

// Load any format into a SoundBuffer by streaming all samples ourselves. SoundBuffer::loadFromFile
// fails on mp3 in this SFML build (the mp3 reader reports no sample count up-front, so the buffer
// ends up empty), even though InputSoundFile/Music read mp3 fine. Reading sample chunks until EOF
// works for mp3 as well as wav/ogg/flac.
static bool loadBuffer(const std::string& path, sf::SoundBuffer& out) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return false; // skip missing files quietly (no SFML spam)
    sf::InputSoundFile file;
    if (!file.openFromFile(path)) return false;
    std::vector<sf::Int16> samples;
    sf::Int16 chunk[8192];
    sf::Uint64 n;
    while ((n = file.read(chunk, 8192)) > 0) samples.insert(samples.end(), chunk, chunk + n);
    if (samples.empty()) return false;
    return out.loadFromSamples(samples.data(), samples.size(), file.getChannelCount(), file.getSampleRate());
}

void AudioManager::loadAll() {
    m_voices.clear();
    m_voices.resize(16); // enough overlapping one-shots for anything the game throws at once

    // UI + event one-shots + whistles. (Optional files: a missing one is just silent, no crash.)
    const char* names[] = {"click", "confirm", "deny", "card", "post", "miss",
                           "whistle_long", "whistle_short_1", "whistle_short_2",
                           "whistle_short_3", "whistle_short_4"};
    // Try formats in order so any of .ogg/.mp3/.wav/.flac works (SFML 2.6+ decodes mp3).
    const char* exts[] = {".ogg", ".mp3", ".wav", ".flac"};
    auto tryLoad = [&](const std::string& name) -> bool {
        sf::SoundBuffer buf;
        for (const char* e : exts)
            if (loadBuffer("assets/sounds/" + name + e, buf)) { m_buffers[name] = buf; return true; }
        return false;
    };
    for (const char* n : names) tryLoad(n);

    // Ball-strike sounds shot_1, shot_2, ... (contiguous); played at random by kick().
    m_shotCount = 0;
    for (int i = 1; i <= 40; ++i) {
        if (!tryLoad("shot_" + std::to_string(i))) break;
        m_shotCount = i;
    }
    m_goalCount = 0;
    for (int i = 1; i <= 40; ++i) {
        if (!tryLoad("goal_" + std::to_string(i))) break;
        m_goalCount = i;
    }
    // Ambience clip list (intershum_1, intershum_2, ...).
    m_ambList.clear();
    for (int i = 1; i <= 40; ++i) {
        std::string base = "assets/sounds/intershum_" + std::to_string(i);
        bool found = false; std::error_code ec;
        for (const char* e : {".ogg", ".mp3", ".wav", ".flac"})
            if (std::filesystem::exists(base + e, ec)) { m_ambList.push_back(base + e); found = true; break; }
        if (!found) break;
    }
    LOG_INFO("Audio: loaded " << m_buffers.size() << " sfx buffers (" << m_shotCount << " ball-strike variants)");
    if (m_buffers.find("whistle_long") == m_buffers.end())
        LOG_WARN("Audio: whistle_long missing - no kick-off/full-time whistle");
}

void AudioManager::kick() {
    if (m_shotCount > 0) sfx("shot_" + std::to_string(1 + rand() % m_shotCount));
}

void AudioManager::goalNet() {
    if (m_goalCount > 0) sfx("goal_" + std::to_string(1 + rand() % m_goalCount));
}

// Open the first existing format for a base path into a Music channel (no SFML "missing" spam).
static bool openMusicFile(sf::Music& m, const std::string& base) {
    for (const char* e : {".ogg", ".mp3", ".wav", ".flac"}) {
        std::error_code ec;
        if (std::filesystem::exists(base + e, ec) && m.openFromFile(base + e)) return true;
    }
    return false;
}

void AudioManager::reaction(bool homeScored) {
    const char* name = homeScored ? "goal_my_team" : "goal_opponents"; // home fans cheer / whistle
    if (!openMusicFile(m_react, std::string("assets/sounds/") + name)) return;
    m_react.setLoop(false);
    m_reactVol = 0.f;              // fade in from silence
    m_reactTarget = 1.f;
    m_react.setVolume(0.f);
    m_react.play();
}

void AudioManager::ambienceNextClip() {
    if (m_ambList.empty()) return;
    std::size_t next = m_ambIdx;
    if (m_ambList.size() > 1) { // avoid the immediate repeat
        do { next = (std::size_t)(rand() % m_ambList.size()); } while (next == m_ambIdx);
    }
    m_ambIdx = next;
    if (m_amb.openFromFile(m_ambList[m_ambIdx])) {
        m_amb.setLoop(false);
        m_amb.setVolume(m_ambVol * g_settings.soundVolume);
        m_amb.play();
    }
}

void AudioManager::setAmbienceEnabled(bool on) {
    if (on == m_ambOn) return;
    m_ambOn = on;
    m_ambTarget = on ? 1.f : 0.f;
    // Leaving the match: fade any crowd reaction (celebration/whistle) out at once, so it doesn't
    // carry on shouting into the menu.
    if (!on) m_reactTarget = 0.f;
    if (on && m_amb.getStatus() != sf::Music::Playing) ambienceNextClip();
}

void AudioManager::sfx(const std::string& name) {
    auto it = m_buffers.find(name);
    if (it == m_buffers.end() || m_voices.empty()) return;

    // Prefer a free voice; otherwise steal the next one round-robin.
    sf::Sound* voice = nullptr;
    for (auto& v : m_voices)
        if (v.getStatus() != sf::Sound::Playing) { voice = &v; break; }
    if (!voice) { voice = &m_voices[m_cursor]; m_cursor = (m_cursor + 1) % m_voices.size(); }

    voice->setBuffer(it->second);
    voice->setVolume(static_cast<float>(g_settings.soundVolume));
    voice->play();
}

void AudioManager::playMusic(const std::string& name, bool loop) {
    // Try common formats in order (only ones that exist, to avoid SFML "couldn't open" spam).
    const std::string base = std::string("assets/sounds/") + name;
    bool opened = false;
    for (const char* e : {".ogg", ".mp3", ".wav", ".flac"}) {
        std::error_code ec;
        if (std::filesystem::exists(base + e, ec) && m_music.openFromFile(base + e)) { opened = true; break; }
    }
    if (!opened) { m_musicPlaying = false; return; }
    m_music.setLoop(loop);
    m_music.setVolume(musicVolume());
    m_music.play();
    m_musicPlaying = true;
    m_usingPlaylist = false;
    m_hasSource = true;
}

std::size_t AudioManager::pickNextTrack() {
    std::size_t n = m_playlist.size();
    if (n <= 1) { return 0; }
    // Candidates = every index NOT among the last m_gap played, so a track can't repeat until
    // at least m_gap others have played.
    std::vector<std::size_t> cand;
    for (std::size_t i = 0; i < n; ++i)
        if (std::find(m_recent.begin(), m_recent.end(), i) == m_recent.end())
            cand.push_back(i);
    std::size_t choice = cand.empty() ? (std::size_t)(rand() % n) : cand[rand() % cand.size()];
    m_recent.push_back(choice);
    if (m_recent.size() > m_gap) m_recent.erase(m_recent.begin());
    return choice;
}

bool AudioManager::musicPlaylist(const std::string& folder) {
    m_playlist.clear();
    m_recent.clear();
    m_playlistIdx = 0;
    m_usingPlaylist = false;

    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(folder, ec) && fs::is_directory(folder, ec)) {
        for (const auto& e : fs::directory_iterator(folder, ec)) {
            std::string ext = e.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            if (ext == ".ogg" || ext == ".mp3" || ext == ".wav" || ext == ".flac")
                m_playlist.push_back(e.path().string());
        }
    }
    std::sort(m_playlist.begin(), m_playlist.end());

    if (m_playlist.empty()) return false;
    // Don't replay a track until 5 others have played (or as many as exist, if fewer).
    m_gap = std::min<std::size_t>(5, m_playlist.size() - 1);

    m_playlistIdx = pickNextTrack();
    if (!m_music.openFromFile(m_playlist[m_playlistIdx])) return false;
    m_music.setLoop(false); // rotation (see update) advances to the next random track
    m_music.setVolume(musicVolume());
    m_music.play();
    m_musicPlaying = true;
    m_usingPlaylist = true;
    m_hasSource = true;
    return true;
}

void AudioManager::update(float dt) {
    const float sv = static_cast<float>(g_settings.soundVolume);
    auto ramp = [&](float& v, float target, float rate) {
        if (v < target) v = std::min(target, v + rate * dt);
        else if (v > target) v = std::max(target, v - rate * dt);
    };

    // Smoothly fade music toward its target (1 on menus, 0 for match/pre-match). ~2s glide.
    ramp(m_musicFade, m_musicFadeTarget, 0.55f);
    if (m_music.getStatus() == sf::Music::Playing) {
        m_music.setVolume(musicVolume());
        // Fully faded out and no longer wanted: pause so it can resume later from the same spot.
        if (!m_musicEnabled && m_musicFade <= 0.001f) m_music.pause();
    }

    // When a music track ends (and music is enabled), pick the next one at random (no near repeats).
    if (m_usingPlaylist && m_musicEnabled && !m_playlist.empty()
        && m_music.getStatus() == sf::Music::Stopped) {
        m_playlistIdx = pickNextTrack();
        if (m_music.openFromFile(m_playlist[m_playlistIdx])) {
            m_music.setLoop(false);
            m_music.setVolume(musicVolume());
            m_music.play();
        }
    }

    // Stadium ambience: fade each clip out near its end and the next one in, so boundaries don't
    // click; the whole loop fades in at kick-off and out at the final whistle (via m_ambOn).
    if (m_ambOn || m_ambVol > 0.001f) {
        if (m_ambOn && m_amb.getStatus() == sf::Music::Playing) {
            float rem = m_amb.getDuration().asSeconds() - m_amb.getPlayingOffset().asSeconds();
            m_ambTarget = (rem < 0.9f) ? 0.f : 1.f;
        }
        ramp(m_ambVol, m_ambTarget, m_ambOn ? 1.1f : 3.5f); // fade in gently, but out fast on match end
        m_amb.setVolume(m_ambVol * sv * 0.4f); // ambience sits well under the SFX (too loud otherwise)
        if (m_ambOn && m_amb.getStatus() == sf::Music::Stopped) { ambienceNextClip(); m_ambTarget = 1.f; }
        if (!m_ambOn && m_ambVol <= 0.001f && m_amb.getStatus() == sf::Music::Playing) m_amb.stop();
    }

    // Goal reaction: fade in quickly, fade out over its tail, then stop.
    if (m_react.getStatus() == sf::Music::Playing || m_reactVol > 0.001f) {
        if (m_react.getStatus() == sf::Music::Playing) {
            float rem = m_react.getDuration().asSeconds() - m_react.getPlayingOffset().asSeconds();
            if (rem < 0.7f) m_reactTarget = 0.f;
        }
        ramp(m_reactVol, m_reactTarget, m_ambOn ? 2.5f : 6.f); // snappier fade-out once the match ends
        m_react.setVolume(m_reactVol * sv);
        if (m_reactVol <= 0.001f && m_reactTarget <= 0.f && m_react.getStatus() == sf::Music::Playing)
            m_react.stop();
    }
}

float AudioManager::musicVolume() const {
    return (g_settings.musicVolume / 1.5f) * m_musicFade; // 1.5x quieter than the slider, times the fade
}

void AudioManager::setMusicEnabled(bool on) {
    m_musicEnabled = on;
    m_musicFadeTarget = on ? 1.f : 0.f; // the actual fade is driven in update(), so it's smooth
    if (on) {
        stopCrowd();                                                                   // ambience is match-only
        // Resume at once (from the paused spot) so the fade-in has something to ramp up.
        if (m_hasSource && m_music.getStatus() == sf::Music::Paused) m_music.play();
    }
    // On disable we DON'T pause here - update() fades the volume down first, then pauses.
}

void AudioManager::startCrowd() {
    auto it = m_buffers.find("crowd_loop");
    if (it == m_buffers.end()) return;
    m_crowd.setBuffer(it->second);
    m_crowd.setLoop(true);
    m_crowd.setVolume(static_cast<float>(g_settings.soundVolume) * 0.5f); // sit under the SFX
    if (m_crowd.getStatus() != sf::Sound::Playing) m_crowd.play();
}

void AudioManager::stopCrowd() {
    if (m_crowd.getStatus() != sf::Sound::Stopped) m_crowd.stop();
}

void AudioManager::stopMusic() {
    m_music.stop();
    m_musicPlaying = false;
    m_usingPlaylist = false;
}

void AudioManager::applyVolumes() {
    if (m_musicPlaying) m_music.setVolume(musicVolume());
    if (m_crowd.getStatus() == sf::Sound::Playing)
        m_crowd.setVolume(static_cast<float>(g_settings.soundVolume) * 0.5f);
    // Other live SFX are short; the next one already picks up the new volume.
}
