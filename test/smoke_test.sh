#!/usr/bin/env bash
# Smoke tests for MP3Gain Revival
# Run from the repo root: bash test/smoke_test.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ -f "$REPO_ROOT/build/Release/mp3gain.exe" ]; then
    EXE="$REPO_ROOT/build/Release/mp3gain.exe"
elif [ -f "$REPO_ROOT/build/mp3gain" ]; then
    EXE="$REPO_ROOT/build/mp3gain"
else
    EXE="$REPO_ROOT/build/mp3gain"
fi
FIXTURE_DIR="$REPO_ROOT/test/fixtures"
TMP_DIR="$(mktemp -d)"
T1="$TMP_DIR/test1.mp3"
T2="$TMP_DIR/test2.mp3"

PASS=0
FAIL=0

pass() { echo "  PASS: $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

check() {
    local desc="$1"
    local pattern="$2"
    local output="$3"
    if echo "$output" | grep -qF "$pattern"; then
        pass "$desc"
    else
        fail "$desc (expected: '$pattern')"
        echo "    --- output ---"
        echo "$output" | sed 's/^/    /'
    fi
}

check_absent() {
    local desc="$1"
    local pattern="$2"
    local output="$3"
    if echo "$output" | grep -qF "$pattern"; then
        fail "$desc (unexpected pattern found: '$pattern')"
    else
        pass "$desc"
    fi
}

cleanup() {
    rm -rf "$TMP_DIR"
}

restore() {
    cp "$FIXTURE_DIR/test1.mp3" "$T1"
    cp "$FIXTURE_DIR/test2.mp3" "$T2"
}

require_fixtures() {
    if [ ! -f "$FIXTURE_DIR/test1.mp3" ] || [ ! -f "$FIXTURE_DIR/test2.mp3" ]; then
        echo "ERROR: fixture files not found in $FIXTURE_DIR"
        exit 1
    fi
}

require_exe() {
    if [ ! -f "$EXE" ]; then
        echo "ERROR: executable not found: $EXE"
        echo "Build first: cmake --build build --config Release"
        exit 1
    fi
}

# -----------------------------------------------------------------------
echo "MP3Gain Revival Smoke Tests"
echo "==========================="
echo "EXE: $EXE"
echo ""

trap cleanup EXIT

require_exe
require_fixtures
restore

# --- Test 1: version ---
echo "[1] Version flag"
out=$("$EXE" -v 2>&1 || true)
check "version string present" "1.6.2" "$out"

# --- Test 2: single-file track analysis ---
echo ""
echo "[2] Single-file track analysis"
restore
out=$("$EXE" "$T1" 2>&1)
check "track dB value"         "Track\" dB change: -8.760000"  "$out"
check "track gain step"        "Track\" mp3 gain change: -6"   "$out"
check "max global gain field"  "Max mp3 global gain field: 210" "$out"
check "min global gain field"  "Min mp3 global gain field: 130" "$out"

# --- Test 3: two-file album analysis ---
echo ""
echo "[3] Two-file album analysis"
restore
out=$("$EXE" "$T1" "$T2" 2>&1)
check "album dB line present"  "Album\" dB change for all files: -8.760000" "$out"
check "album gain step"        "Album\" mp3 gain change for all files: -6"  "$out"

# --- Test 4: apply track gain (-r) ---
echo ""
echo "[4] Apply track gain (-r)"
restore
"$EXE" -r "$T1" > /dev/null 2>&1
out=$("$EXE" "$T1" 2>&1)
check "max global gain reduced to 204" "Max mp3 global gain field: 204" "$out"
check "recommendation is now 0 steps"  "Track\" mp3 gain change: 0"    "$out"

# --- Test 5: undo (-u) ---
echo ""
echo "[5] Undo gain (-u)"
restore
"$EXE" -r "$T1" > /dev/null 2>&1
"$EXE" -u "$T1" > /dev/null 2>&1
out=$("$EXE" "$T1" 2>&1)
check "max global gain restored to 210" "Max mp3 global gain field: 210" "$out"
check "recommendation restored to -6"   "Track\" mp3 gain change: -6"   "$out"

# --- Test 6: apply album gain (-a) ---
echo ""
echo "[6] Apply album gain (-a)"
restore
"$EXE" -a "$T1" "$T2" > /dev/null 2>&1
out1=$("$EXE" "$T1" 2>&1)
out2=$("$EXE" "$T2" 2>&1)
check "file1 max global gain reduced to 204" "Max mp3 global gain field: 204" "$out1"
check "file2 max global gain reduced to 204" "Max mp3 global gain field: 204" "$out2"

# --- Test 7: exit code ---
echo ""
echo "[7] Exit codes"
restore
"$EXE" "$T1" > /dev/null 2>&1
code=$?
if [ "$code" -eq 0 ]; then pass "analysis exits 0"; else fail "analysis exits 0 (got $code)"; fi

# --- Test 8: APE tag write/read/delete ---
echo ""
echo "[8] APE tag round-trip"
restore
"$EXE" -r "$T1" > /dev/null 2>&1
out=$("$EXE" -s c "$T1" 2>&1)
check "APE tag stores zero-step track gain" "Track\" mp3 gain change: 0" "$out"
check "APE tag stores updated max gain" "Max mp3 global gain field: 204" "$out"
"$EXE" -s d "$T1" > /dev/null 2>&1
out=$("$EXE" -s c "$T1" 2>&1)
check_absent "deleted APE tag removes stored track gain" "Track\" mp3 gain change:" "$out"
check_absent "deleted APE tag removes stored min/max gain" "Max mp3 global gain field:" "$out"

# --- Test 9: ID3/APE tag mode separation ---
echo ""
echo "[9] ID3 tag mode separation"
restore
"$EXE" -s i -r "$T1" > /dev/null 2>&1
out_id3=$("$EXE" -s i -s c "$T1" 2>&1)
out_ape=$("$EXE" -s a -s c "$T1" 2>&1)
check "ID3 mode reads stored track gain" "Track\" mp3 gain change: 0" "$out_id3"
check "ID3 mode reads stored max gain" "Max mp3 global gain field: 204" "$out_id3"
check_absent "APE mode stays empty after ID3-only write" "Track\" mp3 gain change:" "$out_ape"

# --- Test 10: auto-clip path ---
echo ""
echo "[10] Auto-clip apply (-k)"
restore
out=$("$EXE" -k -d 20 -r "$T1" 2>&1)
check "auto-clip reports clipped gain" "Applying auto-clipped mp3 gain change of -4" "$out"
check "auto-clip reports original suggestion" "Original suggested gain was 7" "$out"
out=$("$EXE" "$T1" 2>&1)
check "auto-clip leaves adjusted max gain" "Max mp3 global gain field: 206" "$out"

# --- cleanup ---
restore

# --- summary ---
echo ""
echo "==========================="
TOTAL=$((PASS + FAIL))
echo "Results: $PASS/$TOTAL passed"
if [ "$FAIL" -gt 0 ]; then
    echo "FAILED: $FAIL test(s)"
    exit 1
else
    echo "All tests passed."
    exit 0
fi
