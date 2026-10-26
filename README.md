# AgDrag 32:9 — Unofficial Update

An **unofficial, community-made update** of **Agnostic Dragon (AgDrag)** by **Kaldaien**, the ultrawide fix for *Dragon's Dogma: Dark Arisen*.

This version adds proper **32:9 support** (tested at 5120×1440) and makes AgDrag work with the **current game executable** and **recent versions of Special K**.

I originally made these changes for my own setup, but decided to share them in case they help other players with ultrawide monitors. Please note that this is a personal project: **I won't be actively maintaining or providing support for this version**. If you'd like to improve it further, you're very welcome to fork it and build on it.

**AI disclosure:** The changes in this update were developed with the help of an AI assistant (Claude, by Anthropic), including the analysis of the game executable and the code modifications. I tested everything in-game on my own setup (5120×1440), but the code has not been reviewed by an experienced C++ developer. Use at your own risk.

> This update is **not affiliated with or endorsed by** the original author.
> Original project: [https://github.com/Kaldaien/AgDrag](https://github.com/Kaldaien/AgDrag)

---

## What's new

- **32:9 support**: HUD and menus are correctly scaled and centered on 32:9 screens (works by aspect ratio, so 3840×1080 should behave the same as 5120×1440).
- **Full map fixed**:
  - No more crash when opening the map (the original hook used a 2016 memory address that no longer matches the current `DDDA.exe`). The map class is now located automatically at runtime.
  - The map fills the centered 16:9 area with the player arrow, waypoints and icons correctly aligned.
  - Mouse clicks, waypoint placement and edge scrolling work on the map.
- **No more crash with current Special K**: AgDrag no longer uses Special K's internal command processor, whose layout changed in newer versions.
- **Menu fixes**: the title screen *Options* tabs render correctly; menus that use a different internal coordinate space are now centered properly.
- **HUD fixes**: removed a stray dot that appeared in the middle of the screen during gameplay; nametags are aspect-corrected by default.
- **Build fixes**: the project now compiles with current Visual Studio (2022) and the Windows 10 SDK.

---

## Requirements

- *Dragon's Dogma: Dark Arisen* (Steam). Tested with the December 2017 executable (current Steam version).
- The **original AgDrag package** installed: [https://github.com/Kaldaien/AgDrag](https://github.com/Kaldaien/AgDrag)
- **Special K** (`d3d9.dll`), which is included in the original AgDrag package. Tested with Special K **0.8.6**.

---

## Installation

1. Install the original AgDrag package in your game folder (where `DDDA.exe` is).
2. Replace `AgDrag.dll` with the one from this release.
3. **Delete your old `AgDrag.ini`** so a new one is generated with the new options.
4. Start the game.

To confirm it's working, open `logs\AgDrag.log` in the game folder. It should contain lines like:

```
[Minimap] Map height patch applied (5 of 8 sites)
[Minimap] uGUIMap vtable[55] = 00685130, hook create status: 0
```

---

## Recommended game settings

Depth of field does not render correctly at ultrawide resolutions. In the game's `config.ini`, set:

```ini
[GRAPHICS]
DofFilter=OFF
Stereo=TRUE
```

---

## AgDrag.ini options

| Section | Option | Default | Description |
|---|---|---|---|
| `[AgDrag.Render]` | `AspectCorrection` | `true` | Enables HUD/menu aspect ratio correction. |
| | `CenterUI` | `true` | Centers the HUD in a 16:9 area. |
| | `HookMap` | `true` | Detects when the full map is open (required for the map fix). |
| | `MapMode` | `1` | `1` = map fills the centered 16:9 area, mouse aligned (recommended). `0` = native full-width 32:9 map, but mouse/waypoints misaligned. |
| | `MapHeightPatch` | `true` | Makes the full map use a 16:9 layout so it fills the whole centered area (used by `MapMode=1`). |
| | `ForegroundFPS` / `BackgroundFPS` | | **No longer functional**, use Special K's frame limiter instead. |
| `[AgDrag.Nametags]` | `AspectCorrect` | `true` | Prevents nametags/markers from being stretched. |
| | `ShiftCoeff` | `0.0` | Horizontal nametag offset; `0` = automatic. |

## Hotkeys

| Keys | Action |
|---|---|
| `Ctrl+Shift+Alt+A` | Toggle aspect ratio correction |

---

## Supported resolutions

- **32:9** (5120×1440 tested; 3840×1080 expected to work).
- **21:9** should benefit from the same fixes, but has **not been tested** with this version.
- **16:9 and narrower** (1920×1080, 2560×1440, 1920×1200, 1920×1440): AgDrag does not change anything; the game behaves as vanilla.

---

## Known limitations

- **Depth of field** is broken at ultrawide resolutions; use the `config.ini` settings above.
- **AgDrag's FPS limiter** (`ForegroundFPS`/`BackgroundFPS`) is disabled; use Special K's limiter or turn vsync on.
- Recent Special K versions remove some of AgDrag's cursor hooks (`Removing Corrupted Hook` messages in `logs\d3d9.log`). No visible issues were found in testing.
- The full map fix depends on the December 2017 `DDDA.exe`. On a different executable, AgDrag detects the mismatch, disables only the map fix and writes a message to `AgDrag.log` instead of crashing.
- Only tested at 5120×1440.

---

## Source code and license

This is a modified version of Agnostic Dragon (AgDrag) by Kaldaien, distributed under the **GNU General Public License v3** (see `LICENSE`).

- **Modified in 2026 by EveryNightLive.** Changes: 32:9 support, compatibility with the December 2017 `DDDA.exe` and recent Special K versions, full map, menu and HUD fixes, build fixes. Modified code is marked with `FIX` comments in the source.
- **Source code for this release:** https://github.com/EveryNightLive/AgDrag (this repository)

Original author credit: **Kaldaien** — https://github.com/Kaldaien/AgDrag

---

## Original README (by Kaldaien)

The text below is the original README of Agnostic Dragon (only the heading level was adjusted).

### AgDrag
Agnostic Dragon - Aspect Ratio Independence for Dragon's Dogma

Dragon’s Dogma: Dark Arisen is hard-coded to 16:9 on all platforms. It is letter boxed at resolutions narrower than 16:9 and generally works aside from that minor annoyance. At resolutions wider than 16:9 almost nothing works correctly and this project is an extensive effort to fix Capcom’s issues on the PC release.

Most of the large game breaking issues have been addressed, but it will take a prolonged effort to get the map and minimap working correctly.

<a href='https://pledgie.com/campaigns/31007'><img alt='Click here to lend your support to: Agnostic Dragon and make a donation at pledgie.com !' src='https://pledgie.com/campaigns/31007.png?skin_name=chrome' border='0' ></a>
