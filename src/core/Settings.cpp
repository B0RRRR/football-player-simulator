#include "Settings.h"
#include <fstream>
#include <string>
#include <algorithm>

// Definition of the global settings variable
Settings g_settings;

float g_renderScale = 1.f;

static const char* kSettingsFile = "settings.cfg";

void Settings::save() const {
    std::ofstream f(kSettingsFile, std::ios::trunc);
    if (!f) return;
    f << "musicVolume=" << musicVolume << "\n"
      << "soundVolume=" << soundVolume << "\n"
      << "difficulty=" << difficulty << "\n"
      << "fullscreen=" << (isFullscreen ? 1 : 0) << "\n"
      << "resWidth=" << resWidth << "\n"
      << "resHeight=" << resHeight << "\n"
      << "matchSpeed=" << matchSpeed << "\n";
}

void Settings::load() {
    std::ifstream f(kSettingsFile);
    if (!f) return; // first run - keep defaults

    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq), val = line.substr(eq + 1);
        int iv = 0;
        try { iv = std::stoi(val); } catch (...) { continue; }

        if (key == "musicVolume")      musicVolume = std::clamp(iv, 0, 100);
        else if (key == "soundVolume") soundVolume = std::clamp(iv, 0, 100);
        else if (key == "difficulty")  difficulty  = std::clamp(iv, 0, 2);
        else if (key == "fullscreen")  isFullscreen = (iv != 0);
        else if (key == "resWidth")    resWidth  = (unsigned)std::max(640, iv);
        else if (key == "resHeight")   resHeight = (unsigned)std::max(480, iv);
        else if (key == "matchSpeed")  matchSpeed = std::clamp(iv, 0, 4);
    }
}
