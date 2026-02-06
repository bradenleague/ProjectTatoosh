/*
 * Tatoosh - RmlUI Integration Layer
 * Public API for Quake engine integration
 */

#ifndef TATOOSH_UI_MANAGER_H
#define TATOOSH_UI_MANAGER_H

#include "types/input_mode.h"

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
int UI_IsMenuVisible(void);

/* Debug overlay toggle */
void UI_ToggleDebugger(void);

/* Hot reload support (if enabled) */
void UI_ReloadDocuments(void);

/* Input mode control */
void UI_SetInputMode(ui_input_mode_t mode);
ui_input_mode_t UI_GetInputMode(void);
int UI_WantsMenuInput(void);      /* Returns 1 if MENU_ACTIVE */
void UI_HandleEscape(void);       /* Close current menu or deactivate (deferred to next update) */
void UI_CloseAllMenus(void);      /* Close all menus (deferred to next update) */
void UI_CloseAllMenusImmediate(void); /* Close all menus immediately (for internal RmlUI event handlers only) */
void UI_PushMenu(const char* path);   /* Open menu, set MENU_ACTIVE */
void UI_PopMenu(void);            /* Pop current menu from stack */

/* HUD control */
void UI_ShowHUD(const char* hud_document);   /* NULL = resolve from scr_style */
void UI_HideHUD(void);
int UI_IsHUDVisible(void);

/* Scoreboard and intermission overlays */
void UI_ShowScoreboard(void);
void UI_HideScoreboard(void);
void UI_ShowIntermission(void);
void UI_HideIntermission(void);

/* Game state synchronization - call each frame from sbar.c */
void UI_SyncGameState(const int* stats, int items,
                      int intermission, int gametype,
                      int maxclients,
                      const char* level_name, const char* map_name,
                      double game_time);

/* Key capture support (for rebinding UI) */
int UI_IsCapturingKey(void);
void UI_OnKeyCaptured(int key, const char* key_name);

/* Vulkan integration - call after vkQuake initializes Vulkan */
#include <vulkan/vulkan.h>
typedef struct ui_vulkan_config_s {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkQueue graphics_queue;
    uint32_t queue_family_index;
    VkFormat color_format;
    VkFormat depth_format;
    VkSampleCountFlagBits sample_count;
    VkRenderPass render_pass;
    uint32_t subpass;
    VkPhysicalDeviceMemoryProperties memory_properties;
    PFN_vkCmdBindPipeline cmd_bind_pipeline;
    PFN_vkCmdBindDescriptorSets cmd_bind_descriptor_sets;
    PFN_vkCmdBindVertexBuffers cmd_bind_vertex_buffers;
    PFN_vkCmdBindIndexBuffer cmd_bind_index_buffer;
    PFN_vkCmdDraw cmd_draw;
    PFN_vkCmdDrawIndexed cmd_draw_indexed;
    PFN_vkCmdPushConstants cmd_push_constants;
    PFN_vkCmdSetScissor cmd_set_scissor;
    PFN_vkCmdSetViewport cmd_set_viewport;
} ui_vulkan_config_t;
void UI_InitializeVulkan(const void* config);  /* Takes ui_vulkan_config_t* */

/* Frame rendering hooks - called by vkQuake's render loop */
void UI_BeginFrame(void* cmd, int width, int height);
void UI_EndFrame(void);

/* Garbage collection - call after GPU fence wait to safely destroy resources */
void UI_CollectGarbage(void);

#ifdef __cplusplus
}
#endif

#endif /* TATOOSH_UI_MANAGER_H */
