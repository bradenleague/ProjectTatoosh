/*
 * Tatoosh - RmlUI System Interface Implementation
 */

#include "system_interface.h"
#include <SDL.h>
#include <cstdio>

// Forward declare vkQuake's console print function
extern "C" {
    void Con_Printf(const char* fmt, ...);
    void Con_DPrintf(const char* fmt, ...);  // Debug print
}

namespace Tatoosh {

SystemInterface::SystemInterface()
    : m_engine_realtime(nullptr)
    , m_start_time(0.0)
{
}

SystemInterface::~SystemInterface()
{
}

void SystemInterface::Initialize(double* engine_realtime)
{
    m_engine_realtime = engine_realtime;
    if (m_engine_realtime) {
        m_start_time = *m_engine_realtime;
    } else {
        // Fallback to SDL time if engine time not available
        m_start_time = SDL_GetTicks() / 1000.0;
    }
}

double SystemInterface::GetElapsedTime()
{
    if (m_engine_realtime) {
        return *m_engine_realtime - m_start_time;
    }
    // Fallback to SDL
    return (SDL_GetTicks() / 1000.0) - m_start_time;
}

bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
    const char* type_str = "";
    switch (type) {
        case Rml::Log::LT_ALWAYS:   type_str = "";        break;
        case Rml::Log::LT_ERROR:    type_str = "ERROR: "; break;
        case Rml::Log::LT_WARNING:  type_str = "WARN: ";  break;
        case Rml::Log::LT_INFO:     type_str = "INFO: ";  break;
        case Rml::Log::LT_DEBUG:    type_str = "DEBUG: "; break;
        default: break;
    }

    // Use vkQuake's console for output
    if (type == Rml::Log::LT_DEBUG) {
        Con_DPrintf("[RmlUI] %s%s\n", type_str, message.c_str());
    } else {
        Con_Printf("[RmlUI] %s%s\n", type_str, message.c_str());
    }

    // Return true to continue execution (false would break into debugger)
    return true;
}

void SystemInterface::SetMouseCursor(const Rml::String& cursor_name)
{
    SDL_Cursor* cursor = nullptr;

    if (cursor_name.empty() || cursor_name == "arrow") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    } else if (cursor_name == "move") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    } else if (cursor_name == "pointer" || cursor_name == "hand") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    } else if (cursor_name == "resize" || cursor_name == "ew-resize") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    } else if (cursor_name == "ns-resize") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    } else if (cursor_name == "nesw-resize") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    } else if (cursor_name == "nwse-resize") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    } else if (cursor_name == "text" || cursor_name == "ibeam") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    } else if (cursor_name == "crosshair") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    } else if (cursor_name == "wait" || cursor_name == "progress") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    } else if (cursor_name == "not-allowed" || cursor_name == "no-drop") {
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
    }

    if (cursor) {
        SDL_SetCursor(cursor);
        // Note: SDL_FreeCursor should be called when done, but SDL manages
        // system cursors internally. For custom cursors, track and free them.
    }
}

void SystemInterface::SetClipboardText(const Rml::String& text)
{
    SDL_SetClipboardText(text.c_str());
}

void SystemInterface::GetClipboardText(Rml::String& text)
{
    char* clipboard = SDL_GetClipboardText();
    if (clipboard) {
        text = clipboard;
        SDL_free(clipboard);
    } else {
        text.clear();
    }
}

} // namespace Tatoosh
