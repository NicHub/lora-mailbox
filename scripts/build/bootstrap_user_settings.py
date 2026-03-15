"""
SCons bootstrap for local user settings.

This script runs before compilation and copies example user settings
files into the local settings directory when they are missing.
"""

from pathlib import Path
import shutil

Import("env")

project_dir = Path(env["PROJECT_DIR"])
examples_dir = project_dir / "src" / "user_settings-examples"
settings_dir = project_dir / "src" / "user_settings"

if not examples_dir.is_dir():
    raise SystemExit(
        "[ouilogique.com]\n"
        f"Missing directory: {examples_dir.relative_to(project_dir)}"
    )

settings_dir.mkdir(parents=True, exist_ok=True)

copied_files = []
for source in sorted(examples_dir.iterdir()):
    if not source.is_file():
        continue

    destination = settings_dir / source.name
    if destination.exists():
        continue

    shutil.copy2(source, destination)
    copied_files.append(destination.relative_to(project_dir))

if copied_files:
    print("[ouilogique.com] Created local user settings files:")
    for path in copied_files:
        print(f"  - {path}")
    print("[ouilogique.com] Update these files with your local configuration.")
