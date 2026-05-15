/**
 * @file DebugCLI.cpp
 * @brief Implementation of CDebugCLI class
 * @author TestMATE Development Team
 * @date 2025-11-23
 */

#include "testmate/debug/DebugCLI.h"
#include <algorithm>
#include <iomanip>

namespace TestMATE {

// ==================== Constructor & Destructor ====================

CDebugCLI::CDebugCLI(
    std::shared_ptr<CDebugSession> in_session,
    std::shared_ptr<CBreakpointManager> in_breakpointManager,
    const SDebugCLIConfig& in_config,
    std::istream& in_input,
    std::ostream& in_output)
    : m_config(in_config)
    , m_session(std::move(in_session))
    , m_breakpointManager(std::move(in_breakpointManager))
    , m_input(in_input)
    , m_output(in_output) {
}

CDebugCLI::~CDebugCLI() {
    Stop();
}

// ==================== CLI Lifecycle ====================

void CDebugCLI::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_running) {
        return; // Already running
    }

    m_running = true;
    m_shouldQuit = false;

    m_cliThread = std::make_unique<std::thread>(&CDebugCLI::CLIThread, this);
}

void CDebugCLI::Stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shouldQuit = true;
    }

    if (m_cliThread && m_cliThread->joinable()) {
        m_cliThread->join();
    }

    m_running = false;
}

void CDebugCLI::Wait() {
    if (m_cliThread && m_cliThread->joinable()) {
        m_cliThread->join();
    }
}

bool CDebugCLI::IsRunning() const {
    return m_running;
}

// ==================== Command Execution ====================

SCommandResult CDebugCLI::ExecuteCommand(const TString& in_command) {
    auto [cmd, args] = ParseCommandLine(in_command);

    if (cmd.empty()) {
        return {true, "", true};
    }

    // Command dispatch table
    if (cmd == "continue" || cmd == "c") {
        return HandleContinue(args);
    } else if (cmd == "step" || cmd == "s") {
        return HandleStep(args);
    } else if (cmd == "stepi" || cmd == "si") {
        return HandleStepInto(args);
    } else if (cmd == "finish" || cmd == "f") {
        return HandleFinish(args);
    } else if (cmd == "break" || cmd == "b") {
        return HandleBreak(args);
    } else if (cmd == "delete" || cmd == "d") {
        return HandleDelete(args);
    } else if (cmd == "list" || cmd == "l") {
        return HandleList(args);
    } else if (cmd == "print" || cmd == "p") {
        return HandlePrint(args);
    } else if (cmd == "set") {
        return HandleSet(args);
    } else if (cmd == "backtrace" || cmd == "bt") {
        return HandleBacktrace(args);
    } else if (cmd == "where" || cmd == "w") {
        return HandleWhere(args);
    } else if (cmd == "info" || cmd == "i") {
        return HandleInfo(args);
    } else if (cmd == "help" || cmd == "h" || cmd == "?") {
        return HandleHelp(args);
    } else if (cmd == "quit" || cmd == "q") {
        return HandleQuit(args);
    } else {
        return {false, "Unknown command: " + cmd + " (type 'help' for commands)", true};
    }
}

void CDebugCLI::RunLoop() {
    m_output << "TestMATE Interactive Debugger\n";
    m_output << "Type 'help' for command list\n\n";

    while (!m_shouldQuit) {
        // Show current location if paused
        if (m_session && m_session->IsPaused()) {
            PrintLocation();
        }

        // Print prompt
        if (m_config.showPrompt) {
            PrintPrompt();
        }

        // Read command
        TString line;
        if (!std::getline(m_input, line)) {
            break; // EOF or error
        }

        // Echo if configured
        if (m_config.echoCommands) {
            m_output << line << "\n";
        }

        // Execute command
        auto result = ExecuteCommand(line);

        // Print result message
        if (!result.message.empty()) {
            m_output << result.message << "\n";
        }

        // Check if should continue
        if (!result.shouldContinue) {
            break;
        }
    }

    m_output << "Debugger exiting.\n";
}

// ==================== Private Methods ====================

void CDebugCLI::CLIThread() {
    RunLoop();
    m_running = false;
}

void CDebugCLI::PrintPrompt() {
    m_output << m_config.promptString;
    m_output.flush();
}

void CDebugCLI::PrintHelp() {
    m_output << "TestMATE Debugger Commands:\n";
    m_output << "  continue (c)        - Continue execution until next breakpoint\n";
    m_output << "  step (s)            - Step over current line\n";
    m_output << "  stepi (si)          - Step into function/substep\n";
    m_output << "  finish (f)          - Run until current function returns\n";
    m_output << "  break (b) <step>    - Set breakpoint at step\n";
    m_output << "  delete (d) <id>     - Delete breakpoint by ID\n";
    m_output << "  list (l)            - List all breakpoints\n";
    m_output << "  print (p) <var>     - Print variable value\n";
    m_output << "  set <var> <value>   - Set variable value\n";
    m_output << "  backtrace (bt)      - Show call stack\n";
    m_output << "  where (w)           - Show current location\n";
    m_output << "  info (i)            - Show session information\n";
    m_output << "  help (h, ?)         - Show this help\n";
    m_output << "  quit (q)            - Quit debugger\n";
}

void CDebugCLI::PrintLocation() {
    if (!m_session) return;

    auto stepId = m_session->GetCurrentStepId();
    auto stepName = m_session->GetCurrentStepName();

    if (!stepId.empty()) {
        m_output << "Stopped at: " << stepId;
        if (!stepName.empty()) {
            m_output << " (" << stepName << ")";
        }
        m_output << "\n";
    }
}

void CDebugCLI::PrintCallStack() {
    if (!m_session) return;

    auto callStack = m_session->GetCallStack();

    if (callStack.empty()) {
        m_output << "No call stack available\n";
        return;
    }

    m_output << "Call Stack:\n";
    for (size_t i = 0; i < callStack.size(); ++i) {
        const auto& frame = callStack[i];
        m_output << "  #" << i << " " << frame.stepId;
        if (!frame.stepName.empty()) {
            m_output << " (" << frame.stepName << ")";
        }
        m_output << "\n";
    }
}

// ==================== Command Handlers ====================

SCommandResult CDebugCLI::HandleContinue(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    if (!m_session->IsPaused()) {
        return {false, "Not paused", true};
    }

    m_session->Continue();
    return {true, "Continuing...", true};
}

SCommandResult CDebugCLI::HandleStep(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    if (!m_session->IsPaused()) {
        return {false, "Not paused", true};
    }

    m_session->StepOver();
    return {true, "Stepping...", true};
}

SCommandResult CDebugCLI::HandleStepInto(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    if (!m_session->IsPaused()) {
        return {false, "Not paused", true};
    }

    m_session->StepInto();
    return {true, "Stepping into...", true};
}

SCommandResult CDebugCLI::HandleFinish(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    if (!m_session->IsPaused()) {
        return {false, "Not paused", true};
    }

    m_session->StepOut();
    return {true, "Running until function returns...", true};
}

SCommandResult CDebugCLI::HandleBreak(const TVector<TString>& args) {
    if (args.empty()) {
        return {false, "Usage: break <step-id>", true};
    }

    if (!m_breakpointManager) {
        return {false, "No breakpoint manager available", true};
    }

    auto stepId = args[0];
    auto bpId = m_breakpointManager->AddBreakpoint(EBreakpointType::kStepEntry, stepId);

    if (bpId == 0) {
        return {false, "Failed to set breakpoint", true};
    }

    std::ostringstream oss;
    oss << "Breakpoint " << bpId << " set at " << stepId;
    return {true, oss.str(), true};
}

SCommandResult CDebugCLI::HandleDelete(const TVector<TString>& args) {
    if (args.empty()) {
        return {false, "Usage: delete <breakpoint-id>", true};
    }

    if (!m_breakpointManager) {
        return {false, "No breakpoint manager available", true};
    }

    try {
        TUInt32 bpId = std::stoul(args[0]);
        if (m_breakpointManager->RemoveBreakpoint(bpId)) {
            return {true, "Breakpoint deleted", true};
        } else {
            return {false, "Breakpoint not found", true};
        }
    } catch (...) {
        return {false, "Invalid breakpoint ID", true};
    }
}

SCommandResult CDebugCLI::HandleList(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_breakpointManager) {
        return {false, "No breakpoint manager available", true};
    }

    auto breakpoints = m_breakpointManager->ListBreakpoints();

    if (breakpoints.empty()) {
        return {true, "No breakpoints set", true};
    }

    std::ostringstream oss;
    oss << "Breakpoints:\n";
    for (const auto& bp : breakpoints) {
        oss << "  " << bp << "\n";
    }

    return {true, oss.str(), true};
}

SCommandResult CDebugCLI::HandlePrint(const TVector<TString>& args) {
    if (args.empty()) {
        return {false, "Usage: print <variable-name>", true};
    }

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    auto varName = args[0];
    auto value = m_session->GetVariable(varName);

    if (!value) {
        return {false, "Variable not found: " + varName, true};
    }

    std::ostringstream oss;
    oss << varName << " = " << VariableValueToString(*value);
    return {true, oss.str(), true};
}

SCommandResult CDebugCLI::HandleSet(const TVector<TString>& args) {
    if (args.size() < 2) {
        return {false, "Usage: set <variable-name> <value>", true};
    }

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    auto varName = args[0];
    auto value = ParseVariableValue(args[1]);

    if (m_session->SetVariable(varName, value)) {
        return {true, "Variable set", true};
    } else {
        return {false, "Failed to set variable (not found or read-only)", true};
    }
}

SCommandResult CDebugCLI::HandleBacktrace(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    PrintCallStack();
    return {true, "", true};
}

SCommandResult CDebugCLI::HandleWhere(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    PrintLocation();
    return {true, "", true};
}

SCommandResult CDebugCLI::HandleInfo(const TVector<TString>& args) {
    (void)args; // Unused

    if (!m_session) {
        return {false, "No debug session active", true};
    }

    auto stats = m_session->GetStats();
    auto state = m_session->GetState();

    std::ostringstream oss;
    oss << "Debug Session Information:\n";
    oss << "  State: ";
    switch (state) {
        case EDebugState::kIdle:     oss << "Idle"; break;
        case EDebugState::kRunning:  oss << "Running"; break;
        case EDebugState::kPaused:   oss << "Paused"; break;
        case EDebugState::kStepping: oss << "Stepping"; break;
        case EDebugState::kFinished: oss << "Finished"; break;
        case EDebugState::kAborted:  oss << "Aborted"; break;
    }
    oss << "\n";
    oss << "  Steps Executed: " << stats.stepsExecuted << "\n";
    oss << "  Breakpoints Hit: " << stats.breakpointsHit << "\n";
    oss << "  Exceptions: " << stats.exceptionsOccurred << "\n";
    oss << "  Execution Time: " << std::fixed << std::setprecision(2)
        << stats.executionTimeMs << " ms\n";

    return {true, oss.str(), true};
}

SCommandResult CDebugCLI::HandleHelp(const TVector<TString>& args) {
    (void)args; // Unused

    PrintHelp();
    return {true, "", true};
}

SCommandResult CDebugCLI::HandleQuit(const TVector<TString>& args) {
    (void)args; // Unused

    m_shouldQuit = true;
    return {true, "Quitting debugger", false};
}

// ==================== Utilities ====================

std::pair<TString, TVector<TString>> CDebugCLI::ParseCommandLine(const TString& in_line) {
    std::istringstream iss(in_line);
    TString command;
    iss >> command;

    TVector<TString> args;
    TString arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    return {command, args};
}

TString CDebugCLI::VariableValueToString(const TVariableValue& in_value) {
    return std::visit([](auto&& val) -> TString {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>) {
            return val ? "true" : "false";
        } else if constexpr (std::is_same_v<T, TString>) {
            return "\"" + val + "\"";
        } else {
            return std::to_string(val);
        }
    }, in_value);
}

TVariableValue CDebugCLI::ParseVariableValue(const TString& in_str) {
    // Try to parse as different types

    // Boolean
    if (in_str == "true") return true;
    if (in_str == "false") return false;

    // String (if quoted)
    if (!in_str.empty() && in_str[0] == '"' && in_str.back() == '"') {
        return in_str.substr(1, in_str.length() - 2);
    }

    // Try integer
    try {
        if (in_str.find('.') == TString::npos) {
            // No decimal point - integer
            TInt64 val = std::stoll(in_str);
            if (val >= 0) {
                return static_cast<TUInt64>(val);
            } else {
                return val;
            }
        }
    } catch (...) {
        // Not an integer
    }

    // Try double
    try {
        return std::stod(in_str);
    } catch (...) {
        // Not a double
    }

    // Default to string
    return in_str;
}

} // namespace TestMATE
