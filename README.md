# Unity Single-Instance Fix

Fixes old Unity games (pre-5.4.0) that fail with **"Player is already running"**
on modern Linux systems.

## The Problem

Unity 4.x detects duplicate instances by scanning `/proc` and comparing
executable paths. Chromium/CEF/Electron processes (Signal Desktop, VS Code/Cursor,
Slack, Discord, Steam since mid-2023) list `/proc/self/exe` in their cmdline.
When Unity reads this, it resolves it from its *own* perspective, finds itself,
and refuses to start.

## Usage

```sh
# 1. Build the fix libraries
make

# 2. Run the game with the library preloaded
# Choose the library matching your game's architecture
LD_PRELOAD=/path/to/unity-fix-64.so ./GameBinary
```

## Confirmed Working Games

- Shadowrun Returns / Dragonfall / Hong Kong
- Wasteland 2: Director's Cut
- Infinifactory
- Motorsport Manager
- Creeper World 3: Arc Eternal
- Poly Bridge
- The Novelist
- Beatbuddy: Tale of the Guardians
- Agatha Christie – The ABC Murders
- And many other Unity 4.x titles

## Setting the LD_PRELOAD

### Steam

Set this in Properties > Launch Options:

```
LD_PRELOAD="/path/to/unity-fix-32.so:${LD_PRELOAD}" %command%
```

### GOG (game-in-Browser wrapper)

The `.so` lives at `<game-dir>/start.sh`, which sources the GOG runtime. Edit
it and export `LD_PRELOAD` inside the `run_game()` function before the binary
is executed:

```sh
run_game() {
    export LD_PRELOAD="${CURRENT_DIR}/game/unity-fix-64.so:${LD_PRELOAD}"
    # ... rest of the function
}
```

Note that GOG installs into `~/Games/gog/<gametitle>`, with the actual game
binary inside the `game/` subdirectory.

## Technical Details

The LD_PRELOAD library intercepts:
- `fopen()` / `fopen64()` — opening `/proc/*` files
- `fread()` — replacing `/proc/self/exe` with a fake path
- `open()` / `open64()` — direct file I/O to `/proc`
- `read()` — replacing `/proc/self/exe` in read buffers
- `readlink()` — preventing symlink resolution trick
- `close()` — avoiding stale fd tracking

Based on analysis by janinko and Eike Sauer:
https://ein-eike.de/2025/06/05/fix-for-old-unity-games-for-linux-not-starting-anymore/
