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
./build.sh

# 2. Apply to any game directory
./apply.sh /path/to/game/directory

# Or manually:
LD_PRELOAD=/path/to/unity-fix-32.so ./GameBinary
```

## Confirmed Working Games

- Shadowrun Returns / Dragonfall / Hong Kong
- Infinifactory
- Motorsport Manager
- Creeper World 3: Arc Eternal
- Poly Bridge
- The Novelist
- Beatbuddy: Tale of the Guardians
- Agatha Christie – The ABC Murders
- And many other Unity 4.x titles

## Steam Launch Options

Set this in Properties > Launch Options:

```
LD_PRELOAD="/path/to/unity-fix-32.so:${LD_PRELOAD}" %command%
```

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
