#!/bin/sh
# Measure search on a real device, against the real card. Run it over SSH on the
# handheld, not on your PC.
#
#   sh bench/device_bench.sh on      # turn logging on, then open search yourself
#   sh bench/device_bench.sh report  # print what was measured
#   sh bench/device_bench.sh off     # turn it back off
#
# This is the only way to get the number that actually matters. The index walk
# is I/O bound and a microSD is nothing like a desktop's storage - measurements
# taken anywhere else are about a machine nobody owns.
#
# Logging only adds timing calls and log lines. It never changes what the menu
# does, and the auto-run mode that exits on its own is desktop-only, so this
# cannot leave the device respawning nextui in a loop.

SDCARD_PATH="${SDCARD_PATH:-/mnt/SDCARD}"
MARKER="$SDCARD_PATH/.userdata/shared/.minui/bench"
LOG="$SDCARD_PATH/.userdata/shared/logs/nextui.txt"
[ -f "$LOG" ] || LOG="$SDCARD_PATH/.userdata/$(uname -m)/logs/nextui.txt"

case "${1:-}" in
on)
	mkdir -p "$(dirname "$MARKER")"
	touch "$MARKER"
	sync
	echo "benchmark logging on."
	echo
	echo "now, on the device:"
	echo "  1. quit to the menu (so nextui restarts and picks this up)"
	echo "  2. open the quick menu and go to Search"
	echo "  3. type a few characters"
	echo "  4. come back here and run: sh $0 report"
	;;
report)
	if [ ! -f "$MARKER" ]; then
		echo "benchmark logging is off - run '$0 on' first" >&2
		exit 1
	fi
	if [ ! -f "$LOG" ]; then
		echo "no log at $LOG (set SDCARD_PATH if your card is mounted elsewhere)" >&2
		exit 1
	fi

	roms=$(find "$SDCARD_PATH/Roms" -type f 2>/dev/null | wc -l)
	echo "roms on card: $roms"
	echo

	if ! grep -q "BENCH search.index" "$LOG"; then
		echo "no measurements in the log yet - open Search once, then re-run this" >&2
		exit 1
	fi

	echo "index build (once per launch, on first open of Search):"
	grep "BENCH search.index" "$LOG" | tail -1 | tr ' ' '\n' | grep '=' | sed 's/^/  /'
	echo
	echo "filter (once per keystroke):"
	grep "BENCH search.filter" "$LOG" | tail -10 | sed 's/.*BENCH/  BENCH/'
	;;
off)
	rm -f "$MARKER"
	sync
	echo "benchmark logging off (takes effect next time nextui restarts)"
	;;
*)
	echo "usage: $0 on|report|off" >&2
	exit 1
	;;
esac
