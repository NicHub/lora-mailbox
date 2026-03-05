"""
get_build_flags.py
"""

import datetime
import os
import sys
import subprocess

NO_GIT_COMMIT_YET = "no_git_commit_yet"


def get_last_commit_id():
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


def get_uncommitted_files_count():
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
    utc_now = datetime.datetime.now()
    compilation_date = utc_now.strftime("%Y-%m-%d")
    compilation_time = utc_now.strftime("%H:%M:%S")
    project_path = os.getcwd().replace("\\", "\\\\")
    python_version = (
        f"{sys.version_info.major}"
        f".{sys.version_info.minor}"
        f".{sys.version_info.micro}"
    )
    python_path = os.path.realpath(sys.executable).replace("\\", "\\\\")
    last_commit_id = get_last_commit_id() or NO_GIT_COMMIT_YET
    uncommitted_files_count = (
        0 if last_commit_id == NO_GIT_COMMIT_YET else get_uncommitted_files_count()
    )
    flag_list = (
        ("COMPILATION_DATE", compilation_date),
        ("COMPILATION_TIME", compilation_time),
        ("PROJECT_PATH", project_path),
        ("PYTHON_VERSION", python_version),
        ("PYTHON_PATH", python_path),
        (
            "LAST_COMMIT_ID",
            f"{last_commit_id[:7]} ({uncommitted_files_count} uncommitted files)",
        ),
    )

    return dict(flag_list)


def main():
    flag_dict = get_flag_dict()
    flags = ""
    for name, value in flag_dict.items():
        flags += f"-D {name}='\"{value}\"'\n"

    return flags


if __name__ == "__main__":

    flags = main()
    print(flags)
    with open("flags_nogit.txt", "w") as f:
        f.write(flags)
