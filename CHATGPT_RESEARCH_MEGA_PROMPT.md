# ChatGPT Research Mega Prompt

Copy the prompt below into ChatGPT Research after attaching the recommended files.

## Recommended Attachment Set

Attach these first, in this order:

1. `README.md`
2. `CREATE_A_GAME_WALKTHROUGH.md`
3. `main.c`
4. `assets.h`
5. `assets.c`
6. `assets_precomputed.h`
7. `raycasting.h`
8. `raycasting.c`
9. `HUDStuff.h`
10. `HUDStuff.c`
11. `dogm128_fast.h`
12. `dogm128_fast.c`
13. `fx8.h`
14. `fx8.c`
15. `utils.h`
16. `Makefile`
17. `Resources/precompute_assets.py`

Highly beneficial additional files:

- `moduleDogm128.h`
- `moduleDogm128.c`
- `spi.h`
- `spi.c`
- `Resources/MapMaker.html`
- `Resources/map_config.json`
- `Resources/SpriteConverter.html`
- `Resources/img_mask_merger.py`
- `Resources/Story.txt`

If you have room for only a smaller set, do not drop `assets.h`, `main.c`, `raycasting.c`, `dogm128_fast.c`, `HUDStuff.c`, `Makefile`, or `Resources/precompute_assets.py`.

## Prompt

You are doing a deep reverse-engineering style technical analysis of an embedded C game project for a custom board built around a PIC18F4550 and a 128x64 LCD.

I am attaching source files from the repository. Your job is to generate a very detailed report explaining exactly how the entire project works.

The report must be split into two major parts:

1. Part I: The engine
2. Part II: The game / content layer

Part I is the most important part by far and should dominate the document. Treat the engine as the primary subject of the report. The current game/demo is unfinished, so Part II should focus less on the specifics of this one unfinished game and more on how a game is created on top of this engine, how content plugs into it, how maps/dialogues/assets are authored, and what the intended workflow appears to be.

I do not want a shallow summary. I want a report that reads like a serious internal architecture document or reverse-engineered design spec.

## Core Objectives

Produce a report that explains:

- what each module is responsible for
- how modules communicate with each other
- which headers define the major contracts
- which global variables exist, where they live, and who controls them
- which structs carry the important state
- how control flows from boot to the main loop to per-frame rendering
- how timing and interrupts affect behavior
- how rendering works internally
- how input, movement, collision, map events, and dialogue triggers work
- how HUD rendering is layered on top of the world view
- how assets are authored, generated, transformed, and consumed
- what is engine code versus game/content code
- how someone would safely create a new game on top of this codebase
- what parts look unfinished, transitional, duplicated, legacy, or only partially integrated

Everything that can reasonably be drawn out should be drawn out with diagrams.

## Source Discipline Rules

Follow these rules strictly:

- Base claims on the attached files.
- If something is directly supported by source, say so confidently.
- If something is only implied, mark it as inferred.
- If something is missing or ambiguous, say it is unknown or incomplete.
- Do not invent systems or workflows that are not supported by the repository.
- Call out legacy overlap or duplicate subsystems when present.
- Distinguish carefully between build-time behavior and runtime behavior.
- Distinguish carefully between reusable engine surfaces and current game/demo content.

Whenever practical, mention the file and symbol that support a claim.

## Required Output Style

The report should be long, highly structured, and diagram-heavy.

Use Markdown.

Prefer Mermaid diagrams whenever possible. If Mermaid is not possible for a specific diagram, fall back to ASCII diagrams.

For every major section, include:

- a concise explanation in prose
- at least one diagram if the topic can be visualized
- a table where a table communicates better than prose
- clear references to the files/functions/structs involved

Do not optimize for brevity. Optimize for usefulness, clarity, and completeness.

## Required Top-Level Structure

Use this structure exactly:

1. Executive Summary
2. Reading Guide
3. Part I - Engine
4. Part II - Game / Content Layer
5. Incomplete, Legacy, or Transitional Areas
6. Appendix A - File-by-File Responsibility Matrix
7. Appendix B - Global State and Data Ownership Matrix
8. Appendix C - Important Symbols Index

## Part I - Engine Requirements

This section should be the bulk of the output. Cover the engine from multiple angles.

### 1. Engine Boundary and Layering

Explain which files belong to the reusable engine and which files are content/game-owned.

Include:

- a layered architecture diagram
- a module dependency diagram
- a table classifying each major file as engine, game, build tooling, generated artifact, or hardware support

### 2. Boot, Initialization, and Startup Control Flow

Trace startup in detail.

Include:

- hardware init path
- display init path
- PWM/backlight init
- player initialization
- map selection
- callback registration
- any duplicated or overlapping display-init logic

Required diagram:

- sequence diagram from reset / `main()` through first rendered frame

### 3. Main Loop and Frame Pipeline

Explain the full per-frame sequence in order.

Include:

- input polling
- movement update
- map-trigger handling
- world rendering
- HUD rendering
- dialogue advancement behavior
- FPS/debug overlay if present
- display refresh
- LED/backlight updates

Required diagrams:

- per-frame sequence diagram
- high-level dataflow diagram showing what data enters and leaves each stage of a frame

### 4. Timing, Interrupts, and Temporal Behavior

Explain how time is tracked and how time-dependent behavior works.

Cover:

- timer interrupt behavior
- `millis`
- dithering / paint counter interactions
- dialogue timing
- use-button timing
- blink timing or other HUD timing

Required diagrams:

- interrupt-to-runtime timing diagram
- state/timing diagram for any time-driven UI behavior you find

### 5. Data Model and Core State Objects

Explain the important structs and global state in detail.

At minimum analyze:

- `player_t`
- `map_t`
- `dialogue_t`
- `buttons_t`
- bitmap structs
- entity-related structs if present, even if incomplete

Also inventory important globals and stateful statics, for example:

- framebuffer
- timer/global time
- active map/dialogue pointers
- player instance
- button state
- event callback pointer
- rendering counters
- HUD state caches
- movement tile-tracking state

Required diagrams:

- struct relationship diagram
- global state ownership diagram
- memory/data placement style diagram separating const asset data, mutable runtime state, and generated data

Required table:

- variable inventory with columns for name, file, type, lifetime, mutability, owner, readers, writers, and purpose

### 6. Rendering Architecture

Explain how the renderer works at a system level and at an algorithm level.

Cover:

- framebuffer model
- LCD/page model if visible in code
- low-level drawing primitives
- line/rect/blit routines
- masked versus unmasked bitmap behavior
- any optimizations aimed at the microcontroller
- how the 3D world viewport relates to the full display
- how the renderer turns player/camera state into wall columns
- tile interpretation in the renderer

Also explain any overlap between older display modules and the current fast display path.

Required diagrams:

- framebuffer / display memory diagram
- rendering pipeline diagram from `player_t` and `map_t` to pixels on screen
- call graph for the major draw path
- HUD/world screen layout diagram showing how the 128x64 screen is partitioned

### 7. Input, Movement, Collision, and Tile Semantics

Explain exactly how movement works.

Cover:

- button sampling
- forward/back movement
- rotation
- fixed-point direction/plane reconstruction
- collision rules
- walkable versus non-walkable tile semantics
- tile-change detection
- stepping on/off event tiles
- stepping onto dialogue tiles

Required diagrams:

- movement and collision flowchart
- tile callback/control-flow diagram
- tile taxonomy diagram or table covering tile ranges and meanings

### 8. Dialogue and HUD Systems

Explain how dialogue and HUD systems are represented and rendered.

Cover:

- dialogue chaining
- time-based auto-advance
- manual advance behavior
- generated layout macros and how they get used
- banner/minimap/compass/stats/item slot rendering
- POV item overlay rendering
- any caching or static HUD state

Required diagrams:

- dialogue lifecycle/state diagram
- HUD composition diagram
- dependency diagram showing how HUD code consumes player/map/dialogue/assets

### 9. Fixed-Point Math and Numeric Conventions

Explain the numeric model.

Cover:

- fixed-point representation
- angle representation
- trig lookup strategy
- multiplication/division helpers
- reciprocal/clamping behavior
- why this design makes sense on the target hardware

Required diagrams:

- numeric representation diagram for 8.8 fixed-point and angle units
- trig/dataflow diagram if helpful

### 10. Build-Time Tooling and Generated Artifacts

Explain the asset-generation pipeline in detail.

Cover:

- Makefile pre-build hook
- `Resources/precompute_assets.py`
- how dialogue layout is generated
- how spawn points are generated
- what comes from authored source versus generated header output
- which tools are editor-only versus build-critical versus optional helpers

Required diagrams:

- build pipeline diagram
- authored-assets-to-generated-header-to-runtime-consumption diagram

Required table:

- tool matrix with columns for file/tool, role, input, output, when used, and whether runtime depends on it

### 11. Hardware Support and Abstraction Boundaries

Explain how hardware-facing code is separated from game/engine logic.

Cover:

- ports, SPI, display, backlight, LEDs, buttons
- what is board-specific
- what is likely reusable only for this exact board
- where hardware concerns leak into higher layers

Required diagrams:

- hardware abstraction boundary diagram
- module interaction diagram for low-level I/O

### 12. Extension Points, Constraints, and Risks

Explain where a future developer can extend the engine and where they need caution.

Cover:

- adding new maps
- adding new event types
- adding new dialogue systems
- adding new HUD widgets
- replacing assets
- unfinished entity/sprite pipeline
- RAM/flash constraints implied by the design
- tight coupling points

Required output:

- a risk/opportunity table
- a short “what to touch / what not to touch first” guide for engine modifications

## Part II - Game / Content Layer Requirements

Do not spend most of the section retelling the unfinished current demo narrative.

Instead, explain how a game is made with this codebase.

Cover:

- which files a game author changes first
- what belongs in `assets.c` and `assets.h`
- how maps are authored and exported
- how spawn tiles, event tiles, and dialogue tiles work as authoring tools
- how a level is represented
- how banners, minimaps, item icons, and POV assets are represented
- how dialogue content becomes renderable dialogue boxes
- how map callbacks connect game logic into the engine
- how `main.c` selects the active map and game startup state
- how someone would fork this repo into a new game safely
- what parts of the game layer are still unfinished or unstable

Required diagrams:

- “how to create a new game” workflow diagram
- content authoring pipeline diagram
- map-to-runtime-data diagram
- engine versus game ownership diagram
- example event/dialogue authoring flow diagram

Required tables:

- file edit priority table for someone starting a new game
- replace/keep table distinguishing demo content from reusable engine systems

## Incomplete, Legacy, or Transitional Areas

Create a dedicated section that explicitly calls out systems that look unfinished, partially wired, redundant, legacy, or transitional.

Examples of the kinds of things to look for:

- duplicate or overlapping display initialization paths
- partially implemented entity/sprite systems
- content pipelines that exist but are not fully integrated
- older helper modules that coexist with newer optimized paths
- static/demo-specific logic that a future game would probably replace

For each item, say:

- what it is
- where it lives
- why it seems incomplete or transitional
- what impact that has on understanding or extending the project

## Appendix Requirements

### Appendix A - File-by-File Responsibility Matrix

For every important attached file, provide:

- primary responsibility
- exported symbols
- major consumers
- whether it is engine, game, tooling, generated, or hardware support

### Appendix B - Global State and Data Ownership Matrix

Include global variables and important file-static state. Explain who owns each piece of state and who mutates it.

### Appendix C - Important Symbols Index

List key functions, structs, typedefs, globals, and callbacks with a one-line explanation and where they are used.

## Diagram Checklist

Make sure the final report contains as many of these as the evidence supports, and do not skip diagram opportunities:

- layered architecture diagram
- module dependency graph
- startup sequence diagram
- per-frame sequence diagram
- interrupt/timing diagram
- framebuffer/display memory diagram
- render pipeline diagram
- movement/collision flowchart
- tile semantics diagram or table
- dialogue lifecycle diagram
- HUD composition diagram
- struct relationship diagram
- global state ownership diagram
- build/tooling pipeline diagram
- content authoring workflow diagram
- engine versus game ownership diagram
- legacy/overlap diagram if applicable

## Quality Bar

Assume the audience wants to understand the project deeply enough to:

- maintain it
- fork it into a new game
- refactor parts of the engine
- identify weak coupling points
- understand what data is compile-time versus runtime
- see exactly how information flows through the system

Be explicit, diagram-heavy, and technically rigorous.