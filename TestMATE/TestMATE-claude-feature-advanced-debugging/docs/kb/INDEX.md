# TestMATE Knowledge Base

**Version:** 2.0
**Last Updated:** 2025-11-23
**Status:** Production Ready

Welcome to the comprehensive TestMATE Knowledge Base! This documentation provides everything you need to understand, use, and extend the TestMATE test automation framework.

---

## 📚 Table of Contents

### Getting Started
- [Quick Start Guide](../../QUICKSTART.md) - Get up and running in 10 minutes
- [Installation Guide](guides/INSTALLATION.md) - Detailed installation instructions
- [Your First Test](tutorials/FIRST_TEST.md) - Step-by-step tutorial
- [Project Overview](../README.md) - What is TestMATE?

### Architecture & Design
- [System Architecture](architecture/SYSTEM_ARCHITECTURE.md) - High-level system design
- [Process Models](architecture/PROCESS_MODELS.md) - Sequential, Parallel, and Batch execution
- [Threading & Concurrency](architecture/THREADING.md) - Multi-threaded execution design
- [Resource Management](architecture/RESOURCE_MANAGEMENT.md) - Resource scheduling and allocation
- [Plugin System](architecture/PLUGIN_SYSTEM.md) - Extensibility architecture
- [Design Patterns](architecture/DESIGN_PATTERNS.md) - Patterns used in TestMATE
- [Data Flow](architecture/DATA_FLOW.md) - How data flows through the system

### API Reference
- [Core API](api/CORE_API.md) - Core execution engine APIs
- **[Database API](api/DATABASE_API.md)** - **Data storage with parameterized queries and thread safety**
- [Test Sequence API](api/TEST_SEQUENCE_API.md) - Sequence and step management
- [Process Models API](api/PROCESS_MODELS_API.md) - Execution model interfaces
- [Plugin API](api/PLUGIN_API.md) - Plugin development interfaces
- [Configuration API](api/CONFIGURATION_API.md) - Settings management
- [Reporting API](api/REPORTING_API.md) - Report generation
- [Utilities API](api/UTILITIES_API.md) - Common utilities

### Developer Guides
- [Plugin Development Guide](guides/PLUGIN_DEVELOPMENT.md) - Create custom plugins
- [Test Step Development](guides/TEST_STEP_DEVELOPMENT.md) - Implement custom test steps
- [Instrument Driver Development](guides/INSTRUMENT_DEVELOPMENT.md) - Add instrument support
- [Custom Process Models](guides/CUSTOM_PROCESS_MODELS.md) - Extend execution models
- [Database Backend Development](guides/DATABASE_BACKEND.md) - Add new database support
- [Contributing Guide](guides/CONTRIBUTING.md) - How to contribute to TestMATE
- [Coding Standards](../CodingGuideline/SDG-Coding-Guidelines_3.txt) - Code style guide

### User Guides
- [User Manual](../USER_MANUAL.md) - Complete user manual
- [Test Sequence Creation](guides/TEST_SEQUENCES.md) - Creating test sequences
- [Configuration Management](guides/CONFIGURATION.md) - System configuration
- [Database Setup](guides/DATABASE_SETUP.md) - Database configuration
- [Report Generation](guides/REPORTS.md) - Generating test reports
- [Performance Profiling](../PERFORMANCE_PROFILING.md) - Performance analysis
- [Qt GUI Guide](guides/QT_GUI.md) - Using the graphical interface

### Tutorials
- [Tutorial 1: Basic Power Supply Test](tutorials/POWER_SUPPLY_TEST.md)
- [Tutorial 2: Multi-Socket Parallel Testing](tutorials/PARALLEL_TESTING.md)
- [Tutorial 3: Semiconductor Parametric Testing](tutorials/SEMICONDUCTOR_TEST.md)
- [Tutorial 4: Creating Custom Plugins](tutorials/CUSTOM_PLUGIN.md)
- [Tutorial 5: Database Integration](tutorials/DATABASE_INTEGRATION.md)
- [Tutorial 6: Advanced Sequences](tutorials/ADVANCED_SEQUENCES.md)

### Security & Best Practices
- **[Security Guidelines](../../SECURITY.md)** - **📚 Comprehensive security best practices and threat model**
- [Security Fixes Implemented](../../SECURITY_FIXES_IMPLEMENTED.md) - Recent security improvements
- [Code Review Report](../../CODE_REVIEW_REPORT.md) - Code quality and security audit
- **[Best Practices](reference/BEST_PRACTICES.md)** - **📚 Security, threading, and coding best practices**

### Reference
- [Test Step Reference](reference/TEST_STEPS.md) - All available test steps
- [Error Codes](reference/ERROR_CODES.md) - Complete error code reference
- [Configuration Keys](reference/CONFIG_KEYS.md) - All configuration parameters
- [SCPI Commands](reference/SCPI_COMMANDS.md) - Instrument command reference
- [File Formats](reference/FILE_FORMATS.md) - Sequence file specifications
- [Environment Variables](reference/ENVIRONMENT.md) - Environment configuration
- [Command Line Options](reference/CLI.md) - Command line interface
- [Keyboard Shortcuts](reference/SHORTCUTS.md) - Qt GUI shortcuts

### Quick Reference
- [Cheat Sheet](reference/CHEAT_SHEET.md) - Quick reference card
- [Common Patterns](reference/COMMON_PATTERNS.md) - Frequently used code patterns
- [Performance Tips](reference/PERFORMANCE_TIPS.md) - Optimization guidelines

### FAQ & Troubleshooting
- [FAQ](faq/FAQ.md) - Frequently asked questions
- [Troubleshooting Guide](faq/TROUBLESHOOTING.md) - Common issues and solutions
- [Migration Guide](faq/MIGRATION.md) - Upgrading from older versions
- [Known Issues](faq/KNOWN_ISSUES.md) - Current limitations

### Examples
- [Example Test Sequences](../../examples/basic_test_plan/README.md)
- [Example Plugins](../../examples/custom_plugin/README.md)
- [Essential Test Steps](../../examples/test_steps/README.md)
- [Database Examples](../../examples/database/README.md)
- [Semiconductor Testing](../../examples/semiconductor/README.md)

---

## 🎯 Quick Links

### For New Users
1. Start with [Quick Start Guide](../../QUICKSTART.md)
2. Follow [Your First Test](tutorials/FIRST_TEST.md) tutorial
3. Read the [User Manual](../USER_MANUAL.md)

### For Developers
1. Review [System Architecture](architecture/SYSTEM_ARCHITECTURE.md)
2. Study [API Reference](api/CORE_API.md) and [Database API](api/DATABASE_API.md)
3. **Read [Security Guidelines](../../SECURITY.md) and [Best Practices](reference/BEST_PRACTICES.md)**
4. Check [Plugin Development Guide](guides/PLUGIN_DEVELOPMENT.md)
5. Follow [Coding Standards](../CodingGuideline/SDG-Coding-Guidelines_3.txt)

### For Plugin Authors
1. Read [Plugin Development Guide](guides/PLUGIN_DEVELOPMENT.md)
2. Study [Example Plugins](../../examples/custom_plugin/README.md)
3. Review [Plugin API](api/PLUGIN_API.md)
4. Check [Best Practices](reference/BEST_PRACTICES.md)

### For Test Engineers
1. Learn [Test Sequence Creation](guides/TEST_SEQUENCES.md)
2. Study [Test Step Reference](reference/TEST_STEPS.md)
3. Review [Example Sequences](../../examples/basic_test_plan/README.md)
4. Use [Cheat Sheet](reference/CHEAT_SHEET.md)

---

## 📊 Framework Overview

### What is TestMATE?

TestMATE (Test Management and Automation Tool Environment) is a comprehensive, production-ready C++20 framework for automated testing of electronic devices and systems. It provides:

**Core Features:**
- ✅ **Multiple Execution Models** - Sequential, Parallel (multi-socket), and Batch processing
- ✅ **Plugin Architecture** - Extensible instrument drivers and test steps
- ✅ **Database Integration** - SQLite, PostgreSQL, MySQL/MariaDB support
- ✅ **Advanced Threading** - Thread pool, synchronization primitives, resource scheduling
- ✅ **Comprehensive Reporting** - HTML reports, STDF format, custom templates
- ✅ **Qt GUI Application** - Professional graphical interface with real-time monitoring
- ✅ **Configuration Management** - Flexible hierarchical configuration system
- ✅ **Performance Profiling** - Built-in profiler with statistical analysis

**Quality Metrics:**
- 261 unit tests (100% pass rate)
- 7 integration tests
- ~21,000 lines of production code
- Zero compilation warnings
- Cross-platform (Windows, Linux, macOS)

### System Requirements

**Minimum:**
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.15+
- SQLite3

**Optional:**
- Qt5 5.15+ or Qt6 (for GUI)
- PostgreSQL client library (for PostgreSQL support)
- MySQL client library (for MySQL support)

### Architecture at a Glance

```
┌─────────────────────────────────────────────────────┐
│                 Qt GUI Application                  │
│              (testmate_qt - optional)               │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│                    API Layer                        │
│              (testmate_api - facade)                │
└─────────────────┬───────────────────────────────────┘
                  │
    ┌─────────────┼─────────────┬──────────────┐
    │             │             │              │
┌───▼────┐  ┌────▼─────┐  ┌────▼─────┐  ┌────▼─────┐
│ Core   │  │ Database │  │    UI    │  │ Utilities│
│ Engine │  │  Layer   │  │ Adapter  │  │          │
└───┬────┘  └──────────┘  └──────────┘  └──────────┘
    │
    ├── Process Models (Sequential, Parallel, Batch)
    ├── Test Executor
    ├── Threading & Synchronization
    ├── Resource Scheduler
    ├── Plugin Manager
    ├── Instrument Manager
    └── Report Generator
```

### Key Components

| Component | Purpose | Status |
|-----------|---------|--------|
| **Process Models** | Define test execution strategies | ✅ Complete |
| **Test Executor** | Execute test sequences | ✅ Complete |
| **Plugin System** | Dynamic loading of extensions | ✅ Complete |
| **Threading** | Multi-threaded parallel execution | ✅ Complete |
| **Resource Scheduler** | Manage shared resources | ✅ Complete |
| **Database Layer** | Store test results | ✅ Complete |
| **Qt GUI** | Graphical user interface | ✅ Complete |
| **Reporting** | Generate test reports | ✅ Complete |

---

## 🔍 Search Tips

**Finding Information:**
- Use your browser's search (Ctrl+F / Cmd+F) within documents
- Check the [Cheat Sheet](reference/CHEAT_SHEET.md) for quick answers
- Browse [FAQ](faq/FAQ.md) for common questions
- Review [Troubleshooting Guide](faq/TROUBLESHOOTING.md) for issues

**Navigation:**
- Click section headers to jump to topics
- Use browser back/forward buttons
- Bookmark frequently used pages

---

## 📞 Support & Community

### Getting Help
- **Documentation Issues:** File an issue on GitHub
- **Bug Reports:** Use GitHub issue tracker
- **Feature Requests:** Submit GitHub issues with enhancement tag

### Contributing
See [Contributing Guide](guides/CONTRIBUTING.md) for:
- Code contribution process
- Documentation improvements
- Bug reporting guidelines
- Feature request process

---

## 📄 License

This project follows SDG coding guidelines and best practices for C++ development in test and measurement systems.

---

## 🎓 Learning Path

### Beginner (Week 1)
1. Complete [Quick Start Guide](../../QUICKSTART.md)
2. Read [System Architecture](architecture/SYSTEM_ARCHITECTURE.md) overview
3. Try [Tutorial 1: Power Supply Test](tutorials/POWER_SUPPLY_TEST.md)
4. Study [Test Step Reference](reference/TEST_STEPS.md)

### Intermediate (Week 2-3)
1. Learn [Test Sequence Creation](guides/TEST_SEQUENCES.md)
2. Complete [Tutorial 2: Parallel Testing](tutorials/PARALLEL_TESTING.md)
3. Study [Configuration Management](guides/CONFIGURATION.md)
4. Review [Database Setup](guides/DATABASE_SETUP.md)

### Advanced (Week 4+)
1. Read [Plugin Development Guide](guides/PLUGIN_DEVELOPMENT.md)
2. Complete [Tutorial 4: Custom Plugins](tutorials/CUSTOM_PLUGIN.md)
3. Study [Threading & Concurrency](architecture/THREADING.md)
4. Review [Performance Profiling](../PERFORMANCE_PROFILING.md)

---

**Happy Testing!** 🚀

*Last updated: 2025-11-23 | TestMATE v2.0 | Production Ready*
