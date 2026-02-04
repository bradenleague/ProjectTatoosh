/*
 * Tatoosh - Menu Stack Manager Implementation
 */

#include "menu_stack.h"

extern "C" {
    void Con_Printf(const char* fmt, ...);
}

namespace Tatoosh {

// Static member initialization
std::vector<std::string> MenuStack::s_stack;
bool MenuStack::s_pending_escape = false;
bool MenuStack::s_pending_close_all = false;
double MenuStack::s_open_time = 0.0;
const double* MenuStack::s_time_ptr = nullptr;

std::function<void(const std::string&)> MenuStack::s_show_doc;
std::function<void(const std::string&)> MenuStack::s_hide_doc;
std::function<bool(const std::string&)> MenuStack::s_load_doc;
InputModeCallback MenuStack::s_set_mode;

void MenuStack::Initialize(
    std::function<void(const std::string&)> show_doc,
    std::function<void(const std::string&)> hide_doc,
    std::function<bool(const std::string&)> load_doc,
    InputModeCallback set_mode)
{
    s_show_doc = std::move(show_doc);
    s_hide_doc = std::move(hide_doc);
    s_load_doc = std::move(load_doc);
    s_set_mode = std::move(set_mode);

    s_stack.clear();
    s_pending_escape = false;
    s_pending_close_all = false;
    s_open_time = 0.0;
}

void MenuStack::Shutdown()
{
    CloseAllImmediate();
    s_show_doc = nullptr;
    s_hide_doc = nullptr;
    s_load_doc = nullptr;
    s_set_mode = nullptr;
    s_time_ptr = nullptr;
}

void MenuStack::Push(const std::string& path)
{
    // Load document if needed
    if (s_load_doc && !s_load_doc(path)) {
        Con_Printf("MenuStack::Push: Failed to load '%s'\n", path.c_str());
        return;
    }

    // Hide current menu if there is one
    if (!s_stack.empty() && s_hide_doc) {
        s_hide_doc(s_stack.back());
    }

    // Push and show new menu
    s_stack.push_back(path);
    if (s_show_doc) {
        s_show_doc(path);
    }

    // Set menu mode
    if (s_set_mode) {
        s_set_mode(UI_INPUT_MENU_ACTIVE);
    }

    // Record open time for debouncing
    if (s_time_ptr) {
        s_open_time = *s_time_ptr;
    }

    Con_Printf("MenuStack::Push: Opened '%s' (depth: %zu)\n", path.c_str(), s_stack.size());
}

void MenuStack::Pop()
{
    RequestEscape();
}

void MenuStack::RequestEscape()
{
    s_pending_escape = true;
}

void MenuStack::RequestCloseAll()
{
    s_pending_close_all = true;
}

void MenuStack::ProcessPending()
{
    if (s_pending_escape) {
        s_pending_escape = false;
        ProcessEscape();
    }

    if (s_pending_close_all) {
        s_pending_close_all = false;
        CloseAllImmediate();
    }
}

void MenuStack::ProcessEscape()
{
    // Debounce: prevent immediate close if menu was just opened
    if (s_time_ptr && (*s_time_ptr - s_open_time) < 0.1) {
        return;
    }

    if (s_stack.empty()) {
        // No menus, just ensure inactive mode
        if (s_set_mode) {
            s_set_mode(UI_INPUT_INACTIVE);
        }
        return;
    }

    // Pop and hide current menu
    std::string current = s_stack.back();
    s_stack.pop_back();

    if (s_hide_doc) {
        s_hide_doc(current);
    }
    Con_Printf("MenuStack::ProcessEscape: Closed '%s'\n", current.c_str());

    if (s_stack.empty()) {
        // Stack empty, return to game
        if (s_set_mode) {
            s_set_mode(UI_INPUT_INACTIVE);
        }
        Con_Printf("MenuStack::ProcessEscape: Stack empty, returning to game\n");
    } else {
        // Show previous menu
        if (s_show_doc) {
            s_show_doc(s_stack.back());
        }
    }
}

void MenuStack::CloseAllImmediate()
{
    while (!s_stack.empty()) {
        std::string current = s_stack.back();
        s_stack.pop_back();
        if (s_hide_doc) {
            s_hide_doc(current);
        }
    }

    if (s_set_mode) {
        s_set_mode(UI_INPUT_INACTIVE);
    }
}

bool MenuStack::IsEmpty()
{
    return s_stack.empty();
}

const std::string& MenuStack::Current()
{
    static const std::string empty;
    return s_stack.empty() ? empty : s_stack.back();
}

size_t MenuStack::Depth()
{
    return s_stack.size();
}

bool MenuStack::WantsInput()
{
    // If about to close all menus, report no input wanted
    if (s_pending_close_all) {
        return false;
    }

    // If escape pending and will close last menu, report no input wanted
    if (s_pending_escape && s_stack.size() <= 1) {
        return false;
    }

    return !s_stack.empty();
}

void MenuStack::SetTimeReference(const double* time_ptr)
{
    s_time_ptr = time_ptr;
}

} // namespace Tatoosh
