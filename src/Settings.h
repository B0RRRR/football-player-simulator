#pragma once

struct Settings {
    int musicVolume = 50;
    int soundVolume = 50;
    int difficulty = 1; // 0: Easy, 1: Normal, 2: Hard
    bool isFullscreen = false;
    unsigned int resWidth = 1280;
    unsigned int resHeight = 720;
    int matchSpeed = 1; // 0: Slow, 1: Normal, 2: Fast, 3: Instant
};

// Global settings instance
extern Settings g_settings;
