# CMake build for the isometric editor demo.
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# SFML 3 is located with
# find_package(SFML 3 COMPONENTS Graphics Window System).
# Point CMake at your SDK with -DCMAKE_PREFIX_PATH=/path/to/SFML-3.0.0.
# Set -DISO_BUILD_GAME=OFF to build only the (SFML-free) geometry tests.
cmake_minimum_required(VERSION 3.20)
project(IsometricSFMLgame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(ISO_BUILD_GAME "Build the SFML game executable" ON)

# ---------------------------------------------------------------------------
# Game executable
# ---------------------------------------------------------------------------
if(ISO_BUILD_GAME)
  # SFML's config compares component names case-sensitively and exports
  # SFML::Graphics / SFML::Window / SFML::System.
  find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

  set(ISO_SOURCES
    IsometricSFMLgame/src/main.cpp
    IsometricSFMLgame/src/core/asset_path.cpp
    IsometricSFMLgame/src/core/game.cpp
    IsometricSFMLgame/src/core/input_handler.cpp
    IsometricSFMLgame/src/core/texture_manager.cpp
    IsometricSFMLgame/src/map/map.cpp
    IsometricSFMLgame/src/map/tile.cpp
    IsometricSFMLgame/src/map/tile_type.cpp
    IsometricSFMLgame/src/states/game_state_editor.cpp
    IsometricSFMLgame/src/states/game_state_start.cpp
    IsometricSFMLgame/src/ui/button.cpp
    IsometricSFMLgame/src/animation/animation_handler.cpp
  )

  add_executable(IsometricGame ${ISO_SOURCES})
  target_include_directories(IsometricGame PRIVATE IsometricSFMLgame/src)
  target_link_libraries(IsometricGame PRIVATE SFML::Graphics SFML::Window SFML::System)

  if(MSVC)
    target_compile_options(IsometricGame PRIVATE /W3 /permissive-)
  else()
    target_compile_options(IsometricGame PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
  endif()

  # The game resolves art through core/asset_path.cpp, which also searches next
  # to and above the executable, so these copies are only a convenience.
  add_custom_command(TARGET IsometricGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/assets"
      "$<TARGET_FILE_DIR:IsometricGame>/assets"
    COMMENT "Copying assets next to the executable")

  if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    add_custom_command(TARGET IsometricGame POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:IsometricGame> $<TARGET_FILE_DIR:IsometricGame>
      COMMAND_EXPAND_LISTS
      COMMENT "Copying SFML DLLs next to the executable")
  endif()
endif()

# ---------------------------------------------------------------------------
# Tests (no SFML, no graphics context required)
# ---------------------------------------------------------------------------
enable_testing()

add_executable(grid_geometry_test tests/grid_geometry_test.cpp)
target_include_directories(grid_geometry_test PRIVATE IsometricSFMLgame/src)
if(MSVC)
  target_compile_options(grid_geometry_test PRIVATE /W3 /permissive-)
else()
  target_compile_options(grid_geometry_test PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()
add_test(NAME grid_geometry COMMAND grid_geometry_test)
