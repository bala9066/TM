/**
 * @file DebugCLI.h
 * @brief Command-line interface for interactive debugging
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This file defines the CDebugCLI class which provides an interactive
 * command-line debugger interface similar to GDB/LLDB.
 */

#ifndef TESTMATE_DEBUG_DEBUG_CLI_H
#define TESTMATE_DEBUG_DEBUG_CLI_H

#include "testmate/debug/DebugSession.h"
#include <iostream>
#include <sstream>
#include <thread>

namespace TestMATE {

/**
 * @brief CLI command result
 */
struct SCommandResult {
    bool success{true};                ///< Command succeeded
    TString message;                   ///< Result message
    bool shouldContinue{true};         ///< Continue CLI loop
};

/**
 * @brief CLI configuration
 */
struct SDebugCLIConfig {
    bool showPrompt{true};             ///< Show command prompt
    bool echoCommands{false};          ///< Echo commands back
    TString promptString{" (testmate-dbg) "};  ///< Prompt string
    bool useColor{true};               ///< Use ANSI colors (if terminal supports it)
};

/**
 * @brief Interactive command-line debugger interface
 *
 * CDebugCLI provides a GDB/LLDB-style command-line interface for debugging.
 * It runs in a separate thread and communicates with the debug session.
 *
 * Supported commands:
 * - continue (c)     : Continue execution
 * - step (s)         : Step over
 * - stepi (si)       : Step into
 * - finish (f)       : Step out
 * - break (b)        : Set breakpoint
 * - delete (d)       : Delete breakpoint
 * - list (l)         : List breakpoints
 * - print (p)        : Print variable
 * - set              : Set variable value
 * - backtrace (bt)   : Show call stack
 * - where (w)        : Show current location
 * - info             : Show session info
 * - help (h, ?)      : Show help
 * - quit (q)         : Quit debugging
 *
 * Example usage:
 * @code
 * auto session = std::make_shared<CDebugSession>(breakpointManager);
 * CDebugCLI cli(session);
 *
 * // Start CLI in background
 * cli.Start();
 *
 * // CLI will run until user types 'quit'
 * cli.Wait();
 * @endcode
 */
class CDebugCLI {
public:
    /**
     * @brief Construct a new debug CLI
     * @param in_session Debug session to control
     * @param in_breakpointManager Breakpoint manager for break/delete/list commands
     * @param in_config CLI configuration
     * @param in_input Input stream (default: std::cin)
     * @param in_output Output stream (default: std::cout)
     */
    explicit CDebugCLI(
        std::shared_ptr<CDebugSession> in_session,
        std::shared_ptr<CBreakpointManager> in_breakpointManager,
        const SDebugCLIConfig& in_config = {},
        std::istream& in_input = std::cin,
        std::ostream& in_output = std::cout);

    /**
     * @brief Destructor
     */
    ~CDebugCLI();

    // Prevent copying
    CDebugCLI(const CDebugCLI&) = delete;
    CDebugCLI& operator=(const CDebugCLI&) = delete;

    // ==================== CLI Lifecycle ====================

    /**
     * @brief Start the CLI in a background thread
     */
    void Start();

    /**
     * @brief Stop the CLI
     */
    void Stop();

    /**
     * @brief Wait for CLI to finish
     */
    void Wait();

    /**
     * @brief Check if CLI is running
     * @return true if running, false otherwise
     */
    [[nodiscard]] bool IsRunning() const;

    // ==================== Command Execution ====================

    /**
     * @brief Execute a single command (for testing/scripting)
     * @param in_command Command string
     * @return Command result
     */
    SCommandResult ExecuteCommand(const TString& in_command);

    /**
     * @brief Run the interactive CLI loop (blocking)
     */
    void RunLoop();

private:
    /**
     * @brief CLI thread function
     */
    void CLIThread();

    /**
     * @brief Print the command prompt
     */
    void PrintPrompt();

    /**
     * @brief Print help message
     */
    void PrintHelp();

    /**
     * @brief Print current location
     */
    void PrintLocation();

    /**
     * @brief Print call stack
     */
    void PrintCallStack();

    // ==================== Command Handlers ====================

    SCommandResult HandleContinue(const TVector<TString>& args);
    SCommandResult HandleStep(const TVector<TString>& args);
    SCommandResult HandleStepInto(const TVector<TString>& args);
    SCommandResult HandleFinish(const TVector<TString>& args);
    SCommandResult HandleBreak(const TVector<TString>& args);
    SCommandResult HandleDelete(const TVector<TString>& args);
    SCommandResult HandleList(const TVector<TString>& args);
    SCommandResult HandlePrint(const TVector<TString>& args);
    SCommandResult HandleSet(const TVector<TString>& args);
    SCommandResult HandleBacktrace(const TVector<TString>& args);
    SCommandResult HandleWhere(const TVector<TString>& args);
    SCommandResult HandleInfo(const TVector<TString>& args);
    SCommandResult HandleHelp(const TVector<TString>& args);
    SCommandResult HandleQuit(const TVector<TString>& args);

    // ==================== Utilities ====================

    /**
     * @brief Parse command line into command and arguments
     * @param in_line Command line
     * @return Pair of (command, arguments)
     */
    std::pair<TString, TVector<TString>> ParseCommandLine(const TString& in_line);

    /**
     * @brief Convert variable value to string
     * @param in_value Variable value
     * @return String representation
     */
    TString VariableValueToString(const TVariableValue& in_value);

    /**
     * @brief Parse variable value from string
     * @param in_str String representation
     * @return Variable value
     */
    TVariableValue ParseVariableValue(const TString& in_str);

    // Configuration
    SDebugCLIConfig m_config;

    // Debug session
    std::shared_ptr<CDebugSession> m_session;

    // Breakpoint manager (convenience reference)
    std::shared_ptr<CBreakpointManager> m_breakpointManager;

    // I/O streams
    std::istream& m_input;
    std::ostream& m_output;

    // CLI thread
    std::unique_ptr<std::thread> m_cliThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldQuit{false};

    // State
    mutable std::mutex m_mutex;
};

} // namespace TestMATE

#endif // TESTMATE_DEBUG_DEBUG_CLI_H
