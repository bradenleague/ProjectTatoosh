/*
 * Tatoosh - Quake Command Executor Implementation
 *
 * Implements ICommandExecutor using Quake's command buffer.
 */

#ifndef TATOOSH_QUAKE_COMMAND_EXECUTOR_H
#define TATOOSH_QUAKE_COMMAND_EXECUTOR_H

#include "../domain/ports/command_executor.h"

namespace Tatoosh {

// Quake engine implementation of ICommandExecutor
class QuakeCommandExecutor : public ICommandExecutor {
public:
    // Singleton access - the executor is stateless, just wraps engine functions
    static QuakeCommandExecutor& Instance();

    void Execute(const std::string& command) override;
    void ExecuteImmediate(const std::string& command) override;

private:
    QuakeCommandExecutor() = default;
};

} // namespace Tatoosh

#endif // TATOOSH_QUAKE_COMMAND_EXECUTOR_H
