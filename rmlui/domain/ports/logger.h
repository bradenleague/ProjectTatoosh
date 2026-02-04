/*
 * Tatoosh - ILogger Port Interface
 *
 * Abstracts logging for testability.
 * Infrastructure layer provides the Quake engine implementation (Con_Printf).
 */

#ifndef TATOOSH_PORTS_LOGGER_H
#define TATOOSH_PORTS_LOGGER_H

#include <string>

namespace Tatoosh {

// Interface for logging
// Implemented by infrastructure layer (QuakeLogger)
class ILogger {
public:
    virtual ~ILogger() = default;

    // Log a message (printf-style formatting done by caller)
    virtual void Log(const std::string& message) = 0;

    // Log a warning
    virtual void Warn(const std::string& message) = 0;

    // Log an error
    virtual void Error(const std::string& message) = 0;
};

} // namespace Tatoosh

#endif // TATOOSH_PORTS_LOGGER_H
