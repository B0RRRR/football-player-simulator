#pragma once
#include <string>

class Player;
class CareerManager;
class Database;

class SaveManager {
public:
    static bool saveGame(const std::string& filepath, Player* p, CareerManager* cm, Database* db);
    static bool loadGame(const std::string& filepath, Player* p, CareerManager* cm, Database* db);
    static bool hasSaveGame(const std::string& filepath);
};
