// ESXiminator — Korg Electribe ESX-1 definitions
// Derived from Korg "ELECTRIBE SX MIDI IMPLEMENTATION" v1.1 (see ESX1_MIDI_SPEC.md)
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace esx
{
// ---------- Parts ----------
enum PartIndex
{
    Drum1 = 0, Drum2, Drum3, Drum4, Drum5, Drum6A, Drum6B, Drum7A, Drum7B,
    Stretch1, Stretch2, Slice, AudioIn, Keyboard1, Keyboard2,
    NumParts
};

struct PartDef
{
    const char* id;        // parameter-id prefix
    const char* name;      // display name
    int nrpnBase;          // 14-bit NRPN base (msb<<7|lsb), -1 = CC-driven (keyboard parts)
    int defaultNote;       // trigger note (drum-type parts), -1 = chromatic keyboard part
    bool hasPitch, hasStart, hasReverse, hasSample, hasSliceNo, hasGlide;
};

// NRPN bases: nm<<7 | nl
static constexpr PartDef parts[NumParts] = {
    { "d1",  "DRUM 1",    (0x0C<<7)|0x00, 36, true,  true,  true,  true,  true,  false },
    { "d2",  "DRUM 2",    (0x0C<<7)|0x20, 38, true,  true,  true,  true,  true,  false },
    { "d3",  "DRUM 3",    (0x0C<<7)|0x40, 40, true,  true,  true,  true,  true,  false },
    { "d4",  "DRUM 4",    (0x0C<<7)|0x60, 41, true,  true,  true,  true,  true,  false },
    { "d5",  "DRUM 5",    (0x0D<<7)|0x00, 43, true,  true,  true,  true,  true,  false },
    { "d6a", "DRUM 6A",   (0x0D<<7)|0x20, 42, true,  true,  true,  true,  true,  false },
    { "d6b", "DRUM 6B",   (0x0D<<7)|0x40, 46, true,  true,  true,  true,  true,  false },
    { "d7a", "DRUM 7A",   (0x0D<<7)|0x60, 49, true,  true,  true,  true,  true,  false },
    { "d7b", "DRUM 7B",   (0x0E<<7)|0x00, 51, true,  true,  true,  true,  true,  false },
    { "st1", "STRETCH 1", (0x0E<<7)|0x60,  9, true,  true,  true,  true,  false, false },
    { "st2", "STRETCH 2", (0x0F<<7)|0x00, 10, true,  true,  true,  true,  false, false },
    { "sl",  "SLICE",     (0x0F<<7)|0x20, 11, true,  true,  true,  true,  false, false },
    { "ai",  "AUDIO IN",  (0x0F<<7)|0x40, 12, false, false, false, false, false, false },
    { "k1",  "KEYS 1",    (0x0E<<7)|0x20, -1, false, true,  true,  true,  true,  true  },
    { "k2",  "KEYS 2",    (0x0E<<7)|0x20, -1, false, true,  true,  true,  true,  true  },
};

// ---------- Per-part parameter slots ----------
enum ParamSlot
{
    pSample = 0, pSliceNo, pGlide, pPitch, pFilterType, pCutoff, pReso, pEGInt,
    pStart, pLevel, pPan, pEGTime, pAmpEG, pRoll, pReverse, pFxSend, pFxSelect,
    pModType, pModDepth, pModSpeed, pModDest, pModBpmSync, pMotionSeq,
    NumSlots
};

struct SlotDef
{
    const char* id; const char* name;
    int nrpnOffset;   // offset from part base, -1 = n/a via NRPN
    int cc;           // default panel CC (keyboard parts / fallback), -1 = none
    int type;         // 0=cont 0-127, 1=bool, 2=choice, 3=14bit, 4=centered 0-127 (64=0)
    const char* const* choices; int numChoices;
};

static const char* const filterTypes[] = { "LPF", "HPF", "BPF", "BPF+" };
static const char* const fxSelects[]   = { "FX1", "FX2", "FX3" };
static const char* const modTypes[]    = { "SAW", "SQU", "TRI", "S&H", "EG" };
static const char* const modDests[]    = { "PITCH", "CUTOFF", "AMP", "PAN" };
static const char* const modDestsAI[]  = { "CUTOFF", "AMP", "PAN" };
static const char* const mseqTypes[]   = { "OFF", "SMOOTH", "TRIG HOLD" };
static const char* const fxTypes[] = {
    "REVERB", "BPM SYNC DELAY", "SHORT DELAY", "MOD DELAY", "GRAIN SHIFTER",
    "CHO/FLG", "PHASER", "RING MOD", "TALKING MOD", "PITCH SHIFTER",
    "COMPRESSOR", "DISTORTION", "DECIMATOR", "EQ", "LPF", "HPF" };
static const char* const fxChains[]  = { "NONE", "FX1>FX2", "FX2>FX3", "FX1>FX2>FX3" };
static const char* const rollTypes[] = { "2", "3", "4" };

static constexpr int NoChoices = 0;
static const SlotDef slots[NumSlots] = {
    { "sample",  "SAMPLE",      0x00, -1, 3, nullptr, 0 },
    { "sliceno", "SLICE NO",    0x02, -1, 3, nullptr, 0 },
    { "glide",   "GLIDE",       -1,    5, 0, nullptr, 0 },
    { "pitch",   "PITCH",       0x04, -1, 4, nullptr, 0 },
    { "ftype",   "FILT TYPE",   0x05, 83, 2, filterTypes, 4 },
    { "cutoff",  "CUTOFF",      0x06, 74, 0, nullptr, 0 },
    { "reso",    "RESONANCE",   0x07, 71, 0, nullptr, 0 },
    { "egint",   "EG INT",      0x08, 79, 4, nullptr, 0 },
    { "start",   "START PT",    0x09, 18, 0, nullptr, 0 },
    { "level",   "LEVEL",       0x0A,  7, 0, nullptr, 0 },
    { "pan",     "PAN",         0x0B, 10, 4, nullptr, 0 },
    { "egtime",  "EG TIME",     0x0C, 75, 0, nullptr, 0 },
    { "ampeg",   "AMP EG",      0x0D, 86, 1, nullptr, 0 },
    { "roll",    "ROLL",        0x0E, 85, 1, nullptr, 0 },
    { "reverse", "REVERSE",     0x0F, 19, 1, nullptr, 0 },
    { "fxsend",  "FX SEND",     0x10, 91, 1, nullptr, 0 },
    { "fxsel",   "FX SELECT",   0x11, 81, 2, fxSelects, 3 },
    { "modtype", "MOD TYPE",    0x12, 87, 2, modTypes, 5 },
    { "moddepth","MOD DEPTH",   0x13, 90, 4, nullptr, 0 },
    { "modspeed","MOD SPEED",   0x14, 89, 0, nullptr, 0 },
    { "moddest", "MOD DEST",    0x15, 88, 2, modDests, 4 },
    { "modsync", "MOD BPM SYNC",0x16, 82, 1, nullptr, 0 },
    { "mseq",    "MOTION SEQ",  0x17, 80, 2, mseqTypes, 3 },
};

// does this part have this slot?
inline bool partHasSlot (int part, int slot)
{
    const auto& p = parts[part];
    switch (slot)
    {
        case pSample:  return p.hasSample;
        case pSliceNo: return p.hasSliceNo;
        case pGlide:   return p.hasGlide;
        case pPitch:   return p.hasPitch;
        case pStart:   return p.hasStart;
        case pReverse: return p.hasReverse;
        default:       return true;
    }
}

inline juce::String paramID (int part, int slot)
{
    return juce::String (parts[part].id) + "_" + slots[slot].id;
}

// map choice index -> transmitted MIDI value (mid of documented range)
inline int choiceToMidi (int slot, int idx, int part)
{
    if (slot == pFilterType) return idx * 32 + 8;                 // 0/32/64/96
    if (slot == pFxSelect)   return idx == 0 ? 0 : (idx == 1 ? 43 : 86);
    if (slot == pModType)    return idx < 4 ? idx * 16 + 4 : 64;  // saw/squ/tri/s&h/eg
    if (slot == pModDest)
    {
        if (part == AudioIn)  return idx == 0 ? 0 : (idx == 1 ? 64 : 96); // cutoff/amp/pan
        return idx * 32 + 8;                                              // pitch/cutoff/amp/pan
    }
    if (slot == pMotionSeq)  return idx == 0 ? 0 : (idx == 1 ? 43 : 86);
    return idx;
}

// FX section (global channel, CC-driven)
struct FxCC { int type, edit1, edit2, mseq; };
static constexpr FxCC fxCC[3] = { { 12, 92, 93, 20 }, { 13, 94, 95, 21 }, { 24, 25, 26, 22 } };
static constexpr int ccFxChain = 23;
inline int fxTypeToMidi (int idx)  { return idx * 8 + 2; }   // 16 types, blocks of 8
inline int fxChainToMidi (int idx) { return idx * 32 + 8; }  // 4 options, blocks of 32

// Global NRPNs
static constexpr int nrpnAccentLevel = (0x0F<<7)|0x60;
static constexpr int nrpnAccentMSeq  = (0x0F<<7)|0x61;
static constexpr int nrpnSwing       = (0x0F<<7)|0x70;
static constexpr int nrpnRollType    = (0x0F<<7)|0x71;
static constexpr int nrpnMute1       = (0x0F<<7)|0x76;  // kbd/stretch/slice/audioin + solo status
static constexpr int nrpnMute2       = (0x0F<<7)|0x77;  // drum parts

// Mute bit layout (returns {nrpn, msbBit, lsbBit}; bit<0 = unused)
struct MuteBit { int nrpn, msbBit, lsbBit; };
inline MuteBit muteBitFor (int part)
{
    switch (part)
    {
        case Drum1:   return { nrpnMute2, -1, 0 };
        case Drum2:   return { nrpnMute2, -1, 1 };
        case Drum3:   return { nrpnMute2, -1, 2 };
        case Drum4:   return { nrpnMute2, -1, 3 };
        case Drum5:   return { nrpnMute2, -1, 4 };
        case Drum6A:  return { nrpnMute2, -1, 5 };
        case Drum6B:  return { nrpnMute2, -1, 6 };
        case Drum7A:  return { nrpnMute2,  0, -1 };
        case Drum7B:  return { nrpnMute2,  1, -1 };
        case Keyboard1: return { nrpnMute1, -1, 0 };
        case Keyboard2: return { nrpnMute1, -1, 1 };
        case Stretch1:  return { nrpnMute1, -1, 2 };
        case Stretch2:  return { nrpnMute1, -1, 3 };
        case Slice:     return { nrpnMute1, -1, 4 };
        case AudioIn:   return { nrpnMute1, -1, 5 };
        default:        return { 0, -1, -1 };
    }
}

// swing % (50-75) -> MIDI value 0-127
inline int swingToMidi (int pct) { return juce::jlimit (0, 127, juce::roundToInt ((pct - 50) * 127.0 / 25.0)); }
} // namespace esx
