/*
 * Tatoosh - Menu Event Handler Implementation
 *
 * Handles UI events and executes Quake commands.
 */

#include "menu_event_handler.h"
#include "cvar_binding.h"
#include "quake_command_executor.h"

#include <cctype>

extern "C" {
    // Quake engine functions
    void Con_Printf(const char* fmt, ...);

    // UI functions (from ui_manager.h)
    void UI_PushMenu(const char* path);
    void UI_PopMenu(void);
    void UI_SetInputMode(int mode);
    int UI_WantsMenuInput(void);
    void UI_HandleEscape(void);
    void UI_CloseAllMenusImmediate(void);

    // Input mode for returning to game
    void IN_Activate(void);

    // Key dest for menu state
    extern int key_dest;
    #define key_game 0
    #define key_console 1
    #define key_menu 2
}

namespace Tatoosh {

namespace {

std::string TrimWhitespace(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }

    if (start == value.size()) {
        return "";
    }

    size_t end = value.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(value[end]))) {
        end--;
    }

    return value.substr(start, end - start + 1);
}

std::vector<std::string> SplitActions(const std::string& action)
{
    std::vector<std::string> parts;
    std::string current;
    char quote = '\0';

    for (size_t i = 0; i < action.size(); i++) {
        char c = action[i];
        if (c == '\'' || c == '"') {
            if (quote == '\0') {
                quote = c;
            } else if (quote == c) {
                quote = '\0';
            }
        }

        if (c == ';' && quote == '\0') {
            std::string trimmed = TrimWhitespace(current);
            if (!trimmed.empty()) {
                parts.push_back(trimmed);
            }
            current.clear();
            continue;
        }

        current.push_back(c);
    }

    std::string trimmed = TrimWhitespace(current);
    if (!trimmed.empty()) {
        parts.push_back(trimmed);
    }

    return parts;
}

} // namespace

// Static member definitions
MenuEventHandler MenuEventHandler::s_instance;
Rml::Context* MenuEventHandler::s_context = nullptr;
KeyCaptureCallback MenuEventHandler::s_key_callback;
bool MenuEventHandler::s_capturing_key = false;
std::string MenuEventHandler::s_key_action;
bool MenuEventHandler::s_initialized = false;
ICommandExecutor* MenuEventHandler::s_executor = nullptr;

void MenuEventHandler::SetExecutor(ICommandExecutor* executor)
{
    s_executor = executor;
}

ICommandExecutor* MenuEventHandler::GetExecutor()
{
    // Return injected executor, or default to QuakeCommandExecutor
    if (!s_executor) {
        s_executor = &QuakeCommandExecutor::Instance();
    }
    return s_executor;
}

MenuEventHandler& MenuEventHandler::Instance()
{
    return s_instance;
}

bool MenuEventHandler::Initialize(Rml::Context* context)
{
    if (s_initialized) {
        Con_Printf("MenuEventHandler: Already initialized\n");
        return true;
    }

    if (!context) {
        Con_Printf("MenuEventHandler: ERROR - null context\n");
        return false;
    }

    s_context = context;

    // NOTE: We intentionally do NOT register a custom event listener instancer here.
    // RmlUI automatically attaches listeners for any "on*" attributes (onclick, onchange, etc.)
    // via the global EventListenerInstancer. If we also attach our document-level listeners,
    // actions will fire twice (once via the instancer, once via MenuEventHandler::ProcessEvent),
    // which causes duplicate menu pushes and double-back behavior.

    s_initialized = true;
    Con_Printf("MenuEventHandler: Initialized\n");
    return true;
}

void MenuEventHandler::Shutdown()
{
    if (!s_initialized) return;

    s_context = nullptr;
    s_key_callback = nullptr;
    s_capturing_key = false;
    s_key_action.clear();
    s_initialized = false;

    Con_Printf("MenuEventHandler: Shutdown\n");
}

void MenuEventHandler::RegisterWithDocument(Rml::ElementDocument* document)
{
    if (!document) return;

    const Rml::String bound_flag = "data-menu-event-handler-bound";
    if (document->HasAttribute(bound_flag)) {
        return;
    }

    document->AddEventListener(Rml::EventId::Click, &s_instance, true);
    // Let data-value controllers update model values before we handle change events.
    document->AddEventListener(Rml::EventId::Change, &s_instance, false);
    document->SetAttribute(bound_flag, "1");
}

void MenuEventHandler::ProcessAction(const std::string& action)
{
    s_instance.ExecuteAction(action);
}

void MenuEventHandler::SetKeyCaptureCallback(KeyCaptureCallback callback)
{
    s_key_callback = callback;
}

void MenuEventHandler::ClearKeyCaptureCallback()
{
    s_key_callback = nullptr;
    s_capturing_key = false;
    s_key_action.clear();
}

bool MenuEventHandler::IsCapturingKey()
{
    return s_capturing_key;
}

void MenuEventHandler::OnKeyCaptured(int key, const char* key_name)
{
    if (!s_capturing_key) return;

    s_capturing_key = false;

    if (s_key_callback) {
        s_key_callback(key, key_name);
    }

    // Execute the bind command
    if (!s_key_action.empty() && key_name) {
        std::string cmd = "bind \"" + std::string(key_name) + "\" \"" + s_key_action + "\"";
        GetExecutor()->Execute(cmd);
        Con_Printf("MenuEventHandler: Bound '%s' to '%s'\n", key_name, s_key_action.c_str());
    }

    s_key_action.clear();
}

void MenuEventHandler::ProcessEvent(Rml::Event& event)
{
    // Get the action from the element's attribute
    Rml::Element* element = event.GetTargetElement();
    if (!element) return;

    const Rml::String& event_type = event.GetType();

    Rml::String action;
    if (!event_type.empty()) {
        const Rml::String data_event_attr = "data-event-" + event_type;
        for (Rml::Element* current = element; current != nullptr; current = current->GetParentNode()) {
            action = current->GetAttribute<Rml::String>(data_event_attr, "");
            if (!action.empty()) {
                break;
            }
        }
    }

    if (action.empty() && !event_type.empty()) {
        const Rml::String on_event_attr = "on" + event_type;
        for (Rml::Element* current = element; current != nullptr; current = current->GetParentNode()) {
            action = current->GetAttribute<Rml::String>(on_event_attr, "");
            if (!action.empty()) {
                break;
            }
        }
    }

    if (action.empty()) {
        for (Rml::Element* current = element; current != nullptr; current = current->GetParentNode()) {
            action = current->GetAttribute<Rml::String>("data-action", "");
            if (!action.empty()) {
                break;
            }
            action = current->GetAttribute<Rml::String>("onclick", "");
            if (!action.empty()) {
                break;
            }
        }
    }

    if (action.empty()) {
        return;
    }

    Con_Printf("MenuEventHandler: event=%s action=%s target=%s id=%s\n",
               event.GetType().c_str(),
               action.c_str(),
               element->GetTagName().c_str(),
               element->GetId().c_str());

    ExecuteAction(action.c_str());
}

void MenuEventHandler::ExecuteAction(const std::string& action)
{
    if (action.empty()) return;

    if (action.find(';') != std::string::npos) {
        auto actions = SplitActions(action);
        if (actions.size() > 1) {
            for (const auto& item : actions) {
                ExecuteAction(item);
            }
            return;
        }
    }

    // Parse action type
    size_t paren_pos = action.find('(');
    std::string func_name = (paren_pos != std::string::npos)
        ? action.substr(0, paren_pos)
        : action;

    // Dispatch to handler
    if (func_name == "navigate") {
        ActionNavigate(ExtractArg(action));
    }
    else if (func_name == "command") {
        ActionCommand(ExtractArg(action));
    }
    else if (func_name == "cvar_changed") {
        ActionCvarChanged(ExtractArg(action));
    }
    else if (func_name == "cycle_cvar") {
        auto args = ExtractTwoArgs(action);
        ActionCycleCvar(args.first, args.second);
    }
    else if (func_name == "close") {
        ActionClose();
    }
    else if (func_name == "close_all") {
        ActionCloseAll();
    }
    else if (func_name == "quit") {
        ActionQuit();
    }
    else if (func_name == "new_game") {
        ActionNewGame();
    }
    else if (func_name == "load_game") {
        ActionLoadGame(ExtractArg(action));
    }
    else if (func_name == "save_game") {
        ActionSaveGame(ExtractArg(action));
    }
    else if (func_name == "bind_key") {
        ActionBindKey(ExtractArg(action));
    }
    else if (func_name == "main_menu") {
        ActionMainMenu();
    }
    else {
        Con_Printf("MenuEventHandler: Unknown action '%s'\n", func_name.c_str());
    }
}

std::string MenuEventHandler::ExtractArg(const std::string& action)
{
    // Extract argument from "func('arg')" or "func(arg)"
    size_t start = action.find('(');
    if (start == std::string::npos) return "";

    start++;
    size_t end = action.rfind(')');
    if (end == std::string::npos || end <= start) return "";

    std::string arg = action.substr(start, end - start);

    // Remove surrounding quotes if present
    if (arg.length() >= 2) {
        if ((arg.front() == '\'' && arg.back() == '\'') ||
            (arg.front() == '"' && arg.back() == '"')) {
            arg = arg.substr(1, arg.length() - 2);
        }
    }

    return arg;
}

std::pair<std::string, int> MenuEventHandler::ExtractTwoArgs(const std::string& action)
{
    // Extract two arguments from "func('arg1', 2)"
    size_t start = action.find('(');
    if (start == std::string::npos) return {"", 0};

    start++;
    size_t end = action.rfind(')');
    if (end == std::string::npos || end <= start) return {"", 0};

    std::string args = action.substr(start, end - start);

    // Find comma separator
    size_t comma = args.find(',');
    if (comma == std::string::npos) {
        // Single arg, default delta to 1
        std::string arg1 = args;
        if (arg1.length() >= 2 &&
            ((arg1.front() == '\'' && arg1.back() == '\'') ||
             (arg1.front() == '"' && arg1.back() == '"'))) {
            arg1 = arg1.substr(1, arg1.length() - 2);
        }
        return {arg1, 1};
    }

    std::string arg1 = args.substr(0, comma);
    std::string arg2 = args.substr(comma + 1);

    // Trim whitespace and quotes from arg1
    while (!arg1.empty() && (arg1.front() == ' ' || arg1.front() == '\'' || arg1.front() == '"'))
        arg1.erase(arg1.begin());
    while (!arg1.empty() && (arg1.back() == ' ' || arg1.back() == '\'' || arg1.back() == '"'))
        arg1.pop_back();

    // Trim whitespace from arg2 and convert to int
    while (!arg2.empty() && arg2.front() == ' ')
        arg2.erase(arg2.begin());
    while (!arg2.empty() && arg2.back() == ' ')
        arg2.pop_back();

    int delta = 1;
    try {
        delta = std::stoi(arg2);
    } catch (...) {
        delta = 1;
    }

    return {arg1, delta};
}

void MenuEventHandler::ActionNavigate(const std::string& menu_path)
{
    if (menu_path.empty()) {
        Con_Printf("MenuEventHandler: navigate() requires menu path\n");
        return;
    }

    // Build full path if needed
    std::string full_path = menu_path;
    if (menu_path.find('/') == std::string::npos) {
        full_path = "ui/rml/menus/" + menu_path + ".rml";
    }

    Con_Printf("MenuEventHandler: Navigating to '%s'\n", full_path.c_str());
    UI_PushMenu(full_path.c_str());
}

void MenuEventHandler::ActionCommand(const std::string& command)
{
    if (command.empty()) {
        Con_Printf("MenuEventHandler: command() requires command string\n");
        return;
    }

    Con_Printf("MenuEventHandler: Executing '%s'\n", command.c_str());

    GetExecutor()->Execute(command);
}

void MenuEventHandler::ActionCvarChanged(const std::string& ui_name)
{
    if (ui_name.empty()) return;

    if (CvarBindingManager::ShouldIgnoreUIChange()) {
        return;
    }

    Con_Printf("MenuEventHandler: Cvar changed '%s'\n", ui_name.c_str());
    CvarBindingManager::SyncFromUI(ui_name);
}

void MenuEventHandler::ActionCycleCvar(const std::string& ui_name, int delta)
{
    if (ui_name.empty()) return;

    Con_Printf("MenuEventHandler: Cycling '%s' by %d\n", ui_name.c_str(), delta);
    CvarBindingManager::CycleEnum(ui_name, delta);
}

void MenuEventHandler::ActionClose()
{
    Con_Printf("MenuEventHandler: Closing current menu\n");
    UI_PopMenu();
}

void MenuEventHandler::ActionCloseAll()
{
    Con_Printf("MenuEventHandler: Closing all menus\n");

    // Close all menus immediately - we're already in the update phase
    // (called from RmlUI event handler), so this is safe.
    UI_CloseAllMenusImmediate();

    // Ensure we're back to game mode
    UI_SetInputMode(0);  // UI_INPUT_INACTIVE
    IN_Activate();
    key_dest = key_game;
}

void MenuEventHandler::ActionQuit()
{
    Con_Printf("MenuEventHandler: Quitting game\n");

    // Close menus and force console input to bypass Quake's native quit menu.
    UI_CloseAllMenusImmediate();
    UI_SetInputMode(0);  // UI_INPUT_INACTIVE
    IN_Activate();
    key_dest = key_console;

    GetExecutor()->ExecuteImmediate("quit");
}

void MenuEventHandler::ActionNewGame()
{
    Con_Printf("MenuEventHandler: Starting new game\n");

    // Close menus first
    ActionCloseAll();

    // Start new game (will show skill selection or start immediately)
    GetExecutor()->Execute("maxplayers 1");
    GetExecutor()->Execute("deathmatch 0");
    GetExecutor()->Execute("coop 0");
    GetExecutor()->Execute("map start");
}

void MenuEventHandler::ActionLoadGame(const std::string& slot)
{
    if (slot.empty()) {
        Con_Printf("MenuEventHandler: load_game() requires slot name\n");
        return;
    }

    Con_Printf("MenuEventHandler: Loading game from '%s'\n", slot.c_str());
    ActionCloseAll();

    GetExecutor()->Execute("load " + slot);
}

void MenuEventHandler::ActionSaveGame(const std::string& slot)
{
    if (slot.empty()) {
        Con_Printf("MenuEventHandler: save_game() requires slot name\n");
        return;
    }

    Con_Printf("MenuEventHandler: Saving game to '%s'\n", slot.c_str());

    GetExecutor()->Execute("save " + slot);
}

void MenuEventHandler::ActionBindKey(const std::string& action)
{
    if (action.empty()) {
        Con_Printf("MenuEventHandler: bind_key() requires action name\n");
        return;
    }

    Con_Printf("MenuEventHandler: Waiting for key to bind '%s'\n", action.c_str());

    s_capturing_key = true;
    s_key_action = action;

    // The actual key capture is handled by the input system
    // which will call OnKeyCaptured when a key is pressed
}

void MenuEventHandler::ActionMainMenu()
{
    Con_Printf("MenuEventHandler: Returning to main menu\n");

    // Close all menus immediately
    UI_CloseAllMenusImmediate();

    // Use "demos" command to properly disconnect and restart demo loop
    // This handles: disconnect, shutdown server, and CL_NextDemo()
    GetExecutor()->Execute("demos");

    // Show main menu
    UI_PushMenu("ui/rml/menus/main_menu.rml");
}

// ActionEventListener implementation
void ActionEventListener::ProcessEvent(Rml::Event& event)
{
    MenuEventHandler::ProcessAction(m_action.c_str());
}

void ActionEventListener::OnDetach(Rml::Element* element)
{
    // Listener will be cleaned up by MenuEventInstancer
}

// MenuEventInstancer implementation
Rml::EventListener* MenuEventInstancer::InstanceEventListener(const Rml::String& value,
                                                               Rml::Element* element)
{
    // Create a new listener and store it - we must manage the lifetime ourselves
    // RmlUI does NOT take ownership of the returned listener
    auto listener = std::make_unique<ActionEventListener>(value);
    ActionEventListener* ptr = listener.get();
    m_listeners.push_back(std::move(listener));
    return ptr;
}

void MenuEventInstancer::ReleaseAllListeners()
{
    m_listeners.clear();
}

} // namespace Tatoosh

// C API Implementation
extern "C" {

int MenuEventHandler_Init(void)
{
    // Deferred initialization
    return 1;
}

void MenuEventHandler_Shutdown(void)
{
    Tatoosh::MenuEventHandler::Shutdown();
}

void MenuEventHandler_ProcessAction(const char* action)
{
    if (action) {
        Tatoosh::MenuEventHandler::ProcessAction(action);
    }
}

int MenuEventHandler_IsCapturingKey(void)
{
    return Tatoosh::MenuEventHandler::IsCapturingKey() ? 1 : 0;
}

void MenuEventHandler_OnKeyCaptured(int key, const char* key_name)
{
    Tatoosh::MenuEventHandler::OnKeyCaptured(key, key_name);
}

} // extern "C"
