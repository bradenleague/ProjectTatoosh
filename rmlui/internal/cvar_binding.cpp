/*
 * Tatoosh - Cvar Binding Manager Implementation
 *
 * Two-way sync between RmlUI data model and Quake cvars.
 */

#include "cvar_binding.h"
#include "quake_cvar_provider.h"

#include <algorithm>
#include <cmath>

extern "C" {
    void Con_Printf(const char* fmt, ...);
}

namespace Tatoosh {

// Static member definitions
Rml::Context* CvarBindingManager::s_context = nullptr;
Rml::DataModelHandle CvarBindingManager::s_model_handle;
std::unordered_map<std::string, CvarBinding> CvarBindingManager::s_bindings;
std::unordered_map<std::string, std::unique_ptr<float>> CvarBindingManager::s_float_values;
std::unordered_map<std::string, std::unique_ptr<int>> CvarBindingManager::s_int_values;
std::unordered_map<std::string, std::unique_ptr<Rml::String>> CvarBindingManager::s_string_values;
bool CvarBindingManager::s_initialized = false;
ICvarProvider* CvarBindingManager::s_provider = nullptr;
bool CvarBindingManager::s_ignore_ui_changes = false;
int CvarBindingManager::s_ignore_ui_changes_frames = 0;

namespace {

bool IsInvertMouseBinding(const std::string& ui_name)
{
    return ui_name == "invert_mouse";
}

int GetInvertMouseValue(ICvarProvider* provider)
{
    if (!provider) return 0;
    return provider->GetFloat("m_pitch") < 0.0f ? 1 : 0;
}

void SetInvertMouseValue(ICvarProvider* provider, bool inverted)
{
    if (!provider) return;
    float pitch = provider->GetFloat("m_pitch");
    float magnitude = std::fabs(pitch);
    if (magnitude < 0.0001f) {
        magnitude = 0.022f;
    }
    provider->SetFloat("m_pitch", inverted ? -magnitude : magnitude);
}

} // namespace

void CvarBindingManager::SetProvider(ICvarProvider* provider)
{
    s_provider = provider;
}

ICvarProvider* CvarBindingManager::GetProvider()
{
    // Return injected provider, or default to QuakeCvarProvider
    if (!s_provider) {
        s_provider = &QuakeCvarProvider::Instance();
    }
    return s_provider;
}

bool CvarBindingManager::Initialize(Rml::Context* context)
{
    if (s_initialized) {
        Con_Printf("CvarBindingManager: Already initialized\n");
        return true;
    }

    if (!context) {
        Con_Printf("CvarBindingManager: ERROR - null context\n");
        return false;
    }

    Rml::DataModelConstructor constructor = context->CreateDataModel("cvars");
    if (!constructor) {
        Con_Printf("CvarBindingManager: ERROR - Failed to create data model\n");
        return false;
    }

    // The actual bindings will be registered later via Register* functions
    // We'll bind scalar getters that look up values from our maps

    s_model_handle = constructor.GetModelHandle();
    s_context = context;
    s_initialized = true;

    Con_Printf("CvarBindingManager: Initialized successfully\n");
    return true;
}

void CvarBindingManager::Shutdown()
{
    if (!s_initialized) return;

    s_bindings.clear();
    s_float_values.clear();
    s_int_values.clear();
    s_string_values.clear();
    s_model_handle = Rml::DataModelHandle();
    s_context = nullptr;
    s_initialized = false;
    s_ignore_ui_changes = false;
    s_ignore_ui_changes_frames = 0;

    Con_Printf("CvarBindingManager: Shutdown\n");
}

void CvarBindingManager::RegisterFloat(const char* cvar, const char* ui_name,
                                        float min, float max, float step)
{
    if (!s_initialized) return;

    CvarBinding binding;
    binding.cvar_name = cvar;
    binding.ui_name = ui_name;
    binding.type = CvarType::Float;
    binding.min_value = min;
    binding.max_value = max;
    binding.step = step;

    s_bindings[ui_name] = binding;

    // Initialize with current cvar value
    float value = GetProvider()->GetFloat(cvar);
    auto it = s_float_values.find(ui_name);
    if (it == s_float_values.end()) {
        auto value_ptr = std::make_unique<float>(value);
        float* raw_ptr = value_ptr.get();
        s_float_values[ui_name] = std::move(value_ptr);

        if (s_context) {
            Rml::DataModelConstructor constructor = s_context->GetDataModel("cvars");
            if (constructor && !constructor.Bind(ui_name, raw_ptr)) {
                Con_Printf("CvarBindingManager: ERROR - Failed to bind float '%s'\n", ui_name);
            }
        }
    } else if (it->second) {
        *(it->second) = value;
    }

    Con_Printf("CvarBindingManager: Registered float '%s' -> '%s' (%.2f-%.2f)\n",
               cvar, ui_name, min, max);
}

void CvarBindingManager::RegisterBool(const char* cvar, const char* ui_name)
{
    if (!s_initialized) return;

    CvarBinding binding;
    binding.cvar_name = cvar;
    binding.ui_name = ui_name;
    binding.type = CvarType::Bool;

    s_bindings[ui_name] = binding;

    // Initialize with current cvar value
    int value = IsInvertMouseBinding(ui_name)
        ? GetInvertMouseValue(GetProvider())
        : static_cast<int>(GetProvider()->GetFloat(cvar));
    auto it = s_int_values.find(ui_name);
    if (it == s_int_values.end()) {
        auto value_ptr = std::make_unique<int>(value);
        int* raw_ptr = value_ptr.get();
        s_int_values[ui_name] = std::move(value_ptr);

        if (s_context) {
            Rml::DataModelConstructor constructor = s_context->GetDataModel("cvars");
            if (constructor && !constructor.Bind(ui_name, raw_ptr)) {
                Con_Printf("CvarBindingManager: ERROR - Failed to bind bool '%s'\n", ui_name);
            }
        }
    } else if (it->second) {
        *(it->second) = value;
    }

    Con_Printf("CvarBindingManager: Registered bool '%s' -> '%s'\n", cvar, ui_name);
}

void CvarBindingManager::RegisterInt(const char* cvar, const char* ui_name,
                                      int min, int max)
{
    if (!s_initialized) return;

    CvarBinding binding;
    binding.cvar_name = cvar;
    binding.ui_name = ui_name;
    binding.type = CvarType::Int;
    binding.min_value = static_cast<float>(min);
    binding.max_value = static_cast<float>(max);

    s_bindings[ui_name] = binding;

    // Initialize with current cvar value
    int value = static_cast<int>(GetProvider()->GetFloat(cvar));
    auto it = s_int_values.find(ui_name);
    if (it == s_int_values.end()) {
        auto value_ptr = std::make_unique<int>(value);
        int* raw_ptr = value_ptr.get();
        s_int_values[ui_name] = std::move(value_ptr);

        if (s_context) {
            Rml::DataModelConstructor constructor = s_context->GetDataModel("cvars");
            if (constructor && !constructor.Bind(ui_name, raw_ptr)) {
                Con_Printf("CvarBindingManager: ERROR - Failed to bind int '%s'\n", ui_name);
            }
        }
    } else if (it->second) {
        *(it->second) = value;
    }

    Con_Printf("CvarBindingManager: Registered int '%s' -> '%s' (%d-%d)\n",
               cvar, ui_name, min, max);
}

void CvarBindingManager::RegisterEnum(const char* cvar, const char* ui_name,
                                       int num_values, const char** labels)
{
    if (!s_initialized) return;

    CvarBinding binding;
    binding.cvar_name = cvar;
    binding.ui_name = ui_name;
    binding.type = CvarType::Enum;
    binding.num_values = num_values;
    binding.min_value = 0.0f;
    binding.max_value = static_cast<float>(std::max(0, num_values - 1));
    binding.enum_values.reserve(num_values);
    for (int i = 0; i < num_values; i++) {
        binding.enum_values.push_back(i);
    }

    if (labels) {
        for (int i = 0; i < num_values; i++) {
            binding.enum_labels.push_back(labels[i]);
        }
    }

    s_bindings[ui_name] = binding;

    // Initialize with current cvar value
    int value = static_cast<int>(GetProvider()->GetFloat(cvar));
    auto it = s_int_values.find(ui_name);
    if (it == s_int_values.end()) {
        auto value_ptr = std::make_unique<int>(value);
        int* raw_ptr = value_ptr.get();
        s_int_values[ui_name] = std::move(value_ptr);

        if (s_context) {
            Rml::DataModelConstructor constructor = s_context->GetDataModel("cvars");
            if (constructor && !constructor.Bind(ui_name, raw_ptr)) {
                Con_Printf("CvarBindingManager: ERROR - Failed to bind enum '%s'\n", ui_name);
            }
        }
    } else if (it->second) {
        *(it->second) = value;
    }

    Con_Printf("CvarBindingManager: Registered enum '%s' -> '%s' (%d values)\n",
               cvar, ui_name, num_values);
}

void CvarBindingManager::RegisterEnumValues(const char* cvar, const char* ui_name,
                                            const std::vector<int>& values, const char** labels)
{
    if (!s_initialized) return;
    if (values.empty()) {
        Con_Printf("CvarBindingManager: ERROR - Enum values list is empty for '%s'\n", ui_name);
        return;
    }

    CvarBinding binding;
    binding.cvar_name = cvar;
    binding.ui_name = ui_name;
    binding.type = CvarType::Enum;
    binding.num_values = static_cast<int>(values.size());
    binding.enum_values = values;

    auto minmax = std::minmax_element(values.begin(), values.end());
    binding.min_value = static_cast<float>(*minmax.first);
    binding.max_value = static_cast<float>(*minmax.second);

    if (labels) {
        for (int i = 0; i < binding.num_values; i++) {
            binding.enum_labels.push_back(labels[i]);
        }
    }

    s_bindings[ui_name] = binding;

    int value = static_cast<int>(GetProvider()->GetFloat(cvar));
    auto it = s_int_values.find(ui_name);
    if (it == s_int_values.end()) {
        auto value_ptr = std::make_unique<int>(value);
        int* raw_ptr = value_ptr.get();
        s_int_values[ui_name] = std::move(value_ptr);

        if (s_context) {
            Rml::DataModelConstructor constructor = s_context->GetDataModel("cvars");
            if (constructor && !constructor.Bind(ui_name, raw_ptr)) {
                Con_Printf("CvarBindingManager: ERROR - Failed to bind enum '%s'\n", ui_name);
            }
        }
    } else if (it->second) {
        *(it->second) = value;
    }

    Con_Printf("CvarBindingManager: Registered enum values '%s' -> '%s' (%d values)\n",
               cvar, ui_name, binding.num_values);
}

void CvarBindingManager::RegisterString(const char* cvar, const char* ui_name)
{
    if (!s_initialized) return;

    CvarBinding binding;
    binding.cvar_name = cvar;
    binding.ui_name = ui_name;
    binding.type = CvarType::String;

    s_bindings[ui_name] = binding;

    // Initialize with current cvar value
    Rml::String value = GetProvider()->GetString(cvar);
    auto it = s_string_values.find(ui_name);
    if (it == s_string_values.end()) {
        auto value_ptr = std::make_unique<Rml::String>(value);
        Rml::String* raw_ptr = value_ptr.get();
        s_string_values[ui_name] = std::move(value_ptr);

        if (s_context) {
            Rml::DataModelConstructor constructor = s_context->GetDataModel("cvars");
            if (constructor && !constructor.Bind(ui_name, raw_ptr)) {
                Con_Printf("CvarBindingManager: ERROR - Failed to bind string '%s'\n", ui_name);
            }
        }
    } else if (it->second) {
        *(it->second) = value;
    }

    Con_Printf("CvarBindingManager: Registered string '%s' -> '%s'\n", cvar, ui_name);
}

void CvarBindingManager::SyncToUI()
{
    if (!s_initialized) return;

    // Suppress UI change handling for the next update tick. Data binding updates
    // can emit "change" events while syncing values to the UI.
    s_ignore_ui_changes = true;
    s_ignore_ui_changes_frames = 1;

    for (auto& pair : s_bindings) {
        const CvarBinding& binding = pair.second;
        const char* cvar_name = binding.cvar_name.c_str();

        switch (binding.type) {
            case CvarType::Float:
                if (auto it = s_float_values.find(binding.ui_name); it != s_float_values.end() && it->second) {
                    *(it->second) = GetProvider()->GetFloat(cvar_name);
                }
                break;
            case CvarType::Bool:
            case CvarType::Int:
            case CvarType::Enum:
                if (auto it = s_int_values.find(binding.ui_name); it != s_int_values.end() && it->second) {
                    if (IsInvertMouseBinding(binding.ui_name)) {
                        *(it->second) = GetInvertMouseValue(GetProvider());
                    } else {
                        *(it->second) = static_cast<int>(GetProvider()->GetFloat(cvar_name));
                    }
                }
                break;
            case CvarType::String: {
                if (auto it = s_string_values.find(binding.ui_name); it != s_string_values.end() && it->second) {
                    *(it->second) = GetProvider()->GetString(cvar_name);
                }
                break;
            }
        }
    }

    MarkDirty();
    Con_Printf("CvarBindingManager: Synced %zu cvars to UI\n", s_bindings.size());
}

bool CvarBindingManager::ShouldIgnoreUIChange()
{
    return s_ignore_ui_changes;
}

void CvarBindingManager::NotifyUIUpdateComplete()
{
    if (s_ignore_ui_changes_frames > 0) {
        s_ignore_ui_changes_frames--;
        if (s_ignore_ui_changes_frames == 0) {
            s_ignore_ui_changes = false;
        }
    }
}

void CvarBindingManager::SyncFromUI(const std::string& ui_name)
{
    if (!s_initialized) return;

    auto it = s_bindings.find(ui_name);
    if (it == s_bindings.end()) {
        Con_Printf("CvarBindingManager: Unknown UI binding '%s'\n", ui_name.c_str());
        return;
    }

    const CvarBinding& binding = it->second;
    const char* cvar_name = binding.cvar_name.c_str();

    switch (binding.type) {
        case CvarType::Float: {
            auto val_it = s_float_values.find(ui_name);
            if (val_it != s_float_values.end() && val_it->second) {
                GetProvider()->SetFloat(cvar_name, *(val_it->second));
            }
            break;
        }
        case CvarType::Bool:
        case CvarType::Int:
        case CvarType::Enum: {
            auto val_it = s_int_values.find(ui_name);
            if (val_it != s_int_values.end() && val_it->second) {
                if (IsInvertMouseBinding(binding.ui_name)) {
                    SetInvertMouseValue(GetProvider(), *(val_it->second) != 0);
                } else {
                    GetProvider()->SetFloat(cvar_name, static_cast<float>(*(val_it->second)));
                }
            }
            break;
        }
        case CvarType::String: {
            auto val_it = s_string_values.find(ui_name);
            if (val_it != s_string_values.end() && val_it->second) {
                GetProvider()->SetString(cvar_name, val_it->second->c_str());
            }
            break;
        }
    }
}

void CvarBindingManager::SyncAllFromUI()
{
    if (!s_initialized) return;

    for (auto& pair : s_bindings) {
        SyncFromUI(pair.first);
    }
}

const CvarBinding* CvarBindingManager::GetBinding(const std::string& ui_name)
{
    auto it = s_bindings.find(ui_name);
    if (it != s_bindings.end()) {
        return &it->second;
    }
    return nullptr;
}

void CvarBindingManager::MarkDirty()
{
    if (s_initialized && s_model_handle) {
        s_model_handle.DirtyAllVariables();
    }
}

bool CvarBindingManager::IsInitialized()
{
    return s_initialized;
}

float CvarBindingManager::GetFloatValue(const std::string& ui_name)
{
    auto it = s_float_values.find(ui_name);
    if (it != s_float_values.end() && it->second) {
        return *(it->second);
    }
    return 0.0f;
}

void CvarBindingManager::SetFloatValue(const std::string& ui_name, float value)
{
    auto binding = GetBinding(ui_name);
    if (binding) {
        // Clamp to range
        if (value < binding->min_value) value = binding->min_value;
        if (value > binding->max_value) value = binding->max_value;
    }
    auto it = s_float_values.find(ui_name);
    if (it != s_float_values.end() && it->second) {
        *(it->second) = value;
    }
    SyncFromUI(ui_name);
    MarkDirty();
}

bool CvarBindingManager::GetBoolValue(const std::string& ui_name)
{
    auto it = s_int_values.find(ui_name);
    if (it != s_int_values.end() && it->second) {
        return *(it->second) != 0;
    }
    return false;
}

void CvarBindingManager::SetBoolValue(const std::string& ui_name, bool value)
{
    auto it = s_int_values.find(ui_name);
    if (it != s_int_values.end() && it->second) {
        *(it->second) = value ? 1 : 0;
    }
    SyncFromUI(ui_name);
    MarkDirty();
}

int CvarBindingManager::GetIntValue(const std::string& ui_name)
{
    auto it = s_int_values.find(ui_name);
    if (it != s_int_values.end() && it->second) {
        return *(it->second);
    }
    return 0;
}

void CvarBindingManager::SetIntValue(const std::string& ui_name, int value)
{
    auto binding = GetBinding(ui_name);
    if (binding && binding->type == CvarType::Int) {
        // Clamp to range
        int min_val = static_cast<int>(binding->min_value);
        int max_val = static_cast<int>(binding->max_value);
        if (value < min_val) value = min_val;
        if (value > max_val) value = max_val;
    }
    auto it = s_int_values.find(ui_name);
    if (it != s_int_values.end() && it->second) {
        *(it->second) = value;
    }
    SyncFromUI(ui_name);
    MarkDirty();
}

Rml::String CvarBindingManager::GetStringValue(const std::string& ui_name)
{
    auto it = s_string_values.find(ui_name);
    if (it != s_string_values.end() && it->second) {
        return *(it->second);
    }
    return "";
}

void CvarBindingManager::SetStringValue(const std::string& ui_name, const Rml::String& value)
{
    auto it = s_string_values.find(ui_name);
    if (it != s_string_values.end() && it->second) {
        *(it->second) = value;
    }
    SyncFromUI(ui_name);
    MarkDirty();
}

void CvarBindingManager::CycleEnum(const std::string& ui_name, int delta)
{
    auto binding = GetBinding(ui_name);
    if (!binding) return;

    if (binding->type == CvarType::Bool) {
        SetBoolValue(ui_name, !GetBoolValue(ui_name));
        return;
    }

    if (binding->type != CvarType::Enum) return;

    if (binding->enum_values.empty()) return;

    int current = GetIntValue(ui_name);
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < binding->enum_values.size(); i++) {
        if (binding->enum_values[i] == current) {
            index = i;
            found = true;
            break;
        }
    }

    if (!found) {
        index = 0;
    }

    int size = static_cast<int>(binding->enum_values.size());
    int new_index = (static_cast<int>(index) + delta) % size;
    if (new_index < 0) new_index += size;

    SetIntValue(ui_name, binding->enum_values[static_cast<size_t>(new_index)]);
}

} // namespace Tatoosh

// C API Implementation
extern "C" {

int CvarBinding_Init(void)
{
    // Deferred initialization - called after context is ready
    return 1;
}

void CvarBinding_Shutdown(void)
{
    Tatoosh::CvarBindingManager::Shutdown();
}

void CvarBinding_RegisterFloat(const char* cvar, const char* ui_name,
                                float min, float max, float step)
{
    Tatoosh::CvarBindingManager::RegisterFloat(cvar, ui_name, min, max, step);
}

void CvarBinding_RegisterBool(const char* cvar, const char* ui_name)
{
    Tatoosh::CvarBindingManager::RegisterBool(cvar, ui_name);
}

void CvarBinding_RegisterInt(const char* cvar, const char* ui_name, int min, int max)
{
    Tatoosh::CvarBindingManager::RegisterInt(cvar, ui_name, min, max);
}

void CvarBinding_RegisterEnum(const char* cvar, const char* ui_name, int num_values)
{
    Tatoosh::CvarBindingManager::RegisterEnum(cvar, ui_name, num_values, nullptr);
}

void CvarBinding_RegisterString(const char* cvar, const char* ui_name)
{
    Tatoosh::CvarBindingManager::RegisterString(cvar, ui_name);
}

void CvarBinding_SyncToUI(void)
{
    Tatoosh::CvarBindingManager::SyncToUI();
}

void CvarBinding_SyncFromUI(const char* ui_name)
{
    if (ui_name) {
        Tatoosh::CvarBindingManager::SyncFromUI(ui_name);
    }
}

void CvarBinding_CycleEnum(const char* ui_name, int delta)
{
    if (ui_name) {
        Tatoosh::CvarBindingManager::CycleEnum(ui_name, delta);
    }
}

} // extern "C"
