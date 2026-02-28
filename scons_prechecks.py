"""
SCons pre-checks for the PlatformIO build.

This script runs before compilation and stops the build when required
project files are missing.
"""

from pathlib import Path

Import("env")

project_dir = Path(env["PROJECT_DIR"])
raw = env.GetProjectConfig().get("scons_prechecks", "required_files")
required_files = [line.strip() for line in raw.splitlines() if line.strip()]

missing_files = [p for p in required_files if not (project_dir / p).is_file()]
if missing_files:
    msg = (
        "[ouilogique.com]"
        + "\nMissing files:\n  - "
        + "\n  - ".join(missing_files)
        + "\nSee README.md for setup instructions."
    )
    raise SystemExit(msg)
