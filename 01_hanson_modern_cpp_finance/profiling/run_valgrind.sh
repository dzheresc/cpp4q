#!/usr/bin/env bash
# Runs the valgrind suite over book 1.
#
#   memcheck  - every unit-test binary and demo (leaks, invalid access,
#               uninitialised reads); fails on any error.
#   helgrind,
#   drd       - the multi-threaded Monte Carlo workload (data races).
#   callgrind - instruction-level hotspots per workload.
#   cachegrind- D1/LL cache simulation for the matrix and tree workloads.
#   massif    - heap profile of the statistics workload.
#
# Usage: profiling/run_valgrind.sh [build-dir]   (default: build-prof)
# Configure the build with -DCMAKE_BUILD_TYPE=RelWithDebInfo so symbols and
# line numbers are available while the code is still optimised.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${1:-${ROOT}/build-prof}"
BIN="${BUILD}/01_hanson_modern_cpp_finance"
OUT="${ROOT}/01_hanson_modern_cpp_finance/profiling/results/valgrind"
mkdir -p "${OUT}"

if [[ ! -x "${BIN}/hanson_profile" ]]; then
  cmake -S "${ROOT}" -B "${BUILD}" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCPP4Q_BUILD_BENCHMARKS=OFF
  cmake --build "${BUILD}" -j
fi

echo "== memcheck"
status=0
for exe in "${BIN}"/tests/*_test "${BIN}"/*_demo; do
  name="$(basename "${exe}")"
  if valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
      --errors-for-leak-kinds=all --track-origins=yes --error-exitcode=99 \
      "${exe}" >/dev/null 2>"${OUT}/memcheck.${name}.log"; then
    echo "  ok    ${name}"
  else
    echo "  FAIL  ${name} (see ${OUT}/memcheck.${name}.log)"
    status=1
  fi
done

echo "== helgrind / drd (mc_parallel)"
valgrind --tool=helgrind --error-exitcode=99 "${BIN}/hanson_profile" \
  mc_parallel >/dev/null 2>"${OUT}/helgrind.log" || status=1
valgrind --tool=drd --error-exitcode=99 "${BIN}/hanson_profile" \
  mc_parallel >/dev/null 2>"${OUT}/drd.log" || status=1
grep -h "ERROR SUMMARY" "${OUT}/helgrind.log" "${OUT}/drd.log"

echo "== callgrind"
for w in stats mc matrix tree bonds orderbook; do
  valgrind --tool=callgrind --callgrind-out-file="${OUT}/callgrind.${w}.out" \
    "${BIN}/hanson_profile" "${w}" >/dev/null 2>&1
  callgrind_annotate --inclusive=no "${OUT}/callgrind.${w}.out" \
    >"${OUT}/callgrind.${w}.txt" 2>/dev/null
  printf '  %-10s %s Ir\n' "${w}" \
    "$(grep -m1 -E '^(summary|totals):' "${OUT}/callgrind.${w}.out" |
       awk '{print $2}')"
done

echo "== cachegrind"
for w in matrix tree; do
  valgrind --tool=cachegrind --cache-sim=yes \
    --cachegrind-out-file="${OUT}/cachegrind.${w}.out" \
    "${BIN}/hanson_profile" "${w}" 2>&1 >/dev/null |
    grep -E "D1  miss rate|LL miss rate" | sed "s/^/  ${w}: /"
done

echo "== massif (stats)"
valgrind --tool=massif --massif-out-file="${OUT}/massif.stats.out" \
  "${BIN}/hanson_profile" stats >/dev/null 2>&1
ms_print "${OUT}/massif.stats.out" >"${OUT}/massif.stats.txt"
printf '  peak heap: %s bytes\n' \
  "$(grep mem_heap_B "${OUT}/massif.stats.out" | cut -d= -f2 | sort -n |
     tail -1)"

exit "${status}"
