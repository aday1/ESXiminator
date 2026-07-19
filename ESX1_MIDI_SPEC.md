# Korg Electribe ESX-1 — Real-time MIDI Control Spec

Distilled from the official **ELECTRIBE SX MIDI IMPLEMENTATION v1.1** (Korg, Sep 2003).
This is the ground truth used by the ESXiminator plugin.

## Channels (defaults, configurable in ESX Global mode)
| Part | Default MIDI Ch |
|---|---|
| Keyboard 1 (= Global ch) | 1 |
| Keyboard 2 | 2 |
| Drum / Stretch / Slice / AudioIn | 10 |

## Note messages
Drum-type parts trigger on the Drum channel at these default notes:
Drum1 C2(36), Drum2 D2(38), Drum3 E2(40), Drum4 F2(41), Drum5 G2(43),
Drum6A F#2(42), Drum6B A#2(46), Drum7A C#3(49), Drum7B D#3(51),
Stretch1 A-1(9), Stretch2 A#-1(10), Slice B-1(11), AudioIn C0(12).
Keyboard 1/2 play chromatically on their own channels.

## Pattern select (Program MIDI Filter must be "o")
Bank Select MSB(CC0)=0, LSB(CC32)=0 → banks A/B, LSB=1 → C/D; then Program Change:
0–63 = A01–A64 (or C), 64–127 = B01–B64 (or D).

## Transport
MIDI Clock (F8), Start (FA), Continue (FB), Stop (FC), Song Position Pointer, Song Select (0–63).

## NRPN parameter control (Control MIDI Filter must be "o")
Send on the part's channel: CC99 (NRPN MSB) = nm, CC98 (NRPN LSB) = nl,
CC6 (Data Entry MSB) = value; add CC38 (LSB) for 14-bit params
(value = MSB×128+LSB; e.g. Sample OFF = 16383).

### Part blocks (Drum ch, except Keyboard sample on kbd ch)
Base (nm nl): Drum1 0C00, Drum2 0C20, Drum3 0C40, Drum4 0C60, Drum5 0D00,
Drum6A 0D20, Drum6B 0D40, Drum7A 0D60, Drum7B 0E00, Keyboard 0E20 (sample/slice only),
Stretch1 0E60, Stretch2 0F00, Slice 0F20, AudioIn 0F40.

Offsets within block:
| +off | Parameter | Value |
|---|---|---|
| 00 | Sample (14-bit) | 16383 = OFF |
| 02 | Slice No. (14-bit) | 16383 = ALL |
| 04 | Pitch | 0–127 (64 = equal) — not AudioIn |
| 05 | Filter Type | 0–31 LPF / 32–63 HPF / 64–95 BPF / 96–127 BPF+ |
| 06 | Filter Cutoff | 0–127 |
| 07 | Filter Resonance | 0–127 |
| 08 | Filter EG Int | 0–127 (64 = 0) |
| 09 | Start Point | 0–127 — not AudioIn |
| 0A | Level | 0–127 |
| 0B | Pan | 0–127 (64 = C) |
| 0C | EG Time | 0–127 |
| 0D | Amp EG | ≥64 = On |
| 0E | Roll | ≥64 = On |
| 0F | Reverse | ≥64 = On — not AudioIn |
| 10 | Effect Send | ≥64 = On |
| 11 | Effect Select | 0–42 FX1 / 43–85 FX2 / 86–127 FX3 |
| 12 | Modulation Type | 0–15 Saw / 16–31 Squ / 32–47 Tri / 48–63 S&H / 64–127 EG |
| 13 | Mod Depth | 0–127 (64 = 0) |
| 14 | Mod Speed | 0–127 |
| 15 | Mod Destination | 0–31 Pitch / 32–63 Cutoff / 64–95 Amp / 96–127 Pan (AudioIn: 0–63 Cutoff / 64–95 Amp / 96–127 Pan) |
| 16 | Mod BPM Sync | ≥64 = On |
| 17 | Motion Seq Type | 0–42 Off / 43–85 Smooth / 86–127 TrigHold |

Stretch2/Slice have no Slice No. at +02 (Stretch2 sample at 0F00, next at +04).
AudioIn has no Sample/Slice/Pitch/Start/Reverse entries.

### Global NRPNs (Global = Keyboard1 channel)
| nm nl | Parameter | Value |
|---|---|---|
| 0F 60 | Accent Level | 0–127 |
| 0F 61 | Accent Motion Seq | 0–42 Off / 43–127 TrigHold |
| 0F 70 | Swing | 0–127 → 50–75% (approx value = round((swing−50)×127/25)) |
| 0F 71 | Roll Type | 0–42 "2" / 43–85 "3" / 86–127 "4" |
| 0F 76 | Mute 1 (14-bit) | MSB bit0 = Solo status; LSB bits: 0 Kbd1, 1 Kbd2, 2 Stretch1, 3 Stretch2, 4 Slice, 5 AudioIn (1 = mute) |
| 0F 77 | Mute 2 (14-bit) | MSB bits1–0 = Drum7B,7A; LSB bits6–0 = Drum6B,6A,5,4,3,2,1 (1 = mute) |

## Panel-control CCs (assignable; defaults below; Control MIDI Filter "o")
Sent on a part's channel these act like turning that knob for that part
(Keyboard1/2 edit params are ONLY reachable this way; on the Drum channel they affect
the drum-type part currently selected on the ESX).

Part section: Glide 5, Level 7, Pan 10, FX1 Type 12, FX2 Type 13, Start Point 18,
Reverse 19, FX1 MSeq 20, FX2 MSeq 21, FX3 MSeq 22, FX Chain 23, FX3 Type 24,
FX3 Edit1 25, FX3 Edit2 26, Filter Resonance 71, Filter Cutoff 74, EG Time 75,
Filter EG Int 79, Part Motion Seq 80, FX Select 81, Mod BPM Sync 82, Filter Type 83,
Roll 85, Amp EG 86, Mod Type 87, Mod Dest 88, Mod Speed 89, Mod Depth 90,
FX Send 91, FX1 Edit1 92, FX1 Edit2 93, FX2 Edit1 94, FX2 Edit2 95.

FX Type values (all three FX slots): 0–7 Reverb, 8–15 BPM Sync Delay, 16–23 Short Delay,
24–31 Mod Delay, 32–39 Grain Shifter, 40–47 Cho/Flg, 48–55 Phaser, 56–63 Ring Mod,
64–71 Talking Mod, 72–79 Pitch Shifter, 80–87 Compressor, 88–95 Distortion,
96–103 Decimator, 104–111 EQ, 112–119 LPF, 120–127 HPF.
FX Chain: 0–31 none, 32–63 FX1→FX2, 64–95 FX2→FX3, 96–127 FX1→FX2→FX3.

## ESX-1 Global-mode settings required
- MIDI Filter: P (program), C (control), E (exclusive), N (note) all "o" (enabled).
- Clock: EXT or AUTO to slave tempo to the DAW.
- Keep the Control Change Assign map at factory defaults (the plugin uses them).

Sources: Korg "ELECTRIBE SX MIDI IMPLEMENTATION" v1.1 (korg-datastorage.jp, mirrored at
yumpu.com/en/document/view/32567913), ESX-1 Owner's Manual (korg.com).
