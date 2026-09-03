# Data Defense — Nintendo Switch port (Unity 6000.3.9f1 / IL2CPP wrapper)
 
This is a native wrapper / loader that runs the original ARM64 Android build of
**Data Defense** on Switch homebrew. It contains no game code and no game assets
— it loads the game's own libraries and recreates, natively, the thin
Android/JNI layer the Unity engine expects.
 
## Install & run
 
You need files from **Data Defense 1.3.11** (`com.IIBlocks.DataDefense`).
 
Put the `.nro` in any folder under `sdmc:/switch/` and place your game files
next to it — the loader finds its folder at runtime, so the name is up to you:
 
```
sdmc:/switch/datadefense_nx
├── datadefense_nx.nro
├── libmain.so  libunity.so  libil2cpp.so   <- from your APK: lib/arm64-v8a/
├── cursor.png                              <- optional
└── assets/                                 <- the APK's assets/ folder, whole
```
 
Launch via title override (hold **R** while starting an installed game) or a
forwarder.
 
Optionally drop a `cursor.png` (up to 64×64, transparency respected, top-left
pixel is the hotspot) in the same folder to replace the on-screen cursor with
your own.
 
## Controls
 
| Input | Action |
|---|---|
| **+** | Toggle the on-screen cursor |
| **–** | Toggle gyro pointing (tilt/turn the controller to aim) |
| **Left stick** | Move the cursor |
| **L / R** | Recenter the cursor to the middle of the screen (helps gyro aiming) |
| **A / ZR / ZL** | Tap / confirm (ZL and ZR let you play one-handed) |
| **B** | Android Back |
| **D-pad up / down** | Adjust sensitivity of whatever is driving the cursor |
 
 Your stick, mouse and gyro sensitivities
are remembered in `pointer.cfg` automatically after in-game adjustment.
 
## Settings
 
`config.txt` is written next to the `.nro` on first launch, with the options
documented inline:
 
```
handheld_res 720     # 720 or 1080
docked_res   1080    # 720 or 1080
```
 
## Building
 
Requires devkitPro with the `switch-dev` group plus these portlibs:
 
```
pacman -S switch-dev
pacman -S switch-mesa switch-libdrm_nouveau switch-sdl2 switch-libpng switch-zlib
 
export DEVKITPRO=/opt/devkitpro
make                        # -> datadefense_nx.nro
```
 
`make check` runs the static checks and a cross-compile syntax pass before you
spend a build.
 
## Credits
 
The loader/shim infrastructure (`so_util`, `libc_shim`, `jni_fake`, `unity_jni`,
`opensles`, `nx_pointer`, diagnostics) derives from the open-source Switch
`.so`-loader lineage — Andy Nguyen, fgsfds and ChanseyIsTheBest, building on
TheOfficialFloW's Vita/Switch loader tradition — reaching this project via the
Zookeeper DX, PvZ Fusion and Killer Bean Unleashed ports, with the Clay Jam and
Hitman Sniper ports as references for Unity 6. All MIT-licensed. Thanks to
everyone in that lineage for making this approach possible.
 
