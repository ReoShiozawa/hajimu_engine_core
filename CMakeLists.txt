cmake_minimum_required(VERSION 3.25)
project(engine_core VERSION 1.0.0 LANGUAGES CXX)

# ── C++20 必須 ──────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── プラットフォーム検出 ─────────────────────────────────
if(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
endif()

# ── ソースファイル ───────────────────────────────────────
set(ENGINE_SOURCES
    # Core
    src/core/memory.cpp
    src/core/log.cpp
    src/core/reflection.cpp
    src/core/task_graph.cpp
    # ECS
    src/ecs/archetype.cpp
    src/ecs/world.cpp
    src/ecs/system.cpp
    src/ecs/command_buffer.cpp
    # Input
    src/input/input_system.cpp
    # Scene
    src/scene/scene_graph.cpp
    src/scene/transform.cpp
    # Resource
    src/resource/vfs.cpp
    src/resource/resource_manager.cpp
    # Render
    src/render/render_graph.cpp
    src/render/render_backend.cpp
    # Physics
    src/physics/physics_world.cpp
    # Audio
    src/audio/audio_system.cpp
    # Network
    src/network/net_system.cpp
    # Script
    src/script/script_runtime.cpp
    # Build
    src/build/build_pipeline.cpp
    # Plugin Entry
    src/plugin.cpp
)

# ── はじむインクルードパス ────────────────────────────────
set(HAJIMU_INCLUDE_DIR "" CACHE PATH "はじむ include path")
if(NOT HAJIMU_INCLUDE_DIR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../jp/include")
        set(HAJIMU_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../jp/include")
    elseif(EXISTS "/usr/local/include/hajimu")
        set(HAJIMU_INCLUDE_DIR "/usr/local/include/hajimu")
    else()
        set(HAJIMU_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
    endif()
endif()

# ── 共有ライブラリ (.hjp) ────────────────────────────────
add_library(engine_core SHARED ${ENGINE_SOURCES})

target_include_directories(engine_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${HAJIMU_INCLUDE_DIR}
)

target_compile_options(engine_core PRIVATE
    -Wall -Wextra -O2
    $<$<PLATFORM_ID:Darwin>:-fPIC>
    $<$<PLATFORM_ID:Linux>:-fPIC>
)

# ── プラットフォーム固有リンク ────────────────────────────
if(APPLE)
    target_link_libraries(engine_core PRIVATE pthread)
elseif(UNIX)
    target_link_libraries(engine_core PRIVATE pthread)
elseif(WIN32)
    # Win32 — no extra libs needed
endif()

# ── 出力名 = engine_core.hjp ────────────────────────────
set_target_properties(engine_core PROPERTIES
    PREFIX ""
    SUFFIX ".hjp"
    OUTPUT_NAME "engine_core"
)

# ── テスト ───────────────────────────────────────────────
option(BUILD_TESTS "テストをビルドする" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_ecs tests/test_ecs.cpp)
    target_include_directories(test_ecs PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${HAJIMU_INCLUDE_DIR}
    )
    target_link_libraries(test_ecs PRIVATE engine_core)
    add_test(NAME test_ecs COMMAND test_ecs)
endif()

# ── インストール ─────────────────────────────────────────
install(TARGETS engine_core
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/engine DESTINATION include)
