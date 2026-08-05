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
    src/core/PlatformDisplay.cpp
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
    src/screens/DrillArena.cpp
    src/screens/UpgradeScreen.cpp
    src/screens/AwardsScreen.cpp
)

# Create executable
add_executable(football_sim ${SOURCES})

# Link SFML libraries
target_link_libraries(football_sim sfml-graphics sfml-window sfml-system sfml-audio)

# Linux: query exact per-monitor bounds via XRandR so borderless fullscreen lands on a single
# monitor even when monitors differ in size (SFML 2.x has no multi-monitor API of its own).
# We declare the RandR API ourselves (see GameManager.cpp), so only the runtime lib is needed -
# fall back to the versioned soname when the -dev symlink (libXrandr.so) isn't installed.
if(UNIX AND NOT APPLE)
    find_library(X11_LIB NAMES X11)
    find_library(XRANDR_LIB NAMES Xrandr)
    # If the -dev symlink (libXrandr.so) isn't installed, fall back to the versioned soname.
    if(NOT XRANDR_LIB)
        foreach(_d /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/lib /lib)
            if(EXISTS "${_d}/libXrandr.so.2")
                set(XRANDR_LIB "${_d}/libXrandr.so.2")
                break()
            endif()
        endforeach()
    endif()
    target_link_libraries(football_sim ${X11_LIB} ${XRANDR_LIB})
endif()

# Copy assets folder to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Headless engine tests. Off by default (needs no SFML, but the main build shouldn't grow a
# second binary unasked). Enable with: cmake -B build -DBUILD_TESTS=ON, then ctest or run
# ./build/engine_tests directly.
option(BUILD_TESTS "Build the headless engine tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_executable(engine_tests
        tests/engine_tests.cpp
        src/engine/MatchEngine.cpp
        src/core/Settings.cpp
        src/models/Player.cpp
        src/models/Database.cpp
        src/models/NameGenerator.cpp
    )
    add_test(NAME engine_tests COMMAND engine_tests)
endif()
