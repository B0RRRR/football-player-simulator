#include "GameManager.h"
#include "MenuScreen.h"
#include <memory>
#include <iostream>

int main()
{
    try {
        GameManager game;

        // Start with the main menu screen
        game.changeScreen(std::make_shared<MenuScreen>());

        // Run the game loop
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
