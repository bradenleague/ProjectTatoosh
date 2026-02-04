# Project Tatoosh
# Top-level Meson build that wraps vkQuake and adds RmlUI integration

project('tatoosh', ['c', 'cpp'],
    version : '0.1.0',
    default_options : [
        'c_std=gnu11',
        'cpp_std=c++17',
        'buildtype=release'
    ]
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

# Options
tatoosh_hot_reload = get_option('hot_reload')

# Dependencies
sdl_dep = dependency('sdl2')
threads_dep = dependency('threads')
m_dep = cc.find_library('m', required : false)
dl_dep = cc.find_library('dl', required : false)

if build_machine.system() == 'darwin'
    molten_dirs = []
    if import('fs').exists('/opt/homebrew/lib/libMoltenVK.dylib')
        molten_dirs += '/opt/homebrew/lib'
    endif
    vulkan_dep = cc.find_library('MoltenVK', required : true, dirs : molten_dirs)
else
    vulkan_dep = dependency('vulkan')
endif

# Build RmlUI as a subproject using CMake
# Note: RmlUI needs to be built separately with CMake first, then linked here
# cmake -B build_rmlui -S external/rmlui -DBUILD_SHARED_LIBS=OFF
# cmake --build build_rmlui
rmlui_inc = include_directories('external/rmlui/Include')

# Tatoosh UI integration layer
# Organized into layered architecture:
#   - interface/: C API facade for vkQuake integration
#   - application/: Document management, menu navigation
#   - domain/: Pure types (GameState, InputMode, CvarSchema)
#   - infrastructure/: RmlUI adapters, Vulkan renderer
tatoosh_ui_sources = [
    # Interface layer (C API)
    'rmlui/interface/ui_manager.cpp',
    # Application layer
    'rmlui/application/document_manager.cpp',
    'rmlui/application/menu_stack.cpp',
    # Infrastructure layer
    'rmlui/infrastructure/render_interface_vk.cpp',
    'rmlui/infrastructure/system_interface.cpp',
    'rmlui/infrastructure/game_data_model.cpp',
    'rmlui/infrastructure/cvar_binding.cpp',
    'rmlui/infrastructure/menu_event_handler.cpp',
    'rmlui/infrastructure/stb_image_impl.c',
    # Infrastructure layer - port implementations
    'rmlui/infrastructure/quake_cvar_provider.cpp',
    'rmlui/infrastructure/quake_command_executor.cpp',
    'rmlui/infrastructure/quake_logger.cpp',
]

tatoosh_ui_inc = include_directories(
    'rmlui',
    'rmlui/interface',
    'rmlui/application',
    'rmlui/domain',
    'rmlui/domain/ports',
    'rmlui/infrastructure',
    'engine/Quake',
)

tatoosh_defines = []
if tatoosh_hot_reload
    tatoosh_defines += '-DTATOOSH_HOT_RELOAD=1'
endif

tatoosh_ui_lib = static_library('tatoosh_ui',
    tatoosh_ui_sources,
    include_directories : [tatoosh_ui_inc, rmlui_inc],
    dependencies : [sdl_dep, vulkan_dep],
    cpp_args : tatoosh_defines,
)

# To integrate with vkQuake:
# 1. Build vkQuake from engine/ directory
# 2. Link tatoosh_ui_lib into the final executable
#
# For now, we declare the dependency for downstream use
tatoosh_ui_dep = declare_dependency(
    link_with : tatoosh_ui_lib,
    include_directories : tatoosh_ui_inc,
)

message('Tatoosh configuration:')
message('  Version: ' + meson.project_version())
message('  Hot reload: ' + tatoosh_hot_reload.to_string())
message('  Renderer: Vulkan')
