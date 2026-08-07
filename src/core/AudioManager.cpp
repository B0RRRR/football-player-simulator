#include "AudioManager.h"
#include "Settings.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>

AudioManager& AudioManager::get() {
    static AudioManager instance;
    return instance;
}

void AudioManager::loadAll() {
    m_voices.clear();
    m_voices.resize(16); // enough overlapping one-shots for anything the game throws at once

    const char* names[] = {"ui_click", "ui_hover", "ui_confirm", "ui_deny",
                           "whistle", "kick", "goal"};
    for (const char* n : names) {
        sf::SoundBuffer buf;
        if (buf.loadFromFile(std::string("assets/sounds/") + n + ".wav"))
            m_buffers[n] = buf;
        else
            std::cerr << "Audio: missing assets/sounds/" << n << ".wav\n";
    }
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
    // Try common formats in order. SFML 2.6+ supports mp3; ogg is still the nicest for streaming.
    const std::string base = std::string("assets/sounds/") + name;
    if (!m_music.openFromFile(base + ".ogg") &&
        !m_music.openFromFile(base + ".mp3") &&
        !m_music.openFromFile(base + ".wav") &&
        !m_music.openFromFile(base + ".flac")) {
        m_musicPlaying = false;
        return;
    }
    m_music.setLoop(loop);
    m_music.setVolume(static_cast<float>(g_settings.musicVolume));
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
    m_music.setVolume(static_cast<float>(g_settings.musicVolume));
    m_music.play();
    m_musicPlaying = true;
    m_usingPlaylist = true;
    m_hasSource = true;
    return true;
}

void AudioManager::update() {
    // When a track ends (and music is enabled), pick the next one at random (no near repeats).
    if (m_usingPlaylist && m_musicEnabled && !m_playlist.empty()
        && m_music.getStatus() == sf::Music::Stopped) {
        m_playlistIdx = pickNextTrack();
        if (m_music.openFromFile(m_playlist[m_playlistIdx])) {
            m_music.setLoop(false);
            m_music.setVolume(static_cast<float>(g_settings.musicVolume));
            m_music.play();
        }
    }
}

void AudioManager::setMusicEnabled(bool on) {
    m_musicEnabled = on;
    if (on) {
        if (m_hasSource && m_music.getStatus() == sf::Music::Paused) m_music.play(); // resume
    } else {
        if (m_music.getStatus() == sf::Music::Playing) m_music.pause();               // silence for the match
    }
}

void AudioManager::stopMusic() {
    m_music.stop();
    m_musicPlaying = false;
    m_usingPlaylist = false;
}

void AudioManager::applyVolumes() {
    if (m_musicPlaying) m_music.setVolume(static_cast<float>(g_settings.musicVolume));
    // Live SFX are short; the next one already picks up the new volume.
}
