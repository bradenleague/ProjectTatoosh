cmake_minimum_required(VERSION 3.16)
project(Tatoosh VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(TATOOSH_HOT_RELOAD "Enable UI hot reload support" ON)
option(TATOOSH_BUILD_RMLUI "Build RmlUI from source" ON)

# Find required packages
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)

# RmlUI configuration - build with Vulkan support
if(TATOOSH_BUILD_RMLUI)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build RmlUI as static library")
    set(RMLUI_SAMPLES OFF CACHE BOOL "Disable RmlUI samples")
    set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "Disable Lua bindings")
    set(RMLUI_THIRDPARTY_CONTAINERS ON CACHE BOOL "Use RmlUI's robin_hood containers")
    add_subdirectory(external/rmlui)
    set(RMLUI_LIB rmlui)
else()
    # Use pre-installed RmlUI
    find_package(RmlUi REQUIRED)
    set(RMLUI_LIB RmlUi::RmlUi)
endif()

# Tatoosh UI integration layer
# Organized into layered architecture:
#   - interface/: C API facade for vkQuake integration
#   - application/: Document management, menu navigation
#   - domain/: Pure types (GameState, InputMode, CvarSchema)
#   - infrastructure/: RmlUI adapters, Vulkan renderer
add_library(tatoosh_ui STATIC
    # Interface layer (C API)
    rmlui/interface/ui_manager.cpp
    # Application layer
    rmlui/application/document_manager.cpp
    rmlui/application/menu_stack.cpp
    # Infrastructure layer
    rmlui/infrastructure/render_interface_vk.cpp
    rmlui/infrastructure/system_interface.cpp
    rmlui/infrastructure/game_data_model.cpp
    rmlui/infrastructure/cvar_binding.cpp
    rmlui/infrastructure/menu_event_handler.cpp
    rmlui/infrastructure/stb_image_impl.c
    # Infrastructure layer - port implementations
    rmlui/infrastructure/quake_cvar_provider.cpp
    rmlui/infrastructure/quake_command_executor.cpp
    rmlui/infrastructure/quake_logger.cpp
)

target_include_directories(tatoosh_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/rmlui
    ${CMAKE_CURRENT_SOURCE_DIR}/rmlui/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/rmlui/application
    ${CMAKE_CURRENT_SOURCE_DIR}/rmlui/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/rmlui/domain/ports
    ${CMAKE_CURRENT_SOURCE_DIR}/rmlui/infrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/Quake
    ${CMAKE_CURRENT_SOURCE_DIR}/external/rmlui/Include
)

target_include_directories(tatoosh_ui PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(tatoosh_ui PUBLIC
    ${RMLUI_LIB}
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${FREETYPE_LIBRARIES}
)

target_compile_definitions(tatoosh_ui PRIVATE
    $<$<BOOL:${TATOOSH_HOT_RELOAD}>:TATOOSH_HOT_RELOAD=1>
)

# Copy UI assets to build directory
add_custom_target(copy_ui_assets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ui/rml
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ui/rcss
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ui/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ui/rml ${CMAKE_BINARY_DIR}/ui/rml
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ui/rcss ${CMAKE_BINARY_DIR}/ui/rcss
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ui/fonts ${CMAKE_BINARY_DIR}/ui/fonts
    COMMENT "Copying UI assets to build directory"
)

add_dependencies(tatoosh_ui copy_ui_assets)

# vkQuake Integration Notes:
# -------------------------
# vkQuake uses Meson as its build system. To integrate tatoosh_ui:
#
# Option A: Build tatoosh_ui with CMake, then link into vkQuake's Meson build
#   1. cmake -B build && cmake --build build
#   2. Copy libtatoosh_ui.a to engine/
#   3. Modify engine/meson.build to link libtatoosh_ui.a
#
# Option B: Use the provided meson.build wrapper
#   1. meson setup build
#   2. meson compile -C build
#
# Hook points in vkQuake:
#   - engine/Quake/host.c: Host_Init() / Host_Shutdown()
#   - engine/Quake/gl_screen.c: SCR_UpdateScreen()
#   - engine/Quake/in_sdl.c or in_sdl2.c: Input events
#   - SCBX_GUI command buffer context for UI rendering

message(STATUS "Tatoosh configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Hot reload: ${TATOOSH_HOT_RELOAD}")
message(STATUS "  Renderer: Vulkan")
