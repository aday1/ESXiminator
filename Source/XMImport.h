// Minimal FastTracker II .XM pattern importer for ESXiminator.
// Reads pattern 0 in the song order, extracts note-on events from the first 16 rows
// and maps them onto the 13 ESX drum-type lanes (instrument number round-robins lanes).
#pragma once
#include <juce_core/juce_core.h>
#include <atomic>

namespace esx
{
inline bool importXMFile (const juce::File& file, std::atomic<uint32_t>* lanes,
                          int numLanes, int numSteps, juce::String& report)
{
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb)) { report = "Cannot read file"; return false; }
    auto* d = static_cast<const uint8_t*> (mb.getData());
    size_t n = mb.getSize();
    if (n < 80 || memcmp (d, "Extended Module: ", 17) != 0) { report = "Not an XM file"; return false; }

    auto u16 = [&] (size_t o) { return (uint32_t) d[o] | ((uint32_t) d[o+1] << 8); };
    auto u32 = [&] (size_t o) { return u16 (o) | (u16 (o+2) << 16); };

    size_t hdrOff = 60;
    uint32_t hdrSize    = u32 (hdrOff);
    uint32_t numCh      = u16 (hdrOff + 8);
    uint32_t numPats    = u16 (hdrOff + 10);
    if (numPats == 0 || numCh == 0) { report = "Empty module"; return false; }

    // first pattern in file order
    size_t off = hdrOff + hdrSize;
    if (off + 9 > n) { report = "Truncated header"; return false; }
    uint32_t patHdrLen = u32 (off);
    uint32_t numRows   = u16 (off + 5);
    uint32_t packedLen = u16 (off + 7);
    size_t   dataOff   = off + patHdrLen;
    if (dataOff + packedLen > n) { report = "Truncated pattern"; return false; }

    for (int l = 0; l < numLanes; ++l) lanes[l] = 0;

    size_t p = dataOff, end = dataOff + packedLen;
    int hits = 0;
    for (uint32_t row = 0; row < numRows && p < end; ++row)
    {
        for (uint32_t ch = 0; ch < numCh && p < end; ++ch)
        {
            uint8_t note = 0, inst = 0;
            uint8_t b = d[p++];
            if (b & 0x80)
            {
                if (b & 0x01) note = d[p++];
                if (b & 0x02) inst = d[p++];
                if (b & 0x04) p++;           // volume
                if (b & 0x08) p++;           // effect
                if (b & 0x10) p++;           // effect param
            }
            else { note = b; if (p < end) inst = d[p++]; if (p < end) p++; if (p < end) p++; if (p < end) p++; } // uncompressed: note,inst,vol,fx,param

            if (row < (uint32_t) numSteps && note > 0 && note < 97) // real note (97 = key off)
            {
                int lane = inst > 0 ? (inst - 1) % numLanes : (int) (ch % (uint32_t) numLanes);
                lanes[lane] |= (1u << (int) row);
                ++hits;
            }
        }
    }
    report = "Imported " + juce::String (hits) + " hits from '" + file.getFileName()
           + "' (" + juce::String (numCh) + " ch, " + juce::String (numRows) + " rows; first 16 rows used)";
    return hits > 0;
}
} // namespace esx
