#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-}"
PIO_BIN="${PIO_BIN:-pio}"
TARGET="${1:-all}"
SANITIZED_DB_DIR=".pio/clang-tidy"

if [[ -z "${CLANG_TIDY_BIN}" ]]; then
    if command -v clang-tidy >/dev/null 2>&1; then
        CLANG_TIDY_BIN="$(command -v clang-tidy)"
    elif [[ -x "/opt/homebrew/opt/llvm/bin/clang-tidy" ]]; then
        CLANG_TIDY_BIN="/opt/homebrew/opt/llvm/bin/clang-tidy"
    else
        echo "Error: clang-tidy not found. Set CLANG_TIDY_BIN or install llvm with Homebrew." >&2
        exit 1
    fi
fi

if ! command -v "${PIO_BIN}" >/dev/null 2>&1; then
    echo "Error: PlatformIO not found. Set PIO_BIN if needed." >&2
    exit 1
fi

find_clang_bin() {
    local clang_dir

    clang_dir="$(cd "$(dirname "${CLANG_TIDY_BIN}")" && pwd)"
    if [[ -x "${clang_dir}/clang" ]]; then
        printf '%s\n' "${clang_dir}/clang"
        return 0
    fi

    if command -v clang >/dev/null 2>&1; then
        command -v clang
        return 0
    fi

    return 1
}

clang_supports_target() {
    local target="$1"
    local clang_bin

    clang_bin="$(find_clang_bin)" || return 1
    "${clang_bin}" --print-targets 2>/dev/null | rg -q "^[[:space:]]*${target}[[:space:]]+-"
}

ensure_env_supported() {
    local env_name="$1"

    case "${env_name}" in
        seeed_xiao_esp32s3-rx)
            if ! clang_supports_target "xtensa"; then
                cat >&2 <<'EOF'
Error: the selected clang/clang-tidy toolchain does not support the Xtensa target used by seeed_xiao_esp32s3-rx.

The script now sanitizes the PlatformIO compile database, but ESP32 analysis still requires a clang build with Xtensa backend support.
Install an Xtensa-capable LLVM/clang-tidy toolchain, or run the TX target only:
  scripts/tools/run_clang_tidy.sh tx
EOF
                return 1
            fi
            ;;
    esac
}

run_target() {
    local env_name="$1"
    shift
    local paths=("$@")

    echo "==> Generating compile_commands.json for ${env_name}"
    (
        cd "${PROJECT_ROOT}"
        "${PIO_BIN}" run -e "${env_name}" -t compiledb
    )

    echo "==> Running clang-tidy for ${env_name}"
    (
        cd "${PROJECT_ROOT}"
        local sanitized_db="${SANITIZED_DB_DIR}/${env_name}"
        mkdir -p "${sanitized_db}"
        sanitize_compile_commands "${env_name}" "${PROJECT_ROOT}/compile_commands.json" "${sanitized_db}/compile_commands.json"
        ensure_env_supported "${env_name}"
        "${CLANG_TIDY_BIN}" -p "${sanitized_db}" "${paths[@]}"
    )
}

sanitize_compile_commands() {
    local _env_name="$1"
    local input_db="$2"
    local output_db="$3"

    python3 - "${input_db}" "${output_db}" <<'PY'
import json
import shlex
import shutil
import sys
from pathlib import Path

input_db = Path(sys.argv[1])
output_db = Path(sys.argv[2])

unsupported_exact = {
    "-fstrict-volatile-bitfields",
    "-fno-tree-switch-conversion",
    "-mlongcalls",
    "-mlong-calls",
}
unsupported_prefixes = (
    "-mfix-esp32-psram-cache-issue",
    "-fmacro-prefix-map=",
    "-fdebug-prefix-map=",
    "-ffile-prefix-map=",
)


def compiler_target_args(compiler: str) -> list[str]:
    compiler_name = Path(compiler).name
    if any(name in compiler_name for name in ("xtensa-esp32s3-elf-g++", "xtensa-esp32s3-elf-gcc", "xtensa-esp32s3-elf-c++", "xtensa-esp32s3-elf-cc")):
        return ["--target=xtensa"]
    if any(name in compiler_name for name in ("arm-none-eabi-g++", "arm-none-eabi-gcc", "arm-none-eabi-c++", "arm-none-eabi-cc")):
        return ["--target=arm-none-eabi"]
    return []


def resolve_compiler_path(compiler: str) -> Path:
    compiler_path = Path(compiler)
    if compiler_path.is_absolute():
        return compiler_path.resolve()

    resolved = shutil.which(compiler)
    if resolved:
        return Path(resolved).resolve()

    platformio_packages = Path.home() / ".platformio" / "packages"
    if platformio_packages.exists():
        matches = sorted(platformio_packages.glob(f"*/bin/{compiler}"))
        if matches:
            return matches[0].resolve()

    return compiler_path.resolve()


def compiler_include_args(compiler: str) -> list[str]:
    compiler_name = Path(compiler).name
    compiler_path = resolve_compiler_path(compiler)
    args: list[str] = []

    if any(name in compiler_name for name in ("xtensa-esp32s3-elf-g++", "xtensa-esp32s3-elf-gcc", "xtensa-esp32s3-elf-c++", "xtensa-esp32s3-elf-cc")):
        toolchain_root = compiler_path.parent.parent
        cxx_root = toolchain_root / "xtensa-esp32s3-elf" / "include" / "c++" / "8.4.0"
        args.extend(
            [
                "-isystem",
                str(cxx_root),
                "-isystem",
                str(cxx_root / "xtensa-esp32s3-elf"),
                "-isystem",
                str(cxx_root / "backward"),
                "-isystem",
                str(toolchain_root / "xtensa-esp32s3-elf" / "include"),
                "-isystem",
                str(toolchain_root / "xtensa-esp32s3-elf" / "sys-include"),
                "-isystem",
                str(toolchain_root / "lib" / "gcc" / "xtensa-esp32s3-elf" / "8.4.0" / "include"),
                "-isystem",
                str(toolchain_root / "lib" / "gcc" / "xtensa-esp32s3-elf" / "8.4.0" / "include-fixed"),
            ]
        )
    elif any(name in compiler_name for name in ("arm-none-eabi-g++", "arm-none-eabi-gcc", "arm-none-eabi-c++", "arm-none-eabi-cc")):
        toolchain_root = compiler_path.parent.parent
        cxx_root = toolchain_root / "arm-none-eabi" / "include" / "c++"
        versions = sorted(cxx_root.iterdir(), reverse=True) if cxx_root.exists() else []
        cxx_version_root = versions[0] if versions else None
        gcc_root = toolchain_root / "lib" / "gcc" / "arm-none-eabi"
        gcc_versions = sorted(gcc_root.iterdir(), reverse=True) if gcc_root.exists() else []
        gcc_version_root = gcc_versions[0] if gcc_versions else None

        if cxx_version_root is not None:
            args.extend(
                [
                    "-isystem",
                    str(cxx_version_root),
                    "-isystem",
                    str(cxx_version_root / "arm-none-eabi"),
                    "-isystem",
                    str(cxx_version_root / "backward"),
                ]
            )
        if gcc_version_root is not None:
            args.extend(
                [
                    "-isystem",
                    str(gcc_version_root / "include"),
                    "-isystem",
                    str(gcc_version_root / "include-fixed"),
                ]
            )
        args.extend(
            [
                "-isystem",
                str(toolchain_root / "arm-none-eabi" / "include"),
                "-isystem",
                str(toolchain_root / "arm-none-eabi" / "include" / "sys"),
            ]
        )

    return [arg for arg in args if Path(arg).exists() or arg == "-isystem"]


with input_db.open() as fh:
    entries = json.load(fh)

sanitized_entries = []
for entry in entries:
    tokens = shlex.split(entry["command"])
    compiler = tokens[0]
    source_file = entry.get("file", "")
    language = "-xc++" if source_file.endswith((".cpp", ".cc", ".cxx", ".h", ".hpp")) else "-xc"
    sanitized = ["clang++", language, *compiler_target_args(compiler)]

    skip_next = False
    for token in tokens[1:]:
        if skip_next:
            skip_next = False
            continue
        if token.startswith("-D "):
            token = "-D" + token[3:]
        elif token.startswith("-U "):
            token = "-U" + token[3:]
        if token in unsupported_exact:
            continue
        if any(token.startswith(prefix) for prefix in unsupported_prefixes):
            continue
        if token in {"-c", "-o", "-MMD", "-MF", "-MT", "-MQ"}:
            if token != "-c":
                skip_next = token in {"-o", "-MF", "-MT", "-MQ"}
            continue
        if token.startswith("-o"):
            continue
        sanitized.append(token)

    sanitized.extend(compiler_include_args(compiler))
    entry["command"] = shlex.join(sanitized)
    sanitized_entries.append(entry)

with output_db.open("w") as fh:
    json.dump(sanitized_entries, fh, indent=2)
    fh.write("\n")
PY
}

collect_files() {
    (
        cd "${PROJECT_ROOT}"
        rg --files "$@" -g '*.h' -g '*.cpp' | sort
    )
}

case "${TARGET}" in
    rx)
        files=()
        while IFS= read -r file; do
            files+=("${file}")
        done < <(collect_files src/common src/RX_ESP32)
        run_target "seeed_xiao_esp32s3-rx" "${files[@]}"
        ;;
    tx)
        files=()
        while IFS= read -r file; do
            files+=("${file}")
        done < <(collect_files src/common src/TX_nRF52)
        run_target "seeed_xiao_nrf52840-tx" "${files[@]}"
        ;;
    all)
        rx_files=()
        while IFS= read -r file; do
            rx_files+=("${file}")
        done < <(collect_files src/common src/RX_ESP32)
        run_target "seeed_xiao_esp32s3-rx" "${rx_files[@]}"

        tx_files=()
        while IFS= read -r file; do
            tx_files+=("${file}")
        done < <(collect_files src/common src/TX_nRF52)
        run_target "seeed_xiao_nrf52840-tx" "${tx_files[@]}"
        ;;
    *)
        echo "Usage: $0 [rx|tx|all]" >&2
        exit 1
        ;;
esac
