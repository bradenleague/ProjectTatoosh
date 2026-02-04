/*
 * Tatoosh - Document Manager
 *
 * Manages RmlUI document lifecycle: loading, unloading, showing, hiding.
 * Provides document caching and visibility control.
 */

#ifndef TATOOSH_APPLICATION_DOCUMENT_MANAGER_H
#define TATOOSH_APPLICATION_DOCUMENT_MANAGER_H

#include <RmlUi/Core.h>
#include <string>
#include <unordered_map>

namespace Tatoosh {

class DocumentManager {
public:
    // Initialize with RmlUI context
    static void Initialize(Rml::Context* context);

    // Shutdown and cleanup all documents
    static void Shutdown();

    // Load a document (caches if already loaded)
    // Returns true on success
    static bool Load(const std::string& path);

    // Unload and remove a document from cache
    static void Unload(const std::string& path);

    // Show a document (loads if not cached)
    // modal: if true, shows as modal dialog
    static void Show(const std::string& path, bool modal = false);

    // Hide a document
    static void Hide(const std::string& path);

    // Check if document is loaded
    static bool IsLoaded(const std::string& path);

    // Check if document is visible
    static bool IsVisible(const std::string& path);

    // Get raw document pointer (may be nullptr)
    static Rml::ElementDocument* Get(const std::string& path);

    // Reload all documents (for hot reload)
    static void ReloadAll();

    // Close all documents
    static void CloseAll();

    // Set UI base path for asset loading
    static void SetBasePath(const std::string& path);
    static const std::string& GetBasePath();

private:
    static Rml::Context* s_context;
    static std::unordered_map<std::string, Rml::ElementDocument*> s_documents;
    static std::string s_base_path;
};

} // namespace Tatoosh

#endif // TATOOSH_APPLICATION_DOCUMENT_MANAGER_H
