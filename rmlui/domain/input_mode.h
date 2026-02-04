/*
 * Tatoosh - Input Mode Domain Type
 *
 * Defines the input capture states for the UI system.
 * This is a C-compatible enum for use across the C/C++ boundary.
 */

#ifndef TATOOSH_DOMAIN_INPUT_MODE_H
#define TATOOSH_DOMAIN_INPUT_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Input mode - controls how RmlUI interacts with vkQuake's input system */
typedef enum {
    UI_INPUT_INACTIVE,      /* Not handling input - game/Quake menu works normally */
    UI_INPUT_MENU_ACTIVE,   /* Menu captures all input (except escape handled specially) */
    UI_INPUT_OVERLAY        /* HUD mode - visible but passes input through to game */
} ui_input_mode_t;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace Tatoosh {

// C++ enum class wrapper for type safety in C++ code
enum class InputMode {
    Inactive = UI_INPUT_INACTIVE,
    MenuActive = UI_INPUT_MENU_ACTIVE,
    Overlay = UI_INPUT_OVERLAY
};

// Convert between C and C++ types
inline ui_input_mode_t to_c(InputMode mode) {
    return static_cast<ui_input_mode_t>(mode);
}

inline InputMode from_c(ui_input_mode_t mode) {
    return static_cast<InputMode>(mode);
}

} // namespace Tatoosh
#endif

#endif // TATOOSH_DOMAIN_INPUT_MODE_H
