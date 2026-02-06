/*
 * Tatoosh - ICommandExecutor Port Interface
 *
 * Abstracts console command execution for testability.
 * Infrastructure layer provides the Quake engine implementation.
 */

#ifndef TATOOSH_PORTS_COMMAND_EXECUTOR_H
#define TATOOSH_PORTS_COMMAND_EXECUTOR_H

#include <string>

namespace Tatoosh {

// Interface for command execution
// Implemented by infrastructure layer (QuakeCommandExecutor)
class ICommandExecutor {
public:
    virtual ~ICommandExecutor() = default;

    // Execute a console command (adds to command buffer)
    // Command should NOT include trailing newline - implementation adds it
    virtual void Execute(const std::string& command) = 0;

    // Execute a command immediately (inserts at front of buffer)
    virtual void ExecuteImmediate(const std::string& command) = 0;
};

} // namespace Tatoosh

#endif // TATOOSH_PORTS_COMMAND_EXECUTOR_H
