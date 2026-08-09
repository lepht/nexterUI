# nexterUI hooks

Hooks are the only mechanism that runs pak-supplied code *outside* of a pak launch. A hook is a shell script the OS executes at a fixed point in its lifecycle — at boot, around every ROM and pak launch, and around suspend and resume.

If you are writing a pak, read **[PAKS.md](PAKS.md)** first; hooks are an extension of that model, not a replacement for it.

## The idea

Hooks are platform-specific, just like paks. The launcher reads them from `$USERDATA_PATH/.hooks/`:

```
$USERDATA_PATH/.hooks/
    boot.d/          # after auto.sh, before the launcher starts
    pre-launch.d/    # before a ROM or pak launches
    post-launch.d/   # after it exits
    pre-sleep.d/     # before the device suspends
    post-resume.d/   # after it wakes
```

On device, `USERDATA_PATH` resolves to `/mnt/SDCARD/.userdata/$PLATFORM`, so the real paths are:

```
/mnt/SDCARD/.userdata/tg5040/.hooks/post-launch.d/shortcuts-resume.sh
```

If a directory doesn't exist, nothing happens and there is no overhead. The directories are not created for you — `mkdir -p` them at install time.

Note that hooks live in `.userdata/`, **not** inside your pak. See [Installing and removing hooks](#installing-and-removing-hooks) for what that implies.

## Phases

| Directory | Runs | Fired by |
|---|---|---|
| `boot.d` | once at boot, after `auto.sh`, before the launcher's first start | `MinUI.pak/launch.sh` |
| `pre-launch.d` | before every ROM **and** pak launch | `MinUI.pak/launch.sh` |
| `post-launch.d` | after the ROM or pak exits, before the launcher restarts | `MinUI.pak/launch.sh` |
| `pre-sleep.d` | before suspend, while wifi and bluetooth are **still up** | `bin/suspend` |
| `post-resume.d` | immediately after wake, before wifi and bluetooth are **restarted** | `bin/suspend` |

Two things worth planning around:

- **`boot.d` is where a resident daemon belongs.** It runs before the launcher exists and its children outlive it, so a daemon started here lives for the whole session — the closest thing to a persistent background service a pak can get.
- **Sleep hooks have no network.** `pre-sleep.d` runs before wifi and bluetooth are stopped, so it can still reach the network. `post-resume.d` runs before they are restarted, so it cannot. Anything needing connectivity after wake must poll or retry.

### Platform availability

`boot.d`, `pre-launch.d` and `post-launch.d` exist on every platform. `pre-sleep.d` and `post-resume.d` are fired by `bin/suspend`, which exists on `tg5040` and `tg5050` but **not** on `desktop` — sleep hooks never fire on the desktop build.

## Environment variables

Hook scripts inherit all standard nexterUI environment variables (`SDCARD_PATH`, `PLATFORM`, `USERDATA_PATH`, `SHARED_USERDATA_PATH`, `LOGS_PATH`, …). See the environment table in [PAKS.md](PAKS.md#environment).

`run_hooks.sh` additionally exports:

| Variable | Value | Set in |
|---|---|---|
| `HOOK_CATEGORY` | the directory name, e.g. `pre-launch.d` | every phase |
| `HOOK_PHASE` | `pre`, `post`, or `boot` | every phase |

And for launch phases only, the boot script exports:

| Variable | Description |
|---|---|
| `HOOK_TYPE` | `rom` or `pak` |
| `HOOK_CMD` | the raw launch command |
| `HOOK_EMU_PATH` | path to the emulator or pak `launch.sh` |
| `HOOK_ROM_PATH` | path to the ROM (empty for pak launches) |
| `HOOK_LAST` | contents of `/tmp/last.txt`, the last selected menu entry |

> **`HOOK_PHASE` does not identify the event.** It is derived from the directory name prefix, so `pre-launch.d` and `pre-sleep.d` both report `pre`, and `post-launch.d` and `post-resume.d` both report `post`. Use `HOOK_CATEGORY` when you need to know what actually happened.

> **The `HOOK_*` launch variables are only set for `pre-launch.d` and `post-launch.d`.** They are populated by `parse_hook_cmd()` in the boot loop, which only runs on the launch path. In `boot.d`, `pre-sleep.d` and `post-resume.d` they are unset — guard with `${HOOK_TYPE:-}` if a script is shared across phases.

## Writing a hook script

A hook is a `.sh` file in one of the hook directories. It is invoked directly, so give it a shebang.

```sh
#!/bin/sh
# my-hook.sh — log every ROM launch

[ "${HOOK_TYPE:-}" = "rom" ] || exit 0
echo "$(date): launched $HOOK_ROM_PATH" >> "$LOGS_PATH/launches.log"
```

### Execution model

`run_hooks.sh` iterates the directory's `*.sh` files in glob order (alphabetical) and, for each:

- a file ending in **`.sync.sh`** runs **synchronously** — the runner blocks until it exits
- every other file is started in a **background subshell**

After starting everything, the runner `wait`s for all background scripts before returning.

Two consequences that are easy to get wrong:

- **Scripts are *started* in alphabetical order, but background scripts *run concurrently*.** Filenames do not give you ordering. If script B depends on script A having finished, both must be `.sync.sh`, and A must sort first.
- **Backgrounding does not make a slow hook free.** The `wait` means every hook, background or not, delays whatever comes next — the launch, the return to the menu, or the boot. Slow work must be detached explicitly:

```sh
#!/bin/sh
# fire-and-forget: survives the runner's wait
( sleep 30; do_slow_thing ) </dev/null >/dev/null 2>&1 &
```

`pre-sleep.d` is invoked with `--sync-only`, which forces *every* script in it to run synchronously regardless of name — suspend must not race with a half-finished hook.

### Rules

- Each script runs in a subshell. A crash or non-zero exit cannot affect the launcher, the suspend sequence, or other hooks.
- **Output is discarded.** `run_hooks.sh` redirects stdout and stderr to `/dev/null`. If you need logging, write to your own file under `$LOGS_PATH`.
- **Pre-launch hooks cannot cancel a launch.** They are for observation and setup only; the return value is ignored.
- Keep hooks fast, for the reason above.
- Unlike `auto.sh`, hooks are composable — every pak manages its own script. Use a descriptive, namespaced filename to avoid collisions.

## Installing and removing hooks

Hooks live in `.userdata/`, outside your pak. This has a consequence worth designing for: **`.userdata/` survives both nexterUI updates and pak deletion.** A hook you install keeps running after the user deletes the pak that installed it, and nothing runs on uninstall to clean it up.

So:

**1. Namespace the filename.** `myapp-resume.sh`, never `resume.sh`. Collisions are silent.

**2. Make the hook self-checking**, so an orphan exits cleanly instead of failing on every launch forever:

```sh
#!/bin/sh
# myapp-sync.sh
MYAPP="$SDCARD_PATH/Tools/$PLATFORM/MyApp.pak"
[ -x "$MYAPP/myapp" ] || exit 0    # pak is gone; nothing to do
"$MYAPP/myapp" --sync >> "$LOGS_PATH/myapp-sync.txt" 2>&1
```

**3. Install idempotently** from your pak's `launch.sh`, since it may run many times:

```sh
HOOK_DIR="$USERDATA_PATH/.hooks/post-launch.d"
mkdir -p "$HOOK_DIR"
cp -f "$(dirname "$0")/hooks/myapp-sync.sh" "$HOOK_DIR/myapp-sync.sh"
```

**4. Offer a way to remove it.** A "Disable" entry in your pak that deletes its own hook is the only uninstall path a user has that doesn't involve a file manager.

## Example: sync after ROM exit

```sh
#!/bin/sh
# shortcuts-resume.sh — one-shot resume metadata sync after a ROM exits

[ "${HOOK_TYPE:-}" = "rom" ] || exit 0

SHORTCUTS_PAK="$SDCARD_PATH/Tools/$PLATFORM/Shortcuts.pak"
[ -x "$SHORTCUTS_PAK/shortcuts" ] || exit 0

"$SHORTCUTS_PAK/shortcuts" --resume-sync-hook >> "$LOGS_PATH/shortcuts-resume-sync.txt" 2>&1
```

## Example: a boot daemon

```sh
#!/bin/sh
# myapp-daemon.sh — start a background service for the session

MYAPP="$SDCARD_PATH/Tools/$PLATFORM/MyApp.pak"
[ -x "$MYAPP/myappd" ] || exit 0
pgrep -f myappd >/dev/null && exit 0    # already running

"$MYAPP/myappd" >> "$LOGS_PATH/myappd.txt" 2>&1 &
```

Started from `boot.d`, this outlives the runner and every launcher restart. It still cannot draw to the screen or intercept menu input — see [What paks cannot do](PAKS.md#what-paks-cannot-do).

## Debugging hooks

Hook output goes to `/dev/null`, so a broken hook is silent. To see what happened, log explicitly:

```sh
exec >> "$LOGS_PATH/myhook.txt" 2>&1
set -x
```

To verify a hook fires at all, have it touch a file and check the timestamp:

```sh
date >> "$LOGS_PATH/myhook-fired.txt"
```

You can also run the phase by hand from a shell with the nexterUI environment loaded:

```sh
"$SYSTEM_PATH/bin/run_hooks.sh" post-launch.d
```

## See also

- **[PAKS.md](PAKS.md)** — what paks are, how they run, and what they can and cannot do
- `skeleton/SYSTEM/<platform>/bin/run_hooks.sh` — the runner itself, 36 lines
- `skeleton/SYSTEM/<platform>/paks/MinUI.pak/launch.sh` — the boot loop that fires launch hooks
- `skeleton/SYSTEM/<platform>/bin/suspend` — the sleep sequence that fires sleep hooks
