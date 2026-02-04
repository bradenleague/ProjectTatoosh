/*
 * Tatoosh - Quake Logger Implementation
 *
 * Implements ILogger using Quake's Con_Printf.
 */

#ifndef TATOOSH_QUAKE_LOGGER_H
#define TATOOSH_QUAKE_LOGGER_H

#include "../domain/ports/logger.h"

namespace Tatoosh {

// Quake engine implementation of ILogger
class QuakeLogger : public ILogger {
public:
    // Singleton access - the logger is stateless, just wraps Con_Printf
    static QuakeLogger& Instance();

    void Log(const std::string& message) override;
    void Warn(const std::string& message) override;
    void Error(const std::string& message) override;

private:
    QuakeLogger() = default;
};

} // namespace Tatoosh

#endif // TATOOSH_QUAKE_LOGGER_H
