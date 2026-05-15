# Third-Party Dependencies

This directory contains external dependencies used by TestMATE.

## Dependencies

### Required
- **Qt 6.2+** - UI framework (system install or vcpkg)
- **gRPC** - Remote procedure calls
- **PostgreSQL** - Database client library
- **Python 3.8+** - Scripting support

### Bundled (submodules)
- **googletest** - Unit testing framework
- **spdlog** - Fast logging library
- **pybind11** - Python bindings
- **nlohmann_json** - JSON library

## Setup

To initialize submodules:
```bash
git submodule update --init --recursive
```

## Package Managers

On Windows, use vcpkg:
```bash
vcpkg install qt6 grpc postgresql pybind11
```

On Ubuntu/Debian:
```bash
sudo apt install qt6-base-dev libgrpc++-dev libpq-dev python3-dev
```

On macOS:
```bash
brew install qt@6 grpc postgresql python
```
