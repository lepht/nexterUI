#!/usr/bin/env bash
# Benchmark the search index and filter on the desktop build, across a range of
# library sizes. Prints a table and keeps the raw BENCH lines.
#
#   bench/run_desktop.sh                  # 1000 5000 20000
#   bench/run_desktop.sh 500 40000        # explicit sizes
#
# Numbers from this are for comparing two commits on the same machine. They are
# not device numbers - a Cortex-A53 reading a microSD is a different machine in
# both the ways that matter. Use bench/device_bench.sh for that.
set -euo pipefail

SIZES=("$@")
[ ${#SIZES[@]} -eq 0 ] && SIZES=(1000 5000 20000)

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# SDCARD_PATH is compiled into the binary, so the benchmark has to use the same
# card the desktop build was built for. The generator only ever adds its own
# BENCH* systems to it and removes exactly those afterwards.
CARD="${BENCH_SDCARD:-/var/tmp/nextui/sdcard}"
DISPLAY_NUM="${BENCH_DISPLAY:-:97}"
OUT="${BENCH_OUT:-$(mktemp -d)}"

for tool in Xvfb python3; do
	command -v "$tool" >/dev/null || { echo "need $tool on PATH" >&2; exit 1; }
done

ELF="$REPO/workspace/all/nextui/build/desktop/nextui.elf"
if [ ! -x "$ELF" ]; then
	echo "no desktop build at $ELF" >&2
	echo "build it with: make build PLATFORM=desktop" >&2
	exit 1
fi

cleanup() {
	python3 "$REPO/bench/gen_library.py" --sdcard "$CARD" --clean >/dev/null 2>&1 || true
	[ -n "${XVFB_PID:-}" ] && kill "$XVFB_PID" 2>/dev/null
	return 0
}
trap cleanup EXIT

# GFX_init runs before the benchmark and needs a real display - the SDL dummy
# driver returns no renderer and the video layer dereferences its name.
if ! xdpyinfo -display "$DISPLAY_NUM" >/dev/null 2>&1; then
	Xvfb "$DISPLAY_NUM" -screen 0 640x480x24 >"$OUT/xvfb.log" 2>&1 &
	XVFB_PID=$!
	sleep 2
fi

mkdir -p "$CARD/.userdata/shared/.minui" "$CARD/.userdata/desktop"

printf '%-8s %-8s %9s %9s %9s %10s %12s %9s\n' \
	roms entries walk_ms sort_ms dedup_ms total_ms heap_bytes b/entry
printf '%s\n' "--------------------------------------------------------------------------------"

for size in "${SIZES[@]}"; do
	python3 "$REPO/bench/gen_library.py" --sdcard "$CARD" --clean >/dev/null 2>&1 || true
	python3 "$REPO/bench/gen_library.py" --sdcard "$CARD" --count "$size" >/dev/null

	log="$OUT/bench-$size.txt"
	DISPLAY="$DISPLAY_NUM" \
	LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}:/var/tmp/nextui/lib" \
	SDL_VIDEODRIVER=x11 \
	LIBGL_ALWAYS_SOFTWARE=1 \
	SHARED_USERDATA_PATH="$CARD/.userdata/shared" \
	USERDATA_PATH="$CARD/.userdata/desktop" \
	NEXTUI_BENCH=run \
		"$ELF" >"$log" 2>&1 || { echo "run failed at size $size:" >&2; tail -20 "$log" >&2; exit 1; }

	grep -q "BENCH search.index" "$log" || { echo "no measurements at size $size" >&2; exit 1; }

	awk -v size="$size" '
		/BENCH search.index/ {
			for (i=1;i<=NF;i++) { split($i,kv,"="); v[kv[1]]=kv[2] }
			printf "%-8s %-8s %9s %9s %9s %10s %12s %9s\n", size, v["entries"],
				v["walk_ms"], v["sort_ms"], v["dedupe_ms"], v["total_ms"],
				v["heap_bytes"], v["heap_per_entry"]
		}' "$log"
done

echo
echo "filter, best of 5 per query (ms):"
printf '%-8s %-10s %8s %10s\n' roms query hits ms
printf '%s\n' "----------------------------------------"
for size in "${SIZES[@]}"; do
	log="$OUT/bench-$size.txt"
	[ -f "$log" ] || continue
	# minimum is the sample least contaminated by whatever else the box was doing
	awk -v size="$size" '
		/BENCH search.filter/ {
			for (i=1;i<=NF;i++) { split($i,kv,"="); v[kv[1]]=kv[2] }
			q=v["query"]
			if (!(q in best) || v["ms"]+0 < best[q]+0) best[q]=v["ms"]
			hits[q]=v["hits"]
			if (!(q in seen)) { seen[q]=1; order[++n]=q }
		}
		END { for (i=1;i<=n;i++) { q=order[i]
			printf "%-8s %-10s %8s %10s\n", size, q, hits[q], best[q] } }' "$log"
done

echo
echo "raw output kept in $OUT"
