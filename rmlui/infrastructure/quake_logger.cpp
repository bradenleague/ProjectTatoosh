/*
 * Tatoosh - Quake Logger Implementation
 */

#include "quake_logger.h"

extern "C" {
    void Con_Printf(const char* fmt, ...);
}

namespace Tatoosh {

QuakeLogger& QuakeLogger::Instance()
{
    static QuakeLogger instance;
    return instance;
}

void QuakeLogger::Log(const std::string& message)
{
    Con_Printf("%s", message.c_str());
}

void QuakeLogger::Warn(const std::string& message)
{
    Con_Printf("WARNING: %s", message.c_str());
}

void QuakeLogger::Error(const std::string& message)
{
    Con_Printf("ERROR: %s", message.c_str());
}

} // namespace Tatoosh
