# Unity single-instance fix

Fixes old Unity games, pre-5.4.0, that fail with "Player is already running" on modern Linux systems.

## The problem

Unity 4.x detects duplicate instances by scanning `/proc` and comparing executable paths. Chromium, CEF and Electron processes, for example Signal Desktop, VS Code, Cursor, Slack, Discord and Steam since mid-2023, list `/proc/self/exe` in their cmdline. When Unity reads it, it resolves the path from its own perspective, finds itself, and refuses to start.

## Usage

```bash
# 1. Build the fix libraries
make

# 2. Run the game with the library preloaded
# Choose the library matching your game's architecture
LD_PRELOAD=/path/to/unity-fix-64.so ./GameBinary
```

## Confirmed working games

- Shadowrun Returns / Dragonfall / Hong Kong
- Wasteland 2: Director's Cut
- Infinifactory
- Motorsport Manager
- Creeper World 3: Arc Eternal
- Poly Bridge
- The Novelist
- Beatbuddy: Tale of the Guardians
- Agatha Christie: The ABC Murders
- Many other Unity 4.x titles

## Setting LD_PRELOAD

### Steam

Set this in Properties > Launch Options:

```
LD_PRELOAD="/path/to/unity-fix-32.so:${LD_PRELOAD}" %command%
```

### GOG, the game-in-browser wrapper

The `.so` lives at `<game-dir>/start.sh`, which sources the GOG runtime. Edit it and export `LD_PRELOAD` inside the `run_game()` function, before the binary runs:

```bash
run_game() {
    export LD_PRELOAD="${CURRENT_DIR}/game/unity-fix-64.so:${LD_PRELOAD}"
    # ... rest of the function
}
```

GOG installs into `~/Games/gog/<gametitle>`, with the game binary in the `game/` subdirectory.

## Technical details

The library intercepts:

| Function | Why |
| --- | --- |
| `fopen()` / `fopen64()` | Opening `/proc/*` files |
| `fread()` | Replaces `/proc/self/exe` with a fake path |
| `open()` / `open64()` | Direct file I/O to `/proc` |
| `read()` | Replaces `/proc/self/exe` in read buffers |
| `readlink()` | Stops the symlink resolution check |
| `close()` | Avoids stale fd tracking |

Based on analysis by janinko and Eike Sauer:
https://ein-eike.de/2025/06/05/fix-for-old-unity-games-for-linux-not-starting-anymore/
