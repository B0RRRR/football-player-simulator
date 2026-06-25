cmake_minimum_required(VERSION 3.10)
project(FootballSim VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SFML (Graphics, Window, System, Audio)
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

# Include directories
include_directories(src)

# Add source files here
set(SOURCES
    src/main.cpp
    src/Settings.cpp
    src/AssetManager.cpp
    src/GameManager.cpp
    src/MenuScreen.cpp
    src/MatchEngine.cpp
    src/MatchStatsScreen.cpp
    src/SeasonEndScreen.cpp
    src/SettingsScreen.cpp
    src/NewCareerScreen.cpp
    src/CareerHubScreen.cpp
    src/LeagueTableScreen.cpp
    src/TrainingScreen.cpp
    src/EventScreen.cpp
    src/TransferScreen.cpp
    src/CareerManager.cpp
    src/Player.cpp
    src/Match.cpp
    src/MatchScreen.cpp
    src/UpgradeScreen.cpp
    src/Database.cpp
)

# Create executable
add_executable(football_sim ${SOURCES})

# Link SFML libraries
target_link_libraries(football_sim sfml-graphics sfml-window sfml-system sfml-audio)

# Copy assets folder to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
