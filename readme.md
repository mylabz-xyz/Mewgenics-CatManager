# Mewgenics CatManager

> **Status: 🚧 In Progress**

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
* Ancestor traversal
* Common ancestor detection with depth tracking
* Cat relationship analysis:

  * Parent / Child
  * Full siblings
  * Half siblings
  * Other related cats
  * Unrelated cats
* Inbreeding coefficient (COI) extraction
* Family tree data construction
* Runtime in-game UI integration using [MewUI](https://github.com/Pseudonym-Tim/mewgenics-ui-api), a community C/C++ API for Mewgenics DLL mods
* Interactive cat selection through the in-game UI
* Display of real save data directly in-game
* Regression tests for pedigree and relationship analysis

## In Progress

* Dedicated CatManager UI
* Cat list and navigation
* Search and filtering
* Pedigree visualization
* Family tree navigation
* Inbreeding / breeding analysis
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
        ├── Parents        ▼
        ├── Children   Mewgenics UI
        ├── Ancestors
        └── Relationships
```

## Tech Stack

* **C++20**
* SQLite
* LZ4
* Windows DLL / native runtime integration
* Mewgenics UI API
* Mewjector
* SWF-based UI assets

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
│
├── Tests/
│   └── CatInspectorRegressionTests.cpp
│
├── data/
├── doc/
├── swfs/
├── build/
└── build.bat
```

## Development

The project is currently developed against a local Mewgenics installation and is **not yet considered production-ready**.

The project currently uses a standalone regression test executable for core pedigree analysis.

Run the tests with:

```bat
build.bat test
```

The normal build runs the tests before compiling and deploying the mod:

```bat
build.bat
```

A failed regression test stops the build before the DLL is deployed.

The current UI is still based on the test SWF used during development. A dedicated CatManager interface will replace it as the project progresses.

## Testing

The current regression tests cover the core relationship analysis layer, including:

* Cat lookup
* Parent / child relationships
* Full siblings
* Half siblings
* Unrelated cats
* Common ancestors
* Related cats with non-direct relationships

Tests operate on synthetic `SaveData` structures and do not modify real save files.

## Save Safety

CatManager currently operates in **read-only analysis mode**.

The mod parses and analyzes save data but does not modify the original save file.

## Goals

The long-term goal is to provide a complete in-game cat management and breeding analysis tool, including pedigree exploration, relationship analysis and breeding assistance without modifying the original save data.

---

**This project is a work in progress. Features, architecture and UI are still evolving.**
