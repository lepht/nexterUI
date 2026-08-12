CORES= a2600
CORES+= a5200
CORES+= a7800
CORES+= handy
CORES+= c64
CORES+= c128
CORES+= fake-08
CORES+= fbneo
CORES+= fceumm
CORES+= gambatte
CORES+= gpsp
CORES+= libretro-cap32
CORES+= libretro-uae
CORES+= mednafen_pce_fast
CORES+= mednafen_supafaust
CORES+= mednafen_vb
CORES+= mgba
CORES+= pcsx_rearmed
CORES+= pet
CORES+= picodrive
CORES+= plus4
CORES+= pokemini
CORES+= race
CORES+= snes9x
CORES+= vic
CORES+= prboom
CORES+= bluemsx
CORES+= gearcoleco

###############################

# optional core vars
# *_REPO=
# *_HASH=
# *_CORE=
# *_FLAGS=
# *_MAKEFILE=
# *_BUILD_PATH=
# *_SUBMODULES=

a2600_REPO = https://github.com/libretro/stella2014-libretro
a2600_CORE = stella2014_libretro.so

a5200_CORE = a5200_libretro.so

a7800_CORE = prosystem_libretro.so
a7800_REPO = https://github.com/libretro/prosystem-libretro

handy_REPO = https://github.com/libretro/libretro-handy

fbneo_REPO = https://github.com/libretro/FBNeo
fbneo_CORE = fbneo_libretro.so
fbneo_BUILD_PATH = fbneo/src/burner/libretro
fbneo_MAKE = make
fbneo_HASH = 6a5cc250c2db8d874a06ee86e302a2a78918b4c1

# libretro/mgba removed Makefile.libretro in 8940477 (2026-08-04) when its CI moved
# to cmake. patches/mgba.patch existed only to add a tg5040 block to that makefile,
# so it is gone and the cmake build carries the same flags directly.
mgba_HASH = e31759b24e7a4e3899285ff720d7b573ac328ae7
mgba_CORE = build-tg5040/mgba_libretro.so
mgba_MAKE = cmake -S . -B build-tg5040 -DCMAKE_BUILD_TYPE=Release -DLIBMGBA_ONLY=ON -DBUILD_LIBRETRO=ON -DLIBRETRO_STATIC=OFF -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=$(CROSS_COMPILE)gcc -DCMAKE_CXX_COMPILER=$(CROSS_COMPILE)g++ -DCMAKE_C_FLAGS="-fomit-frame-pointer -ffast-math -mtune=cortex-a53 -mcpu=cortex-a53 -march=armv8-a -fno-common -ftree-vectorize -funswitch-loops" -DCMAKE_CXX_FLAGS="-fomit-frame-pointer -ffast-math -mtune=cortex-a53 -mcpu=cortex-a53 -march=armv8-a -fno-common -ftree-vectorize -funswitch-loops" && cmake --build build-tg5040 --target mgba_libretro --

# libretro-fceumm's Makefile.libretro drifted in 8ed0cb2 (2026-07-23, "Replace bundled
# zlib with libretro-common's clean-room DEFLATE codec"). patches/fceumm.patch is
# rewritten for the post-drift file, so this pin has to sit after it, not before.
fceumm_HASH = b5e3566515c27dc66c9c20572171673126532e06

# Every core below is pinned to the upstream commit it was verified against, so a
# build is reproducible and a third-party repo cannot break CI without a commit
# here. See #16. Verified by fetching each commit and running `git apply --check`
# with both the tg5040 and tg5050 patches. Bump deliberately, ideally per release.
a2600_HASH = 4a7da82595d27b8df7af1ecb467a64b642a41bc9
a5200_HASH = 40c6f2f1ad4a3145b328d5baaf010fae6c7e752b
a7800_HASH = 363b6dfbd3e240762e022c2b4897b4fe55722be3
bluemsx_HASH = 0f32f52c48d3e772bfdf0379756f81f00b4e08bc
c128_HASH = c8c242db75a559246d6d51017e6dd4ecd75d6a9f
c64_HASH = c8c242db75a559246d6d51017e6dd4ecd75d6a9f
fake-08_HASH = 814991a2571ad3970e386cef48f3b148aa1c27b9
gambatte_HASH = 96174369b3c30d9fc57c926fa3379c273dc6a9a5
gearcoleco_HASH = 8916216764ca8a7965be5f4b4d093a437a1749a6
gpsp_HASH = 5b6e751f4abf368509146cd143c949c1946ac1ae
handy_HASH = bc55d462f0b2d6b073ea93dc552ebd73cec60fd1
libretro-cap32_HASH = 4abfb8be233bec630f369379fb6c1d92d31f1c7d
# NOTE: this core tracks the 2.6.1 branch (see libretro-uae_BRANCH below), not the
# default branch. 2.6.1 builds TARGET_NAME puae2021, master builds puae, so a pin
# off master silently produces a .so this makefile cannot find. Keep this SHA on
# the 2.6.1 branch.
libretro-uae_HASH = 6636d5f3fd4417f4e57ab9144fdba35b77fa3a40
mednafen_pce_fast_HASH = b211204c7026dff6e86e79b00185512e2421fff8
mednafen_supafaust_HASH = d6187e5337e6c2646d003db3ab1936727ca75301
mednafen_vb_HASH = 3f53a40bf8aa18777514fd4b220960427e312a3f
pcsx_rearmed_HASH = da2cb8ecd17fd0932ab6d94774c0522beebce6e3
pet_HASH = c8c242db75a559246d6d51017e6dd4ecd75d6a9f
plus4_HASH = c8c242db75a559246d6d51017e6dd4ecd75d6a9f
prboom_HASH = c180d47a5f9f1b5f74be18bf74deb5eccf97057e
race_HASH = c7810dd7f172827bfa2004813bc000b13786636b
snes9x_HASH = 97c65a34a2eb8592de6c7b44a0ad681895684a41
snes9x2005_plus_HASH = deb49d80d1836e3e737480a326e31a54c46c04ae
vic_HASH = c8c242db75a559246d6d51017e6dd4ecd75d6a9f

mednafen_pce_fast_REPO = https://github.com/libretro/beetle-pce-fast-libretro
mednafen_vb_REPO = https://github.com/libretro/beetle-vb-libretro

fake-08_REPO = https://github.com/jtothebell/fake-08
fake-08_CORE = fake08_libretro.so
fake-08_BUILD_PATH = fake-08/platform/libretro

fceumm_REPO = https://github.com/libretro/libretro-fceumm

gambatte_REPO = https://github.com/libretro/gambatte-libretro

mednafen_supafaust_REPO = https://github.com/libretro/supafaust

pcsx_rearmed_MAKEFILE = Makefile.libretro
# pcsx has a disabled submodule for gnulib, which takes forever to check out and is unused - skil checkout
pcsx_rearmed_SUBMODULES = frontend/libpicofe

picodrive_REPO = https://github.com/irixxxx/picodrive
picodrive_MAKEFILE = Makefile.libretro
# No background Audio on Sega CD
# https://github.com/LoveRetro/NextUI/issues/68
picodrive_HASH = b0be121b7d58d6ee1ee2809974e62893c80a8264

pokemini_REPO = https://github.com/libretro/PokeMini
pokemini_MAKEFILE = Makefile.libretro
# Our patchfile doesn't work on latest Pokemon mini commits, pinning to last working hash for now
pokemini_HASH = 78656d4615691c393e65e48672ce5fff5b1f97d3

prboom_REPO = https://github.com/libretro/libretro-prboom

snes9x2005_plus_REPO = https://github.com/libretro/snes9x2005
snes9x2005_plus_FLAGS = USE_BLARGG_APU=1

snes9x_REPO = https://github.com/libretro/snes9x
snes9x_CORE = snes9x_libretro.so
snes9x_BUILD_PATH = snes9x/libretro

libretro-uae_REPO = https://github.com/libretro/libretro-uae
libretro-uae_BRANCH = 2.6.1
libretro-uae_CORE = puae2021_libretro.so

libretro-cap32_REPO = https://github.com/libretro/libretro-cap32
libretro-cap32_CORE = cap32_libretro.so

c64_REPO  = https://github.com/libretro/vice-libretro
c64_CORE  = vice_x64_libretro.so
c64_FLAGS = EMUTYPE=x64

c128_REPO  = https://github.com/libretro/vice-libretro
c128_CORE  = vice_x128_libretro.so
c128_FLAGS = EMUTYPE=x128

vic_REPO  = https://github.com/libretro/vice-libretro
vic_CORE  = vice_xvic_libretro.so
vic_FLAGS = EMUTYPE=xvic

pet_REPO  = https://github.com/libretro/vice-libretro
pet_CORE  = vice_xpet_libretro.so
pet_FLAGS = EMUTYPE=xpet

plus4_REPO  = https://github.com/libretro/vice-libretro
plus4_CORE  = vice_xplus4_libretro.so
plus4_FLAGS = EMUTYPE=xplus4

bluemsx_REPO = https://github.com/libretro/blueMSX-libretro
bluemsx_CORE = bluemsx_libretro.so
bluemsx_MAKEFILE = Makefile.libretro

gearcoleco_REPO = https://github.com/drhelius/Gearcoleco.git
gearcoleco_CORE = gearcoleco_libretro.so
gearcoleco_BUILD_PATH = gearcoleco/platforms/libretro

###############################

include ../../all/cores/makefile
