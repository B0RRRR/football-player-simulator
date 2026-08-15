#include "GameManager.h"
#include "MenuScreen.h"
#include "Logger.h"
#include <memory>
#include <iostream>
#include <cstdlib>

int main()
{
#if defined(__linux__)
    // WSLg / PulseAudio crackles with a tiny buffer - streaming music underruns and pops even
    // though the files are fine. A larger latency buffer smooths it out. `0` = don't override a
    // value the user already set. (Slightly more audio delay; unnoticeable for background music.)
    setenv("PULSE_LATENCY_MSEC", "120", 0);
#endif

    Log::init("game.log");
    Log::installCrashHandlers();
    LOG_INFO("Football Player Simulator starting up");

    try {
        GameManager game;

        // Start with the main menu screen
        game.changeScreen(std::make_shared<MenuScreen>());

        // Run the game loop
        game.run();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Fatal exception: " << e.what());
        return 1;
    }
    catch (...) {
        LOG_ERROR("Fatal unknown exception");
        return 1;
    }

    LOG_INFO("Clean shutdown");
    return 0;
}
