# RmlUI Integration Guide

This document describes how RmlUI is integrated with vkQuake in Tatoosh, including the input handling system that coordinates between RmlUI menus and Quake's native systems.

## Overview

RmlUI provides HTML/CSS-based UI rendering within vkQuake's Vulkan pipeline. The integration consists of:

- **Render Interface** (`rmlui/internal/render_interface_vk.cpp`) - Custom Vulkan renderer using vkQuake's context
- **System Interface** (`rmlui/internal/system_interface.cpp`) - Time and logging integration
- **UI Manager** (`rmlui/ui_manager.cpp`) - Document management, input handling, state control

The public C API is defined in `rmlui/ui_manager.h` with a `UI_` prefix. All engine-side calls are gated with `#ifdef USE_RMLUI`.

## Input Handling System

### The Problem

When RmlUI menus are visible, they need to capture input (keyboard, mouse) to function. However, users must always be able to press Escape to close menus and return to the game. Without proper coordination:
- RmlUI would consume ALL input when visible
- Users would get stuck in menus
- No way to bring up the console or return to game

### Solution: Input Modes

RmlUI uses three input modes that integrate with vkQuake's `key_dest` system:

| Mode | Description | Input Behavior |
|------|-------------|----------------|
| `UI_INPUT_INACTIVE` | Default state | RmlUI doesn't handle input, game works normally |
| `UI_INPUT_MENU_ACTIVE` | Menu is open | RmlUI captures all input except Escape |
| `UI_INPUT_OVERLAY` | HUD elements | RmlUI visible but passes input through to game |

### Menu Stack

When menus are opened, they're pushed onto a stack. Pressing Escape:
1. Pops the current menu from the stack
2. Hides that menu document
3. If the stack is empty, returns to `UI_INPUT_INACTIVE` mode
4. If menus remain, shows the previous menu

This allows nested menus (e.g., Main Menu → Options → Video Settings) where Escape navigates back through the hierarchy.

## Console Commands

### Basic Control

| Command | Description |
|---------|-------------|
| `ui_menu [path]` | Open an RmlUI menu (default: `ui/rml/menus/main_menu.rml`) |
| `ui_closemenu` | Close all RmlUI menus and return to game |
| `ui_toggle` | Toggle main menu open/close |
| `ui_show` | Alias for `ui_menu` |
| `ui_hide` | Alias for `ui_closemenu` |
| `ui_debugger` | Toggle RmlUI visual debugger |
| `ui_debuger` | Alias for `ui_debugger` |

### Configuration

| Cvar | Default | Description |
|------|---------|-------------|
| `ui_use_rmlui_menus` | 0 | Use RmlUI for Quake menus (main/options/pause) |
| `ui_use_rmlui_hud` | 0 | Use RmlUI HUD (in-game overlay) |
| `ui_use_rmlui` | 0 | Convenience master switch (sets both HUD + menus) |

## Input Flow

```
SDL Event
    │
    ▼
in_sdl2.c: IN_SendKeyEvents()
    │
    ├──[ESCAPE]──► keys.c: Key_EventWithKeycode()
    │                   │
    │                   ├──[UI_WantsMenuInput()?]
    │                   │       │
    │                   │       YES ──► UI_HandleEscape()
    │                   │       │           │
    │                   │       │           └──[menus left?]
    │                   │       │                 NO ──► key_dest=key_game, IN_Activate()
    │                   │       │
    │                   │       NO ──► [ui_use_rmlui_menus?]
    │                   │                   │
    │                   │                   YES ──► UI_PushMenu(), key_dest=key_menu
    │                   │                   │
    │                   │                   NO ──► M_ToggleMenu_f() (Quake menu)
    │
    └──[Other keys]
            │
            └──[UI_WantsMenuInput()?]
                    │
                    YES ──► UI_KeyEvent() [consume]
                    │
                    NO ──► Quake key handling
```

## API Reference

### C API (rmlui/ui_manager.h)

```c
/* Input mode enum (from types/input_mode.h) */
typedef enum {
    UI_INPUT_INACTIVE,      /* Not handling input */
    UI_INPUT_MENU_ACTIVE,   /* Menu captures all input */
    UI_INPUT_OVERLAY        /* HUD mode, pass-through */
} ui_input_mode_t;

/* Lifecycle */
int UI_Init(int width, int height, const char *base_path);
void UI_Shutdown(void);
void UI_ProcessPending(void);
void UI_Update(double dt);
void UI_Render(void);
void UI_Resize(int width, int height);

/* Input mode control */
void UI_SetInputMode(ui_input_mode_t mode);
ui_input_mode_t UI_GetInputMode(void);
int UI_WantsMenuInput(void);      /* Returns 1 if MENU_ACTIVE */
void UI_HandleEscape(void);       /* Close current menu or deactivate */
void UI_PushMenu(const char* path);   /* Open menu, set MENU_ACTIVE */
void UI_PopMenu(void);            /* Pop current menu from stack */
void UI_CloseAllMenus(void);      /* Close all menus (deferred) */

/* Visibility (independent of input mode) */
void UI_SetVisible(int visible);
int UI_IsVisible(void);
void UI_Toggle(void);
int UI_IsMenuVisible(void);

/* Document management */
int UI_LoadDocument(const char *path);
void UI_UnloadDocument(const char *path);
void UI_ShowDocument(const char *path, int modal);
void UI_HideDocument(const char *path);

/* Input events (returns 1 if consumed) */
int UI_KeyEvent(int key, int scancode, int pressed, int repeat);
int UI_CharEvent(unsigned int codepoint);
int UI_MouseMove(int x, int y, int dx, int dy);
int UI_MouseButton(int button, int pressed);
int UI_MouseScroll(float x, float y);

/* HUD control */
void UI_ShowHUD(const char* hud_document);
void UI_HideHUD(void);
int UI_IsHUDVisible(void);

/* Scoreboard and intermission */
void UI_ShowScoreboard(void);
void UI_HideScoreboard(void);
void UI_ShowIntermission(void);
void UI_HideIntermission(void);

/* Game state sync - call each frame from sbar.c */
void UI_SyncGameState(const int* stats, int items,
                      int intermission, int gametype,
                      int maxclients,
                      const char* level_name, const char* map_name,
                      double game_time);

/* Key capture (for rebinding UI) */
int UI_IsCapturingKey(void);
void UI_OnKeyCaptured(int key, const char* key_name);

/* Vulkan integration */
void UI_InitializeVulkan(const void* config);
void UI_BeginFrame(void* cmd, int width, int height);
void UI_EndFrame(void);
void UI_CollectGarbage(void);

/* Debug and hot reload */
void UI_ToggleDebugger(void);
void UI_ReloadDocuments(void);
```

## Integration Points

### Initialization (host.c)

```c
// In Host_Init(), before VID_Init():
#ifdef USE_RMLUI
    UI_Init(1280, 720, com_basedir);
    Cmd_AddCommand("ui_menu", UI_Menu_f);
    Cmd_AddCommand("ui_toggle", UI_Toggle_f);
    // ... other commands
#endif

// After VID_Init():
#ifdef USE_RMLUI
    UI_Resize(vid.width, vid.height);
#endif
```

### Vulkan Setup (gl_vidsdl.c)

```c
// After Vulkan device/renderpass creation:
#ifdef USE_RMLUI
    ui_vulkan_config_t rmlui_config = { ... };
    UI_InitializeVulkan(&rmlui_config);
#endif

// After GPU fence wait:
#ifdef USE_RMLUI
    UI_CollectGarbage();
#endif
```

### Frame Rendering (gl_screen.c)

```c
#ifdef USE_RMLUI
    UI_ProcessPending();   // Deferred operations (menu close, etc.)
    UI_BeginFrame(cbx->cb, vid.width, vid.height);
    UI_Update(host_frametime);
    UI_Render();
    UI_EndFrame();
#endif
```

### Input Handling (in_sdl2.c)

```c
// Key events - check input mode, exclude escape
if (UI_WantsMenuInput() && key != SDLK_ESCAPE) {
    if (UI_KeyEvent(...))
        break;  // Consumed
}

// Mouse events - RmlUI consumes all when menu active
if (UI_WantsMenuInput()) {
    UI_MouseButton(...);
    break;
}

// Mouse motion - always update cursor, block game in menu mode
UI_MouseMove(...);
if (UI_WantsMenuInput())
    break;
```

### Escape Key (keys.c)

```c
if (key == K_ESCAPE) {
    // RmlUI handles escape first if it has active menu
    if (UI_WantsMenuInput()) {
        UI_HandleEscape();
        if (!UI_WantsMenuInput()) {
            IN_Activate();
            key_dest = key_game;
        }
        return;
    }

    // Otherwise, check if RmlUI menus should be used
    if (ui_use_rmlui_menus.value) {
        IN_Deactivate(modestate == MS_WINDOWED);
        key_dest = key_menu;
        UI_PushMenu("ui/rml/menus/main_menu.rml");
    } else {
        M_ToggleMenu_f();  // Quake menu
    }
}
```

### HUD and Game State (sbar.c)

```c
#ifdef USE_RMLUI
    if (ui_use_rmlui_hud.value) {
        UI_ShowHUD(NULL);  // Default: hud_classic.rml
        UI_SyncGameState(cl.stats, cl.items, cl.intermission,
                         cl.gametype, cl.maxclients,
                         cl.levelname, cl.mapname, cl.time);
    } else {
        UI_HideHUD();
    }
#endif
```

### Disconnect Cleanup (cl_main.c)

```c
#ifdef USE_RMLUI
    UI_HideHUD();
    UI_HideScoreboard();
    UI_HideIntermission();
#endif
```

## Event Handling Architecture

### Inline Event Handlers

RmlUI uses inline event handlers in RML similar to HTML. Use the `onclick` attribute:

```html
<div class="menu-item" onclick="new_game()">New Game</div>
<div class="menu-item" onclick="navigate('options')">Options</div>
<div class="menu-item" onclick="command('quit')">Quit</div>
```

**Important**: Use `onclick`, NOT `data-action`. The `onclick` attribute triggers RmlUI's event listener instancer system.

### Supported Actions

| Action | Description | Example |
|--------|-------------|---------|
| `navigate('menu')` | Push menu onto stack | `onclick="navigate('options')"` |
| `command('cmd')` | Execute console command | `onclick="command('map e1m1')"` |
| `close()` | Pop current menu | `onclick="close()"` |
| `close_all()` | Close all menus, return to game | `onclick="close_all()"` |
| `quit()` | Quit the game | `onclick="quit()"` |
| `new_game()` | Start new game | `onclick="new_game()"` |
| `load_game('slot')` | Load saved game | `onclick="load_game('s0')"` |
| `save_game('slot')` | Save current game | `onclick="save_game('s0')"` |
| `cycle_cvar('name', n)` | Cycle cvar value | `onclick="cycle_cvar('crosshair', 1)"` |

### Event Listener Instancer

The `MenuEventInstancer` class creates event listeners for inline handlers. **Critical**: RmlUI does NOT take ownership of returned listeners - we must manage their lifetime ourselves.

```cpp
// MenuEventInstancer stores listeners to keep them alive
class MenuEventInstancer : public Rml::EventListenerInstancer {
    std::vector<std::unique_ptr<ActionEventListener>> m_listeners;

    Rml::EventListener* InstanceEventListener(const Rml::String& value,
                                               Rml::Element* element) override {
        auto listener = std::make_unique<ActionEventListener>(value);
        ActionEventListener* ptr = listener.get();
        m_listeners.push_back(std::move(listener));  // Keep alive!
        return ptr;
    }
};
```

Reference: [RmlUI Events Documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/events.html)

## Mouse Input Handling

### The SDL Event Filter Problem

vkQuake uses an SDL event filter (`IN_FilterMouseEvents`) to discard mouse motion events when menus are inactive. This filter is installed by `IN_Deactivate()`:

```c
// Problem: This blocks ALL mouse motion when active
static int IN_FilterMouseEvents(const SDL_Event *event) {
    if (event->type == SDL_MOUSEMOTION)
        return 0;  // Discard
    return 1;
}
```

### Solution

The filter must allow mouse events through when RmlUI menus are active:

```c
static int IN_FilterMouseEvents(const SDL_Event *event) {
#ifdef USE_RMLUI
    if (UI_WantsMenuInput())
        return 1;  // Allow all events for RmlUI
#endif
    if (event->type == SDL_MOUSEMOTION)
        return 0;
    return 1;
}
```

### Cursor Release

When opening RmlUI menus, always release the cursor (even in fullscreen):

```c
// In keys.c when opening RmlUI menu:
IN_Deactivate(true);  // Always free cursor, not just in windowed mode
```

## Data Binding System

### GameDataModel

Syncs Quake game state to RmlUI for HUD display:

```cpp
// Updates each frame with cl.stats[] values
GameDataModel::Update();

// Available in RML via data binding:
// {{ health }}, {{ armor }}, {{ ammo }}, etc.
```

### CvarBindingManager

Two-way binding between cvars and UI elements:

```cpp
CvarBindingManager::RegisterFloat("sensitivity", "mouse_speed", 1.0f, 11.0f, 0.5f);
CvarBindingManager::RegisterBool("scr_showfps", "show_fps");
```

## File Structure

```
ui/
├── fonts/
│   ├── LatoLatin-Regular.ttf
│   ├── LatoLatin-Bold.ttf
│   ├── LatoLatin-Italic.ttf
│   ├── LatoLatin-BoldItalic.ttf
│   ├── OpenSans.ttf
│   └── LICENSE.txt
├── rcss/
│   ├── base.rcss        # Reset, typography, colors, animations
│   ├── menu.rcss         # Menu layouts, panels, buttons
│   ├── main_menu.rcss    # Main menu specific styles
│   ├── hud.rcss          # HUD positioning
│   └── widgets.rcss      # Form elements (sliders, checkboxes)
└── rml/
    ├── hud.rml
    ├── menus/
    │   ├── main_menu.rml
    │   ├── pause_menu.rml
    │   ├── options.rml
    │   ├── options_game.rml
    │   ├── options_graphics.rml
    │   ├── options_sound.rml
    │   ├── options_keys.rml
    │   ├── singleplayer.rml
    │   ├── multiplayer.rml
    │   ├── player_setup.rml
    │   ├── load_save.rml
    │   ├── confirm_reset.rml
    │   └── quit.rml
    └── hud/
        ├── hud_classic.rml
        ├── hud_modern.rml
        ├── scoreboard.rml
        └── intermission.rml
```

## Troubleshooting

### Clicks Not Registering on Buttons

1. **Check mouse coordinates**: Add debug logging to `UI_MouseButton()` to verify cursor position
2. **Verify event filter**: Ensure `IN_FilterMouseEvents` allows events when `UI_WantsMenuInput()` is true
3. **Check listener lifetime**: Event listeners must be stored (RmlUI doesn't take ownership)

### Mouse Position Always (0,0)

The SDL event filter is blocking `SDL_MOUSEMOTION` events. Fix:
- Modify `IN_FilterMouseEvents()` to check `UI_WantsMenuInput()`
- Ensure `IN_Deactivate(true)` is called to release cursor

### Menu Opens and Immediately Closes

Add a cooldown to prevent the same keypress from opening and closing:

```cpp
static double g_menu_open_time = 0.0;

void UI_PushMenu(const char* path) {
    // ... load document ...
    g_menu_open_time = realtime;
}

void UI_HandleEscape() {
    if (realtime - g_menu_open_time < 0.1)
        return;  // Ignore escape within 100ms of opening
    // ... close menu ...
}
```

### User Can't Close Menu

Check that:
1. Escape key events are reaching `keys.c`
2. `UI_WantsMenuInput()` returns 1 when menu is active
3. `UI_HandleEscape()` properly pops the menu stack

### Input Goes to Game While Menu Open

Verify:
1. `UI_WantsMenuInput()` returns 1
2. Input events in `in_sdl2.c` check mode before forwarding
3. `key_dest` is set to `key_menu`

### RmlUI Menu Doesn't Appear

Check:
1. Document path is correct
2. `UI_PushMenu()` logs success
3. `g_visible` is set to true (automatic with `MENU_ACTIVE` mode)
4. Vulkan render interface is initialized
