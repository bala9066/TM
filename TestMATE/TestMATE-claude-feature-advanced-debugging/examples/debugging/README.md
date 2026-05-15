# TestMATE Interactive Debugging Example

This example demonstrates how to use TestMATE's powerful interactive debugging system for test development and troubleshooting.

## Features Demonstrated

1. **Breakpoint Types**
   - Step entry breakpoints (pause at specific test steps)
   - Conditional breakpoints (pause when condition is true)
   - Hit count breakpoints (pause after N executions)
   - Data watchpoints (monitor variable changes)

2. **Debug Session Management**
   - Start/stop debugging sessions
   - Pause and resume execution
   - Step over, step into, step out
   - Run to cursor

3. **Variable Inspection**
   - Register variables for real-time inspection
   - Read/write variable values during debugging
   - View all variables at current location

4. **Interactive CLI**
   - GDB/LLDB-style command-line interface
   - Command aliases for quick access
   - Call stack visualization
   - Session information display

## Building the Example

```bash
cd build
cmake -DTESTMATE_BUILD_EXAMPLES=ON ..
make debug_example
```

## Running the Example

### Basic Run (Auto-continue)

```bash
./bin/debug_example
```

The example will:
1. Create a test sequence with 4 voltage measurement steps
2. Set up 3 different types of breakpoints
3. Execute the sequence with auto-continue at breakpoints
4. Display results and debug statistics

### Interactive Debugging

To enable interactive CLI debugging, uncomment this line in `DebugExample.cpp`:

```cpp
// cli->Start();  // Uncomment this line
```

Then rebuild and run:

```bash
make debug_example
./bin/debug_example
```

When execution pauses at a breakpoint, you can use these commands:

## CLI Commands Reference

| Command | Alias | Description |
|---------|-------|-------------|
| `continue` | `c` | Continue execution until next breakpoint |
| `step` | `s` | Step over current step |
| `stepi` | `si` | Step into substep/function |
| `finish` | `f` | Run until current step exits |
| `break <step-id>` | `b` | Set breakpoint at step |
| `delete <bp-id>` | `d` | Delete breakpoint |
| `list` | `l` | List all breakpoints |
| `print <var>` | `p` | Print variable value |
| `set <var> <value>` | - | Set variable value |
| `backtrace` | `bt` | Show call stack |
| `where` | `w` | Show current location |
| `info` | `i` | Show session information |
| `help` | `h, ?` | Show help |
| `quit` | `q` | Quit debugger |

## Example CLI Session

```
(testmate-dbg) break STEP-002
Breakpoint 1 set at STEP-002

(testmate-dbg) list
Breakpoints:
  [1] Enabled STEP-002 (Entry)
  [2] Enabled STEP-003 (Conditional) when (voltage > 10.0)
  [3] Enabled STEP-004 (HitCount) after 1 hits

(testmate-dbg) continue
Continuing...

[Stopped at STEP-002 (Measure 5.0V Rail)]

(testmate-dbg) print step2_voltage
step2_voltage = 5.023

(testmate-dbg) backtrace
Call Stack:
  #0 STEP-002 (Measure 5.0V Rail)

(testmate-dbg) step
Stepping...

[Execution continues...]
```

## Integrating Debugging in Your Tests

### 1. Set Up Breakpoint Manager

```cpp
auto breakpointManager = std::make_shared<CBreakpointManager>();

// Add breakpoints
auto bp1 = breakpointManager->AddBreakpoint(
    EBreakpointType::kStepEntry,
    "MY-STEP-ID"
);
```

### 2. Create Debug Session

```cpp
auto debugSession = std::make_shared<CDebugSession>(breakpointManager);

// Register variables for inspection
debugSession->RegisterVariableAccessor("voltage", [&]() -> TVariableValue {
    return GetCurrentVoltage();
});

debugSession->Start();
```

### 3. Attach to Executor

```cpp
CTestExecutor executor;
executor.SetDebugSession(debugSession);

// Execute normally - debugging happens automatically
executor.Execute(sequence, config);
```

### 4. Optional: Start Interactive CLI

```cpp
CDebugCLI cli(debugSession, breakpointManager);
cli.Start();  // Runs in background thread

// Wait for CLI to finish
cli.Wait();
```

## Advanced Techniques

### Conditional Breakpoints

Break only when a specific condition is true:

```cpp
auto bp = breakpointManager->AddBreakpoint(
    EBreakpointType::kConditional,
    "STEP-ID"
);

SBreakpointCondition condition;
condition.expression = "temperature > 85.0";
condition.evaluator = [&]() {
    return GetTemperature() > 85.0;
};

breakpointManager->SetBreakpointCondition(bp, condition);
```

### Data Watchpoints

Monitor when a variable changes:

```cpp
auto bp = breakpointManager->AddBreakpoint(
    EBreakpointType::kDataAccess,
    "STEP-ID"
);

SDataWatchpoint watchpoint;
watchpoint.variableName = "status";
watchpoint.breakOnWrite = true;
watchpoint.breakOnRead = false;

breakpointManager->SetBreakpointWatchpoint(bp, watchpoint);
```

### Programmatic Debugging

Control debugging from code without CLI:

```cpp
// In executor callback or separate thread
if (debugSession->IsPaused()) {
    // Inspect state
    auto stepId = debugSession->GetCurrentStepId();
    auto voltage = debugSession->GetVariable("voltage");

    // Make decision
    if (ShouldContinue(voltage)) {
        debugSession->Continue();
    } else {
        debugSession->StepOver();
    }
}
```

## Best Practices

1. **Strategic Breakpoints**
   - Set breakpoints at critical decision points
   - Use conditional breakpoints for rare conditions
   - Avoid too many breakpoints (slows execution)

2. **Variable Registration**
   - Register only variables you need to inspect
   - Use lambdas for live variable capture
   - Keep variable names short and descriptive

3. **Performance**
   - Disable debugging in production
   - Use hit count breakpoints for loops
   - Remove breakpoints when not needed

4. **Debugging Complex Issues**
   - Start with step-over to understand flow
   - Use step-into for detailed analysis
   - Check call stack for context
   - Print variables at each step

## Troubleshooting

### Breakpoint Not Hitting

- Verify step ID matches exactly
- Check breakpoint is enabled (`list` command)
- Ensure debug session is attached to executor
- Confirm step is actually executing

### Cannot See Variables

- Verify variable is registered with debug session
- Check variable name spelling
- Ensure lambda captures variable correctly
- Try `print` command to test

### Execution Hangs

- Press Ctrl+C to stop
- Check for WaitForDebugAction() deadlocks
- Verify debug thread is calling Continue()
- Use timeout in production code

## See Also

- **IMPLEMENTATION_PLAN.md** - Complete debugging system design
- **QUICK_START_DEBUGGING.md** - Week-by-week implementation guide
- **TESTMATE_V2_COMPLETE_ROADMAP.md** - Full feature roadmap

## Support

For issues or questions:
- Check unit tests in `tests/unit/debug/`
- Review source code in `src/debug/` and `include/testmate/debug/`
- See integration tests in `tests/integration/`
