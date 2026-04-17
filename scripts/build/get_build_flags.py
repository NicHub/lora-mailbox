"""Generate PlatformIO build flags containing build and Git metadata.

This script can be used in two ways:

1. During a PlatformIO build, via ``Import("env")``, to inject generated
   ``-D`` flags into the build environment with ``env.ProcessFlags(...)``.
2. When run directly with ``python scripts/build/get_build_flags.py``, to print
   the generated flags and write them to ``flags_nogit.txt``. This is useful
   for debugging or verifying the generated values outside the PlatformIO build
   process.

## Generated flags

- BUILD_LOCAL_TIME='"2026-04-13T12:39:21.411690+02:00"'
- BUILD_SOURCE_PATH='"$HOME/./././lora-mailbox/scripts/build"'
- BUILD_PYTHON_VERSION='"3.14.4"'
- BUILD_PYTHON_PATH='"/opt/homebrew/Cellar/python@3.14/3.14.4/Frameworks/Python.framework/Versions/3.14/bin/python3.14"'
- GIT_HEAD_COMMIT_ID='"ee1a16c"'
- GIT_UNCOMMITTED_FILES_COUNT='"5"'
- BUILD_ID=1745923200u  (numeric, unique per build invocation)

If no Git commit exists yet, the value ``no_git_commit_yet`` is used and the
uncommitted file count is forced to 0.

Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
"""

import datetime
import os
from pathlib import Path
import subprocess
import sys

try:
    Import("env")
except Exception:
    env = None

NO_GIT_COMMIT_YET = "no_git_commit_yet"


def get_build_source_path():
    """Return the PlatformIO project root path."""
    if env is not None and "PROJECT_DIR" in env:
        return str(Path(env["PROJECT_DIR"]).resolve())

    return str(Path(__file__).resolve().parents[2])


def get_last_commit_id():
    """Return the full SHA of the current Git HEAD commit."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if result.returncode == 0:
            return result.stdout.strip()
        else:
            return NO_GIT_COMMIT_YET
    except Exception as e:
        print(f"Exception occurred: {e}")
        return NO_GIT_COMMIT_YET


def get_git_uncommitted_files_count():
    """Count uncommitted files reported by Git."""
    try:
        result = subprocess.run(
            ["git", "status", "--porcelain"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if result.returncode == 0:
            modified_files = result.stdout.strip()
            if modified_files:
                return len(modified_files.split("\n"))
            else:
                return 0
        else:
            print(f"Error counting uncommitted files: {result.stderr}")
            return None
    except Exception as e:
        print(f"Exception occurred: {e}")
        return None


def get_flag_dict():
    """Build the dictionary of preprocessor flag names and values.

    Each value is a ``(value, is_numeric)`` pair. Numeric values are emitted
    without surrounding quotes so they can be used as integer literals in C.
    """
    build_local_time = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    build_source_path = get_build_source_path().replace("\\", "\\\\")
    build_python_version = (
        f"{sys.version_info.major}"
        f".{sys.version_info.minor}"
        f".{sys.version_info.micro}"
    )
    build_python_path = os.path.realpath(sys.executable).replace("\\", "\\\\")
    git_head_commit_id = get_last_commit_id() or NO_GIT_COMMIT_YET
    git_uncommitted_files_count = (
        0 if git_head_commit_id == NO_GIT_COMMIT_YET else get_git_uncommitted_files_count()
    )
    # Unique per build invocation, used to detect a fresh flash.
    build_id = int(datetime.datetime.now().timestamp())
    flag_list = (
        ("BUILD_LOCAL_TIME", build_local_time, False),
        ("BUILD_SOURCE_PATH", build_source_path, False),
        ("BUILD_PYTHON_VERSION", build_python_version, False),
        ("BUILD_PYTHON_PATH", build_python_path, False),
        ("GIT_HEAD_COMMIT_ID", git_head_commit_id[:7], False),
        ("GIT_UNCOMMITTED_FILES_COUNT", str(git_uncommitted_files_count), False),
        ("BUILD_ID", f"{build_id}u", True),
    )

    return {name: (value, is_numeric) for name, value, is_numeric in flag_list}


def main():
    """Format generated metadata as PlatformIO-compatible ``-D`` flags."""
    flag_dict = get_flag_dict()
    flags = ""
    for name, (value, is_numeric) in flag_dict.items():
        if is_numeric:
            flags += f"-D {name}={value}\n"
        else:
            flags += f"-D {name}='\"{value}\"'\n"

    return flags


def inject_build_flags():
    """Inject generated flags into the active PlatformIO environment."""
    if env is None:
        raise RuntimeError("PlatformIO build environment is not available.")

    env.ProcessFlags(main())


if env is not None:
    inject_build_flags()


if __name__ == "__main__" and env is None:

    flags = main()
    print(flags)
    with open("flags_nogit.txt", "w") as f:
        f.write(flags)
