# ESXiminator

A Windows VST3 that turns a DAW into the front panel of a **Korg Electribe ESX-1** —
built for units with a broken front panel, driven entirely over USB-MIDI.

## Install
The VST ends up at:
`C:\Program Files\Common Files\VST3\ESXiminator.vst3`
(unless you pick a different VST folder on the MSI's **second** folder page).

**Easiest — MSI:** download `ESXiminator-v*-Setup.msi` and run it. Two separate folder pages:
1. **Hub & standalone** — Program Files (not the VST)
2. **Which VST3 folder should the plugin land in?** — your DAW's VST3 scan path
   (default above). Hub remembers that VST path.

Rescan plugins afterward.

**Zip / Hub:** unzip the win64 package, then open **ESXiminator-Hub.exe**:
- **Launch** the standalone app
- **Install VST3...** — asks which VST3 folder; status always shows the full path
- **Check / install updates** from GitHub (refreshes that same VST path)

Or manually: run `ESXiminator.exe`, or copy `ESXiminator.vst3` into `C:\Program Files\Common Files\VST3\`.

Showcase: https://aday1.github.io/ESXiminator/

### In-plugin updates (opt-in)
SETUP tab → enable **CHECK GITHUB FOR UPDATES**. Off by default. When enabled, the plugin
quietly checks GitHub Releases after opening the editor.

## Features
- **Full parameter control** of every ESX-1 part: Drum 1–7B, Stretch 1/2, Slice, Audio In,
  Keys 1/2 — sample select, slice, pitch/glide, filter (type/cutoff/res/EG), level, pan,
  EG time, amp EG, roll, reverse, FX send/select, modulation (type/depth/speed/dest/BPM sync),
  motion-seq mode. Drum-type parts use per-part NRPN, keyboard parts use panel CCs — exactly
  per Korg's MIDI implementation (see `ESX1_MIDI_SPEC.md`).
- **FX section**: all 3 FX slots (16 effect types), edit 1/2, motion seq, FX chain.
- **Global**: accent level/motion-seq, swing (50–75 %), roll type, per-part **mutes**.
- **Every control is a VST parameter** → fully automatable from the DAW.
- **Step sequencer**: 16 steps × 13 lanes, follows host transport & tempo, per-pattern velocity.
- **Tempo sync**: the plugin generates MIDI clock + Start/Stop on its own output, so the ESX
  follows your DAW tempo even though the MIDI goes out of the plugin directly.
- **XM import**: load a FastTracker II `.xm` module (e.g. from the Adlibtium pipeline); the first
  16 rows of its first pattern are mapped onto the step grid (instrument № → lane).
- **Pattern select**: bank A–D + number, sent as Bank Select + Program Change.
- **Direct MIDI output**: the plugin opens your USB-MIDI interface itself, bypassing hosts
  (like Bitwig) that filter plugin MIDI. It also mirrors everything to the host MIDI output,
  so in Reaper you can instead route the track's MIDI to hardware and set the device to
  "(host output only)".

## ESX-1 setup (Global mode — via MIDI dump or as last set before the panel died)
- MIDI FILTER: P, C, E, N all enabled ("o")
- CLOCK: EXT (or AUTO)
- CC Assign map: factory defaults
- Channels must match the SETUP tab (defaults: Keys1/Global = 1, Keys2 = 2, Drum parts = 10)

## Workflow tips
- Click **SYNC ALL PARAMETERS TO ESX** after powering the ESX or loading a project so the
  hardware matches the plugin state.
- The drum channel carries all drum-type parts; NRPN addresses each part individually,
  so no "part select" is ever needed.
- Keys 1/2 knob parameters ride the panel CCs on their own channels (a hardware limitation:
  Korg exposes keyboard-part editing only that way).

## Building from source
Cross-compiled from Linux with Zig as the C/C++ compiler:

```
pip install ziglang cmake ninja
git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git
cmake -B build -G Ninja -DJUCE_DIR=./JUCE \
      -DCMAKE_TOOLCHAIN_FILE=toolchain-zig-mingw64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Roadmap
- OPL/AdLib file ingestion (Adlibtium formats) → step patterns
- Motion-sequence recording from DAW automation
- Pattern dump (SysEx) read/write for offline pattern editing
