# Mewgenics CatManager

> **Status: 🚧 In Progress — v0.1 development**

Native C++ mod for **Mewgenics** focused on inspecting and analyzing cat data directly from the game's save files, with a runtime UI integrated into the game.

The project combines save-file reverse engineering, binary data decoding, pedigree reconstruction, relationship analysis and native UI integration.

## Current Features

* Native C++ DLL injected into Mewgenics
* Save discovery and loading
* SQLite save parsing
* LZ4 decompression
* Cat data decoding
* Living / deceased cat tracking
* Pedigree data parsing
* Parent / child relationship reconstruction
* Ancestor traversal with depth tracking
* Common ancestor detection
* Cat relationship analysis:

  * Parent / Child
  * Full siblings
  * Half siblings
  * Other related cats
  * Unrelated cats
* Inbreeding coefficient (COI) extraction
* Family tree data construction
* Runtime in-game UI integration using [MewUI](https://github.com/Pseudonym-Tim/mewgenics-ui-api), a community C/C++ API for Mewgenics DLL mods
* Interactive living-cat navigation directly in-game
* Case-insensitive cat search and filtering
* Exact-match prioritization in search results
* Display of real save data directly in-game
* Modular UI state management
* Regression tests for pedigree, relationship and search analysis

## In Progress

* Dedicated CatManager menu
* Dedicated search interface
* Two-cat selection workflow
* Breeding analysis
* Expected offspring COI calculation
* Breeding risk presentation
* Pedigree visualization
* Family tree navigation
* Breeding assistant
* UI/UX cleanup and finalization

## Architecture

```text
Game / DLL Injection
        │
        ▼
   CatManager Core
        │
        ├── Save Locator
        ├── SQLite Parser
        ├── LZ4 Decoder
        ├── Cat Decoder
        └── Pedigree Parser
                 │
                 ▼
             SaveData
                 │
        ┌────────┴────────┐
        ▼                 ▼
   Family Analysis      Native UI
        │                  │
        ├── Parents        ├── State
        ├── Children       ├── Search
        ├── Ancestors      ├── View
        └── Relationships └── MewUI Integration
                 │
                 ▼
          Breeding Analysis
```

## Tech Stack

* **C++20**
* SQLite
* LZ4
* Windows DLL / native runtime integration
* Mewgenics UI API
* Mewjector
* SWF-based UI assets
* MSVC

## Project Structure

```text
CatManager/
├── Native/
│   ├── Save/
│   │   ├── Analysis/
│   │   ├── Compression/
│   │   ├── Decoder/
│   │   ├── Model/
│   │   ├── Pedigree/
│   │   └── Repository/
│   └── UI/
│       ├── CatManagerState.h
│       ├── CatManagerSearch.cpp/.h
│       ├── CatManagerView.cpp/.h
│       ├── CatManagerBreeding.cpp/.h
│       └── mew_ui_api.c/.h
│
├── Tests/
│   ├── TestUtils.h
│   ├── CatInspectorTests.cpp
│   ├── CatSearchTests.cpp
│   └── TestMain.cpp
│
├── data/
├── doc/
├── swfs/
├── build/
└── build.bat
```

## Development

The project is currently developed against a local Mewgenics installation and is **not yet considered production-ready**.

The project uses a standalone regression test executable for the core analysis layer.

Run the tests with:

```bat
build.bat test
```

The normal build runs the tests before compiling and deploying the mod:

```bat
build.bat
```

A failed regression test stops the build before the DLL is deployed.

The current runtime UI is still built on top of the development/test SWF assets. The next stage is to turn the working UI infrastructure into the dedicated CatManager interface.

## Testing

The regression tests currently cover:

* Cat lookup
* Parent / child relationships
* Full siblings
* Half siblings
* Unrelated cats
* Common ancestors
* Ancestor depth tracking
* Cat search
* Case-insensitive matching
* Exact-match prioritization
* Living/deceased filtering

Tests operate on synthetic `SaveData` structures and do not modify real save files.

Example successful test run:

```text
===== RUNNING TESTS =====
===== CatManager Tests =====
[PASS] CatInspectorTests
[PASS] CatSearchTests
===== ALL TESTS PASSED =====
```

## Save Safety

CatManager currently operates in **read-only analysis mode**.

The mod parses and analyzes save data but does not modify the original save file.

## Goals

The long-term goal is to provide a complete in-game cat management and breeding analysis tool, including:

* Cat search and management
* Pedigree exploration
* Relationship analysis
* Common ancestor analysis
* Expected offspring COI calculation
* Breeding risk information
* Breeding assistance
* Family tree navigation

All of this is intended to operate without modifying the original save data.

---

**This project is a work in progress. Features, architecture and UI are still evolving.**
