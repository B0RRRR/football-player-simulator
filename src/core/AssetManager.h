#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <string>

class AssetManager {
public:
    // Singleton pattern
    static AssetManager& get();

    bool loadFont(const std::string& name, const std::string& filename);
    sf::Font& getFont(const std::string& name);

    bool loadSoundBuffer(const std::string& name, const std::string& filename);
    sf::SoundBuffer& getSoundBuffer(const std::string& name);

    // Lazy load texture
    sf::Texture& getTexture(const std::string& name, bool isFlag = false);

private:
    AssetManager() = default;
    ~AssetManager() = default;

    std::map<std::string, sf::Font> m_fonts;
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::map<std::string, sf::Texture> m_textures;
};
