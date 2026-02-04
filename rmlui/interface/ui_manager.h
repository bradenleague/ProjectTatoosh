/*
 * Tatoosh - RmlUI Integration Layer
 * Public API for Quake engine integration
 */

#ifndef TATOOSH_UI_MANAGER_H
#define TATOOSH_UI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the RmlUI subsystem. Call from Host_Init() */
int UI_Init(int width, int height, const char *base_path);

/* Shutdown and cleanup. Call from Host_Shutdown() */
void UI_Shutdown(void);

/* Process pending UI operations - MUST call from main thread before rendering */
void UI_ProcessPending(void);

/* Update UI state. Call each frame before rendering */
void UI_Update(double dt);

/* Render the UI. Call from SCR_UpdateScreen() after 3D scene */
void UI_Render(void);

/* Handle window resize */
void UI_Resize(int width, int height);

/* Input event handling - returns 1 if event was consumed by UI */
int UI_KeyEvent(int key, int scancode, int pressed, int repeat);
int UI_CharEvent(unsigned int codepoint);
int UI_MouseMove(int x, int y, int dx, int dy);
int UI_MouseButton(int button, int pressed);
int UI_MouseScroll(float x, float y);

/* Document management */
int UI_LoadDocument(const char *path);
void UI_UnloadDocument(const char *path);
void UI_ShowDocument(const char *path, int modal);
void UI_HideDocument(const char *path);

/* Visibility control */
void UI_SetVisible(int visible);
int UI_IsVisible(void);
void UI_Toggle(void);

/* Data binding - connect QuakeC globals to UI elements */
void UI_BindInt(const char *name, int *value);
void UI_BindFloat(const char *name, float *value);
void UI_BindString(const char *name, const char **value);

/* Update bound data (call when QuakeC values change) */
void UI_UpdateBindings(void);

/* Debug overlay toggle */
void UI_ToggleDebugger(void);

/* Hot reload support (if enabled) */
void UI_ReloadDocuments(void);

/* Input mode - controls how RmlUI interacts with vkQuake's input system */
typedef enum {
    UI_INPUT_INACTIVE,      /* Not handling input - game/Quake menu works normally */
    UI_INPUT_MENU_ACTIVE,   /* Menu captures all input (except escape handled specially) */
    UI_INPUT_OVERLAY        /* HUD mode - visible but passes input through to game */
} ui_input_mode_t;

/* Input mode control */
void UI_SetInputMode(ui_input_mode_t mode);
ui_input_mode_t UI_GetInputMode(void);
int UI_WantsMenuInput(void);      /* Returns 1 if MENU_ACTIVE */
void UI_HandleEscape(void);       /* Close current menu or deactivate (deferred to next update) */
void UI_CloseAllMenus(void);      /* Close all menus (deferred to next update) */
void UI_CloseAllMenusImmediate(void); /* Close all menus immediately (for internal RmlUI event handlers only) */
void UI_PushMenu(const char* path);   /* Open menu, set MENU_ACTIVE */
void UI_PopMenu(void);            /* Pop current menu from stack */

/* Vulkan integration - call after vkQuake initializes Vulkan */
/* Note: VulkanConfig is defined in render_interface_vk.h (Tatoosh::VulkanConfig) */
struct Tatoosh_VulkanConfig;  /* Forward declaration for C compatibility */
void UI_InitializeVulkan(const void* config);  /* Takes Tatoosh::VulkanConfig* */

/* Frame rendering hooks - called by vkQuake's render loop */
void UI_BeginFrame(void* cmd, int width, int height);
void UI_EndFrame(void);

/* Garbage collection - call after GPU fence wait to safely destroy resources */
void UI_CollectGarbage(void);

#ifdef __cplusplus
}
#endif

#endif /* TATOOSH_UI_MANAGER_H */
