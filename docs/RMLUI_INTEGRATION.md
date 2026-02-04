# RmlUI Integration Guide

This document describes how RmlUI is integrated with vkQuake in Tatoosh, including the input handling system that coordinates between RmlUI menus and Quake's native systems.

## Overview

RmlUI provides HTML/CSS-based UI rendering within vkQuake's Vulkan pipeline. The integration consists of:

- **Render Interface** (`rmlui/render_interface_vk.cpp`) - Custom Vulkan renderer using vkQuake's context
- **System Interface** (`rmlui/system_interface.cpp`) - Time and logging integration
- **UI Manager** (`rmlui/ui_manager.cpp`) - Document management, input handling, state control
- **C Bridge** (`engine/Quake/rmlui_bridge.cpp`) - C-compatible API for vkQuake integration

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
| `RMLUI_INACTIVE` | Default state | RmlUI doesn't handle input, game works normally |
| `RMLUI_MENU_ACTIVE` | Menu is open | RmlUI captures all input except Escape |
| `RMLUI_OVERLAY` | HUD elements | RmlUI visible but passes input through to game |

### Menu Stack

When menus are opened, they're pushed onto a stack. Pressing Escape:
1. Pops the current menu from the stack
2. Hides that menu document
3. If the stack is empty, returns to `RMLUI_INACTIVE` mode
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

### Menu Commands

| Command | Description |
|---------|-------------|
| `ui_menu [path]` | Open an RmlUI menu (default: `ui/rml/menus/main_menu.rml`) |
| `ui_closemenu` | Close all RmlUI menus and return to game |

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
    │                   ├──[RmlUI_WantsMenuInput()?]
    │                   │       │
    │                   │       YES ──► RmlUI_HandleEscape()
    │                   │       │           │
    │                   │       │           └──[menus left?]
    │                   │       │                 NO ──► key_dest=key_game, IN_Activate()
    │                   │       │
    │                   │       NO ──► [ui_use_rmlui_menus?]
    │                   │                   │
    │                   │                   YES ──► RmlUI_PushMenu(), key_dest=key_menu
    │                   │                   │
    │                   │                   NO ──► M_ToggleMenu_f() (Quake menu)
    │
    └──[Other keys]
            │
            └──[RmlUI_WantsMenuInput()?]
                    │
                    YES ──► RmlUI_KeyEvent() [consume]
                    │
                    NO ──► Quake key handling
```

## API Reference

### C API (rmlui_bridge.h)

```c
/* Input mode enum */
typedef enum {
    RMLUI_INACTIVE,      /* Not handling input */
    RMLUI_MENU_ACTIVE,   /* Menu captures all input */
    RMLUI_OVERLAY        /* HUD mode, pass-through */
} rmlui_input_mode_t;

/* Input mode control */
void RmlUI_SetInputMode(rmlui_input_mode_t mode);
rmlui_input_mode_t RmlUI_GetInputMode(void);
int RmlUI_WantsMenuInput(void);   /* Returns 1 if MENU_ACTIVE */
void RmlUI_HandleEscape(void);    /* Close current menu or deactivate */
void RmlUI_PushMenu(const char* path);  /* Open menu, set MENU_ACTIVE */
void RmlUI_PopMenu(void);         /* Pop current menu from stack */

/* Visibility (independent of input mode) */
void RmlUI_SetVisible(int visible);
int RmlUI_IsVisible(void);
void RmlUI_Toggle(void);

/* Document management */
int RmlUI_LoadDocument(const char* path);
void RmlUI_UnloadDocument(const char* path);
void RmlUI_ShowDocument(const char* path, int modal);
void RmlUI_HideDocument(const char* path);

/* Input events (returns 1 if consumed) */
int RmlUI_KeyEvent(int key, int scancode, int pressed, int repeat);
int RmlUI_CharEvent(unsigned int codepoint);
int RmlUI_MouseMove(int x, int y, int dx, int dy);
int RmlUI_MouseButton(int button, int pressed);
int RmlUI_MouseScroll(float x, float y);
```

### C++ API (ui_manager.h)

The C++ API mirrors the C API with `UI_` prefix instead of `RmlUI_`. The bridge layer handles type mapping between `rmlui_input_mode_t` and `ui_input_mode_t`.

## Integration Points

### Initialization (host.c)

```c
// In Host_Init(), after VID_Init():
#ifdef USE_RMLUI
    RmlUI_Init(1280, 720, com_basedir);
    Cvar_RegisterVariable(&ui_use_rmlui_menus);
    Cmd_AddCommand("ui_menu", UI_Menu_f);
    // ... other commands
#endif
```

### Input Handling (in_sdl2.c)

```c
// Key events - check input mode, exclude escape
if (RmlUI_WantsMenuInput() && key != SDLK_ESCAPE) {
    if (RmlUI_KeyEvent(...))
        break;  // Consumed
}

// Mouse events - RmlUI consumes all when menu active
if (RmlUI_WantsMenuInput()) {
    RmlUI_MouseButton(...);
    break;
}

// Mouse motion - always update cursor, block game in menu mode
RmlUI_MouseMove(...);
if (RmlUI_WantsMenuInput())
    break;
```

### Escape Key (keys.c)

```c
if (key == K_ESCAPE) {
    // RmlUI handles escape first if it has active menu
    if (RmlUI_WantsMenuInput()) {
        RmlUI_HandleEscape();
        if (!RmlUI_WantsMenuInput()) {
            IN_Activate();
            key_dest = key_game;
        }
        return;
    }

    // Otherwise, check if RmlUI menus should be used
    if (ui_use_rmlui_menus.value) {
        IN_Deactivate(modestate == MS_WINDOWED);
        key_dest = key_menu;
        RmlUI_PushMenu("ui/rml/menus/main_menu.rml");
    } else {
        M_ToggleMenu_f();  // Quake menu
    }
}
```

## Usage Examples

### Opening a Menu from QuakeC or Console

```
// Console
ui_menu ui/rml/menus/options.rml

// Or use default main menu
ui_menu
```

### Enabling RmlUI Menus by Default

```
// In config.cfg or console
ui_use_rmlui_menus 1

// Now pressing Escape opens RmlUI menu instead of Quake menu
```

### Creating Menu Documents

Menus are RML files (HTML-like) with RCSS styles:

```html
<!-- ui/rml/menus/main_menu.rml -->
<rml>
<head>
    <link type="text/rcss" href="../rcss/menu.rcss"/>
</head>
<body>
    <div id="menu">
        <h1>Tatoosh</h1>
        <button onclick="console('map e1m1')">New Game</button>
        <button onclick="ui_menu ui/rml/menus/options.rml">Options</button>
        <button onclick="quit">Quit</button>
    </div>
</body>
</rml>
```

## Troubleshooting

### User Can't Close Menu

Check that:
1. Escape key events are reaching `keys.c`
2. `RmlUI_WantsMenuInput()` returns 1 when menu is active
3. `RmlUI_HandleEscape()` properly pops the menu stack

### Input Goes to Game While Menu Open

Verify:
1. `RmlUI_WantsMenuInput()` returns 1
2. Input events in `in_sdl2.c` check mode before forwarding
3. `key_dest` is set to `key_menu`

### RmlUI Menu Doesn't Appear

Check:
1. Document path is correct
2. `RmlUI_PushMenu()` logs success
3. `g_visible` is set to true (automatic with `MENU_ACTIVE` mode)
4. Vulkan render interface is initialized

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
    if (RmlUI_WantsMenuInput())
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
│   └── LatoLatin-BoldItalic.ttf
├── rcss/
│   ├── base.rcss      # Reset, typography, colors, animations
│   ├── menu.rcss      # Menu layouts, panels, buttons
│   ├── hud.rcss       # HUD positioning
│   └── widgets.rcss   # Form elements (sliders, checkboxes)
└── rml/
    ├── menus/
    │   ├── main_menu.rml
    │   ├── options.rml
    │   ├── options_game.rml
    │   ├── options_graphics.rml
    │   ├── options_sound.rml
    │   ├── options_keys.rml
    │   ├── singleplayer.rml
    │   ├── multiplayer.rml
    │   ├── load_save.rml
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
2. **Verify event filter**: Ensure `IN_FilterMouseEvents` allows events when `RmlUI_WantsMenuInput()` is true
3. **Check listener lifetime**: Event listeners must be stored (RmlUI doesn't take ownership)

### Mouse Position Always (0,0)

The SDL event filter is blocking `SDL_MOUSEMOTION` events. Fix:
- Modify `IN_FilterMouseEvents()` to check `RmlUI_WantsMenuInput()`
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
2. `RmlUI_WantsMenuInput()` returns 1 when menu is active
3. `RmlUI_HandleEscape()` properly pops the menu stack

### Input Goes to Game While Menu Open

Verify:
1. `RmlUI_WantsMenuInput()` returns 1
2. Input events in `in_sdl2.c` check mode before forwarding
3. `key_dest` is set to `key_menu`

### RmlUI Menu Doesn't Appear

Check:
1. Document path is correct
2. `RmlUI_PushMenu()` logs success
3. `g_visible` is set to true (automatic with `MENU_ACTIVE` mode)
4. Vulkan render interface is initialized

## Future Enhancements

1. **HUD Mode** - Use `RMLUI_OVERLAY` for in-game HUD that doesn't capture input
2. **Animated Transitions** - Fade/slide between menu states
3. **Full Menu Replacement** - Implement all Quake menu screens in RmlUI
4. **Hot Reload** - Reload RCSS/RML without restarting game
