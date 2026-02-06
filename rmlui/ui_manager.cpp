/*
 * Tatoosh - UI Manager Implementation
 *
 * Main integration layer between RmlUI and vkQuake.
 * Provides C-compatible API for engine hooks.
 */

#include "ui_manager.h"
#include "internal/render_interface_vk.h"
#include "internal/system_interface.h"
#include "internal/game_data_model.h"
#include "internal/cvar_binding.h"
#include "internal/menu_event_handler.h"

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>

#include <SDL.h>
#include <cstdio>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

// vkQuake externals
extern "C" {
    extern double realtime;  // vkQuake's time reference
    void Con_Printf(const char* fmt, ...);
    void IN_Activate(void);
    void IN_Deactivate(int clear);
    void IN_EndIgnoringMouseEvents(void);
    extern int key_dest;
    #define key_game 0
    #define key_menu 2
}

namespace {

// Debounce window (seconds) to prevent immediate close when a menu was just opened
constexpr double MENU_DEBOUNCE_SECONDS = 0.1;

// Global state
std::unique_ptr<Tatoosh::RenderInterface_VK> g_render_interface;
std::unique_ptr<Tatoosh::SystemInterface> g_system_interface;
Rml::Context* g_context = nullptr;
bool g_initialized = false;
bool g_visible = false;  // Start hidden - toggle with 'ui_toggle' console command
int g_width = 0;
int g_height = 0;
static bool g_assets_loaded = false;

// Input mode state
ui_input_mode_t g_input_mode = UI_INPUT_INACTIVE;
std::vector<std::string> g_menu_stack;  // Stack of menu document paths for escape navigation
double g_menu_open_time = 0.0;  // Time when menu was last opened (to prevent immediate close)

// HUD/overlay tracking
const char* g_current_hud = nullptr;
bool g_hud_visible = false;
bool g_scoreboard_visible = false;
bool g_intermission_visible = false;
int g_last_intermission = 0;

// Deferred operations - processed during UI_Update to avoid race conditions with rendering
bool g_pending_escape = false;  // ESC pressed, handle at next update
bool g_pending_close_all = false;  // Request to close all menus at next update

// Document tracking
std::unordered_map<std::string, Rml::ElementDocument*> g_documents;
std::string g_ui_base_path;       // Base path for UI assets (set during font loading)
std::string g_engine_base_path;   // com_basedir passed from engine

bool IsMenuDocumentPath(const std::string& path)
{
    return path.find("/menus/") != std::string::npos;
}

bool HasVisibleMenuDocument()
{
    for (const auto& pair : g_documents) {
        if (!IsMenuDocumentPath(pair.first)) {
            continue;
        }
        if (pair.second && pair.second->IsVisible()) {
            return true;
        }
    }
    return false;
}

// Convert SDL key to RmlUI key
Rml::Input::KeyIdentifier TranslateKey(int sdl_key)
{
    using namespace Rml::Input;

    switch (sdl_key) {
        case SDLK_UNKNOWN:      return KI_UNKNOWN;
        case SDLK_SPACE:        return KI_SPACE;
        case SDLK_0:            return KI_0;
        case SDLK_1:            return KI_1;
        case SDLK_2:            return KI_2;
        case SDLK_3:            return KI_3;
        case SDLK_4:            return KI_4;
        case SDLK_5:            return KI_5;
        case SDLK_6:            return KI_6;
        case SDLK_7:            return KI_7;
        case SDLK_8:            return KI_8;
        case SDLK_9:            return KI_9;
        case SDLK_a:            return KI_A;
        case SDLK_b:            return KI_B;
        case SDLK_c:            return KI_C;
        case SDLK_d:            return KI_D;
        case SDLK_e:            return KI_E;
        case SDLK_f:            return KI_F;
        case SDLK_g:            return KI_G;
        case SDLK_h:            return KI_H;
        case SDLK_i:            return KI_I;
        case SDLK_j:            return KI_J;
        case SDLK_k:            return KI_K;
        case SDLK_l:            return KI_L;
        case SDLK_m:            return KI_M;
        case SDLK_n:            return KI_N;
        case SDLK_o:            return KI_O;
        case SDLK_p:            return KI_P;
        case SDLK_q:            return KI_Q;
        case SDLK_r:            return KI_R;
        case SDLK_s:            return KI_S;
        case SDLK_t:            return KI_T;
        case SDLK_u:            return KI_U;
        case SDLK_v:            return KI_V;
        case SDLK_w:            return KI_W;
        case SDLK_x:            return KI_X;
        case SDLK_y:            return KI_Y;
        case SDLK_z:            return KI_Z;
        case SDLK_SEMICOLON:    return KI_OEM_1;
        case SDLK_PLUS:         return KI_OEM_PLUS;
        case SDLK_COMMA:        return KI_OEM_COMMA;
        case SDLK_MINUS:        return KI_OEM_MINUS;
        case SDLK_PERIOD:       return KI_OEM_PERIOD;
        case SDLK_SLASH:        return KI_OEM_2;
        case SDLK_BACKQUOTE:    return KI_OEM_3;
        case SDLK_LEFTBRACKET:  return KI_OEM_4;
        case SDLK_BACKSLASH:    return KI_OEM_5;
        case SDLK_RIGHTBRACKET: return KI_OEM_6;
        case SDLK_QUOTEDBL:     return KI_OEM_7;
        case SDLK_KP_0:         return KI_NUMPAD0;
        case SDLK_KP_1:         return KI_NUMPAD1;
        case SDLK_KP_2:         return KI_NUMPAD2;
        case SDLK_KP_3:         return KI_NUMPAD3;
        case SDLK_KP_4:         return KI_NUMPAD4;
        case SDLK_KP_5:         return KI_NUMPAD5;
        case SDLK_KP_6:         return KI_NUMPAD6;
        case SDLK_KP_7:         return KI_NUMPAD7;
        case SDLK_KP_8:         return KI_NUMPAD8;
        case SDLK_KP_9:         return KI_NUMPAD9;
        case SDLK_KP_ENTER:     return KI_NUMPADENTER;
        case SDLK_KP_MULTIPLY:  return KI_MULTIPLY;
        case SDLK_KP_PLUS:      return KI_ADD;
        case SDLK_KP_MINUS:     return KI_SUBTRACT;
        case SDLK_KP_PERIOD:    return KI_DECIMAL;
        case SDLK_KP_DIVIDE:    return KI_DIVIDE;
        case SDLK_BACKSPACE:    return KI_BACK;
        case SDLK_TAB:          return KI_TAB;
        case SDLK_CLEAR:        return KI_CLEAR;
        case SDLK_RETURN:       return KI_RETURN;
        case SDLK_PAUSE:        return KI_PAUSE;
        case SDLK_CAPSLOCK:     return KI_CAPITAL;
        case SDLK_ESCAPE:       return KI_ESCAPE;
        case SDLK_PAGEUP:       return KI_PRIOR;
        case SDLK_PAGEDOWN:     return KI_NEXT;
        case SDLK_END:          return KI_END;
        case SDLK_HOME:         return KI_HOME;
        case SDLK_LEFT:         return KI_LEFT;
        case SDLK_UP:           return KI_UP;
        case SDLK_RIGHT:        return KI_RIGHT;
        case SDLK_DOWN:         return KI_DOWN;
        case SDLK_INSERT:       return KI_INSERT;
        case SDLK_DELETE:       return KI_DELETE;
        case SDLK_HELP:         return KI_HELP;
        case SDLK_F1:           return KI_F1;
        case SDLK_F2:           return KI_F2;
        case SDLK_F3:           return KI_F3;
        case SDLK_F4:           return KI_F4;
        case SDLK_F5:           return KI_F5;
        case SDLK_F6:           return KI_F6;
        case SDLK_F7:           return KI_F7;
        case SDLK_F8:           return KI_F8;
        case SDLK_F9:           return KI_F9;
        case SDLK_F10:          return KI_F10;
        case SDLK_F11:          return KI_F11;
        case SDLK_F12:          return KI_F12;
        case SDLK_F13:          return KI_F13;
        case SDLK_F14:          return KI_F14;
        case SDLK_F15:          return KI_F15;
        case SDLK_NUMLOCKCLEAR: return KI_NUMLOCK;
        case SDLK_SCROLLLOCK:   return KI_SCROLL;
        case SDLK_LSHIFT:       return KI_LSHIFT;
        case SDLK_RSHIFT:       return KI_RSHIFT;
        case SDLK_LCTRL:        return KI_LCONTROL;
        case SDLK_RCTRL:        return KI_RCONTROL;
        case SDLK_LALT:         return KI_LMENU;
        case SDLK_RALT:         return KI_RMENU;
        case SDLK_LGUI:         return KI_LMETA;
        case SDLK_RGUI:         return KI_RMETA;
        default:                return KI_UNKNOWN;
    }
}

int GetKeyModifiers()
{
    SDL_Keymod sdl_mods = SDL_GetModState();
    int mods = 0;

    if (sdl_mods & KMOD_CTRL)  mods |= Rml::Input::KM_CTRL;
    if (sdl_mods & KMOD_SHIFT) mods |= Rml::Input::KM_SHIFT;
    if (sdl_mods & KMOD_ALT)   mods |= Rml::Input::KM_ALT;
    if (sdl_mods & KMOD_GUI)   mods |= Rml::Input::KM_META;
    if (sdl_mods & KMOD_NUM)   mods |= Rml::Input::KM_NUMLOCK;
    if (sdl_mods & KMOD_CAPS)  mods |= Rml::Input::KM_CAPSLOCK;

    return mods;
}

} // anonymous namespace

// C API Implementation

extern "C" {

int UI_Init(int width, int height, const char* base_path)
{
    if (g_initialized) {
        Con_Printf("UI_Init: Already initialized\n");
        return 1;
    }

    // Reset any leftover state in case we reinitialize within the same process.
    g_visible = false;
    g_input_mode = UI_INPUT_INACTIVE;
    g_menu_stack.clear();
    g_menu_open_time = 0.0;
    g_pending_escape = false;
    g_pending_close_all = false;
    g_documents.clear();
    g_ui_base_path.clear();
    g_assets_loaded = false;
    g_current_hud = nullptr;
    g_hud_visible = false;
    g_scoreboard_visible = false;
    g_intermission_visible = false;
    g_last_intermission = 0;

    g_width = width;
    g_height = height;
    g_engine_base_path = (base_path && base_path[0]) ? base_path : "";

    // Create interfaces
    g_system_interface = std::make_unique<Tatoosh::SystemInterface>();
    g_system_interface->Initialize(&realtime);

    g_render_interface = std::make_unique<Tatoosh::RenderInterface_VK>();

    // Install interfaces before initializing RmlUI
    Rml::SetSystemInterface(g_system_interface.get());
    Rml::SetRenderInterface(g_render_interface.get());

    // Initialize RmlUI
    if (!Rml::Initialise()) {
        Con_Printf("UI_Init: Failed to initialize RmlUI\n");
        return 0;
    }

    // Create context
    g_context = Rml::CreateContext("main", Rml::Vector2i(width, height));
    if (!g_context) {
        Con_Printf("UI_Init: Failed to create RmlUI context\n");
        Rml::Shutdown();
        return 0;
    }

    // Initialize debugger (optional, for development)
    Rml::Debugger::Initialise(g_context);

    g_initialized = true;
    Con_Printf("UI_Init: RmlUI core initialized (%dx%d)\n", width, height);
    Con_Printf("UI_Init: Fonts and documents will load after Vulkan init\n");

    return 1;
}

// Load fonts and test document - called AFTER Vulkan is initialized
static void UI_LoadAssets()
{
    // Load fonts from ui/fonts directory
    // RmlUI's LoadFontFace auto-detects family, style, weight from the font file
    // Try multiple paths to find the UI assets
    std::vector<std::string> ui_paths;
    if (!g_engine_base_path.empty()) {
        std::string base = g_engine_base_path;
        if (!base.empty() && (base.back() == '/' || base.back() == '\\')) {
            base.pop_back();
        }
        // base_path is typically com_basedir (e.g., external/librequake)
        ui_paths.emplace_back(base + "/ui/");
        ui_paths.emplace_back(base + "/../ui/");
        ui_paths.emplace_back(base + "/../../ui/");
    }

    // Relative to executable location (packaging-friendly)
    if (char* sdl_base = SDL_GetBasePath()) {
        std::string exe_base = sdl_base;
        SDL_free(sdl_base);
        if (!exe_base.empty() && (exe_base.back() == '/' || exe_base.back() == '\\')) {
            exe_base.pop_back();
        }
        ui_paths.emplace_back(exe_base + "/ui/");
        ui_paths.emplace_back(exe_base + "/../ui/");
        ui_paths.emplace_back(exe_base + "/../../ui/");
    }

    // Relative to common run locations
    ui_paths.emplace_back("ui/");        // Repo root or packaged CWD
    ui_paths.emplace_back("../ui/");     // engine/build or build_meson_test
    ui_paths.emplace_back("../../ui/");  // engine/build subdir

    std::string ui_path;
    bool font_loaded = false;

    for (const auto& path : ui_paths) {
        std::string probe = path + "fonts/LatoLatin-Regular.ttf";
        FILE* f = fopen(probe.c_str(), "rb");
        if (!f)
            continue;
        fclose(f);
        if (Rml::LoadFontFace(probe)) {
            ui_path = path;
            Con_Printf("UI_LoadAssets: Found UI assets at: %s\n", path.c_str());
            Con_Printf("UI_LoadAssets: Loaded LatoLatin-Regular.ttf\n");
            font_loaded = true;
            break;
        }
    }

    if (font_loaded) {
        // Load remaining fonts from the same path
        if (Rml::LoadFontFace(ui_path + "fonts/LatoLatin-Bold.ttf")) {
            Con_Printf("UI_LoadAssets: Loaded LatoLatin-Bold.ttf\n");
        }
        if (Rml::LoadFontFace(ui_path + "fonts/LatoLatin-Italic.ttf")) {
            Con_Printf("UI_LoadAssets: Loaded LatoLatin-Italic.ttf\n");
        }
        if (Rml::LoadFontFace(ui_path + "fonts/LatoLatin-BoldItalic.ttf")) {
            Con_Printf("UI_LoadAssets: Loaded LatoLatin-BoldItalic.ttf\n");
        }
    } else {
        Con_Printf("UI_LoadAssets: WARNING - No fonts loaded! UI text will not render.\n");
        Con_Printf("UI_LoadAssets: Tried paths:\n");
        for (const auto& path : ui_paths) {
            Con_Printf("  - %s\n", path.c_str());
        }
    }

    // Store the UI path for later document loading
    if (!ui_path.empty()) {
        g_ui_base_path = ui_path;
        Con_Printf("UI_LoadAssets: UI base path set to '%s'\n", ui_path.c_str());
    }
}

void UI_Shutdown(void)
{
    if (!g_initialized) return;

    // Shutdown data models first
    Tatoosh::MenuEventHandler::Shutdown();
    Tatoosh::CvarBindingManager::Shutdown();
    Tatoosh::GameDataModel::Shutdown();

    // Unload all documents
    for (auto& pair : g_documents) {
        if (pair.second) {
            pair.second->Close();
        }
    }
    g_documents.clear();

    // Shutdown debugger
    Rml::Debugger::Shutdown();

    // Destroy context
    if (g_context) {
        Rml::RemoveContext("main");
        g_context = nullptr;
    }

    // Shutdown RmlUI
    Rml::Shutdown();

    // Cleanup interfaces
    g_render_interface->Shutdown();
    g_render_interface.reset();
    g_system_interface.reset();

    // Reset global UI state so a reinit starts clean.
    g_visible = false;
    g_input_mode = UI_INPUT_INACTIVE;
    g_menu_stack.clear();
    g_menu_open_time = 0.0;
    g_pending_escape = false;
    g_pending_close_all = false;
    g_ui_base_path.clear();
    g_engine_base_path.clear();
    g_assets_loaded = false;
    g_current_hud = nullptr;
    g_hud_visible = false;
    g_scoreboard_visible = false;
    g_intermission_visible = false;
    g_last_intermission = 0;

    g_initialized = false;
    Con_Printf("UI_Shutdown: RmlUI shut down\n");
}

// Internal function to process escape - called from UI_Update to avoid race conditions
static void UI_ProcessPendingEscape(void)
{
    if (!g_initialized || !g_context) return;

    // Prevent immediate close if menu was just opened (same key event causing open+close)
    if (realtime - g_menu_open_time < MENU_DEBOUNCE_SECONDS) {
        return;
    }

    if (g_menu_stack.empty()) {
        // No menus on stack, just deactivate
        UI_SetInputMode(UI_INPUT_INACTIVE);
        return;
    }

    // Pop and hide current menu
    std::string current = g_menu_stack.back();
    g_menu_stack.pop_back();

    auto it = g_documents.find(current);
    if (it != g_documents.end() && it->second) {
        it->second->Hide();
        Con_Printf("UI_HandleEscape: Closed menu '%s'\n", current.c_str());
    }

    // If stack empty after pop, return to inactive mode
    if (g_menu_stack.empty()) {
        UI_SetInputMode(UI_INPUT_INACTIVE);
        Con_Printf("UI_HandleEscape: Menu stack empty, returning to game\n");
#ifdef __cplusplus
        // Restore game input when leaving menus.
        IN_Activate();
        key_dest = key_game;
#endif
    } else {
        // Show the previous menu in the stack (it should already be visible, but ensure it)
        std::string& prev = g_menu_stack.back();
        auto prev_it = g_documents.find(prev);
        if (prev_it != g_documents.end() && prev_it->second) {
            prev_it->second->Show();
        }
    }
}

// Process pending operations - MUST be called from main thread before rendering
void UI_ProcessPending(void)
{
    if (!g_initialized || !g_context) return;

    // Process pending operations BEFORE rendering can start
    // This ensures UI state changes happen atomically between frames
    if (g_pending_escape) {
        g_pending_escape = false;
        UI_ProcessPendingEscape();
    }

    if (g_pending_close_all) {
        g_pending_close_all = false;
        // Close all menus
        while (!g_menu_stack.empty()) {
            UI_ProcessPendingEscape();
        }
    }

    // Reconcile menu state in case external systems changed key_dest or visibility.
    if (HasVisibleMenuDocument()) {
        if (g_input_mode != UI_INPUT_MENU_ACTIVE) {
            UI_SetInputMode(UI_INPUT_MENU_ACTIVE);
        }
        if (key_dest != key_menu) {
            IN_Deactivate(true);
            key_dest = key_menu;
        }
        // Ensure mouse motion events are not filtered while menus are visible.
        IN_EndIgnoringMouseEvents();
        g_visible = true;
    } else if (g_menu_stack.empty() && g_input_mode == UI_INPUT_MENU_ACTIVE) {
        UI_SetInputMode(UI_INPUT_INACTIVE);
    }
}

void UI_Update(double dt)
{
    if (!g_initialized || !g_context) return;

    // Note: Pending operations are now processed in UI_ProcessPending()
    // which is called from the main thread before rendering tasks start.

    // Update game data model to sync with Quake state
    Tatoosh::GameDataModel::Update();

    g_context->Update();

    // Clear any temporary suppression of UI change events after data bindings update.
    Tatoosh::CvarBindingManager::NotifyUIUpdateComplete();

}

void UI_Render(void)
{
    if (!g_initialized || !g_context || !g_visible) return;
    g_context->Render();
}

void UI_Resize(int width, int height)
{
    if (!g_initialized || !g_context) return;

    g_width = width;
    g_height = height;
    g_context->SetDimensions(Rml::Vector2i(width, height));
}

int UI_KeyEvent(int key, int scancode, int pressed, int repeat)
{
    if (!g_initialized || !g_context || !g_visible) return 0;

    Rml::Input::KeyIdentifier rml_key = TranslateKey(key);
    int modifiers = GetKeyModifiers();

    bool consumed = false;
    if (pressed) {
        consumed = g_context->ProcessKeyDown(rml_key, modifiers);
    } else {
        consumed = g_context->ProcessKeyUp(rml_key, modifiers);
    }

    return consumed ? 1 : 0;
}

int UI_CharEvent(unsigned int codepoint)
{
    if (!g_initialized || !g_context || !g_visible) return 0;

    bool consumed = g_context->ProcessTextInput(static_cast<Rml::Character>(codepoint));
    return consumed ? 1 : 0;
}

// Track last mouse position for hit testing debug
static int g_last_mouse_x = 0;
static int g_last_mouse_y = 0;

int UI_MouseMove(int x, int y, int dx, int dy)
{
    // Store position for hit testing debug
    g_last_mouse_x = x;
    g_last_mouse_y = y;

    if (!g_initialized || !g_context || !g_visible) return 0;

    int modifiers = GetKeyModifiers();
    bool consumed = g_context->ProcessMouseMove(x, y, modifiers);
    return consumed ? 1 : 0;
}

int UI_MouseButton(int button, int pressed)
{
    if (!g_initialized || !g_context || !g_visible) {
        Con_Printf("UI_MouseButton: ignored (init=%d ctx=%p visible=%d)\n",
                   g_initialized ? 1 : 0, static_cast<void*>(g_context), g_visible ? 1 : 0);
        return 0;
    }

    int rml_button = 0;
    switch (button) {
        case SDL_BUTTON_LEFT:   rml_button = 0; break;
        case SDL_BUTTON_RIGHT:  rml_button = 1; break;
        case SDL_BUTTON_MIDDLE: rml_button = 2; break;
        default: return 0;
    }

    int modifiers = GetKeyModifiers();
    bool consumed = false;

    if (pressed) {
        consumed = g_context->ProcessMouseButtonDown(rml_button, modifiers);
    } else {
        consumed = g_context->ProcessMouseButtonUp(rml_button, modifiers);
    }

    if (pressed) {
        Rml::Element* hover = g_context->GetHoverElement();
        const char* hover_tag = hover ? hover->GetTagName().c_str() : "<none>";
        const char* hover_id = hover ? hover->GetId().c_str() : "";
        Con_Printf("UI_MouseButton: btn=%d pressed=%d consumed=%d hover=%s id=%s\n",
                   rml_button, pressed ? 1 : 0, consumed ? 1 : 0, hover_tag, hover_id);
    }

    return consumed ? 1 : 0;
}

int UI_MouseScroll(float x, float y)
{
    if (!g_initialized || !g_context || !g_visible) return 0;

    int modifiers = GetKeyModifiers();
    bool consumed = g_context->ProcessMouseWheel(Rml::Vector2f(x, -y), modifiers);
    return consumed ? 1 : 0;
}

// Helper to resolve UI asset paths
// If path starts with "ui/", replace with g_ui_base_path (e.g., "../ui/")
static std::string ResolveUIPath(const char* path)
{
    if (!path) return "";

    std::string p(path);

    // If path starts with "ui/", replace with the base path we found during init
    if (p.length() >= 3 && p.substr(0, 3) == "ui/") {
        if (!g_ui_base_path.empty()) {
            return g_ui_base_path + p.substr(3);  // Replace "ui/" with "../ui/" (or whatever base path)
        }
    }

    return p;
}

int UI_LoadDocument(const char* path)
{
    if (!g_initialized || !g_context) return 0;

    // Check if already loaded (use original path as key)
    auto it = g_documents.find(path);
    if (it != g_documents.end() && it->second) {
        return 1;  // Already loaded
    }

    // Resolve the path relative to UI base directory
    std::string resolved_path = ResolveUIPath(path);

    Rml::ElementDocument* doc = g_context->LoadDocument(resolved_path);
    if (!doc) {
        Con_Printf("UI_LoadDocument: Failed to load '%s' (resolved: '%s')\n", path, resolved_path.c_str());
        return 0;
    }

    // Store with original path as key for consistency
    g_documents[path] = doc;
    Tatoosh::MenuEventHandler::RegisterWithDocument(doc);
    Con_Printf("UI_LoadDocument: Loaded '%s'\n", path);
    return 1;
}

void UI_UnloadDocument(const char* path)
{
    if (!g_initialized || !g_context) return;

    auto it = g_documents.find(path);
    if (it != g_documents.end() && it->second) {
        it->second->Close();
        g_documents.erase(it);
        Con_Printf("UI_UnloadDocument: Unloaded '%s'\n", path);
    }
}

void UI_ShowDocument(const char* path, int modal)
{
    if (!g_initialized || !g_context) return;

    auto it = g_documents.find(path);
    if (it != g_documents.end() && it->second) {
        if (modal) {
            it->second->Show(Rml::ModalFlag::Modal);
        } else {
            it->second->Show();
        }
    }
}

void UI_HideDocument(const char* path)
{
    if (!g_initialized || !g_context) return;

    auto it = g_documents.find(path);
    if (it != g_documents.end() && it->second) {
        it->second->Hide();
    }
}

void UI_SetVisible(int visible)
{
    g_visible = visible != 0;
}

int UI_IsVisible(void)
{
    return g_visible ? 1 : 0;
}

int UI_IsMenuVisible(void)
{
    return HasVisibleMenuDocument() ? 1 : 0;
}

void UI_Toggle(void)
{
    g_visible = !g_visible;
}


void UI_ToggleDebugger(void)
{
    if (!g_initialized || !g_context) return;
    Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
}

void UI_ReloadDocuments(void)
{
#ifdef TATOOSH_HOT_RELOAD
    if (!g_initialized || !g_context) return;

    Con_Printf("UI_ReloadDocuments: Reloading all documents\n");

    // Store visibility state and reload each document
    for (auto& pair : g_documents) {
        if (pair.second) {
            bool was_visible = pair.second->IsVisible();
            std::string path = pair.first;

            pair.second->Close();
            pair.second = g_context->LoadDocument(path);

            if (pair.second) {
                Tatoosh::MenuEventHandler::RegisterWithDocument(pair.second);
                if (was_visible) {
                    pair.second->Show();
                }
            }
        }
    }
#else
    Con_Printf("UI_ReloadDocuments: Hot reload not enabled\n");
#endif
}

// Track if we've done the initial asset load

// Helper to initialize render interface with vkQuake's Vulkan context
// Called from vkQuake after Vulkan is initialized (or reinitialized)
void UI_InitializeVulkan(const void* config)
{
    if (g_render_interface && config) {
        const Tatoosh::VulkanConfig* vk_config = static_cast<const Tatoosh::VulkanConfig*>(config);

        // If already initialized, just reinitialize with new render pass
        // This preserves geometry and textures
        if (g_render_interface->IsInitialized()) {
            if (g_render_interface->Reinitialize(*vk_config)) {
                Con_Printf("UI_InitializeVulkan: Vulkan renderer reinitialized\n");
            } else {
                Con_Printf("UI_InitializeVulkan: ERROR - Failed to reinitialize Vulkan renderer\n");
            }
            return;
        }

        // First-time initialization
        if (g_render_interface->Initialize(*vk_config)) {
            Con_Printf("UI_InitializeVulkan: Vulkan renderer initialized\n");

            // Only load assets on first initialization
            if (!g_assets_loaded) {
                UI_LoadAssets();

                // Initialize data models now that context is ready
                if (g_context) {
                    Tatoosh::GameDataModel::Initialize(g_context);
                    Tatoosh::CvarBindingManager::Initialize(g_context);
                    Tatoosh::MenuEventHandler::Initialize(g_context);
                }
                g_assets_loaded = true;
            }
        } else {
            Con_Printf("UI_InitializeVulkan: ERROR - Failed to initialize Vulkan renderer\n");
        }
    }
}

// Called each frame before UI rendering
void UI_BeginFrame(void* cmd, int width, int height)
{
    if (g_render_interface) {
        g_render_interface->BeginFrame(static_cast<VkCommandBuffer>(cmd), width, height);
    }
}

// Called after UI rendering
void UI_EndFrame(void)
{
    if (g_render_interface) {
        g_render_interface->EndFrame();
    }
}

// Garbage collection - call after GPU fence wait
void UI_CollectGarbage(void)
{
    if (g_render_interface) {
        g_render_interface->CollectGarbage();
    }
}

// Input mode control
void UI_SetInputMode(ui_input_mode_t mode)
{
    ui_input_mode_t old_mode = g_input_mode;
    g_input_mode = mode;

    // Automatically manage visibility based on mode
    if (mode == UI_INPUT_MENU_ACTIVE || mode == UI_INPUT_OVERLAY) {
        g_visible = true;
    } else if (mode == UI_INPUT_INACTIVE && old_mode == UI_INPUT_MENU_ACTIVE) {
        // When exiting menu mode, hide UI unless we have HUD elements
        // For now, just hide - future: check if HUD documents are loaded
        g_visible = false;
        // Note: Unlike native menus, we don't need to restore the demo loop here
        // because RmlUI menus don't disable it in the first place. Demos continue
        // cycling in the background while menus are displayed.
    }

    Con_Printf("UI_SetInputMode: %s -> %s\n",
        old_mode == UI_INPUT_INACTIVE ? "INACTIVE" :
        old_mode == UI_INPUT_MENU_ACTIVE ? "MENU_ACTIVE" : "OVERLAY",
        mode == UI_INPUT_INACTIVE ? "INACTIVE" :
        mode == UI_INPUT_MENU_ACTIVE ? "MENU_ACTIVE" : "OVERLAY");
}

ui_input_mode_t UI_GetInputMode(void)
{
    return g_input_mode;
}

int UI_WantsMenuInput(void)
{
    if (!g_initialized || !g_context) {
        return 0;
    }

    // If the engine is in menu mode and the UI is visible, treat as active.
    if (g_visible && key_dest == key_menu) {
        return 1;
    }

    if (HasVisibleMenuDocument()) {
        return 1;
    }

    // Menu stack is the source of truth for menu input.
    if (!g_menu_stack.empty()) {
        return 1;
    }

    return (g_input_mode == UI_INPUT_MENU_ACTIVE) ? 1 : 0;
}

void UI_HandleEscape(void)
{
    if (!g_initialized || !g_context) return;

    // Defer the actual escape handling to the next UI_Update call.
    // This prevents race conditions between input handling (main thread)
    // and rendering (worker threads). The state change will happen
    // during UI_Update, before rendering is scheduled.
    g_pending_escape = true;
}

void UI_CloseAllMenus(void)
{
    if (!g_initialized || !g_context) return;

    // Defer closing all menus to the next UI_Update call.
    // This prevents race conditions with rendering.
    g_pending_close_all = true;
}

// Immediately close all menus - for internal use when we're already in the update phase
// (e.g., from RmlUI event handlers). Do not call from external code.
void UI_CloseAllMenusImmediate(void)
{
    if (!g_initialized || !g_context) return;

    while (!g_menu_stack.empty()) {
        UI_ProcessPendingEscape();
    }
}

void UI_PushMenu(const char* path)
{
    if (!g_initialized || !g_context || !path) return;

    // Cancel any pending close requests since we're explicitly opening a menu.
    g_pending_escape = false;
    g_pending_close_all = false;

    // Load document if not already loaded
    auto it = g_documents.find(path);
    if (it == g_documents.end() || !it->second) {
        // Resolve the path relative to UI base directory
        std::string resolved_path = ResolveUIPath(path);

        Rml::ElementDocument* doc = g_context->LoadDocument(resolved_path);
        if (!doc) {
            Con_Printf("UI_PushMenu: Failed to load '%s' (resolved: '%s')\n", path, resolved_path.c_str());
            return;
        }
        // Store with original path as key for consistency
        g_documents[path] = doc;
        Tatoosh::MenuEventHandler::RegisterWithDocument(doc);
    }

    // Hide current menu if there is one (optional - could layer them)
    if (!g_menu_stack.empty()) {
        std::string& current = g_menu_stack.back();
        auto current_it = g_documents.find(current);
        if (current_it != g_documents.end() && current_it->second) {
            current_it->second->Hide();
        }
    }

    // Push new menu onto stack and show it
    g_menu_stack.push_back(path);
    Rml::ElementDocument* doc = g_documents[path];
    doc->Show();

    if (Tatoosh::CvarBindingManager::IsInitialized()) {
        Tatoosh::CvarBindingManager::SyncToUI();
    }

    // Set menu mode
    UI_SetInputMode(UI_INPUT_MENU_ACTIVE);

    // Ensure input is routed to menu when a menu is pushed.
    if (key_dest != key_menu) {
        IN_Deactivate(true);
        key_dest = key_menu;
    }
    IN_EndIgnoringMouseEvents();

    // Record open time to prevent immediate close from same key event
    g_menu_open_time = realtime;
}

void UI_PopMenu(void)
{
    // Same as HandleEscape for now - could have different behavior later
    UI_HandleEscape();
}

// ── HUD / Scoreboard / Intermission ────────────────────────────────

void UI_ShowHUD(const char* hud_document)
{
    if (!hud_document) {
        hud_document = "ui/rml/hud/hud_classic.rml";
    }

    // Hide previous HUD if different
    if (g_current_hud && g_current_hud != hud_document && g_hud_visible) {
        UI_HideDocument(g_current_hud);
    }

    // Load and show new HUD
    if (UI_LoadDocument(hud_document)) {
        UI_ShowDocument(hud_document, 0);
        g_current_hud = hud_document;
        g_hud_visible = true;
        UI_SetInputMode(UI_INPUT_OVERLAY);
    }

    // Reset intermission tracking on new game/map
    g_last_intermission = 0;
}

void UI_HideHUD(void)
{
    if (g_current_hud && g_hud_visible) {
        UI_HideDocument(g_current_hud);
        g_hud_visible = false;
    }
    if (g_intermission_visible) {
        UI_HideDocument("ui/rml/hud/intermission.rml");
        g_intermission_visible = false;
    }
    if (g_scoreboard_visible) {
        UI_HideDocument("ui/rml/hud/scoreboard.rml");
        g_scoreboard_visible = false;
    }
    g_last_intermission = 0;

    if (!UI_WantsMenuInput()) {
        UI_SetInputMode(UI_INPUT_INACTIVE);
    }
}

int UI_IsHUDVisible(void)
{
    return g_hud_visible ? 1 : 0;
}

void UI_ShowScoreboard(void)
{
    if (UI_LoadDocument("ui/rml/hud/scoreboard.rml")) {
        UI_ShowDocument("ui/rml/hud/scoreboard.rml", 0);
        g_scoreboard_visible = true;
    }
}

void UI_HideScoreboard(void)
{
    if (g_scoreboard_visible) {
        UI_HideDocument("ui/rml/hud/scoreboard.rml");
        g_scoreboard_visible = false;
    }
}

void UI_ShowIntermission(void)
{
    if (UI_LoadDocument("ui/rml/hud/intermission.rml")) {
        UI_ShowDocument("ui/rml/hud/intermission.rml", 0);
        g_intermission_visible = true;
    }
}

void UI_HideIntermission(void)
{
    if (g_intermission_visible) {
        UI_HideDocument("ui/rml/hud/intermission.rml");
        g_intermission_visible = false;
    }
}

// ── Game state synchronization ─────────────────────────────────────

void UI_SyncGameState(const int* stats, int items,
                      int intermission, int gametype,
                      int maxclients,
                      const char* level_name, const char* map_name,
                      double game_time)
{
    // Ensure OVERLAY mode if HUD is visible and no menu is open
    if (g_hud_visible && !UI_WantsMenuInput()) {
        if (UI_GetInputMode() == UI_INPUT_INACTIVE) {
            UI_SetInputMode(UI_INPUT_OVERLAY);
        }
    }

    // Detect intermission state changes
    if (intermission != g_last_intermission) {
        if (intermission > 0 && g_last_intermission == 0) {
            UI_ShowIntermission();
        } else if (intermission == 0 && g_last_intermission > 0) {
            UI_HideIntermission();
        }
        g_last_intermission = intermission;
    }

    GameDataModel_SyncFromQuake(stats, items, intermission, gametype,
                                maxclients, level_name, map_name, game_time);
}

// ── Key capture ────────────────────────────────────────────────────

int UI_IsCapturingKey(void)
{
    return MenuEventHandler_IsCapturingKey();
}

void UI_OnKeyCaptured(int key, const char* key_name)
{
    MenuEventHandler_OnKeyCaptured(key, key_name);
}

} // extern "C"
