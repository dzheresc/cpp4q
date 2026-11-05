#!/usr/bin/env bash
# Builds book 5 twice, with AddressSanitizer + UndefinedBehaviorSanitizer
# and with ThreadSanitizer, and runs every unit test and demo under each.
#
# Usage: profiling/run_sanitizers.sh

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${ROOT}/05_joshi_design_patterns_pricing/profiling/results/sanitizers"
mkdir -p "${OUT}"
status=0
# Tests and demos run from a scratch directory.
SCRATCH="$(mktemp -d)"
trap 'rm -rf "${SCRATCH}"' EXIT

for san in "address,undefined" "thread"; do
  tag="${san//,/_}"
  build="${ROOT}/build-${tag}"
  cmake -S "${ROOT}" -B "${build}" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCPP4Q_BUILD_BENCHMARKS=OFF -DCPP4Q_SANITIZE="${san}" >/dev/null
  cmake --build "${build}" --target all >/dev/null || { status=1; continue; }
  echo "== ${san}"
  bin="${build}/05_joshi_design_patterns_pricing"
  for exe in "${bin}"/tests/joshi_*_test "${bin}"/joshi_*_demo; do
    name="$(basename "${exe}")"
    log="${OUT}/${tag}.${name}.log"
    ASAN_OPTIONS="detect_leaks=1" \
    UBSAN_OPTIONS="print_stacktrace=1 halt_on_error=1" \
    TSAN_OPTIONS="halt_on_error=0 exitcode=66" \
      bash -c 'cd "$1" && shift && exec "$@"' _ "${SCRATCH}" \
      timeout 900 "${exe}" >"${log}" 2>&1
    rc=$?
    reports=$(grep -cE "ERROR: (Address|Leak)Sanitizer|runtime error:|WARNING: ThreadSanitizer" "${log}")
    if [[ ${rc} -eq 0 && ${reports} -eq 0 ]]; then
      echo "  ok    ${name}"
    else
      echo "  FAIL  ${name} (exit ${rc}, ${reports} reports; see ${log})"
      status=1
    fi
  done
done

exit "${status}"
