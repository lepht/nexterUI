# Benchmarks

Two scripts, for two different questions.

| | question it answers |
|---|---|
| `run_desktop.sh` | did this commit make search slower than the last one? |
| `device_bench.sh` | how long does search actually take on a Brick, with a real card? |

Neither runs in CI, on purpose — see [below](#why-this-is-not-a-ci-gate).

## Turning measurement on

The binary prints nothing unless asked. Two ways to ask:

- `NEXTUI_BENCH=1` — measure the normal flow and log the numbers.
- `NEXTUI_BENCH=run` — measure, print, exit. **Desktop builds only**; on a device
  the launcher would respawn nextui forever.
- Creating `.userdata/shared/.minui/bench` on the card — same as `NEXTUI_BENCH=1`.
  This exists because a device has no convenient way to set an env var for
  `nextui.elf`, which `MinUI.pak/launch.sh` runs in a loop.

Output is one `key=value` line per measured operation, so a script can diff two
runs without parsing prose:

```
BENCH search.index entries=6020 truncated=0 walk_ms=9.52 sort_ms=2.72 dedupe_ms=0.73 total_ms=12.96 heap_bytes=1205328 heap_per_entry=200.2
BENCH search.filter query=star query_len=4 entries=6020 hits=197 ms=1.213
```

When it is off, the cost is one `getenv` and one `stat` at startup, and a
predictable branch at each measurement site. Nothing is timed and nothing is
logged.

## Desktop

```sh
make build PLATFORM=desktop
bench/run_desktop.sh              # 1000, 5000, 20000 roms
bench/run_desktop.sh 500 40000    # or pick sizes
```

Generates a synthetic library, runs the binary once per size, prints a table.
The library is deterministic for a given size and seed, so two commits see
byte-identical input and a difference is a real difference.

`SDCARD_PATH` is compiled into the binary, so the benchmark has to use the same
card the desktop build was built for. Rather than take the card over, the
generator adds its own `Bench System *` folders and matching `BENCH*` emulator
paks — tags that cannot collide with a real one — records every path it created
in `.bench-generated.json`, and on cleanup removes exactly those. It refuses to
start if a generated library is already present, and refuses to touch a system
folder or pak it did not make. Your dev card survives a run untouched.

Needs `Xvfb` and `python3`. `GFX_init` runs before the benchmark and needs a
real display; the SDL dummy driver returns no renderer and the video layer
dereferences its name.

Sample output, on one x86 host — reproduce it before comparing against it:

```
roms     entries    walk_ms   sort_ms  dedup_ms   total_ms   heap_bytes   b/entry
1000     1020          2.09      0.27      0.03       2.39       194304     190.5
5000     5024          5.85      1.70      0.47       8.02      1045424     208.1
20000    20018        19.59      8.47      5.17      33.23      4625664     231.1
```

Worth noticing that bytes/entry climbs with library size: more names collide, so
the dedupe pass allocates a `unique` string for more of them. That is the sort
of thing this is for.

**These numbers only compare commits on the same machine.** They are not device
numbers and should not be quoted as though they were.

## Device

Over SSH, on the handheld:

```sh
sh device_bench.sh on       # then open Search on the device and type
sh device_bench.sh report
sh device_bench.sh off
```

This measures your real library on real storage, which is the only place the
walk cost can be honestly established. The index walk is I/O bound; on a desktop
the same code varies 3x between a warm and cold page cache, and a microSD is a
different machine again.

## Why this is not a CI gate

Wall-clock thresholds on shared runners would be measuring the wrong machine:

- The runners are server-class ARM; the device is an in-order Cortex-A53 with
  small caches, and the two do not scale proportionally.
- The dominant cost is microSD I/O, which CI does not have. Dropping the page
  cache alone moves the walk 3x on one host.
- Multi-tenant runners vary enough on wall clock that any threshold tight enough
  to catch a real regression would also fire on noise — and a check that cries
  wolf gets ignored, then disabled, which is worse than no check.

The regressions worth catching here are structural rather than a few percent:
the index being rebuilt per frame instead of once per launch, a linear scan
turning quadratic, a `stat()` per file appearing in the walk. Those are
**countable**, and counts do not vary by machine — `entries`, `heap_bytes` and
`heap_per_entry` above are already deterministic.

Asserting on them in CI is a reasonable thing to want. It is blocked on
`nextui.c` being a single 4000-line translation unit with everything `static`,
so there is nothing to link a test against. If search ever moves into its own
unit, this gets cheap; until then it would be a refactor wearing a testing
costume.
