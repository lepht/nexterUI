#!/usr/bin/env python3
"""Generate a synthetic rom library for benchmarking.

Deterministic: the same --count and --seed produce byte-identical names every
time, so two runs are comparable and a difference is a real change rather than
a different set of strings. Files are empty - the index reads names and
directory structure, never contents.

Everything created is recorded in a manifest, and --clean removes only what is
listed there. It will not delete a directory or an emulator pak it did not make.
"""

import argparse
import json
import os
import random
import shutil
import sys

# No-Intro style names, since display-name parsing and sort cost depend on shape
FIRST = ["Legend of", "Super", "Mega", "Final", "Dragon", "Castle", "Star", "Battle",
         "Ninja", "Metal", "Shadow", "Crystal", "Golden", "Silver", "Phantom", "Mystic",
         "Ultimate", "Cosmic", "Iron", "Blazing", "Sonic", "Kirby's", "Contra", "Gradius"]
MID = ["Zelda", "Mario", "Man", "Fantasy", "Quest", "Vania", "Fox", "Toads", "Gaiden",
       "Gear", "Warrior", "Chronicles", "Axe", "Saga", "Force", "Knight", "Adventure",
       "Odyssey", "Storm", "Legends", "Hedgehog", "Dream Land", "Racer", "Tactics"]
LAST = ["", " II", " III", " IV", " V", " 2", " 3", " 64", " Advance", " DX", " Deluxe",
        " Zero", " X", " Turbo", " Gold", " Returns"]
REGION = ["(USA)", "(Europe)", "(Japan)", "(USA, Europe)", "(World)", "(USA) (Rev 1)"]

# deliberately not the same tags the dev card uses, so a generated library can
# never collide with a real emulator pak
SYSTEMS = [
    ("Bench System A (BENCHA)", ".bna"),
    ("Bench System B (BENCHB)", ".bnb"),
    ("Bench System C (BENCHC)", ".bnc"),
    ("Bench System D (BENCHD)", ".bnd"),
    ("Bench System E (BENCHE)", ".bne"),
    ("Bench System F (BENCHF)", ".bnf"),
    ("Bench System G (BENCHG)", ".bng"),
    ("Bench System H (BENCHH)", ".bnh"),
    ("Bench System I (BENCHI)", ".bni"),
    ("Bench System J (BENCHJ)", ".bnj"),
    ("Bench System K (BENCHK)", ".bnk"),
    ("Bench System L (BENCHL)", ".bnl"),
    ("Bench System M (BENCHM)", ".bnm"),
    ("Bench System N (BENCHN)", ".bnn"),
]

MANIFEST_NAME = ".bench-generated.json"


def emu_tag(system):
    return system[system.rfind("(") + 1:system.rfind(")")]


def do_clean(manifest_path):
    if not os.path.exists(manifest_path):
        print("nothing to clean (no manifest)", file=sys.stderr)
        return 0
    with open(manifest_path) as f:
        manifest = json.load(f)

    for path in manifest.get("created", []):
        shutil.rmtree(path, ignore_errors=True)
    os.remove(manifest_path)
    print(f"cleaned {len(manifest.get('created', []))} generated directories")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sdcard", required=True, help="card root, eg. /var/tmp/nextui/sdcard")
    ap.add_argument("--count", type=int, default=5000, help="total roms to create")
    ap.add_argument("--platform", default="desktop", help="platform whose paks dir gets stubs")
    ap.add_argument("--seed", type=int, default=1234)
    ap.add_argument("--clean", action="store_true", help="remove what a previous run created")
    args = ap.parse_args()

    roms = os.path.join(args.sdcard, "Roms")
    paks = os.path.join(args.sdcard, ".system", args.platform, "paks", "Emus")
    manifest_path = os.path.join(args.sdcard, MANIFEST_NAME)

    if args.clean:
        return do_clean(manifest_path)

    if os.path.exists(manifest_path):
        print(f"a generated library is already present - run with --clean first",
              file=sys.stderr)
        return 1

    random.seed(args.seed)
    os.makedirs(roms, exist_ok=True)
    os.makedirs(paks, exist_ok=True)

    created = []
    # every system needs an emu pak or hasRoms() skips it and nothing is indexed.
    # BENCH* tags cannot collide with a real pak, so these are safe to remove later.
    for system, _ in SYSTEMS:
        pak = os.path.join(paks, emu_tag(system) + ".pak")
        if os.path.exists(pak):
            print(f"unexpected existing pak {pak}, refusing to touch it", file=sys.stderr)
            return 1
        os.makedirs(pak)
        with open(os.path.join(pak, "launch.sh"), "w") as f:
            f.write("#!/bin/sh\n# benchmark stub, never executed\n")
        os.chmod(os.path.join(pak, "launch.sh"), 0o755)
        created.append(pak)

    seen = set()
    made = 0
    per_system = max(1, args.count // len(SYSTEMS))
    for index, (system, ext) in enumerate(SYSTEMS):
        directory = os.path.join(roms, system)
        if os.path.exists(directory):
            print(f"unexpected existing directory {directory}, refusing to touch it",
                  file=sys.stderr)
            return 1
        os.makedirs(directory)
        created.append(directory)

        # a couple of systems get a subfolder, because real cards have them and
        # the walk recurses
        nested = os.path.join(directory, "Hacks")
        if index % 5 == 0:
            os.makedirs(nested)

        for i in range(per_system):
            if made >= args.count:
                break
            while True:
                name = (f"{random.choice(FIRST)} {random.choice(MID)}"
                        f"{random.choice(LAST)} {random.choice(REGION)}")
                if name not in seen:
                    seen.add(name)
                    break
            target = nested if (index % 5 == 0 and i % 11 == 0) else directory
            open(os.path.join(target, name + ext), "w").close()
            made += 1

    with open(manifest_path, "w") as f:
        json.dump({"count": made, "seed": args.seed, "created": created}, f, indent=1)

    print(f"created {made} roms across {len(SYSTEMS)} systems under {roms}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
