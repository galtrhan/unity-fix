#!/bin/sh
# Build the Unity fix LD_PRELOAD library for both 32-bit and 64-bit.
# Usage: ./build.sh

set -e

SRC="unity-fix.c"
OUTDIR="."

echo "== Building unity-fix libraries =="

# Build 64-bit version
echo "  -> 64-bit: ${OUTDIR}/unity-fix-64.so"
gcc -shared -fPIC -ldl -o "${OUTDIR}/unity-fix-64.so" "$SRC" -Wall -Wextra

# Build 32-bit version (may fail if gcc-multilib not installed)
if gcc -m32 -shared -fPIC -ldl -o "${OUTDIR}/unity-fix-32.so" "$SRC" -Wall -Wextra 2>/dev/null; then
    echo "  -> 32-bit: ${OUTDIR}/unity-fix-32.so"
else
    echo "  !! 32-bit build skipped (install gcc-multilib for 32-bit support)"
fi

echo "== Done =="
ls -la "${OUTDIR}/unity-fix-"*.so 2>/dev/null
