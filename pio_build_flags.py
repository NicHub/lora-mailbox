"""
PlatformIO pre-build script to inject dynamic build flags.
"""

from pathlib import Path
import sys

Import("env")

project_dir = Path(env["PROJECT_DIR"])
sys.path.insert(0, str(project_dir))

import get_build_flags  # noqa: E402

env.ProcessFlags(get_build_flags.main())
