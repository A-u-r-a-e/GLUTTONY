cmake_minimum_required(VERSION 3.28)
project(Gluttony LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(GLUTTONY_BUILD_APP "Build the SFML app (needs SFML's system dependencies)" ON)
option(GLUTTONY_BUILD_TESTS "Build the unit tests" ON)

include(FetchContent)

# --- Dependencies -----------------------------------------------------------

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(nlohmann_json)

# --- Input engine (no SFML: pure, deterministic, testable) ------------------

add_library(gluttony_input
    src/input/Signal.cpp
    src/input/Filter.cpp
    src/input/InputState.cpp
    src/input/Conditions.cpp
    src/input/Channel.cpp
    src/input/Mute.cpp
    src/input/Processors.cpp
    src/input/InputEngine.cpp
    src/input/Config.cpp)
target_include_directories(gluttony_input PUBLIC src)
target_link_libraries(gluttony_input PUBLIC nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(gluttony_input PRIVATE /W4)
else()
    target_compile_options(gluttony_input PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- App (SFML) -------------------------------------------------------------

if(GLUTTONY_BUILD_APP)
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    # Bundle FreeType/HarfBuzz instead of needing them installed (SFML defaults to system libs on Linux).
    set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.1.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(SFML)

    add_executable(gluttony
        src/app/main.cpp
        src/app/SfmlInput.cpp
        src/app/DebugView.cpp)
    target_link_libraries(gluttony PRIVATE gluttony_input SFML::Graphics)
    # Lets the app find data/ when run straight from the build directory.
    target_compile_definitions(gluttony PRIVATE GLUTTONY_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
endif()

# --- Tests ------------------------------------------------------------------

if(GLUTTONY_BUILD_TESTS)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.5.3
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(doctest)

    enable_testing()
    add_executable(gluttony_tests
        tests/main.cpp
        tests/FilterTests.cpp
        tests/ConditionTests.cpp
        tests/ProcessorTests.cpp
        tests/EngineTests.cpp
        tests/ConfigTests.cpp)
    target_link_libraries(gluttony_tests PRIVATE gluttony_input doctest::doctest)
    target_compile_definitions(gluttony_tests PRIVATE GLUTTONY_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
    add_test(NAME gluttony_tests COMMAND gluttony_tests)
endif()
