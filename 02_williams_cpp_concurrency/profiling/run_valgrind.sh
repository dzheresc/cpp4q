#!/usr/bin/env bash
# Runs the valgrind suite over book 2.
#
#   memcheck   - every unit-test binary and demo (leaks, invalid access,
#                uninitialised reads); fails on any error.
#   helgrind,
#   drd        - each threaded workload in profile_driver.cc. Both tools
#                model pthread mutexes/condvars but not C++ atomics, so on
#                workloads synchronised by atomics (ours, or inside
#                std::future / std::barrier) they report races on memory
#                that is in fact ordered by acquire/release. Those are
#                listed separately; ThreadSanitizer (run_tsan.sh) is the
#                check for them.
#   callgrind  - instruction-level hotspots per workload.
#   cachegrind - D1/LL miss rates (note: it simulates one core's caches,
#                so it cannot show false sharing; the benchmark does).
#   massif     - heap profile of the queue / message-passing workloads.
#
# Usage: profiling/run_valgrind.sh [build-dir]   (default: build-prof)

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${1:-${ROOT}/build-prof}"
BIN="${BUILD}/02_williams_cpp_concurrency"
PROF="${BIN}/williams_profile"
OUT="${ROOT}/02_williams_cpp_concurrency/profiling/results/valgrind"
SUPP="${ROOT}/02_williams_cpp_concurrency/profiling/valgrind.supp"
mkdir -p "${OUT}"

if [[ ! -x "${PROF}" ]]; then
  cmake -S "${ROOT}" -B "${BUILD}" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCPP4Q_BUILD_BENCHMARKS=OFF
  cmake --build "${BUILD}"
fi

# Workloads synchronised only through pthread mutexes / condvars and
# thread join: helgrind and drd must report zero errors.
LOCK_BASED="accumulate transfers quotes queue histogram lookup list twolock
            var scanfind gateway"
# Workloads whose synchronisation helgrind/drd can't see: our own atomics
# (ch05, ch07), libstdc++'s futex-based std::future and std::barrier
# (futures, lockstep, pool), and glibc's thread-stack cache under
# std::async (sort). ThreadSanitizer checks these instead.
LOCK_FREE="spinlock seqlock spsc mpmc stack falseshare futures lockstep pool
           sort"

echo "== memcheck"
status=0
for exe in "${BIN}"/tests/williams_*_test "${BIN}"/williams_*_demo; do
  name="$(basename "${exe}")"
  if timeout 900 valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
      --errors-for-leak-kinds=definite,indirect,possible \
      --track-origins=yes --error-exitcode=99 --suppressions="${SUPP}" \
      "${exe}" >/dev/null 2>"${OUT}/memcheck.${name}.log"; then
    echo "  ok    ${name}"
  else
    echo "  FAIL  ${name} (see ${OUT}/memcheck.${name}.log)"
    status=1
  fi
done

errors() { grep -m1 "ERROR SUMMARY" "$1" | awk '{print $4}'; }

for tool in helgrind drd; do
  echo "== ${tool} (lock-based workloads: must be clean)"
  for w in ${LOCK_BASED}; do
    log="${OUT}/${tool}.${w}.log"
    timeout 900 valgrind --tool="${tool}" "${PROF}" "${w}" >/dev/null 2>"${log}"
    n="$(errors "${log}")"
    printf '  %-11s %s errors\n' "${w}" "${n}"
    [[ "${n}" == "0" ]] || status=1
  done
  echo "== ${tool} (atomics-based workloads: not modelled, informational)"
  for w in ${LOCK_FREE}; do
    log="${OUT}/${tool}.${w}.log"
    timeout 900 valgrind --tool="${tool}" "${PROF}" "${w}" >/dev/null 2>"${log}"
    printf '  %-11s %s reports\n' "${w}" "$(errors "${log}")"
  done
done

echo "== callgrind"
for w in sort pool scanfind var histogram lookup gateway queue; do
  valgrind --tool=callgrind --callgrind-out-file="${OUT}/callgrind.${w}.out" \
    "${PROF}" "${w}" >/dev/null 2>&1
  callgrind_annotate --inclusive=no "${OUT}/callgrind.${w}.out" \
    >"${OUT}/callgrind.${w}.txt" 2>/dev/null
  printf '  %-10s %s Ir\n' "${w}" \
    "$(grep -m1 -E '^(summary|totals):' "${OUT}/callgrind.${w}.out" |
       awk '{print $2}')"
done

echo "== cachegrind"
for w in scanfind var histogram lookup; do
  valgrind --tool=cachegrind --cache-sim=yes \
    --cachegrind-out-file="${OUT}/cachegrind.${w}.out" \
    "${PROF}" "${w}" 2>&1 >/dev/null |
    grep -E "D1  miss rate|LL miss rate" | sed "s/^/  ${w}: /"
done

echo "== massif"
for w in queue futures gateway stack; do
  valgrind --tool=massif --massif-out-file="${OUT}/massif.${w}.out" \
    "${PROF}" "${w}" >/dev/null 2>&1
  ms_print "${OUT}/massif.${w}.out" >"${OUT}/massif.${w}.txt"
  printf '  %-10s peak heap: %s bytes\n' "${w}" \
    "$(grep mem_heap_B "${OUT}/massif.${w}.out" | cut -d= -f2 | sort -n |
       tail -1)"
done

exit "${status}"
