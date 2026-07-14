cmake_minimum_required(VERSION 3.10)
project(FootballSim VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Sanitizers: local debugging only. These are GCC/Clang flags - MSVC rejects them
# ("unknown option '-fsanitize=address,undefined'") and the CI Release build has no
# business carrying ASan anyway. Enable with: cmake -B build -DENABLE_SANITIZERS=ON
option(ENABLE_SANITIZERS "Build with address/UB sanitizers (GCC/Clang only)" OFF)
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-O0 -g -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# Find SFML (Graphics, Window, System, Audio)
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

# Include directories
include_directories(
    src
    src/core
    src/models
    src/engine
    src/screens
    src/ui
)

# Add source files here
set(SOURCES
    src/core/main.cpp
    src/core/Settings.cpp
    src/core/AssetManager.cpp
    src/core/GameManager.cpp
    src/core/SaveManager.cpp
    
    src/models/Player.cpp
    src/models/Database.cpp
    src/models/NameGenerator.cpp
    src/models/CareerManager.cpp
    src/models/Match.cpp
    
    src/engine/MatchEngine.cpp

    src/ui/TimingBar.cpp

    src/screens/MenuScreen.cpp
    src/screens/MatchStatsScreen.cpp
    src/screens/SeasonEndScreen.cpp
    src/screens/SettingsScreen.cpp
    src/screens/NewCareerScreen.cpp
    src/screens/CareerHubScreen.cpp
    src/screens/LeagueTableScreen.cpp
    src/screens/TrainingScreen.cpp
    src/screens/MyStatusScreen.cpp
    src/screens/EventScreen.cpp
    src/screens/InterviewScreen.cpp
    src/screens/SquadScreen.cpp
    src/screens/TransferScreen.cpp
    src/screens/EuropeanCupScreen.cpp
    src/screens/MatchScreen.cpp
    src/screens/MatchScreenEpisodes.cpp
    src/screens/UpgradeScreen.cpp
    src/screens/AwardsScreen.cpp
)

# Create executable
add_executable(football_sim ${SOURCES})

# Link SFML libraries
target_link_libraries(football_sim sfml-graphics sfml-window sfml-system sfml-audio)

# Copy assets folder to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
