/*
 * Tatoosh - Quake Command Executor Implementation
 */

#include "quake_command_executor.h"

// Quake command buffer interface
extern "C" {
    void Cbuf_AddText(const char* text);
    void Cbuf_InsertText(const char* text);
}

namespace Tatoosh {

QuakeCommandExecutor& QuakeCommandExecutor::Instance()
{
    static QuakeCommandExecutor instance;
    return instance;
}

void QuakeCommandExecutor::Execute(const std::string& command)
{
    // Add newline to execute the command
    std::string cmd = command + "\n";
    Cbuf_AddText(cmd.c_str());
}

void QuakeCommandExecutor::ExecuteImmediate(const std::string& command)
{
    // Add newline and insert at front of buffer
    std::string cmd = command + "\n";
    Cbuf_InsertText(cmd.c_str());
}

} // namespace Tatoosh
