/*
 * Tatoosh - Menu Stack Manager
 *
 * Manages menu navigation with stack-based push/pop semantics.
 * Handles escape key processing with deferred operations to prevent
 * race conditions between input handling and rendering.
 */

#ifndef TATOOSH_APPLICATION_MENU_STACK_H
#define TATOOSH_APPLICATION_MENU_STACK_H

#include "../domain/input_mode.h"
#include <string>
#include <vector>
#include <functional>

namespace Tatoosh {

// Callback type for mode changes
using InputModeCallback = std::function<void(ui_input_mode_t)>;

class MenuStack {
public:
    // Initialize with callbacks for document and mode management
    // show_doc: called to show a document by path
    // hide_doc: called to hide a document by path
    // load_doc: called to load a document by path (returns success)
    // set_mode: called when input mode changes
    static void Initialize(
        std::function<void(const std::string&)> show_doc,
        std::function<void(const std::string&)> hide_doc,
        std::function<bool(const std::string&)> load_doc,
        InputModeCallback set_mode
    );

    // Shutdown and clear state
    static void Shutdown();

    // Push a menu onto the stack (hides current, shows new)
    // Sets input mode to MENU_ACTIVE
    static void Push(const std::string& path);

    // Pop current menu from stack (hides it, shows previous)
    // If stack becomes empty, sets input mode to INACTIVE
    static void Pop();

    // Request escape handling (deferred to next ProcessPending call)
    static void RequestEscape();

    // Request closing all menus (deferred to next ProcessPending call)
    static void RequestCloseAll();

    // Process any pending operations
    // Call this from the main thread before rendering
    static void ProcessPending();

    // Immediately close all menus (for internal use during event handling)
    static void CloseAllImmediate();

    // Check if any menus are open
    static bool IsEmpty();

    // Get current menu path (top of stack, empty if none)
    static const std::string& Current();

    // Get stack depth
    static size_t Depth();

    // Check if we want menu input (considering pending operations)
    static bool WantsInput();

    // Set the time reference for debouncing (pointer to engine time)
    static void SetTimeReference(const double* time_ptr);

private:
    static void ProcessEscape();

    static std::vector<std::string> s_stack;
    static bool s_pending_escape;
    static bool s_pending_close_all;
    static double s_open_time;
    static const double* s_time_ptr;

    // Callbacks
    static std::function<void(const std::string&)> s_show_doc;
    static std::function<void(const std::string&)> s_hide_doc;
    static std::function<bool(const std::string&)> s_load_doc;
    static InputModeCallback s_set_mode;
};

} // namespace Tatoosh

#endif // TATOOSH_APPLICATION_MENU_STACK_H
