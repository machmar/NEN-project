# NEN Board Engine

This repository is the game engine and firmware base for our specific NEN board built around the PIC18F4550 and a 128x64 LCD. It is no longer just the current demo project.

The goal of this repo is to stay as a reusable engine for this exact hardware target, so new games should be created by forking it and replacing the demo materials with your own maps, dialogue, HUD assets, and game logic.

## Hardware Target

- PIC18F4550
- 2 KB RAM
- 32 KB flash
- 128x64 monochrome LCD
- 5 buttons
- 5 button LEDs
- PWM-controlled backlight

## What The Engine Currently Provides

- a raycasting renderer tuned for this board
- fixed-point math helpers for movement and camera logic
- a `96x64` world viewport with a `32x64` HUD strip
- map-driven collision, event tiles, and dialogue tiles
- minimap, banner, compass, item slot, stats, and dialogue HUD rendering
- a build-time asset precompute step for spawn points and dialogue layout
- low-level LCD and SPI support for the target board

## Start Here

- [Create a game walkthrough](CREATE_A_GAME_WALKTHROUGH.md): full start-to-finish guide for turning this fork into your own game

Recommended workflow:

1. Fork the repo.
2. Build the original project once.
3. Duplicate or replace the demo maps and dialogue in `assets.c`.
4. Point `CurrentMap` in `main.c` at your own level.
5. Remove old demo content once your replacement boots correctly.

## Project Layout

- `main.c`: board init, main loop, active map selection, player startup state
- `assets.c`: maps, dialogue, minimaps, banners, HUD assets, and other game-owned content
- `assets.h`: content declarations and core content structs
- `raycasting.c`: renderer, movement, wall behavior, tile trigger dispatch
- `HUDStuff.c`: HUD and dialogue rendering
- `dogm128_fast.c`: LCD drawing primitives and bitmap blitting
- `Resources/MapMaker.html`: map editor for authoring and exporting map data
- `Resources/precompute_assets.py`: generates `assets_precomputed.h` from `assets.c`

## Build Notes

The top-level `Makefile` runs a pre-build asset generation step:

```powershell
py -3 Resources/precompute_assets.py --source assets.c --output assets_precomputed.h
```

That means:

- `assets_precomputed.h` is generated, not hand-maintained
- changing dialogue text or map spawn tiles in `assets.c` affects generated output on the next build

## Current Status

The engine is already usable for building board-specific games around:

- first-person exploration
- map-based triggers
- dialogue-driven progression
- HUD-heavy gameplay on constrained hardware

The sprite pipeline is still being worked on, so the current walkthrough deliberately focuses on the stable parts of the engine first: maps, HUD, dialogue, events, and board integration.

Current renderer running on hardware:

![Renderer running on hardware](https://github.com/user-attachments/assets/7955fe87-7f4a-43e5-aa2a-92a638bc3312)
