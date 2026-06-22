#include "AssetManager.h"
#include <iostream>

AssetManager& AssetManager::get() {
    static AssetManager instance;
    return instance;
}

bool AssetManager::loadFont(const std::string& name, const std::string& filename) {
    sf::Font font;
    if (font.loadFromFile(filename)) {
        m_fonts[name] = font;
        return true;
    }
    std::cerr << "Error: Failed to load font: " << filename << std::endl;
    return false;
}

sf::Font& AssetManager::getFont(const std::string& name) {
    return m_fonts.at(name); // Throws out_of_range if not found
}

bool AssetManager::loadSoundBuffer(const std::string& name, const std::string& filename) {
    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(filename)) {
        m_soundBuffers[name] = buffer;
        return true;
    }
    std::cerr << "Error: Failed to load sound: " << filename << std::endl;
    return false;
}

sf::SoundBuffer& AssetManager::getSoundBuffer(const std::string& name) {
    return m_soundBuffers.at(name);
}
