# Helix
**The C++ package manager from hell.**

### The Problem
While C++ is a powerful and versatile language, managing dependencies and standardizing the build process remains notoriously complex. Existing solutions are often bloated or require steep learning curves.

### The Solution
Helix is a radically simple package manager and build tool. It standardizes project initialization, dependency fetching, and compilation into a seamless, zero-friction workflow.

---

## Installation
Clone the repository and build the binary using CMake:

```bash
git clone [https://github.com/chirag-diwan/Helix.git](https://github.com/chirag-diwan/Helix.git)
cd Helix
mkdir build && cd build
cmake ..
cmake --build .
mv helix ~/.local/bin/helix
```

## Usage
Helix currently operates via three core commands:

**helix init** — Initializes a new C++ project structure.

**helix add <github_url>** — Fetches and adds a remote dependency.

**helix build** — Compiles the project based on your configuration.

## Configuration (helix.json)
Your project is driven by a straightforward JSON configuration file.

```JSON
{
    "name": "Helix",              // Project name
    "version": "0.0.1",           // Project version
    "build_flags": ["-O3", "-Wall"], // Compiler flags
    "link_libraries": ["git2", "m", "pthread"], // System libraries to link
    "lib_export_flags": [],       // Flags exported by your binary
    "deps": {}                    // Installed dependencies (managed by Helix)
}
```

## Project Structure
A standard Helix workspace initializes with the following hierarchy:

```bash
Plaintext
.
├── deps/           # Directory for managed dependencies
├── src/
│   └── main.cpp    # Application entry point
└── helix.json      # Project configuration
```
