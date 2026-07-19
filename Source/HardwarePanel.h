// ESXiminator - ESX-1-style front panel layout.
// Only exposes what the ESX-1 can actually be driven with over MIDI:
// the +00..+17 part NRPN block, the global NRPNs, FX CC, pattern select and transport.
#pragma once
#include "PluginProcessor.h"

namespace esx
{
// Panel groups mirror the hardware's silkscreen sections.
struct EditGroup { const char* title; std::vector<int> slots; };

inline const std::vector<EditGroup>& editGroups()
{
    static const std::vector<EditGroup> g = {
        { "SAMPLE",             { pSample, pSliceNo, pStart, pReverse, pGlide } },
        { "PITCH / MODULATION", { pPitch, pModType, pModDepth, pModSpeed, pModDest, pModBpmSync } },
        { "FILTER",             { pFilterType, pCutoff, pReso, pEGInt, pEGTime } },
        { "AMP",                { pLevel, pPan, pAmpEG, pRoll } },
        { "FX / MOTION SEQ",    { pFxSend, pFxSelect, pMotionSeq } },
    };
    return g;
}
} // namespace esx
