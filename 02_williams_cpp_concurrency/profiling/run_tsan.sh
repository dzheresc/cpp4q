#!/usr/bin/env bash
# Builds book 2 with ThreadSanitizer and runs every unit test and demo
# under it. Valgrind's helgrind/drd see only pthread calls; TSan also
# understands C++ atomics, so it is the authority for the lock-free code.
#
# Usage: profiling/run_tsan.sh [build-dir]   (default: build-tsan)
# oneTBB is not instrumented, so the chapter 10 binaries run with
# TBB's own threads invisible to TSan; they are reported but not counted.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${1:-${ROOT}/build-tsan}"
BIN="${BUILD}/02_williams_cpp_concurrency"
OUT="${ROOT}/02_williams_cpp_concurrency/profiling/results/tsan"
mkdir -p "${OUT}"

cmake -S "${ROOT}" -B "${BUILD}" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCPP4Q_BUILD_BENCHMARKS=OFF -DCPP4Q_SANITIZE=thread >/dev/null
cmake --build "${BUILD}" >/dev/null

status=0
for exe in "${BIN}"/tests/williams_*_test "${BIN}"/williams_*_demo; do
  name="$(basename "${exe}")"
  log="${OUT}/${name}.log"
  TSAN_OPTIONS="halt_on_error=0 exitcode=66 second_deadlock_stack=1" \
    timeout 300 "${exe}" >"${log}" 2>&1
  rc=$?
  warnings=$(grep -c "WARNING: ThreadSanitizer" "${log}")
  if [[ ${rc} -eq 0 && ${warnings} -eq 0 ]]; then
    echo "  ok    ${name}"
  elif [[ "${name}" == *ch10* ]]; then
    echo "  info  ${name} (rc=${rc}, ${warnings} warnings; TBB uninstrumented)"
  else
    echo "  FAIL  ${name} (rc=${rc}, ${warnings} warnings, see ${log})"
    status=1
  fi
done
exit "${status}"
