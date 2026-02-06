/*
 * Tatoosh - Notification Model Implementation
 *
 * Manages centerprint and notify messages, exposing them to RmlUI
 * via bindings on the "game" data model.
 */

#include "notification_model.h"

extern "C" {
    void Con_Printf(const char* fmt, ...);
    double Cvar_VariableValue(const char* var_name);
    extern double realtime;
}

namespace Tatoosh {

// Static members
NotificationState NotificationModel::s_state;
Rml::DataModelHandle NotificationModel::s_model_handle;
bool NotificationModel::s_initialized = false;
bool NotificationModel::s_centerprint_was_visible = false;
bool NotificationModel::s_notify_was_visible[NUM_NOTIFY_LINES] = {};

// Bound strings — RmlUI binds to these pointers
static Rml::String s_centerprint_text;
static Rml::String s_notify_text[NUM_NOTIFY_LINES];

void NotificationModel::RegisterBindings(Rml::DataModelConstructor& constructor)
{
    // Centerprint text
    constructor.Bind("centerprint", &s_centerprint_text);

    // Centerprint visibility (computed)
    constructor.BindFunc("centerprint_visible",
        [](Rml::Variant& variant) {
            variant = !s_state.centerprint.empty() &&
                      (realtime < s_state.centerprint_expire);
        });

    // Notify lines — 4 fixed slots
    constructor.Bind("notify_0", &s_notify_text[0]);
    constructor.Bind("notify_1", &s_notify_text[1]);
    constructor.Bind("notify_2", &s_notify_text[2]);
    constructor.Bind("notify_3", &s_notify_text[3]);

    // Notify visibility (computed per-slot)
    for (int i = 0; i < NUM_NOTIFY_LINES; i++) {
        Rml::String name = "notify_" + std::to_string(i) + "_visible";
        int slot = i;
        constructor.BindFunc(name,
            [slot](Rml::Variant& variant) {
                double notifytime = Cvar_VariableValue("con_notifytime");
                if (notifytime <= 0.0) notifytime = 3.0;
                const auto& line = s_state.notify[slot];
                variant = !line.text.empty() &&
                          line.time > 0.0 &&
                          (realtime - line.time) < notifytime;
            });
    }

    s_initialized = true;
    Con_Printf("NotificationModel: Bindings registered\n");
}

void NotificationModel::SetModelHandle(Rml::DataModelHandle handle)
{
    s_model_handle = handle;
}

void NotificationModel::Shutdown()
{
    if (!s_initialized) return;

    s_state = NotificationState{};
    s_centerprint_text.clear();
    for (int i = 0; i < NUM_NOTIFY_LINES; i++) {
        s_notify_text[i].clear();
    }
    s_model_handle = Rml::DataModelHandle();
    s_initialized = false;
    s_centerprint_was_visible = false;
    for (int i = 0; i < NUM_NOTIFY_LINES; i++) {
        s_notify_was_visible[i] = false;
    }

    Con_Printf("NotificationModel: Shutdown\n");
}

void NotificationModel::Update(double real_time)
{
    if (!s_initialized || !s_model_handle) return;

    // Check centerprint visibility transition
    bool cp_visible = !s_state.centerprint.empty() &&
                      (real_time < s_state.centerprint_expire);

    if (cp_visible != s_centerprint_was_visible) {
        s_centerprint_text = cp_visible ? s_state.centerprint : "";
        s_model_handle.DirtyVariable("centerprint");
        s_model_handle.DirtyVariable("centerprint_visible");
        s_centerprint_was_visible = cp_visible;
    }

    // Check notify visibility transitions
    double notifytime = Cvar_VariableValue("con_notifytime");
    if (notifytime <= 0.0) notifytime = 3.0;

    for (int i = 0; i < NUM_NOTIFY_LINES; i++) {
        const auto& line = s_state.notify[i];
        bool visible = !line.text.empty() &&
                       line.time > 0.0 &&
                       (real_time - line.time) < notifytime;

        if (visible != s_notify_was_visible[i]) {
            s_notify_text[i] = visible ? line.text : "";
            s_model_handle.DirtyVariable("notify_" + std::to_string(i));
            s_model_handle.DirtyVariable("notify_" + std::to_string(i) + "_visible");
            s_notify_was_visible[i] = visible;
        }
    }
}

void NotificationModel::CenterPrint(const char* text, double real_time)
{
    if (!s_initialized || !text) return;

    double centertime = Cvar_VariableValue("scr_centertime");
    if (centertime <= 0.0) centertime = 2.0;

    s_state.centerprint = text;
    s_state.centerprint_start = real_time;
    s_state.centerprint_expire = real_time + centertime;

    // Immediately update bound string and dirty
    s_centerprint_text = text;
    s_centerprint_was_visible = true;

    if (s_model_handle) {
        s_model_handle.DirtyVariable("centerprint");
        s_model_handle.DirtyVariable("centerprint_visible");
    }
}

void NotificationModel::NotifyPrint(const char* text, double real_time)
{
    if (!s_initialized || !text || !text[0]) return;

    // Write into ring buffer at the head position
    int slot = s_state.notify_head;
    s_state.notify[slot].text = text;
    s_state.notify[slot].time = real_time;

    // Strip trailing newline for display
    auto& str = s_state.notify[slot].text;
    while (!str.empty() && (str.back() == '\n' || str.back() == '\r')) {
        str.pop_back();
    }

    // Update bound string and dirty
    s_notify_text[slot] = s_state.notify[slot].text;
    s_notify_was_visible[slot] = true;

    if (s_model_handle) {
        s_model_handle.DirtyVariable("notify_" + std::to_string(slot));
        s_model_handle.DirtyVariable("notify_" + std::to_string(slot) + "_visible");
    }

    // Advance ring buffer
    s_state.notify_head = (slot + 1) % NUM_NOTIFY_LINES;
}

} // namespace Tatoosh
