/*
 * Tatoosh - Document Manager Implementation
 */

#include "document_manager.h"

extern "C" {
    void Con_Printf(const char* fmt, ...);
}

namespace Tatoosh {

// Static member initialization
Rml::Context* DocumentManager::s_context = nullptr;
std::unordered_map<std::string, Rml::ElementDocument*> DocumentManager::s_documents;
std::string DocumentManager::s_base_path;

void DocumentManager::Initialize(Rml::Context* context)
{
    s_context = context;
    s_documents.clear();
}

void DocumentManager::Shutdown()
{
    CloseAll();
    s_context = nullptr;
}

bool DocumentManager::Load(const std::string& path)
{
    if (!s_context) return false;

    // Check if already loaded
    auto it = s_documents.find(path);
    if (it != s_documents.end() && it->second) {
        return true;  // Already loaded
    }

    Rml::ElementDocument* doc = s_context->LoadDocument(path);
    if (!doc) {
        Con_Printf("DocumentManager::Load: Failed to load '%s'\n", path.c_str());
        return false;
    }

    s_documents[path] = doc;
    Con_Printf("DocumentManager::Load: Loaded '%s'\n", path.c_str());
    return true;
}

void DocumentManager::Unload(const std::string& path)
{
    if (!s_context) return;

    auto it = s_documents.find(path);
    if (it != s_documents.end() && it->second) {
        it->second->Close();
        s_documents.erase(it);
        Con_Printf("DocumentManager::Unload: Unloaded '%s'\n", path.c_str());
    }
}

void DocumentManager::Show(const std::string& path, bool modal)
{
    if (!s_context) return;

    // Load if not cached
    if (!Load(path)) return;

    auto it = s_documents.find(path);
    if (it != s_documents.end() && it->second) {
        if (modal) {
            it->second->Show(Rml::ModalFlag::Modal);
        } else {
            it->second->Show();
        }
    }
}

void DocumentManager::Hide(const std::string& path)
{
    if (!s_context) return;

    auto it = s_documents.find(path);
    if (it != s_documents.end() && it->second) {
        it->second->Hide();
    }
}

bool DocumentManager::IsLoaded(const std::string& path)
{
    auto it = s_documents.find(path);
    return it != s_documents.end() && it->second != nullptr;
}

bool DocumentManager::IsVisible(const std::string& path)
{
    auto it = s_documents.find(path);
    if (it != s_documents.end() && it->second) {
        return it->second->IsVisible();
    }
    return false;
}

Rml::ElementDocument* DocumentManager::Get(const std::string& path)
{
    auto it = s_documents.find(path);
    if (it != s_documents.end()) {
        return it->second;
    }
    return nullptr;
}

void DocumentManager::ReloadAll()
{
#ifdef TATOOSH_HOT_RELOAD
    if (!s_context) return;

    Con_Printf("DocumentManager::ReloadAll: Reloading all documents\n");

    for (auto& pair : s_documents) {
        if (pair.second) {
            bool was_visible = pair.second->IsVisible();
            std::string path = pair.first;

            pair.second->Close();
            pair.second = s_context->LoadDocument(path);

            if (pair.second && was_visible) {
                pair.second->Show();
            }
        }
    }
#else
    Con_Printf("DocumentManager::ReloadAll: Hot reload not enabled\n");
#endif
}

void DocumentManager::CloseAll()
{
    for (auto& pair : s_documents) {
        if (pair.second) {
            pair.second->Close();
        }
    }
    s_documents.clear();
}

void DocumentManager::SetBasePath(const std::string& path)
{
    s_base_path = path;
    Con_Printf("DocumentManager::SetBasePath: '%s'\n", path.c_str());
}

const std::string& DocumentManager::GetBasePath()
{
    return s_base_path;
}

} // namespace Tatoosh
