/*
 * Tatoosh - ICvarProvider Port Interface
 *
 * Abstracts console variable (cvar) operations for testability.
 * Infrastructure layer provides the Quake engine implementation.
 */

#ifndef TATOOSH_PORTS_CVAR_PROVIDER_H
#define TATOOSH_PORTS_CVAR_PROVIDER_H

#include <string>

namespace Tatoosh {

// Interface for cvar operations
// Implemented by infrastructure layer (QuakeCvarProvider)
class ICvarProvider {
public:
    virtual ~ICvarProvider() = default;

    // Get float value of a cvar
    virtual float GetFloat(const std::string& name) const = 0;

    // Get string value of a cvar
    virtual std::string GetString(const std::string& name) const = 0;

    // Set float value of a cvar
    virtual void SetFloat(const std::string& name, float value) = 0;

    // Set string value of a cvar
    virtual void SetString(const std::string& name, const std::string& value) = 0;

    // Check if a cvar exists
    virtual bool Exists(const std::string& name) const = 0;
};

} // namespace Tatoosh

#endif // TATOOSH_PORTS_CVAR_PROVIDER_H
