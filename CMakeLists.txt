cmake_minimum_required(VERSION 3.16)
project(Tatoosh VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build RmlUI from submodule as static libraries
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/rmlui/CMakeLists.txt")
    message(FATAL_ERROR "RmlUI submodule not initialized. Run 'make setup' or 'git submodule update --init --recursive'")
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
set(RMLUI_THIRDPARTY_CONTAINERS ON CACHE BOOL "" FORCE)
add_subdirectory(external/rmlui)
