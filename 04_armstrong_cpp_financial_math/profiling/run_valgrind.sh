#!/usr/bin/env bash
# Runs the valgrind suite over book 4.
#
#   memcheck   - every unit-test binary and demo (leaks, invalid access,
#                uninitialised reads); fails on any error.
#   helgrind,
#   drd        - the threaded workload (chapter 20: atomic block counter
#                + jthread join, std::async futures, and a mutex).
#   callgrind  - instruction-level hotspots per workload.
#   cachegrind - D1/LL miss rates of the matrix and PDE workloads.
#   massif     - heap profile of the workloads that allocate.
#
# Usage: profiling/run_valgrind.sh [build-dir]   (default: build-prof)

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$(realpath -m "${1:-${ROOT}/build-prof}")"
BIN="${BUILD}/04_armstrong_cpp_financial_math"
PROF="${BIN}/armstrong_profile"
OUT="${ROOT}/04_armstrong_cpp_financial_math/profiling/results/valgrind"
mkdir -p "${OUT}"

if [[ ! -x "${PROF}" ]]; then
  cmake -S "${ROOT}" -B "${BUILD}" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCPP4Q_BUILD_BENCHMARKS=OFF
  cmake --build "${BUILD}"
fi

WORKLOADS="normals mc09 asian barrier hedging portfolio matmul linalg basket
           callables impliedvol parallel var pde_eu pde_am crr"

echo "== memcheck"
status=0
# Demos that write files (chapter 7) write them here.
SCRATCH="$(mktemp -d)"
trap 'rm -rf "${SCRATCH}"' EXIT
for exe in "${BIN}"/tests/armstrong_*_test "${BIN}"/armstrong_*_demo; do
  name="$(basename "${exe}")"
  if (cd "${SCRATCH}" && timeout 1800 valgrind --tool=memcheck --leak-check=full \
      --show-leak-kinds=all --errors-for-leak-kinds=definite,indirect,possible \
      --track-origins=yes --error-exitcode=99 \
      "${exe}" >/dev/null 2>"${OUT}/memcheck.${name}.log"); then
    echo "  ok    ${name}"
  else
    echo "  FAIL  ${name} (see ${OUT}/memcheck.${name}.log)"
    status=1
  fi
done

errors() { grep -m1 "ERROR SUMMARY" "$1" | awk '{print $4}'; }

for tool in helgrind drd; do
  echo "== ${tool}"
  log="${OUT}/${tool}.parallel.log"
  timeout 900 valgrind --tool="${tool}" "${PROF}" parallel >/dev/null 2>"${log}"
  n="$(errors "${log}")"
  printf '  %-11s %s errors\n' parallel "${n}"
  [[ "${n}" == "0" ]] || status=1
done

echo "== callgrind"
for w in ${WORKLOADS}; do
  valgrind --tool=callgrind --callgrind-out-file="${OUT}/callgrind.${w}.out" \
    "${PROF}" "${w}" >/dev/null 2>&1
  callgrind_annotate --inclusive=no "${OUT}/callgrind.${w}.out" \
    >"${OUT}/callgrind.${w}.txt" 2>/dev/null
  printf '  %-15s %15s Ir\n' "${w}" \
    "$(grep -m1 -E '^(summary|totals):' "${OUT}/callgrind.${w}.out" |
       awk '{print $2}')"
done

echo "== cachegrind"
for w in matmul linalg pde_eu asian; do
  valgrind --tool=cachegrind --cache-sim=yes \
    --cachegrind-out-file="${OUT}/cachegrind.${w}.out" \
    "${PROF}" "${w}" 2>&1 >/dev/null |
    grep -E "D1  miss rate|LL miss rate" | sed "s/^/  ${w}: /"
done

echo "== massif"
for w in mc09 asian hedging var pde_am; do
  valgrind --tool=massif --massif-out-file="${OUT}/massif.${w}.out" \
    "${PROF}" "${w}" >/dev/null 2>&1
  ms_print "${OUT}/massif.${w}.out" >"${OUT}/massif.${w}.txt"
  printf '  %-10s peak heap: %s bytes\n' "${w}" \
    "$(grep mem_heap_B "${OUT}/massif.${w}.out" | cut -d= -f2 | sort -n |
       tail -1)"
done

exit "${status}"
