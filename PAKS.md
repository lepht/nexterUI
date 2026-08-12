# nexterUI paks

A pak is a folder with a `.pak` extension containing a shell script named `launch.sh`. That is the entire contract. nexterUI launches the script; everything else is up to you.

There are two kinds:

| Kind | Lives in | Purpose |
|---|---|---|
| **Emulator pak** | `/Emus/<platform>/` | Runs a game. Matched to ROMs by folder tag. |
| **Tool pak** | `/Tools/<platform>/` | Anything else. Appears in the Tools menu, launched directly. |

Both are just folders with a `launch.sh`. The only thing that makes one an emulator and the other a tool is which directory it sits in.

> **Compatibility.** nexterUI is a fork of [NextUI](https://github.com/LoveRetro/NextUI) and keeps the pak format unchanged. Paks built for NextUI work here as-is, and everything in this document applies to both.

**Contents**

- [Where paks live](#where-paks-live)
- [How a pak runs](#how-a-pak-runs) — read this first
- [Environment](#environment)
- [Tool paks](#tool-paks)
- [Emulator paks](#emulator-paks)
- [What paks can do](#what-paks-can-do)
- [What paks cannot do](#what-paks-cannot-do)
- [Installing, updating, uninstalling](#installing-updating-uninstalling)
- [Debugging](#debugging)

---

## Where paks live

```
/mnt/SDCARD/
├── Emus/<platform>/<TAG>.pak/          user emulator paks
├── Tools/<platform>/<Name>.pak/        user tool paks
├── Roms/<System Name> (<TAG>)/         games, tagged by folder name
├── Bios/<TAG>/
├── Saves/<TAG>/
├── Cheats/<TAG>/
├── Collections/<name>.txt              plain text game lists
├── Shaders/  Overlays/  Palettes/      drop-in assets
├── .system/<platform>/                 REPLACED ON EVERY UPDATE
│   ├── bin/                            nextui.elf, minarch.elf, helpers
│   ├── cores/                          bundled libretro cores
│   ├── lib/
│   └── paks/                           built-in paks (MinUI.pak, Emus/)
└── .userdata/
    ├── <platform>/                     survives updates
    │   ├── .hooks/                     see HOOKS.md
    │   ├── logs/
    │   └── auto.sh                     legacy boot script
    └── shared/
        ├── minuisettings.txt           all nexterUI settings
        └── .minui/                     recents, resume state
```

Never install a pak into `.system/`. That directory is deleted and recreated by every nexterUI update.

### Platform folders

Paks are platform-specific. The platform folder name matches the `PLATFORM` envar and is always lowercase.

| Platform | Devices | `DEVICE` values |
|---|---|---|
| `tg5040` | Trimui Brick, Smart Pro, Brick Pro | `brick`, `smartpro`, `brickpro` |
| `tg5050` | Trimui Smart Pro S | `smartpros` |
| `desktop` | local development build | *(unset)* |

`DEVICE` distinguishes hardware variants within one platform. A pak can use or ignore it. Do not assume it is set — on `tg5050` it is only exported when the model string matches, so treat an empty `DEVICE` as "the platform's default device".

```sh
case "$DEVICE" in
    brick|brickpro) CFG="tg3040.cfg" ;;
    *)              CFG="tg5040.cfg" ;;
esac
```

---

## How a pak runs

This is the single most important thing to understand, and it explains nearly every limitation further down.

**nexterUI's launcher exits before your pak starts.**

The real init process is the shell loop in `.system/<platform>/paks/MinUI.pak/launch.sh`:

```sh
while [ -f $EXEC_PATH ]; do
    nextui.elf &> $LOGS_PATH/nextui.txt      # the launcher UI runs here, then EXITS

    if [ -f $NEXT_PATH ]; then               # /tmp/next
        CMD=`cat $NEXT_PATH`
        parse_hook_cmd "$CMD"
        run_hooks.sh pre-launch.d
        eval $CMD                            # ← your pak runs here
        run_hooks.sh post-launch.d
        rm -f $NEXT_PATH
    fi
done
```

When you select a pak, `nextui.elf` writes the command to `/tmp/next`, sets its quit flag, and terminates. The shell loop picks the command up, runs it, and re-execs `nextui.elf` when it returns.

### What follows from this

- **Your pak owns the device.** Full screen, full input, no launcher competing for either.
- **The launcher is not running.** You cannot call into it, draw over it, or extend it. It is a dead process.
- **Exiting your pak returns to the launcher.** Just return from `launch.sh`.
- **You cannot chain-launch through `/tmp/next`.** The loop does `rm -f $NEXT_PATH` *after* `eval $CMD` returns, so anything you write there is deleted before the launcher restarts. To launch a game from a pak, exec the emulator directly:
  ```sh
  "$SDCARD_PATH/Emus/$PLATFORM/GB.pak/launch.sh" "$ROMS_PATH/Game Boy (GB)/game.gb"
  ```
- **Startup and teardown are not free.** Every pak launch is a full launcher shutdown and cold restart — a visible black screen for a second or more on each side.
- **Nothing sandboxes you.** Paks run as root with unrestricted filesystem access. The stock `Remove Loading.pak` rewrites `/etc/init.d/runtrimui` with `sed -i`. You can do real and permanent damage; be careful, and be conservative with anything outside the SD card.

### What stays running underneath

Daemons started at boot survive across pak launches:

| Process | Responsibility |
|---|---|
| `keymon.elf` | global button handling — MENU/POWER combos, brightness, volume |
| `batmon.elf` | battery monitoring |
| `audiomon.elf` | audio device hotplug |
| `trimui_inputd` | stock GPIO input daemon |

`keymon.elf` is why global shortcuts sometimes half-work inside a standalone binary, and why they sometimes don't — it reacts to key events, but it cannot make an application that ignores those events behave.

---

## Environment

`launch.sh` is invoked from the boot script, so it inherits everything that script exported.

### Paths

| Variable | Value on device | Notes |
|---|---|---|
| `PLATFORM` | `tg5040` | |
| `DEVICE` | `brick` / `smartpro` / `brickpro` / `smartpros` | may be unset |
| `SDCARD_PATH` | `/mnt/SDCARD` | |
| `ROMS_PATH` | `$SDCARD_PATH/Roms` | |
| `BIOS_PATH` | `$SDCARD_PATH/Bios` | |
| `SAVES_PATH` | `$SDCARD_PATH/Saves` | |
| `CHEATS_PATH` | `$SDCARD_PATH/Cheats` | |
| `SYSTEM_PATH` | `$SDCARD_PATH/.system/$PLATFORM` | wiped on update |
| `CORES_PATH` | `$SYSTEM_PATH/cores` | |
| `USERDATA_PATH` | `$SDCARD_PATH/.userdata/$PLATFORM` | survives updates |
| `SHARED_USERDATA_PATH` | `$SDCARD_PATH/.userdata/shared` | cross-platform |
| `LOGS_PATH` | `$USERDATA_PATH/logs` | |
| `HOOKS_PATH` | `$USERDATA_PATH/.hooks` | see [HOOKS.md](HOOKS.md) |
| `DATETIME_PATH` | `$SHARED_USERDATA_PATH/datetime.txt` | |
| `IS_NEXT` | `yes` | set on nexterUI and NextUI, absent on stock MinUI |
| `TRIMUI_MODEL` | e.g. `Trimui Brick` | Trimui platforms only |

`HOME` is set to `$USERDATA_PATH` on device platforms. It is **not** exported on `desktop`, so set it yourself if your pak depends on it.

Use `IS_NEXT` to detect nexterUI or NextUI when writing a pak that also targets stock MinUI. Nothing in the environment distinguishes the fork from upstream — both set it:

```sh
if [ "$IS_NEXT" = "yes" ]; then
    # not available on stock MinUI
fi
```

### Search paths

```sh
PATH=$SYSTEM_PATH/bin:/usr/trimui/bin:$PATH
LD_LIBRARY_PATH=$SYSTEM_PATH/lib:/usr/trimui/lib:$LD_LIBRARY_PATH
```

Everything in `.system/<platform>/bin` is callable by bare name — `minarch.elf`, `show2.elf`, `nextval.elf`, `syncsettings.elf`, `gametimectl.elf`, `governor.sh`, `run_hooks.sh`.

### CPU governor

The boot loop sets the governor to `performance` immediately before running your pak, and again after it exits. If your pak is long-running and not performance-sensitive, be a good citizen:

```sh
sh "$SYSTEM_PATH/bin/governor.sh" "auto"
```

---

## Tool paks

A tool pak is any pak in `/Tools/<platform>/`. It appears in the Tools menu, named after its folder minus the `.pak` extension.

Minimal example — `/Tools/tg5040/Hello.pak/launch.sh`:

```sh
#!/bin/sh

cd "$(dirname "$0")"
./hello.elf &> "$LOGS_PATH/hello.txt"
```

`cd "$(dirname "$0")"` first is the near-universal convention: it makes bundled binaries and assets addressable by relative path regardless of how the pak was invoked.

### Visibility

An entry is hidden from every menu when its name:

- starts with `.`
- ends with `.disabled`
- is exactly `map.txt`

The `.disabled` suffix is the standard self-uninstall mechanism. `Remove Loading.pak` does a one-shot job and then removes itself from the menu:

```sh
mv "$DIR" "$DIR.disabled"
```

The Tools menu entry itself is hidden entirely when `/Tools/<platform>/` does not exist, or when the user turns off **Show Tools** in Settings.

### Renaming entries

A `map.txt` in a listed directory renames entries for display. One `filename<TAB>Display Name` pair per line. This works in `Roms` subfolders and any browsed directory.

---

## Emulator paks

### Tags and matching

nexterUI maps a ROM to a pak using the tag in parentheses at the end of its parent folder name:

```
/Roms/Game Boy (GB)/Tetris.gb   →   GB.pak
```

Tags are uppercase. Choose one that other frontends already use (`FC`, `MD`, `SFC`). If it's taken, use the core name (`MGBA`), an abbreviation (`PKM` for pokemini), or a truncation (`SUPA` for mednafen_supafaust).

Resolution order when launching, from `getEmuPath()`:

1. `/Emus/<platform>/<TAG>.pak/launch.sh` — user pak
2. `.system/<platform>/paks/Emus/<TAG>.pak/launch.sh` — built-in

**User paks win.** Dropping a `GB.pak` into `/Emus/tg5040/` overrides the built-in one without touching `.system/`. This is the supported way to customize a stock emulator.

### The three types

**1. Reuse a bundled core.** Known-good core, your own defaults and separate configs. Full nexterUI integration. `GG.pak` in the extras bundle does this with the stock picodrive core.

**2. Bundle your own core.** Supports new systems, keeps full integration — resume from menu, quicksave, auto-resume, consistent in-game menus and options. Add one line to point at your own core directory:

```sh
CORES_PATH=$(dirname "$0")
```

`MGBA.pak` in the extras bundle does this.

**3. Bundle a standalone emulator.** Sometimes squeezes out more performance than a libretro core. The cost is total loss of integration: no resume from menu, no quicksave or auto-resume, no consistent in-game menus, behaviors, or options, and MENU/POWER may misbehave or not work at all. Treat this as a last resort.

### Boilerplate

```sh
#!/bin/sh

EMU_EXE=picodrive

###############################

EMU_TAG=$(basename "$(dirname "$0")" .pak)
ROM="$1"
mkdir -p "$BIOS_PATH/$EMU_TAG"
mkdir -p "$SAVES_PATH/$EMU_TAG"
mkdir -p "$CHEATS_PATH/$EMU_TAG"
HOME="$USERDATA_PATH"
cd "$HOME"
minarch.elf "$CORES_PATH/${EMU_EXE}_libretro.so" "$ROM" &> "$LOGS_PATH/$EMU_TAG.txt"
```

Change `EMU_EXE` to any core name (minus `_libretro.so`). Nothing below the hash marks needs editing: it derives the tag from the folder name, creates the matching bios/saves/cheats folders, sets `HOME`, launches the game, and logs to `$LOGS_PATH/<TAG>.txt`.

On Anbernic RG\*XX platforms, replace the trailing `&> "$LOGS_PATH/$EMU_TAG.txt"` with `> "$LOGS_PATH/$EMU_TAG.txt" 2>&1` — the default shell there doesn't support `&>`.

### Option defaults and button bindings

Copy the pak and some ROMs to a card, launch a game, press MENU → Options, and configure Frontend, Emulator, and Controls. Standard practice is to bind only buttons present on the original hardware — no turbo, no core-specific features like palette or disk switching. Then **Save Changes → Save for Console**.

Back on your computer, find `.userdata/<platform>/<TAG>-<core>/minarch.cfg`, copy it into the pak as `default.cfg`, and delete every option you didn't deliberately change.

Two things worth knowing:

- Prefixing an option name with `-` sets it *and hides it* from the user. Useful for disabling features that are unavailable or perform badly on a given platform.
- A `default-<device>.cfg` (e.g. `default-brick.cfg`) takes precedence over `default.cfg` on that device.

Bindings look like this:

```
bind Up = UP
bind A Button = A
bind A Turbo = NONE:X
bind L Button = L1
bind More Sun = NONE:L3
```

Everything between `bind ` and `=` is the label shown in the Controls menu — normalize these (`Up`, not `D-pad up`; `A Button`, not `A`). Everything after `=` up to an optional `:` is the mapping, uppercase, with shoulder and stick buttons numbered (`L1`, not `L`). Use `NONE` to leave a control unbound, and always preserve the core's original default after a `:` when you override or remove one.

### Brightness and volume

Some binaries reset brightness or volume on launch (DinguxCommander, ppssppSDL). `syncsettings.elf` waits one second and restores nexterUI's current values. Usually launching it as a daemon first is enough:

```sh
syncsettings.elf &
./DinguxCommander
```

If the binary takes longer than a second to initialize, loop it for the binary's lifetime:

```sh
while :; do syncsettings.elf; done &
LOOP_PID=$!

./PPSSPPSDL --pause-menu-exit "$ROM_PATH"

kill $LOOP_PID
```

---

## What paks can do

### Bundled helper binaries

All on `PATH`:

| Binary | Purpose |
|---|---|
| `minarch.elf` | the libretro frontend — full nexterUI game integration |
| `show2.elf` | splash / progress UI. Simple, progress, and daemon modes; daemon mode accepts live updates over `/tmp/show2.fifo`. See `workspace/all/show2/README.md` |
| `syncsettings.elf` | restore nexterUI brightness and volume |
| `nextval.elf` | read any nexterUI setting as JSON — `nextval.elf wifi` → `{"wifi": 1}`; no args prints everything |
| `gametimectl.elf` | game time tracking |
| `governor.sh` | set the CPU governor (`auto`, `performance`) |
| `run_hooks.sh` | run a hook directory; see [HOOKS.md](HOOKS.md) |

A progress UI for a long-running pak:

```sh
show2.elf --mode=daemon --image="$SDCARD_PATH/.system/res/logo.png" --text="Working..." &
echo "PROGRESS:50" > /tmp/show2.fifo
echo "TEXT:Almost done" > /tmp/show2.fifo
killall show2.elf
```

### Reading and writing settings

Every nexterUI setting lives in one flat file, `$SHARED_USERDATA_PATH/minuisettings.txt`, as `key=value` lines. Read individual values with `nextval.elf`:

```sh
wifion=$(nextval.elf wifi | sed -n 's/.*"wifi": \([0-9]*\).*/\1/p')
```

You can write the file directly, but the launcher only reads it at startup and rewrites it wholesale on any change — so edit it while the launcher is not running (which, inside a pak, it isn't), and expect concurrent writes from a running `Settings.pak` to clobber yours.

### Hooks

The only mechanism that runs pak-supplied code *outside* of a pak launch. Scripts dropped into `$USERDATA_PATH/.hooks/<phase>.d/` are executed by the OS at boot, around every ROM and pak launch, and around suspend and resume.

A `boot.d` hook is how a pak gets a resident background daemon that lives alongside the launcher for the whole session. See **[HOOKS.md](HOOKS.md)** for the full contract.

### Drop-in assets

No code required — these are read from fixed locations at runtime:

| Location | Contents |
|---|---|
| `/Shaders/` | GLSL shaders and `.cfg` presets |
| `/Overlays/<TAG>/` | per-system overlay images |
| `/Palettes/` | UI color palettes |
| `/Collections/<name>.txt` | game lists — one SD-relative path per line; appear in the main menu |
| `.system/res/palettes/` | built-in palettes (wiped on update) |

Collections are worth calling out: a plain text file is enough to add a curated, user-visible game list to the main menu, with no code at all.

### Named integrations

A small number of paks get special treatment by name, all in `Tools/<platform>/`:

- **`Settings.pak`** and **`Pak Store.pak`** are promoted into the quick menu's toggle row with their own icons.
- Any tool pak can be bound to **FN1**, **FN2**, or **HOME** through Settings, launching it from anywhere in the menu.

These are hardcoded in the launcher. There is no registry for adding more.

---

## What paks cannot do

Because the launcher has exited before your pak starts, and because there is no plugin API:

- **Add or modify anything in the launcher UI.** Menu rows, list entries, settings pages, quick menu items, and button hints are compiled into `nextui.elf` and built in-process at runtime. There is no data-driven registry for any of them.
- **Draw over or alongside the launcher.** There is one framebuffer and `nextui.elf` owns it while it runs.
- **Intercept input in the launcher.** Menu navigation is read directly by `nextui.elf` through SDL. `keymon.elf` sees global key events, but it cannot change how the launcher interprets what it reads.
- **Add a new launcher screen or view.** Screens are enum values in the launcher's own state machine.
- **Use launcher internals** — its entry list, sorting, deduplication, save-state and resume plumbing, theme colors, or fonts. None of it is exported.
- **Cancel a launch from a pre-launch hook.** Hooks observe and set up; they cannot veto.
- **Run concurrently with the launcher**, except as a daemon started from a `boot.d` hook — and such a daemon still cannot draw or intercept input.

If a feature needs to live *inside* the menu — a context menu on a listed game, a new row in the quick menu, a search screen sharing resume state — it cannot be built as a pak. It requires patching `nextui.c` and rebuilding, with the maintenance cost that implies, since `.system/` is replaced by every update.

---

## Installing, updating, uninstalling

**Install:** copy the `.pak` folder to `/Emus/<platform>/` or `/Tools/<platform>/`. That's it — no registration, no manifest, no restart.

**Update behavior:** this asymmetry causes most pak lifecycle bugs.

| Path | On nexterUI update |
|---|---|
| `.system/` | **deleted and replaced** |
| `/Emus/`, `/Tools/` | untouched |
| `.userdata/` | untouched |

So a pak in `/Tools/` survives updates — but anything it installed into `.system/` does not, and anything it installed into `.userdata/` survives *even after the pak is deleted*.

**Uninstall:** there is no uninstall hook, and nothing runs when a user deletes a pak folder. If your pak writes outside its own directory — hooks especially — those files are orphaned on removal and will keep running indefinitely. Two mitigations:

1. Namespace every file you install (`myapp-resume.sh`, not `resume.sh`).
2. Make hooks self-checking, so an orphan exits immediately instead of failing loudly:

```sh
#!/bin/sh
# myapp-sync.sh
MYAPP="$SDCARD_PATH/Tools/$PLATFORM/MyApp.pak"
[ -x "$MYAPP/myapp" ] || exit 0    # pak was removed; nothing to do
"$MYAPP/myapp" --sync
```

Prefer `.disabled` over deletion for anything reversible — it hides the pak from every menu while leaving it in place.

---

## Debugging

Log to `$LOGS_PATH`, which is the first place to look after anything goes wrong:

```sh
./mybinary.elf &> "$LOGS_PATH/mypak.txt"
```

The launcher's own log is `$LOGS_PATH/nextui.txt`, rewritten on every launcher start — so it holds output from the most recent launcher session only, not from before your pak ran.

Note that hook output is discarded entirely (`run_hooks.sh` redirects to `/dev/null`), so a hook must log to its own file or it logs nothing.

`gdbserver` is available at `.system/<platform>/dbg/`:

```sh
"$SYSTEM_PATH/dbg/launch.sh" :1234 ./mybinary.elf
```

For iteration without hardware, the `desktop` platform builds and runs the same launcher and paks locally.

---

## Caveats

- nexterUI only supports the **RGB565** pixel format and does not implement the OpenGL libretro APIs. Cores requiring either will not work under `minarch.elf`. Using the stock firmware's retroarch instead is possible but unsupported.
- Third-party paks are not supported by the nexterUI project, and neither upstream NextUI nor MinUI supports them either. When a console or core is absent from the base or extras bundles that is usually deliberate — poor integration, unreliable save states, weak performance on the target hardware, or arcane ROM set requirements. Make this clear to your users.
- Paths in the launcher are largely fixed-size `char[256]` buffers. Deeply nested directories and very long names can be truncated.
- `$SDCARD_PATH` is FAT32: case-insensitive, no symlinks, no executable permission bit. Scripts run via their shebang regardless of mode bits.

---

## See also

- **[HOOKS.md](HOOKS.md)** — the hook system: phases, environment, installation
- `workspace/all/show2/README.md` — full `show2.elf` reference
- `skeleton/EXTRAS/Tools/<platform>/` — the stock tool paks, all readable shell
- `skeleton/SYSTEM/<platform>/paks/Emus/` — the built-in emulator paks
