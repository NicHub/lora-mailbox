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
copied_missing_files = []


def copy_missing_files(source_dir: Path, destination_dir: Path):
    for source_path in source_dir.rglob("*"):
        if not source_path.is_file():
            continue
        relative_path = source_path.relative_to(source_dir)
        destination_path = destination_dir / relative_path
        if destination_path.exists():
            continue
        destination_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, destination_path)
        copied_missing_files.append(destination_path.relative_to(project_dir))

for source, destination in copy_operations:
    if not source.exists():
        raise SystemExit(
            "[ouilogique.com]\n"
            f"Missing path: {source.relative_to(project_dir)}"
        )

    if destination.exists():
        if source.is_dir() and destination.is_dir():
            copy_missing_files(source, destination)
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

if copied_missing_files:
    print("[ouilogique.com] Added missing local user settings files:")
    for path in copied_missing_files:
        print(f"  - {path}")
