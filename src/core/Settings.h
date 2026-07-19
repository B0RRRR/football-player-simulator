#pragma once

struct Settings {
    int musicVolume = 50;
    int soundVolume = 50;
    int difficulty = 1; // 0: Easy, 1: Normal, 2: Hard
    bool isFullscreen = false;
    unsigned int resWidth = 1280;
    unsigned int resHeight = 720;
    int matchSpeed = 1; // index into the speed table below: 0=0.5x .. 4=2.5x (default 1x)
};

// Global settings instance
extern Settings g_settings;

// Match speed presets. matchSpeed indexes these; 1x is the baseline the sim/animation
// are tuned around. Both MatchScreen and SettingsScreen read from here so labels and
// behaviour never drift apart.
inline float matchSpeedMult(int idx) {
    static const float m[5] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f };
    if (idx < 0) idx = 0;
    if (idx > 4) idx = 4;
    return m[idx];
}
inline const char* matchSpeedLabel(int idx) {
    static const char* l[5] = { "0.5x", "1x", "1.5x", "2x", "2.5x" };
    if (idx < 0) idx = 0;
    if (idx > 4) idx = 4;
    return l[idx];
}
inline int matchSpeedCount() { return 5; }
