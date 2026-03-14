#!/usr/bin/env python3
"""
check_volatile.py — AM32 Safety Audit: Volatile Qualifier Checker

Scans the AM32 codebase and verifies that all variables shared between
the main loop and interrupt service routines are declared with the
`volatile` qualifier in both their definitions and every extern declaration.

Exit code 0 = all checks pass
Exit code 1 = one or more violations found (CI will fail)
"""

import re
import sys
import os
from pathlib import Path

# ---------------------------------------------------------------------------
# Variables that MUST be volatile because they are written by ISRs and read
# by the main loop (or vice-versa).  Format: (variable_name, reason)
# ---------------------------------------------------------------------------
REQUIRED_VOLATILE = [
    ("armed",                 "written by ISR/DShot, read by main state machine"),
    ("running",               "written by commutation ISR, read by main loop"),
    ("step",                  "written by comparator ISR, read by main loop"),
    ("commutation_interval",  "written by commutation ISR, read by PID/timing"),
    ("commutation_intervals", "array written by ISR, read during e_com_time avg"),
    ("duty_cycle",            "written by PID, read by PWM ISR"),
    ("zero_crosses",          "written by ZC ISR, read by state machine"),
    ("zcfound",               "written by ZC ISR, read by main loop"),
    ("bemfcounter",           "written by comparator ISR, read by main loop"),
    ("desync_check",          "written by commutation ISR, read by safety check"),
    ("dma_buffer",            "DMA destination, written by hardware"),
    ("ADCDataDMA",            "DMA destination, written by hardware"),
    ("ADC1DataDMA",           "DMA destination, written by hardware (g431)"),
    ("ADC2DataDMA",           "DMA destination, written by hardware (g431)"),
]

# Regex patterns
# Matches a definition:  [volatile] <type> <name>[...] [= ...];
DEFINITION_RE = re.compile(
    r'^\s*(?P<volatile>volatile\s+)?'
    r'(?:(?:unsigned|signed|const)\s+)?'
    r'(?:uint\d+_t|int\d+_t|char|int|uint16_t|uint32_t|uint8_t)\s+'
    r'(?P<name>[A-Za-z_]\w*)(?:\[.*?\])?\s*(?:=.*?)?;',
    re.MULTILINE
)

# Matches an extern declaration: extern [volatile] <type> <name>...;
EXTERN_RE = re.compile(
    r'^\s*extern\s+(?P<volatile>volatile\s+)?'
    r'(?:(?:unsigned|signed|const)\s+)?'
    r'(?:uint\d+_t|int\d+_t|char|int|uint16_t|uint32_t|uint8_t)\s+'
    r'(?P<name>[A-Za-z_]\w*)(?:\[.*?\])?\s*;',
    re.MULTILINE
)

REPO_ROOT = Path(__file__).parent.parent


def scan_files():
    """Walk repo and classify all definitions and extern declarations."""
    definitions = {}    # name -> list of (file, line, has_volatile)
    externs = {}        # name -> list of (file, line, has_volatile)

    c_files = list(REPO_ROOT.rglob("*.c")) + list(REPO_ROOT.rglob("*.h"))
    # Exclude the Unity test framework sources — they use their own volatile scheme.
    # Exclude third-party BSP Drivers/ — they define unrelated variables with the
    # same names (e.g., `step` in LCD drivers) and are not ISR-shared.
    c_files = [f for f in c_files
               if "unity" not in f.name.lower()
               and ".git" not in str(f)
               and "Drivers" not in str(f)]

    for path in c_files:
        try:
            text = path.read_text(errors="replace")
        except OSError:
            continue

        rel = path.relative_to(REPO_ROOT)

        # Strip single-line comments before scanning to avoid matching
        # commented-out code.  Remove /* ... */ and // ... patterns.
        stripped = re.sub(r'//[^\n]*', '', text)
        stripped = re.sub(r'/\*.*?\*/', '', stripped, flags=re.DOTALL)

        for m in DEFINITION_RE.finditer(stripped):
            name = m.group("name")
            has_vol = m.group("volatile") is not None
            line = stripped[:m.start()].count("\n") + 1

            # Skip struct/union members and local variables inside functions.
            # Count brace depth at the match position: depth > 0 means we are
            # inside a struct, function, or compound statement.
            text_before = stripped[:m.start()]
            brace_depth = text_before.count("{") - text_before.count("}")
            if brace_depth > 0:
                continue

            definitions.setdefault(name, []).append((str(rel), line, has_vol))

        for m in EXTERN_RE.finditer(text):
            name = m.group("name")
            has_vol = m.group("volatile") is not None
            line = text[:m.start()].count("\n") + 1
            externs.setdefault(name, []).append((str(rel), line, has_vol))

    return definitions, externs


def main():
    definitions, externs = scan_files()
    failures = []

    for var_name, reason in REQUIRED_VOLATILE:
        # Check definitions
        defs = definitions.get(var_name, [])
        for file, line, has_vol in defs:
            if not has_vol:
                failures.append(
                    f"DEFINITION missing volatile: {var_name} "
                    f"at {file}:{line}  ({reason})"
                )

        # Check extern declarations
        exts = externs.get(var_name, [])
        for file, line, has_vol in exts:
            if not has_vol:
                failures.append(
                    f"EXTERN missing volatile: {var_name} "
                    f"at {file}:{line}  ({reason})"
                )

    if failures:
        print("\n[FAIL] Volatile qualifier audit — violations found:\n")
        for f in failures:
            print(f"  ✗ {f}")
        print(f"\n{len(failures)} violation(s) found. Fix before merging.\n")
        sys.exit(1)
    else:
        checked = sum(
            len(definitions.get(v, [])) + len(externs.get(v, []))
            for v, _ in REQUIRED_VOLATILE
        )
        print(f"[PASS] Volatile qualifier audit — {checked} declarations checked, 0 violations.")
        sys.exit(0)


if __name__ == "__main__":
    main()
