/**
 * @file DebugExample.cpp
 * @brief Simple example demonstrating interactive debugging in TestMATE
 * @author TestMATE Development Team
 * @date 2025-11-23
 *
 * This example shows how to use TestMATE's interactive debugging system.
 *
 * Build and run:
 *   cd build
 *   make debug_example
 *   ./bin/debug_example
 */

#include "testmate/debug/Breakpoint.h"
#include "testmate/debug/BreakpointManager.h"
#include "testmate/debug/DebugSession.h"
#include "testmate/debug/DebugCLI.h"
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <cmath>

using namespace TestMATE;

// ============================================================================
// Main Example
// ============================================================================

int main() {
    std::cout << "====================================================\n";
    std::cout << "  TestMATE Interactive Debugging Example\n";
    std::cout << "====================================================\n\n";

    // ------------------------------------------------------------------------
    // Step 1: Create breakpoint manager and set breakpoints
    // ------------------------------------------------------------------------
    std::cout << "[1] Creating breakpoint manager...\n";

    auto breakpointManager = std::make_shared<CBreakpointManager>();

    // Add breakpoints
    std::cout << "  Setting breakpoints:\n";

    auto bp1 = breakpointManager->AddBreakpoint(EBreakpointType::kStepEntry, "STEP-002");
    std::cout << "    [" << bp1 << "] Step entry at STEP-002\n";

    auto bp2 = breakpointManager->AddBreakpoint(EBreakpointType::kHitCount, "STEP-003");
    breakpointManager->SetBreakpointHitCount(bp2, 1);
    std::cout << "    [" << bp2 << "] Hit count at STEP-003 (after 1 hit)\n\n";

    // ------------------------------------------------------------------------
    // Step 2: Create debug session
    // ------------------------------------------------------------------------
    std::cout << "[2] Creating debug session...\n";

    auto debugSession = std::make_shared<CDebugSession>(breakpointManager);

    // Register some example variables
    double voltage = 3.3;
    int testCount = 0;

    debugSession->RegisterVariableAccessor("voltage", [&voltage]() -> TVariableValue {
        return voltage;
    });

    debugSession->RegisterVariableAccessor("testCount", [&testCount]() -> TVariableValue {
        return static_cast<TInt32>(testCount);
    });

    std::cout << "  Registered 2 variables for inspection\n\n";

    // ------------------------------------------------------------------------
    // Step 3: Start debug session
    // ------------------------------------------------------------------------
    std::cout << "[3] Starting debug session...\n";
    debugSession->Start();
    std::cout << "  Debug session active\n\n";

    // ------------------------------------------------------------------------
    // Step 4: Simulate test execution with debugging
    // ------------------------------------------------------------------------
    std::cout << "[4] Simulating test execution with breakpoints...\n\n";

    // Simulate executor thread
    std::thread executorThread([&]() {
        std::vector<TString> steps = {"STEP-001", "STEP-002", "STEP-003", "STEP-004"};

        for (const auto& stepId : steps) {
            // Notify step entry
            std::cout << "Entering: " << stepId << "\n";
            debugSession->OnStepEnter(stepId, "Test Step " + stepId);

            // Check if should break
            if (debugSession->ShouldBreak()) {
                std::cout << "  ⚠ BREAKPOINT HIT at " << stepId << "!\n";
                std::cout << "  Waiting for debug action...\n";

                auto action = debugSession->WaitForDebugAction();

                std::cout << "  Debug action received: ";
                switch (action) {
                    case EDebugAction::kContinue: std::cout << "Continue\n"; break;
                    case EDebugAction::kStepOver: std::cout << "Step Over\n"; break;
                    case EDebugAction::kAbort: std::cout << "Abort\n"; break;
                    default: std::cout << "Other\n"; break;
                }

                if (action == EDebugAction::kAbort) {
                    std::cout << "  Execution aborted by debugger\n";
                    break;
                }
            }

            // Simulate step execution
            std::cout << "  Executing " << stepId << "...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Update variables
            voltage += 0.5;
            testCount++;

            std::cout << "  ✓ Step complete\n\n";

            // Notify step exit
            debugSession->OnStepExit(stepId);
        }

        debugSession->Finish();
    });

    // Simulate debugger thread (auto-continue after delay)
    std::thread debuggerThread([&]() {
        while (debugSession->IsActive()) {
            if (debugSession->IsPaused()) {
                std::cout << "\n  [Debugger] Execution paused\n";
                std::cout << "  [Debugger] Current step: " << debugSession->GetCurrentStepName() << "\n";

                // Show variables
                auto vars = debugSession->GetAllVariables();
                std::cout << "  [Debugger] Variables:\n";
                for (const auto& [name, value] : vars) {
                    std::cout << "    " << name << " = ";
                    std::visit([](auto&& v) { std::cout << v; }, value);
                    std::cout << "\n";
                }

                // Wait a moment
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                // Auto-continue
                std::cout << "  [Debugger] Auto-continuing...\n\n";
                debugSession->Continue();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Wait for completion
    executorThread.join();
    debuggerThread.join();

    // ------------------------------------------------------------------------
    // Step 5: Show results
    // ------------------------------------------------------------------------
    std::cout << "\n[5] Execution complete!\n\n";

    auto stats = debugSession->GetStats();
    std::cout << "====================================================\n";
    std::cout << "  Debug Statistics\n";
    std::cout << "====================================================\n";
    std::cout << "Steps executed: " << stats.stepsExecuted << "\n";
    std::cout << "Breakpoints hit: " << stats.breakpointsHit << "\n";
    std::cout << "Exceptions: " << stats.exceptionsOccurred << "\n";
    std::cout << "Total time: " << stats.executionTimeMs << " ms\n";
    std::cout << "====================================================\n\n";

    std::cout << "====================================================\n";
    std::cout << "  Breakpoint Report\n";
    std::cout << "====================================================\n";
    std::cout << breakpointManager->GenerateReport();
    std::cout << "====================================================\n\n";

    std::cout << "Example complete!\n";
    std::cout << "\nFor full executor integration, see TestExecutor tests.\n";

    return 0;
}
