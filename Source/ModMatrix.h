// ESXiminator — modulator (automation-effect) definitions
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace esx
{
static constexpr int numModSlots = 8;

enum ModWave { mwSine = 0, mwTri, mwSawUp, mwSawDn, mwSquare, mwSampleHold, mwSmoothRand, NumModWaves };
static const char* const modWaves[NumModWaves] = { "SINE", "TRI", "SAW UP", "SAW DN", "SQUARE", "S&H", "SMOOTH RND" };

// tempo-synced cycle lengths, in quarter-note beats
struct DivDef { const char* name; double beats; };
static const DivDef modDivs[] = {
    { "8 BARS", 32.0 }, { "4 BARS", 16.0 }, { "2 BARS", 8.0 }, { "1 BAR", 4.0 },
    { "1/2", 2.0 }, { "1/2T", 4.0 / 3.0 }, { "1/4", 1.0 }, { "1/4T", 2.0 / 3.0 },
    { "1/8", 0.5 }, { "1/8T", 1.0 / 3.0 }, { "1/16", 0.25 }, { "1/16T", 1.0 / 6.0 }, { "1/32", 0.125 }
};
static constexpr int numModDivs = 13;
static constexpr int divDefault = 6;   // 1/4

inline juce::String modID (int slot, const char* field)
{
    return "mod" + juce::String (slot + 1) + "_" + field;
}

// phase (0-1) -> unipolar 0-1. shReg/prevReg hold per-slot random state across calls.
inline float modWaveValue (int wave, double phase, bool wrapped,
                           float& shReg, float& prevReg, juce::Random& rng)
{
    switch (wave)
    {
        case mwSine:   return 0.5f + 0.5f * (float) std::sin (phase * juce::MathConstants<double>::twoPi);
        case mwTri:    return (float) (phase < 0.5 ? phase * 2.0 : 2.0 - phase * 2.0);
        case mwSawUp:  return (float) phase;
        case mwSawDn:  return (float) (1.0 - phase);
        case mwSquare: return phase < 0.5 ? 1.0f : 0.0f;
        case mwSampleHold:
            if (wrapped) shReg = rng.nextFloat();
            return shReg;
        case mwSmoothRand:
            if (wrapped) { prevReg = shReg; shReg = rng.nextFloat(); }
            return prevReg + (shReg - prevReg) * (float) phase;
        default: return 0.5f;
    }
}
} // namespace esx
