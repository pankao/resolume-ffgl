# AGENTS.md

# FFGL SDK Development Workspace

This repository contains the official Resolume FFGL SDK together with custom GPU plugins developed for Resolume Arena.

The repository should be treated as a long-term software project rather than a collection of unrelated plugin experiments.

---

# Development Environment

Operating System

* Windows 11

Editor

* Visual Studio Code

Compiler

* Microsoft Visual C++ Build Tools

Build System

* MSBuild

Graphics API

* OpenGL

Shader Language

* GLSL

Target Host

* Resolume Arena

The Visual Studio IDE should never be required.
Assume development is performed entirely from VS Code using MSBuild from the command line.

---

# Repository Philosophy

The official FFGL SDK should remain as untouched as reasonably possible.

Custom plugins should be isolated from the SDK examples.

Avoid modifying official example plugins unless explicitly requested.

When new plugins are needed, create independent copies instead of extending or replacing SDK examples.

---

# Creating a New Plugin

When creating a new plugin:

1. Select the official SDK plugin that most closely matches the desired plugin type.
2. Copy the complete source folder.
3. Copy the corresponding project file.
4. Rename:

   * source files
   * header files
   * C++ classes
   * plugin registration
   * project file
   * output DLL
5. Give the plugin a unique FFGL plugin ID.
6. Give the plugin a unique display name.
7. Ensure the copied plugin builds before implementing any new functionality.

Never modify the original SDK example while creating a new plugin.

---

# Development Workflow

Always work incrementally.

Preferred workflow:

1. Build successfully.
2. Verify the plugin loads in Resolume.
3. Make one visible change.
4. Verify again.
5. Add one feature.
6. Verify again.

Avoid implementing multiple major features simultaneously.

Every intermediate state should compile successfully.

---

# Build Philosophy

Assume builds are performed from the command line.

Do not require Visual Studio solutions or the Visual Studio IDE.

Prefer building individual plugin projects whenever practical.

If project configuration changes are required, make the minimum necessary modifications.

---

# Architecture Guidelines

Keep responsibilities separated.

C++ should primarily handle:

* FFGL lifecycle
* plugin registration
* parameters
* OpenGL resource management
* shader compilation
* framebuffer management
* texture allocation

GLSL should primarily handle:

* visual algorithms
* image processing
* sampling
* color manipulation
* feedback algorithms
* procedural generation

Avoid implementing image processing on the CPU unless absolutely necessary.

---

# Performance Guidelines

Realtime performance is the highest priority.

Prefer GPU solutions whenever possible.

Avoid:

* CPU pixel processing
* texture readback
* unnecessary allocations
* unnecessary OpenGL state changes
* recreating GPU resources every frame

Prefer:

* framebuffer objects
* ping-pong rendering
* texture feedback
* texture lookups
* persistent GPU resources

---

# Code Style

Prefer readable code over clever code.

Use descriptive names.

Comment algorithms.

Keep functions reasonably short.

Avoid unnecessary abstraction.

Future maintainability is more important than reducing line count.

---

# Shader Style

Shaders should be readable.

Organize shaders into logical sections.

Example structure:

* uniforms
* utility functions
* sampling
* algorithm
* output

Comment important mathematical ideas.

Avoid cryptic variable names.

---

# Parameters

Expose artist-friendly parameters.

Good examples:

* Amount
* Feedback
* Brightness
* Contrast
* Decay
* Friction
* Speed
* Phase
* Blur
* Radius
* Scale
* Threshold
* Saturation

Avoid exposing internal implementation variables.

Parameter names should describe artistic behaviour rather than implementation details.

---

# Reusable Code

Before introducing helper classes or utility functions, consider whether they are likely to be reused by multiple plugins.

General-purpose functionality should gradually evolve into shared utilities.

Plugin-specific code should remain inside the plugin.

Avoid premature abstraction.

---

# Working With Existing Code

Before making modifications:

1. Inspect the existing implementation.
2. Explain how it currently works.
3. Identify the smallest required change.
4. Implement that change.
5. Verify the project still builds.

Do not replace large sections of code without justification.

---

# Communication

When implementing a requested feature:

* explain the current implementation
* explain the proposed modification
* implement the change
* explain what changed
* explain why

When uncertain:

Ask for clarification instead of guessing.

---

# Project Goal

The long-term objective is to build a collection of high-performance GPU-based FFGL plugins for Resolume.

The plugins should prioritize:

* realtime performance
* predictable behaviour
* visual quality
* maintainability
* extensibility

The repository should gradually evolve into a reusable FFGL framework while remaining compatible with the official SDK.