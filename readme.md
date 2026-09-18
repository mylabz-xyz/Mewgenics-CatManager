# Mewgenics CatManager

> **Status: 🚧 In Progress — v0.1 development**

Native C++ mod for **Mewgenics** focused on inspecting and analyzing cat data directly from the game's save files, with a runtime UI integrated into the game.

The project combines save-file reverse engineering, binary data decoding, pedigree reconstruction, relationship analysis, breeding analysis and native UI integration.

## Current Features

* Native C++ DLL injected into Mewgenics through **Mewjector**
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
* Breeding analysis
* Expected offspring COI calculation
* Breeding risk estimation
* Two-cat breeding selection
* Independent Cat A / Cat B navigation
* Common ancestor display for selected breeding pairs
* Runtime in-game UI integration using [MewUI](https://github.com/Pseudonym-Tim/mewgenics-ui-api), a community C/C++ API for Mewgenics DLL mods
* Interactive living-cat navigation directly in-game
* Case-insensitive cat search and filtering
* Exact-match prioritization in search results
* Display of real save data directly in-game
* Modular UI state management
* Regression tests for pedigree, relationship, search and breeding analysis

## In Progress

* Dedicated CatManager menu
* Dedicated search interface
* Pedigree visualization
* Family tree navigation
* Breeding assistant
* UI/UX cleanup and finalization
* Further breeding analysis and presentation

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
        ┌───────┴────────┐
        ▼                ▼
   Family Analysis    Native UI
        │                │
        ├── Parents      ├── State
        ├── Children     ├── Search
        ├── Ancestors    ├── View
        ├── Relationships├── Breeding
        └── Common       └── MewUI Integration
            Ancestors
                │
                ▼
        Breeding Analysis
                │
        ┌───────┴────────┐
        ▼                ▼
 Expected Offspring   Breeding Risk
      COI
```

## Tech Stack

* **C++20**
* SQLite
* LZ4
* Windows DLL / native runtime integration
* Mewjector
* Mewgenics UI API
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
│   │
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
│   ├── BreedingAnalyzerTests.cpp
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

The core analysis layer is covered by a standalone regression test executable.

Run the tests with:

```bat
build.bat test
```

The normal build runs the tests before compiling and deploying the mod:

```bat
build.bat
```

A failed regression test stops the build before the DLL is deployed.

The runtime UI is currently implemented using SWF-based Mewgenics UI assets and the Mewgenics UI API.

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
* Breeding analysis
* Expected offspring COI calculation

Tests operate on synthetic `SaveData` structures and do not modify real save files.

Example successful test run:

```text
===== RUNNING TESTS =====

===== CatManager Tests =====

[PASS] CatInspectorTests
[PASS] CatSearchTests
[PASS] BreedingAnalyzerTests

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
* Pedigree visualization

All of this is intended to operate without modifying the original save data.

---

**This project is a work in progress. Features, architecture and UI are still evolving.**
