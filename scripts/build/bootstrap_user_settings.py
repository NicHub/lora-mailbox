"""
SCons bootstrap for local user settings.

This script runs before compilation and copies example user settings
files into the local settings directory when they are missing.
"""

from pathlib import Path
import shutil

Import("env")

project_dir = Path(env["PROJECT_DIR"])
copy_operations = [
    (
        project_dir / "src" / "user_settings-examples",
        project_dir / "src" / "user_settings",
    ),
    (
        project_dir / "user_settings-example.ini",
        project_dir / "user_settings.ini",
    ),
]

copied_files = []
for source, destination in copy_operations:
    if not source.exists():
        raise SystemExit(
            "[ouilogique.com]\n"
            f"Missing path: {source.relative_to(project_dir)}"
        )

    if destination.exists():
        continue

    destination.parent.mkdir(parents=True, exist_ok=True)

    if source.is_dir():
        shutil.copytree(source, destination)
    else:
        shutil.copy2(source, destination)

    copied_files.append(destination.relative_to(project_dir))

if copied_files:
    print("[ouilogique.com] Created local user settings paths:")
    for path in copied_files:
        print(f"  - {path}")
    print("[ouilogique.com] Update these paths with your local configuration.")
