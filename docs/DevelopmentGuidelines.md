# C++ Development Guidelines

This file provides comprehensive guidance to Claude Code when working with C++ code in this repository.

## Core Development Philosophy

### KISS (Keep It Simple, Stupid)
Simplicity should be a key goal in design. Choose straightforward solutions over complex ones whenever possible. Simple solutions are easier to understand, maintain, and debug.

### YAGNI (You Aren't Gonna Need It)
Avoid building functionality on speculation. Implement features only when they are needed, not when you anticipate they might be useful in the future.

### Design Principles
- **Dependency Inversion**: High-level modules should not depend on low-level modules. Both should depend on abstractions (interfaces/abstract classes).
- **Open/Closed Principle**: Software entities should be open for extension but closed for modification.
- **Single Responsibility**: Each function, class, and translation unit should have one clear purpose.
- **Fail Fast**: Use assertions, exceptions, and early validation to catch errors immediately when issues occur.

## 🧱 Code Structure & Modularity

### File and Function Limits
- **Header files should be under 300 lines** and source files **under 500 lines of code**. If approaching these limits, refactor by splitting into multiple translation units.
- **Functions should be under 50 lines** with a single, clear responsibility.
- **Classes should be under 150 lines** and represent a single concept or entity.
- **Organize code into clearly separated header/source pairs**, grouped by feature or responsibility.
- **Line length should be max 100 characters** - configure clang-format or your formatter accordingly.
- **Use the project's build system** (CMake/Make/etc.) whenever compiling and testing, including for unit tests.

### C++ Specific Guidelines
- **Follow RAII principles** - manage resources through constructors/destructors
- **Prefer stack allocation** over dynamic allocation when possible
- **Use smart pointers** (`std::unique_ptr`, `std::shared_ptr`) for dynamic memory management
- **Include guards or `#pragma once`** in all header files
- **Forward declarations** in headers when possible to reduce compilation dependencies
- **Const-correctness** - mark methods and parameters const when they don't modify state