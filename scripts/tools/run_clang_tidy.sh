#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-}"
PIO_BIN="${PIO_BIN:-pio}"
TARGET="${1:-all}"

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
        "${CLANG_TIDY_BIN}" -p . "${paths[@]}"
    )
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
