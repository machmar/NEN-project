# Creating a Game in This Engine

This walkthrough explains how to build a new game by forking this repository and replacing the demo content with your own. It follows the actual structure of this codebase instead of describing a hypothetical engine.

Scope of this guide:

- It covers the project layout, build flow, maps, dialogue, HUD assets, demo cleanup, and practical integration steps.
- It intentionally skips the sprite pipeline for now. That part is still in flux, and the current main loop does not render entities yet.

If you follow this document in order, you should end up with a clean fork that boots into your own level instead of the current demo content.

## 1. Understand What You Are Forking

Before deleting anything, know where the engine behavior actually lives.

Main files you will touch:

- `main.c`: hardware init, main loop, active map selection, player initialization.
- `assets.c`: maps, dialogue chains, minimaps, banners, item bitmaps, and other game-owned content.
- `assets.h`: content declarations and the `map_t`, `dialogue_t`, and `player_t` contracts.
- `raycasting.c`: movement, wall collision, tile interpretation, event and dialogue triggers, wall rendering.
- `HUDStuff.c`: minimap, banner, stats, item slot, dialogue box rendering.
- `Makefile`: build hook that regenerates `assets_precomputed.h` before compile.
- `Resources/MapMaker.html`: map authoring tool.
- `Resources/map_config.json`: palette and tile metadata used by `MapMaker.html` only.

Files you should treat as generated or tool output:

- `assets_precomputed.h`: generated from `assets.c`. Do not hand-edit it.
- `build/...`: compiler output.

Files that are currently demo content rather than engine code:

- Most of the concrete content inside `assets.c`.
- The active map choice in `main.c`.
- `Resources/Story.txt`.
- Most source art in `Resources/Art/`.

## 2. Build the Original Fork Before You Change Anything

Do one clean build before stripping content. That gives you a known-good baseline and proves your toolchain works.

What the build expects:

- MPLAB X / XC8 for the PIC18F4550 project.
- Python 3 available as `py -3` on Windows.

Important build fact:

- The top-level `Makefile` runs:

```powershell
py -3 Resources/precompute_assets.py --source assets.c --output assets_precomputed.h
```

That script scans `assets.c` and generates:

- dialogue layout macros such as `DIALOGUE_LAYOUT_dialogue_Room1`
- spawn-point macros such as `MAP_SPAWN_WallDemoMap`

Practical rule:

- If you rename, add, or delete maps or dialogues in `assets.c`, expect `assets_precomputed.h` to change on the next build.
- Do not fix generated-content problems by editing `assets_precomputed.h` directly. Fix the source in `assets.c` and rebuild.

## 3. Keep the Engine Model in Your Head

The display is `128x64`, but it is not used as one flat full-screen game viewport.

Current layout:

- The 3D world view uses the left `96x64` area.
- The renderer casts `48` rays and draws each as a 2-pixel vertical stripe.
- The right `32x64` area is HUD: minimap, item slot, compass, stats, and dialogue-related UI.

Current frame flow in `main.c` is:

1. Read buttons.
2. Move the player with `MoveCamera(...)`.
3. Render walls with `RenderFrame(...)`.
4. Draw HUD elements.
5. Refresh the LCD.

This matters because your game content must fit that layout. A beautiful 128-pixel-wide HUD banner will still be wrong if the HUD code only expects `76x8` banner art.

## 4. Know the Runtime Tile Contract

Your maps are byte grids. The tile value itself drives collision, events, and dialogue.

Current tile meaning:

| Range | Meaning | Walkable | Notes |
| --- | --- | --- | --- |
| `0x00` | empty space | yes | default floor |
| `0x01-0x0F` | solid walls | no | rendered as opaque wall variants |
| `0x10-0x1F` | transparent wall variants | mixed | only `0x10` is walkable in current movement code |
| `0x20` | spawn marker | yes | the build script scans maps for this tile |
| `0x30-0x3F` | event tiles | yes | `OnEventTile` receives the low nibble as event number |
| `0x40-0xEF` | dialogue tiles | yes | `OnDialogueTile` receives the raw tile value |
| `0xF0-0xFF` | currently unused by built-in logic | yes | safe only if you define your own meaning |

Two very important details:

1. Only `0x10` is walkable among the transparent-wall range in the current code. Do not assume all transparent-looking walls can be passed through.
2. Event and dialogue callbacks only fire when the player enters a new cell. Standing still on a tile does nothing.

## 5. Know the Map Memory Layout Before You Edit Maps

This engine stores maps in an unusual but consistent shape.

The code treats map data as:

- logical dimensions: `width x height`
- memory order: `x-major`
- access rule: `data[x * height + y]`

That means the 2D arrays in `assets.c` are intentionally declared as:

```c
static const uint8_t MyMap_data[WIDTH][HEIGHT] = {
    ...
};
```

Not this:

```c
static const uint8_t MyMap_data[HEIGHT][WIDTH] = {
    ...
};
```

If you transpose that shape, the game will compile but the map will behave incorrectly.

Useful fact:

- `Resources/MapMaker.html` already exports maps in the same `x-major` style, so its C output matches the runtime contract.

## 6. Start the Fork Safely: Duplicate First, Delete Later

The fastest way to break this project is to delete all demo content before you have a replacement wired in.

The safer workflow is:

1. Build the original project once.
2. Pick one demo map to copy as a scaffold. `WallDemoMap` is the best starting point because it already demonstrates event and dialogue tiles.
3. Duplicate it under your own names in `assets.c`.
4. Point `CurrentMap` in `main.c` at your new map.
5. Build again.
6. Only after your map boots correctly, start removing old demo maps, dialogues, banners, and art.

Why this order works:

- you always keep one playable path alive
- the generated spawn/dialogue header keeps matching something real
- you can remove demo pieces gradually instead of debugging a blank repo

## 7. Strip Demo Material in the Right Order

These are the main demo-specific surfaces you will eventually replace.

In `assets.c`:

- demo maps like `SmallMap`, `BigMap`, `TestMap`, `AgentOrangeMap`, `WallDemoMap`
- demo map banners and minimaps
- demo dialogue chains like `dialogue_Room1`, `dialogue_GunFound1`, and similar
- current demo item and HUD art if you do not want to keep placeholders

In `assets.h`:

- extern declarations for maps and dialogues you removed from `assets.c`

In `main.c`:

- `CurrentMap = &WallDemoMap;`
- any demo-specific initialization choices

In `Resources/`:

- `Story.txt`
- old exported art that no longer belongs to your game

Recommended removal order:

1. Add your own map and point `CurrentMap` to it.
2. Replace or remove demo dialogues that are still referenced by your callbacks.
3. Remove unused map declarations from `assets.h`.
4. Remove unused content blocks from `assets.c`.
5. Remove obsolete source art from `Resources/Art/`.

## 8. Create Your First Real Map

### Use `Resources/MapMaker.html`

The map editor is the cleanest way to make a new level.

What it does well:

- edit byte-valued tile grids visually
- import and export map JSON
- export a C array matching the engine layout
- export a wall bitmap PNG for minimap/source-art work
- edit the type/kind palette used by the editor

What `map_config.json` is for:

- it configures the editor palette and labels
- the engine does not load `map_config.json` at runtime
- you can change it freely without touching the firmware unless you also change the actual tile values you export into `assets.c`

Map rules you should follow immediately:

- place exactly one spawn tile `0x20`
- keep the outer border non-walkable unless you intentionally want edge escape
- decide early which event numbers you will reserve from `0x30` to `0x3F`
- decide early which dialogue tile numbers you will reserve from `0x40` upward

Practical tip:

- If your game will have more than one level, reserve event numbers and dialogue ranges consistently from the start. For example, use `0x30-0x33` for pickups, `0x34-0x37` for damage/heal/level triggers, and `0x40-0x4F` for narrative prompts.

### Export the map into `assets.c`

For each map you need:

1. the tile data array
2. a minimap bitmap
3. a banner bitmap
4. a `map_t` initializer
5. optional event and dialogue callbacks

Minimal template:

```c
static const uint8_t MyLevel_data[24][24] = {
    ...
};

static const uint8_t MyLevel_minimap_data[] = {
    ...
};

const dogm128_bitmap_t MyLevel_minimap = {32, 32, MyLevel_minimap_data};

static const uint8_t MyLevel_banner_data[] = {
    ...
};

const dogm128_bitmap_t MyLevel_banner = {76, 8, MyLevel_banner_data};

static void MyLevel_OnEventTile(uint8_t eventNum, _Bool stepOn, player_t *player, const dialogue_t **pDialogue) {
    (void)eventNum;
    (void)stepOn;
    (void)player;
    (void)pDialogue;
}

static void MyLevel_OnDialogueTile(uint8_t tileVal, const dialogue_t **pDialogue) {
    (void)tileVal;
    (void)pDialogue;
}

map_t MyLevel = {
    24,
    24,
    MyLevel_data,
    MAP_SPAWN_MyLevel,
    &MyLevel_minimap,
    {5, 26},
    {4, 4},
    &MyLevel_banner,
    MyLevel_OnEventTile,
    MyLevel_OnDialogueTile,
};
```

Useful facts about `map_t`:

- `DefaultSpwanPoint` is misspelled in the struct. Keep the existing spelling unless you want to rename it across the codebase.
- The spawn point is not typed manually in normal use. `MAP_SPAWN_MyLevel` is generated by scanning the map for tile `0x20`.
- If you omit the last callback fields from the initializer, they default to `NULL`. That is valid for maps without events or dialogue.

## 9. Tune the Minimap and Banner Instead of Fighting Them

The HUD code has strong expectations about map-side art.

Current conventions from the repo:

- map banners are `76x8`
- minimaps are `32` pixels wide
- minimap height can be `32` or taller

How minimap scrolling works:

- if the minimap is taller than `32`, `HUD_DrawMap(...)` scrolls it vertically
- `minimapScrollthresholds[0]` is the top threshold
- `minimapScrollthresholds[1]` is the bottom threshold
- `minimapTopLeftOffset` controls where the player dot sits relative to the minimap pixels

Practical advice:

- For your first level, keep the minimap `32x32` if possible. It removes a whole class of HUD tuning mistakes.
- Only move to taller minimaps after the level actually plays well.

## 10. Plain Bitmap Workflow: What Is Automated and What Is Not

This is the part most likely to confuse a new fork, so be explicit about it.

What is already supported in-repo:

- `MapMaker.html` can export a wall bitmap PNG that is useful as a minimap/source image.
- `SpriteConverter.html` and `img_mask_merger.py` exist for mask/color-interleaved assets.

What is not fully supported in-repo yet:

- there is no dedicated one-click converter in this repo for plain `dogm128_bitmap_t` assets such as banners, minimaps, and simple HUD icons

That means plain bitmaps are currently a manual integration step.

The engine-side format for plain bitmaps is:

```c
const dogm128_bitmap_t MyBitmap = { width, height, data_array };
```

Packing expectations:

- data is stored in 8-pixel-high page rows
- each byte is one vertical column slice
- the top pixel in the byte is bit 0
- aligned blits expect page-aligned data

Practical recommendation while sprites are still in progress:

- keep plain art simple
- reuse the size conventions already present in the repo
- if needed, edit copied demo arrays into placeholders first, then replace them with final art later

## 11. Add Dialogue Only After the Map Boots

Once your map loads and movement works, add narrative.

Dialogue lives in `assets.c` as chained `dialogue_t` instances.

Example:

```c
const dialogue_t dialogue_Intro2 = {
    "Now find a way out",
    1800,
    NULL,
    DIALOGUE_LAYOUT_dialogue_Intro2
};

const dialogue_t dialogue_Intro1 = {
    "I should not be here",
    1800,
    &dialogue_Intro2,
    DIALOGUE_LAYOUT_dialogue_Intro1
};
```

Important build fact:

- the `DIALOGUE_LAYOUT_...` macro is generated from the text string in `assets.c`
- you do not calculate line positions by hand

Useful behavior details from the generator:

- it wraps text at roughly 21 characters per line
- it supports up to 5 lines
- it computes the dialogue box size automatically

Practical writing tips:

- keep lines short and blunt; the font is tiny
- test each dialogue in-game because line breaks are generated from real text, not from your assumptions
- avoid huge dialogue chains until your level flow works

## 12. Wire Events and Dialogue to Tiles

There are two callback types on a map.

### Event tiles: `0x30-0x3F`

When the player steps on or off one of these tiles:

- `OnEventTile(eventNum, stepOn, player, pDialogue)` is called
- `eventNum` is the low nibble only, so tile `0x33` becomes event number `3`

Use event tiles for:

- pickups
- damage zones
- healing stations
- level exits
- scripted state changes

Example pattern:

```c
static void MyLevel_OnEventTile(uint8_t eventNum, _Bool stepOn, player_t *player, const dialogue_t **pDialogue) {
    if (!stepOn)
        return;

    switch (eventNum) {
        case 0:
            player->health = 5;
            *pDialogue = &dialogue_HealNotice;
            break;
        case 1:
            *pDialogue = &dialogue_ExitNotice;
            break;
    }
}
```

### Dialogue tiles: `0x40-0xEF`

When the player enters one of these tiles:

- `OnDialogueTile(tileVal, pDialogue)` is called once on entry

Use dialogue tiles for:

- one-line observations
- room comments
- story beats
- tutorial prompts

Example pattern:

```c
static void MyLevel_OnDialogueTile(uint8_t tileVal, const dialogue_t **pDialogue) {
    if (tileVal == 0x40)
        *pDialogue = &dialogue_Intro1;
    else if (tileVal == 0x41)
        *pDialogue = &dialogue_LabNote1;
}
```

Useful fact:

- `main.c` also exposes a global `MapEventCallback`. You can ignore it for a single-map game. It is just an extra hook if you want global reactions outside the map callbacks.

## 13. Hook Your Game into `main.c`

At minimum, you must replace the active map.

Current demo setup:

```c
map_t *CurrentMap = &WallDemoMap;
```

Change that to your own map:

```c
map_t *CurrentMap = &MyLevel;
```

Then check the rest of the initialization:

- `camera.posX` and `camera.posY` are loaded from the generated spawn macro
- `camera.health`, `camera.kills`, and `camera.currentItem` define the starting state
- `CurrentDialogue` starts as `NULL`

Good first-pass setup:

- start with full health
- start with no active dialogue
- start with the simplest item state you support

If you later add multiple levels, the transition rule is straightforward:

1. switch `CurrentMap`
2. set player position from the new map spawn
3. clear or set `CurrentDialogue`
4. reset any per-level state you care about

## 14. Button Layout and Core Game Feel

The button mapping is currently hard-coded in `main.c`.

Current behavior:

- left button: turn left
- right button: turn right
- one button: move forward
- one button: move backward
- center/use button: advance dialogue and future interactions

Before adding content-heavy mechanics, verify the following on your new map:

- movement feels readable
- door and wall types communicate collision correctly
- the player can recover if they spawn in a tight space
- event tiles cannot be triggered accidentally every second step unless that is intended

## 15. HUD-Side Content You Still Need Even Without Sprites

Skipping sprites does not mean you skip all visuals.

The current HUD still expects:

- a map banner
- a minimap
- optionally item icons
- health/kills display behavior that makes sense for your game

Functions already drawing those elements:

- `HUD_DrawBanner(...)`
- `HUD_DrawMap(...)`
- `HUD_DrawBorders()`
- `HUD_DrawItem(...)`
- `HUD_DrawCompass(...)`
- `HUD_DrawStats(...)`
- `HUD_DrawDialogue(...)`

Practical advice while sprite work is deferred:

- leave `HUD_DrawItemPOV(...)` alone if you are not finalizing first-person weapon art yet
- use clean placeholder icons and banners first
- focus on map readability, timing, and dialogue flow before polish art

Useful fact:

- `DrawEntities(...)` exists in `raycasting.c`, but it is not called by the current main loop. That is why skipping sprite production is a safe choice for now.

## 16. Performance and Memory Rules That Matter on This Hardware

This project targets a PIC18F4550 with very limited memory.

Treat these as real constraints, not nice suggestions.

Rules that help immediately:

- keep large content arrays `static const`
- remove unused maps and art instead of leaving them in the binary
- keep dialogue short
- keep banners and minimaps small and simple
- avoid carrying demo content once you no longer need it

Where memory goes quickly:

- map tile arrays
- minimap arrays
- banner arrays
- dialogue strings
- large placeholder art you forgot to delete

Practical strategy:

- first get one map working
- then add one more map only if you still have flash headroom
- do not build three chapters of content before you know the binary size budget

## 17. Common Mistakes and Their Real Causes

### The player spawns in the wrong place

Likely cause:

- your map layout is transposed
- or the `0x20` spawn tile is not where you think it is

### Event tiles do nothing

Likely cause:

- the tile is not in `0x30-0x3F`
- the callback is missing from the `map_t`
- the player never actually crosses into a new cell

### Dialogue never appears

Likely cause:

- the tile is outside `0x40-0xEF`
- `OnDialogueTile` does not assign `*pDialogue`
- the wrong dialogue symbol name is referenced after a rename

### Build breaks after editing `assets.c`

Likely cause:

- missing spawn tile
- renamed dialogue without matching generated macro on rebuild
- initializer shape no longer matches the struct contract

### Minimap dot looks offset or scrolls badly

Likely cause:

- `minimapTopLeftOffset` is wrong
- `minimapScrollthresholds` do not match the map height and desired framing

### Transparent walls behave differently than expected

Likely cause:

- only `0x10` is walkable in the current movement logic
- the other `0x11-0x1F` variants still block movement even if they render differently

## 18. Recommended First Milestone

Do not aim for a complete game immediately. Aim for one clean playable slice.

Good first milestone:

1. Fork the repo.
2. Build the original demo once.
3. Duplicate `WallDemoMap` into your own map symbol.
4. Replace the active map in `main.c`.
5. Replace the banner.
6. Replace the dialogue text.
7. Add one event tile that changes player state.
8. Remove at least one old demo map from the build.
9. Rebuild and test again.

If that works, you have already proven the whole loop:

- content authoring
- code integration
- generated header regeneration
- HUD integration
- tile callback logic

That is the real foundation of the project.

## 19. A Good Cleanup Checklist Before You Call the Fork Yours

- `CurrentMap` no longer points at `WallDemoMap`
- your map has exactly one `0x20` spawn tile
- your map compiles with `MAP_SPAWN_YourMapName`
- all demo dialogue references used by your game are gone or renamed
- `assets.h` no longer exports maps you deleted
- `Resources/Story.txt` is replaced or removed
- old art in `Resources/Art/` is removed or clearly separated from your game
- `assets_precomputed.h` is only generated content, never hand-maintained
- the game boots into your level and not a demo room

## 20. Final Recommendation

Treat this engine as content-driven through `assets.c` and `main.c`.

The best working rhythm is:

1. define a small level
2. make it compile
3. make it playable
4. add dialogue and triggers
5. remove obsolete demo content
6. only then expand scope

That order matches the repo you have today, keeps the generated header sane, and avoids burning time on unfinished systems before the core game loop is solid.