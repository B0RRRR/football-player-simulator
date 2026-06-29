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

sf::Texture& AssetManager::getTexture(const std::string& name, bool isFlag) {
    if (m_textures.find(name) == m_textures.end()) {
        sf::Texture tex;
        std::string path = isFlag ? "assets/images/flags/" + name + ".png" : "assets/images/logos/" + name + ".png";
        
        // Try to load, if fails create a dummy empty texture
        if (!tex.loadFromFile(path)) {
            // Optional: fallback to a generic placeholder if image doesn't exist
            // For now, we just create a transparent 1x1 texture
            sf::Image dummy;
            dummy.create(1, 1, sf::Color::Transparent);
            tex.loadFromImage(dummy);
        }
        
        // Smooth scaling for 180x180 logos
        tex.setSmooth(true);
        m_textures[name] = tex;
    }
    return m_textures.at(name);
}
