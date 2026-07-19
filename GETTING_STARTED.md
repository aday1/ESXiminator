# ESXiminator — Getting Started (the 5-minute version)

Control a Korg Electribe ESX-1 with a broken front panel, entirely from your PC.

## 1. Plug in
USB-MIDI interface → ESX-1 MIDI IN (and MIDI OUT → interface, if you want sync feedback).
Power on the ESX.

## 2. Install
**Where the VST ends up (default):**
`C:\Program Files\Common Files\VST3\ESXiminator.vst3`

**Easiest — MSI setup:** download and run `ESXiminator-*-Setup.msi`.
Two folder questions (do not confuse them):
1. **Hub & standalone** → Program Files (app files only)
2. **Which VST3 folder should the plugin land in?** → your DAW's VST scan folder
   (default produces the path above)

Hub remembers the VST folder for later updates. Rescan plugins afterward.

**Zip — Hub:** unzip and run `ESXiminator-Hub.exe`.
Status shows the full VST path. **Install VST3...** asks which VST3 folder;
a dialog confirms the exact location. Then **Launch ESXiminator**.

**Standalone only:** double-click `ESXiminator.exe`.

**Classic bat setup:** `Install-VST.bat` (Admin) → same Common Files path as above, or
`Install-VST-User.bat` for a per-user folder.

**Manual VST install:** copy the `ESXiminator.vst3` *folder* to
`C:\Program Files\Common Files\VST3\`, then rescan.

## 3. Point it at the ESX
Open the **SETUP** tab:
- **MIDI OUTPUT** → pick your USB-MIDI interface
- Leave channels at 1 / 2 / 10 (factory ESX defaults)
- Press **SYNC ALL PARAMETERS TO ESX**

## 4. Tweak
**PARTS** tab → click a part (DRUM 1, KEYS 1, …) → turn knobs. The hardware follows instantly.
**TRIG** button plays the selected part. **M** mutes it.

## 5. Make a beat
**SEQUENCER** tab → click cells in the grid → press **PLAY (INT)** (or hit play in your DAW).
The ESX is tempo-synced automatically via MIDI clock.

That's it.

---

### Extras when you want them
- **Right-click any knob** → MIDI learn → twist a knob on your controller.
- **MIDI INPUT** (SETUP): plug in a keyboard and play the ESX's Keys parts live.
- **IMPORT .XM** (SEQUENCER): load a tracker module into the step grid.
- **FX / GLOBAL** tab: all 3 FX slots, chain, accent, swing, roll.
- Every knob is DAW-automatable.

### One-time ESX settings (needs the panel once — see README)
MIDI FILTER P/C/E/N all "o" · CLOCK = AUTO or EXT · CC assign = factory defaults.
Factory-default machines are already set correctly.

### If the app won't start
Run `ESXiminator-debug.exe` from a Command Prompt and send us the output.
