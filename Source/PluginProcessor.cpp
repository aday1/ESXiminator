#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "XMImport.h"

using namespace esx;

//==============================================================================
// ---------- modulator target list (order is stable: state recall depends on it) ----------
juce::StringArray ESXiminatorProcessor::buildTargetIDs()
{
    juce::StringArray ids;
    ids.add ({});                                    // index 0 = none
    for (int part = 0; part < NumParts; ++part)
        for (int slot = 0; slot < NumSlots; ++slot)
            if (partHasSlot (part, slot))
                ids.add (paramID (part, slot));
    for (int f = 1; f <= 3; ++f)
        for (auto* s : { "_type", "_edit1", "_edit2", "_mseq" })
            ids.add ("fx" + juce::String (f) + s);
    ids.addArray ({ "fx_chain", "accent_level", "swing", "roll_type" });
    return ids;
}

juce::StringArray ESXiminatorProcessor::buildTargetNames()
{
    juce::StringArray names;
    names.add ("-- NONE --");
    for (int part = 0; part < NumParts; ++part)
        for (int slot = 0; slot < NumSlots; ++slot)
            if (partHasSlot (part, slot))
                names.add (juce::String (parts[part].name) + " " + slots[slot].name);
    for (int f = 1; f <= 3; ++f)
        for (auto* s : { "TYPE", "EDIT1", "EDIT2", "MOTION SEQ" })
            names.add ("FX" + juce::String (f) + " " + s);
    names.addArray ({ "FX CHAIN", "ACCENT LEVEL", "SWING", "ROLL TYPE" });
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout ESXiminatorProcessor::createLayout()
{
    using P  = juce::AudioParameterInt;
    using PC = juce::AudioParameterChoice;
    using PB = juce::AudioParameterBool;
    using PF = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;

    for (int part = 0; part < NumParts; ++part)
    {
        for (int slot = 0; slot < NumSlots; ++slot)
        {
            if (! partHasSlot (part, slot)) continue;
            auto id   = paramID (part, slot);
            auto name = juce::String (parts[part].name) + " " + slots[slot].name;
            const auto& s = slots[slot];

            juce::StringArray ch;
            if (s.type == 2)
            {
                auto* arr = (slot == pModDest && part == AudioIn) ? modDestsAI : s.choices;
                int   n   = (slot == pModDest && part == AudioIn) ? 3 : s.numChoices;
                for (int i = 0; i < n; ++i) ch.add (arr[i]);
            }

            switch (s.type)
            {
                case 0:  ps.push_back (std::make_unique<P> (id, name, 0, 127, slot == pLevel ? 100 : (slot==pCutoff?127:0))); break;
                case 4:  ps.push_back (std::make_unique<P> (id, name, 0, 127, 64)); break;
                case 1:  ps.push_back (std::make_unique<PB> (id, name, false)); break;
                case 2:  ps.push_back (std::make_unique<PC> (id, name, ch, 0)); break;
                case 3:  ps.push_back (std::make_unique<P> (id, name, 0, 512, 512)); break;
            }
        }
        ps.push_back (std::make_unique<PB> (juce::String (parts[part].id) + "_mute",
                                            juce::String (parts[part].name) + " MUTE", false));
    }

    for (int f = 0; f < 3; ++f)
    {
        auto p = "fx" + juce::String (f + 1);
        juce::StringArray types; for (auto* t : fxTypes) types.add (t);
        ps.push_back (std::make_unique<PC> (p + "_type",  "FX" + juce::String (f+1) + " TYPE", types, f == 0 ? 0 : (f == 1 ? 2 : 5)));
        ps.push_back (std::make_unique<P>  (p + "_edit1", "FX" + juce::String (f+1) + " EDIT1", 0, 127, 64));
        ps.push_back (std::make_unique<P>  (p + "_edit2", "FX" + juce::String (f+1) + " EDIT2", 0, 127, 64));
        ps.push_back (std::make_unique<PB> (p + "_mseq",  "FX" + juce::String (f+1) + " MOTION SEQ", false));
    }
    { juce::StringArray ch; for (auto* c : fxChains) ch.add (c);
      ps.push_back (std::make_unique<PC> ("fx_chain", "FX CHAIN", ch, 0)); }

    ps.push_back (std::make_unique<P>  ("accent_level", "ACCENT LEVEL", 0, 127, 64));
    ps.push_back (std::make_unique<PB> ("accent_mseq",  "ACCENT MOTION SEQ", false));
    ps.push_back (std::make_unique<P>  ("swing",        "SWING %", 50, 75, 50));
    { juce::StringArray ch; for (auto* c : rollTypes) ch.add (c);
      ps.push_back (std::make_unique<PC> ("roll_type", "ROLL TYPE", ch, 0)); }

    // ---------- modulators ----------
    {
        auto tNames = buildTargetNames();
        juce::StringArray waves;  for (auto* w : modWaves) waves.add (w);
        juce::StringArray divs;   for (auto& d : modDivs)  divs.add (d.name);

        for (int m = 0; m < numModSlots; ++m)
        {
            auto n = juce::String (m + 1);
            auto lbl = "MOD" + n + " ";
            ps.push_back (std::make_unique<PB> (modID (m, "on"),     lbl + "ON", false));
            ps.push_back (std::make_unique<PC> (modID (m, "target"), lbl + "TARGET", tNames, 0));
            ps.push_back (std::make_unique<PC> (modID (m, "wave"),   lbl + "WAVE", waves, mwSine));
            ps.push_back (std::make_unique<PB> (modID (m, "sync"),   lbl + "SYNC", true));
            ps.push_back (std::make_unique<PC> (modID (m, "div"),    lbl + "DIV", divs, divDefault));
            ps.push_back (std::make_unique<PF> (modID (m, "hz"),     lbl + "RATE HZ",
                                                juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.3f), 1.0f));
            ps.push_back (std::make_unique<P>  (modID (m, "depth"),  lbl + "DEPTH", 0, 100, 100));
            ps.push_back (std::make_unique<P>  (modID (m, "lo"),     lbl + "LO", 0, 127, 0));
            ps.push_back (std::make_unique<P>  (modID (m, "hi"),     lbl + "HI", 0, 127, 127));
            ps.push_back (std::make_unique<P>  (modID (m, "phase"),  lbl + "PHASE", 0, 359, 0));
        }
    }

    return { ps.begin(), ps.end() };
}

// ---------- where each param id transmits ----------
void ESXiminatorProcessor::buildSendTargets()
{
    for (int part = 0; part < NumParts; ++part)
        for (int slot = 0; slot < NumSlots; ++slot)
            if (partHasSlot (part, slot))
                sendTargets[paramID (part, slot)] = { tPartSlot, part, slot };

    for (int f = 0; f < 3; ++f)
    {
        auto p = "fx" + juce::String (f + 1);
        sendTargets[p + "_type"]  = { tFx, f, 0 };
        sendTargets[p + "_edit1"] = { tFx, f, 1 };
        sendTargets[p + "_edit2"] = { tFx, f, 2 };
        sendTargets[p + "_mseq"]  = { tFx, f, 3 };
    }
    sendTargets["fx_chain"]     = { tFxChain,     0, 0 };
    sendTargets["accent_level"] = { tAccentLevel, 0, 0 };
    sendTargets["accent_mseq"]  = { tAccentMSeq,  0, 0 };
    sendTargets["swing"]        = { tSwing,       0, 0 };
    sendTargets["roll_type"]    = { tRollType,    0, 0 };
}

juce::AudioProcessor::BusesProperties ESXiminatorProcessor::makeBuses()
{
    // standalone hosts need an audio bus to run the engine; plugin hosts treat us as a pure MIDI effect
    if (juce::PluginHostType::jucePlugInClientCurrentWrapperType == juce::AudioProcessor::wrapperType_Standalone)
        return juce::AudioProcessor::BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true);
    return {};
}

ESXiminatorProcessor::ESXiminatorProcessor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "ESXIMINATOR", createLayout())
{
    for (auto& l : seqPattern) l = 0;
    for (auto& o : pendingOffs) o = -1;

    targetNames = buildTargetNames();
    targetIDs   = buildTargetIDs();
    buildSendTargets();
    for (int m = 0; m < numModSlots; ++m) { modOut[m] = -1.0f; lastModSent[m] = -1; }

    for (int part = 0; part < NumParts; ++part)
    {
        for (int slot = 0; slot < NumSlots; ++slot)
            if (partHasSlot (part, slot))
                apvts.addParameterListener (paramID (part, slot), this);
        apvts.addParameterListener (juce::String (parts[part].id) + "_mute", this);
    }
    for (int f = 1; f <= 3; ++f)
        for (auto* s : { "_type", "_edit1", "_edit2", "_mseq" })
            apvts.addParameterListener ("fx" + juce::String (f) + s, this);
    for (auto* s : { "fx_chain", "accent_level", "accent_mseq", "swing", "roll_type" })
        apvts.addParameterListener (s, this);

    auto devs = juce::MidiOutput::getAvailableDevices();
    if (! devs.isEmpty())
        openMidiOutput (devs[0].name);
}

ESXiminatorProcessor::~ESXiminatorProcessor()
{
    if (midiIn) midiIn->stop();
    midiIn.reset();
    const juce::ScopedLock sl (midiOutLock);
    midiOut.reset();
}

//==============================================================================
juce::StringArray ESXiminatorProcessor::getMidiOutputNames() const
{
    juce::StringArray out;
    for (auto& d : juce::MidiOutput::getAvailableDevices()) out.add (d.name);
    return out;
}

void ESXiminatorProcessor::openMidiOutput (const juce::String& name)
{
    const juce::ScopedLock sl (midiOutLock);
    midiOut.reset();
    midiOutName = {};
    for (auto& d : juce::MidiOutput::getAvailableDevices())
        if (d.name == name)
        {
            midiOut = juce::MidiOutput::openDevice (d.identifier);
            if (midiOut) midiOutName = name;
            break;
        }
}

juce::StringArray ESXiminatorProcessor::getMidiInputNames() const
{
    juce::StringArray out;
    for (auto& d : juce::MidiInput::getAvailableDevices()) out.add (d.name);
    return out;
}

void ESXiminatorProcessor::openMidiInput (const juce::String& name)
{
    if (midiIn) midiIn->stop();
    midiIn.reset();
    midiInName = {};
    for (auto& d : juce::MidiInput::getAvailableDevices())
        if (d.name == name)
        {
            midiIn = juce::MidiInput::openDevice (d.identifier, this);
            if (midiIn) { midiIn->start(); midiInName = name; }
            break;
        }
}

void ESXiminatorProcessor::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m)
{
    int s1, n1, s2, n2;
    inFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) inFifoMsgs[s1] = m;
    inFifo.finishedWrite (n1);
}

void ESXiminatorProcessor::armLearn (const juce::String& paramId)
{
    const juce::ScopedLock sl (learnLock);
    learnArmed = paramId;
}

void ESXiminatorProcessor::clearMapping (const juce::String& paramId)
{
    const juce::ScopedLock sl (learnLock);
    for (auto it = ccMap.begin(); it != ccMap.end();)
        it = (it->second == paramId) ? ccMap.erase (it) : std::next (it);
}

juce::String ESXiminatorProcessor::mappingDescription (const juce::String& paramId) const
{
    const juce::ScopedLock sl (learnLock);
    for (auto& [cc, id] : ccMap)
        if (id == paramId) return "CC " + juce::String (cc);
    return {};
}

void ESXiminatorProcessor::handleControlIn (const juce::MidiMessage& m)
{
    if (! m.isController()) return;
    juce::String target;
    {
        const juce::ScopedLock sl (learnLock);
        if (learnArmed.isNotEmpty())
        {
            ccMap[m.getControllerNumber()] = learnArmed;
            learnArmed.clear();
            return;
        }
        auto it = ccMap.find (m.getControllerNumber());
        if (it == ccMap.end()) return;
        target = it->second;
    }
    if (auto* p = apvts.getParameter (target))
        p->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
}

//==============================================================================
void ESXiminatorProcessor::queueMsg (const juce::MidiMessage& m)
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoMsgs[s1] = m;
    fifo.finishedWrite (n1);
}

void ESXiminatorProcessor::queueCC (int ch1, int cc, int val)
{
    queueMsg (juce::MidiMessage::controllerEvent (ch1, cc, juce::jlimit (0, 127, val)));
}

void ESXiminatorProcessor::queueNRPN (int ch1, int nrpn14, int msb, int lsb)
{
    queueCC (ch1, 99, (nrpn14 >> 7) & 0x7F);
    queueCC (ch1, 98,  nrpn14       & 0x7F);
    queueCC (ch1,  6, msb);
    if (lsb >= 0) queueCC (ch1, 38, lsb);
}

static int channelForPart (const ESXiminatorProcessor& p, int part)
{
    if (part == Keyboard1) return p.chKbd1.load();
    if (part == Keyboard2) return p.chKbd2.load();
    return p.chDrum.load();
}

// normalised 0-1 -> the parameter's own native integer value
static int denormToInt (const juce::RangedAudioParameter* p, float n01)
{
    return juce::roundToInt (p->getNormalisableRange().convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n01)));
}

void ESXiminatorProcessor::setParamLimits (const juce::String& id, float lo01, float hi01)
{
    lo01 = juce::jlimit (0.0f, 1.0f, lo01);
    hi01 = juce::jlimit (0.0f, 1.0f, hi01);
    if (lo01 > hi01) std::swap (lo01, hi01);
    {
        const juce::ScopedLock sl (limitsLock);
        limits[id] = { lo01, hi01 };
    }
    // pull the knob inside its new range straight away (lock released first)
    if (auto* p = apvts.getParameter (id))
    {
        const float cur = p->getValue();
        const float cl  = juce::jlimit (lo01, hi01, cur);
        if (std::abs (cl - cur) > 1.0e-6f) p->setValueNotifyingHost (cl);
    }
}

void ESXiminatorProcessor::clearParamLimits (const juce::String& id)
{
    const juce::ScopedLock sl (limitsLock);
    limits.erase (id);
}

bool ESXiminatorProcessor::hasParamLimits (const juce::String& id) const
{
    const juce::ScopedLock sl (limitsLock);
    return limits.find (id) != limits.end();
}

std::pair<float, float> ESXiminatorProcessor::getParamLimits (const juce::String& id) const
{
    const juce::ScopedLock sl (limitsLock);
    auto it = limits.find (id);
    return it == limits.end() ? std::pair<float, float> { 0.0f, 1.0f } : it->second;
}

float ESXiminatorProcessor::applyLimits (const juce::String& id, float norm01) const
{
    const juce::ScopedLock sl (limitsLock);
    auto it = limits.find (id);
    if (it == limits.end()) return juce::jlimit (0.0f, 1.0f, norm01);
    return juce::jlimit (it->second.first, it->second.second, norm01);
}

juce::String ESXiminatorProcessor::limitsDescription (const juce::String& id) const
{
    if (! hasParamLimits (id)) return {};
    auto* p = apvts.getParameter (id);
    if (p == nullptr) return {};
    auto l = getParamLimits (id);
    return p->getText (l.first, 8).trim() + " .. " + p->getText (l.second, 8).trim();
}

void ESXiminatorProcessor::sendPartSlotMidi (int part, int slot, float norm01)
{
    const auto& s = slots[slot];
    auto* param = apvts.getParameter (paramID (part, slot));
    if (param == nullptr) return;

    int ch = channelForPart (*this, part);

    if (s.type == 3)
    {
        int v  = denormToInt (param, norm01);
        int tx = (v >= 512) ? 16383 : v;
        queueNRPN (ch, parts[part].nrpnBase + s.nrpnOffset, (tx >> 7) & 0x7F, tx & 0x7F);
        return;
    }

    int native  = denormToInt (param, norm01);
    int midiVal = native;
    if (s.type == 1)      midiVal = native ? 127 : 0;
    else if (s.type == 2) midiVal = choiceToMidi (slot, native, part);

    bool kbd = (part == Keyboard1 || part == Keyboard2);
    if (! kbd && s.nrpnOffset >= 0)
        queueNRPN (ch, parts[part].nrpnBase + s.nrpnOffset, midiVal);
    else if (s.cc >= 0)
        queueCC (ch, s.cc, midiVal);
}

// every outgoing param value funnels through here, so safe limits always apply
void ESXiminatorProcessor::sendParamMidi (const juce::String& id, float norm01)
{
    auto it = sendTargets.find (id);
    if (it == sendTargets.end()) return;
    auto* param = apvts.getParameter (id);
    if (param == nullptr) return;

    norm01 = applyLimits (id, norm01);
    const auto& t = it->second;
    const int g = chKbd1.load();

    switch (t.kind)
    {
        case tPartSlot: sendPartSlotMidi (t.a, t.b, norm01); break;
        case tFx:
        {
            const auto& cc = fxCC[t.a];
            int native = denormToInt (param, norm01);
            if      (t.b == 0) queueCC (g, cc.type,  fxTypeToMidi (native));
            else if (t.b == 1) queueCC (g, cc.edit1, native);
            else if (t.b == 2) queueCC (g, cc.edit2, native);
            else               queueCC (g, cc.mseq,  native ? 127 : 0);
            break;
        }
        case tFxChain:     queueCC   (g, ccFxChain, fxChainToMidi (denormToInt (param, norm01))); break;
        case tAccentLevel: queueNRPN (g, nrpnAccentLevel, denormToInt (param, norm01)); break;
        case tAccentMSeq:  queueNRPN (g, nrpnAccentMSeq, denormToInt (param, norm01) ? 127 : 0); break;
        case tSwing:       queueNRPN (g, nrpnSwing, swingToMidi (denormToInt (param, norm01))); break;
        case tRollType:
        {
            int idx = denormToInt (param, norm01);
            queueNRPN (g, nrpnRollType, idx == 0 ? 0 : (idx == 1 ? 43 : 86));
            break;
        }
    }
}

void ESXiminatorProcessor::sendParamNow (int part, int slot, float)
{
    auto id = paramID (part, slot);
    if (auto* p = apvts.getParameter (id))
        sendParamMidi (id, p->getValue());
}

void ESXiminatorProcessor::sendMutesNow()
{
    int m1msb = 0, m1lsb = 0, m2msb = 0, m2lsb = 0;
    for (int part = 0; part < NumParts; ++part)
    {
        auto* pb = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (juce::String (parts[part].id) + "_mute"));
        if (pb == nullptr || ! pb->get()) continue;
        auto b = muteBitFor (part);
        if (b.nrpn == nrpnMute1) { if (b.msbBit >= 0) m1msb |= 1 << b.msbBit; if (b.lsbBit >= 0) m1lsb |= 1 << b.lsbBit; }
        else                     { if (b.msbBit >= 0) m2msb |= 1 << b.msbBit; if (b.lsbBit >= 0) m2lsb |= 1 << b.lsbBit; }
    }
    int g = chKbd1.load();
    queueNRPN (g, nrpnMute1, m1msb, m1lsb);
    queueNRPN (g, nrpnMute2, m2msb, m2lsb);
}

void ESXiminatorProcessor::sendFxAndGlobalNow()
{
    for (int f = 1; f <= 3; ++f)
        for (auto* s : { "_type", "_edit1", "_edit2", "_mseq" })
        {
            auto id = "fx" + juce::String (f) + s;
            if (auto* p = apvts.getParameter (id)) sendParamMidi (id, p->getValue());
        }
    for (auto* id : { "fx_chain", "accent_level", "accent_mseq", "swing", "roll_type" })
        if (auto* p = apvts.getParameter (id)) sendParamMidi (id, p->getValue());
}

void ESXiminatorProcessor::parameterChanged (const juce::String& id, float v)
{
    // A limited param is pinned inside its safe range, so the knob itself stops at the
    // limit instead of only the transmitted value being clamped. Recurses exactly once:
    // the clamped write lands back here already in range.
    if (hasParamLimits (id))
        if (auto* p = apvts.getParameter (id))
        {
            const float cur = p->getValue();
            const float cl  = applyLimits (id, cur);
            if (std::abs (cl - cur) > 1.0e-6f) { p->setValueNotifyingHost (cl); return; }
        }

    if (id.endsWith ("_mute")) { sendMutesNow(); return; }
    if (id.startsWith ("fx") || id.startsWith ("accent") || id == "swing" || id == "roll_type")
    { sendFxAndGlobalNow(); return; }

    for (int part = 0; part < NumParts; ++part)
        if (id.startsWith (juce::String (parts[part].id) + "_"))
            for (int slot = 0; slot < NumSlots; ++slot)
                if (partHasSlot (part, slot) && id == paramID (part, slot))
                { sendParamNow (part, slot, v); return; }
}

void ESXiminatorProcessor::sendAllParameters()
{
    for (int part = 0; part < NumParts; ++part)
        for (int slot = 0; slot < NumSlots; ++slot)
            if (partHasSlot (part, slot))
                sendParamNow (part, slot, 0);
    sendFxAndGlobalNow();
    sendMutesNow();
}

void ESXiminatorProcessor::triggerPart (int part, bool on)
{
    int ch = channelForPart (*this, part);
    int note = parts[part].defaultNote >= 0 ? parts[part].defaultNote : 60;
    queueMsg (on ? juce::MidiMessage::noteOn  (ch, note, (juce::uint8) 110)
                 : juce::MidiMessage::noteOff (ch, note));
}

void ESXiminatorProcessor::sendPatternChange (int bankCD, int prog)
{
    int g = chKbd1.load();
    queueCC (g, 0, 0);
    queueCC (g, 32, bankCD ? 1 : 0);
    queueMsg (juce::MidiMessage::programChange (g, juce::jlimit (0, 127, prog)));
}

//==============================================================================
void ESXiminatorProcessor::processModulators (double bpm, double ppq, bool playing, int numSamples, double sr)
{
    const double blockSecs   = numSamples / sr;
    const double minInterval = 1.0 / (double) juce::jlimit (1, 120, modRateHz.load());

    // synced mods keep running with the transport stopped, driven off a free counter
    if (! playing) modFreePpq += (bpm / 60.0) * blockSecs;
    const double syncPpq = playing ? ppq : modFreePpq;

    for (int m = 0; m < numModSlots; ++m)
    {
        auto onP = apvts.getRawParameterValue (modID (m, "on"));
        if (onP == nullptr || onP->load() < 0.5f) { modOut[m].store (-1.0f); continue; }

        const int tIdx = (int) apvts.getRawParameterValue (modID (m, "target"))->load();
        if (tIdx <= 0 || tIdx >= targetIDs.size()) { modOut[m].store (-1.0f); continue; }
        const auto targetId = targetIDs[tIdx];
        auto* tp = apvts.getParameter (targetId);
        if (tp == nullptr) { modOut[m].store (-1.0f); continue; }

        const bool   sync     = apvts.getRawParameterValue (modID (m, "sync"))->load() >= 0.5f;
        const double phaseOff = apvts.getRawParameterValue (modID (m, "phase"))->load() / 360.0;

        double phase;
        if (sync)
        {
            const int d = juce::jlimit (0, numModDivs - 1, (int) apvts.getRawParameterValue (modID (m, "div"))->load());
            phase = syncPpq / modDivs[d].beats + phaseOff;
        }
        else
        {
            modPhase[m] += apvts.getRawParameterValue (modID (m, "hz"))->load() * blockSecs;
            if (modPhase[m] > 1.0e6) modPhase[m] = std::fmod (modPhase[m], 1.0);
            phase = modPhase[m] + phaseOff;
        }
        phase = phase - std::floor (phase);

        const bool wrapped = phase < modLastPhase[m];
        modLastPhase[m] = phase;

        const int wave = juce::jlimit (0, NumModWaves - 1, (int) apvts.getRawParameterValue (modID (m, "wave"))->load());
        const float w  = modWaveValue (wave, phase, wrapped, modSH[m], modSHPrev[m], modRng);

        const float lo    = apvts.getRawParameterValue (modID (m, "lo"))->load() / 127.0f;
        const float hi    = apvts.getRawParameterValue (modID (m, "hi"))->load() / 127.0f;
        const float depth = apvts.getRawParameterValue (modID (m, "depth"))->load() / 100.0f;

        const float modV  = lo + (hi - lo) * w;
        const float base  = tp->getValue();
        const float value = applyLimits (targetId, base + (modV - base) * depth);
        modOut[m].store (value);

        modSendTimer[m] += blockSecs;
        if (modSendTimer[m] < minInterval) continue;
        modSendTimer[m] = 0.0;

        const int native = denormToInt (tp, value);
        if (native == lastModSent[m]) continue;     // only send on real change
        lastModSent[m] = native;
        sendParamMidi (targetId, value);
    }
}

void ESXiminatorProcessor::prepareToPlay (double, int) { lastPpq = -1.0; wasPlaying = false; }

void ESXiminatorProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    audio.clear();
    juce::MidiBuffer out;

    const double sr = getSampleRate();
    const int numSamples = juce::jmax (1, audio.getNumSamples() == 0 ? getBlockSize() : audio.getNumSamples());

    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }
    if (! playing && internalPlay.load())
    {
        playing = true;
        bpm = internalBpm.load();
        ppq = internalPpq;
        internalPpq += (bpm / 60.0) * numSamples / sr;
    }
    else if (playing)
        internalPpq = 0.0;

    auto emit = [&] (const juce::MidiMessage& m, int samplePos)
    {
        out.addEvent (m, samplePos);
        const juce::ScopedLock sl (midiOutLock);
        if (midiOut) midiOut->sendMessageNow (m);
    };

    if (sendClock.load())
    {
        if (playing && ! wasPlaying) { emit (juce::MidiMessage::midiStart(), 0); clockPhase = 0.0; }
        if (! playing && wasPlaying)   emit (juce::MidiMessage::midiStop(), 0);
        if (playing)
        {
            const double clocksPerSample = (bpm / 60.0) * 24.0 / sr;
            for (int i = 0; i < numSamples; ++i)
            {
                clockPhase += clocksPerSample;
                if (clockPhase >= 1.0) { clockPhase -= 1.0; emit (juce::MidiMessage::midiClock(), i); }
            }
        }
    }
    wasPlaying = playing;

    processModulators (bpm, ppq, playing, numSamples, sr);

    if (seqEnabled.load() && playing)
    {
        const double stepLenPpq = 0.25;
        const double blockPpq   = (bpm / 60.0) * numSamples / sr;
        const double endPpq     = ppq + blockPpq;
        int step0 = (int) std::ceil  (ppq   / stepLenPpq - 1.0e-6);
        int step1 = (int) std::floor (endPpq / stepLenPpq - 1.0e-6);
        for (int s = step0; s <= step1; ++s)
        {
            double stepPpq = s * stepLenPpq;
            int samplePos = juce::jlimit (0, numSamples - 1,
                              (int) std::round ((stepPpq - ppq) * 60.0 / bpm * sr));
            int stepIdx = ((s % seqSteps) + seqSteps) % seqSteps;
            seqCurrentStep.store (stepIdx);
            for (int lane = 0; lane < seqLanes; ++lane)
                if (seqPattern[lane].load() & (1u << stepIdx))
                {
                    int ch = chDrum.load();
                    int note = parts[lane].defaultNote;
                    emit (juce::MidiMessage::noteOn (ch, note, (juce::uint8) seqVelocity.load()), samplePos);
                    pendingOffs[lane] = (int) (sr * 60.0 / bpm * 0.12);
                }
        }
        for (int lane = 0; lane < seqLanes; ++lane)
            if (pendingOffs[lane] >= 0)
            {
                pendingOffs[lane] -= numSamples;
                if (pendingOffs[lane] < 0)
                    emit (juce::MidiMessage::noteOff (chDrum.load(), parts[lane].defaultNote), numSamples - 1);
            }
    }
    else if (! playing)
        seqCurrentStep.store (-1);

    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) emit (fifoMsgs[s1 + i], 0);
        for (int i = 0; i < n2; ++i) emit (fifoMsgs[s2 + i], 0);
        fifo.finishedRead (n1 + n2);
    }

    for (const auto meta : midi)
    {
        handleControlIn (meta.getMessage());
        const juce::ScopedLock sl (midiOutLock);
        if (midiOut) midiOut->sendMessageNow (meta.getMessage());
        out.addEvent (meta.getMessage(), meta.samplePosition);
    }

    {
        int s1, n1, s2, n2;
        inFifo.prepareToRead (inFifo.getNumReady(), s1, n1, s2, n2);
        auto handle = [&] (const juce::MidiMessage& m)
        {
            handleControlIn (m);
            const juce::ScopedLock sl (midiOutLock);
            if (midiOut) midiOut->sendMessageNow (m);
            out.addEvent (m, 0);
        };
        for (int i = 0; i < n1; ++i) handle (inFifoMsgs[s1 + i]);
        for (int i = 0; i < n2; ++i) handle (inFifoMsgs[s2 + i]);
        inFifo.finishedRead (n1 + n2);
    }

    midi.swapWith (out);
}

//==============================================================================
void ESXiminatorProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("midiOut",  midiOutName, nullptr);
    state.setProperty ("chKbd1",   chKbd1.load(), nullptr);
    state.setProperty ("chKbd2",   chKbd2.load(), nullptr);
    state.setProperty ("chDrum",   chDrum.load(), nullptr);
    state.setProperty ("sendClock", sendClock.load(), nullptr);
    state.setProperty ("seqEnabled", seqEnabled.load(), nullptr);
    juce::String pat;
    for (int l = 0; l < seqLanes; ++l) pat += juce::String (juce::String::toHexString ((int) seqPattern[l].load())) + ",";
    state.setProperty ("seqPattern", pat, nullptr);
    state.setProperty ("midiIn", midiInName, nullptr);
    state.setProperty ("internalBpm", internalBpm.load(), nullptr);
    {
        const juce::ScopedLock sl (learnLock);
        juce::String maps;
        for (auto& [cc, id] : ccMap) maps += juce::String (cc) + "=" + id + ";";
        state.setProperty ("ccMap", maps, nullptr);
    }
    state.setProperty ("modRateHz", modRateHz.load(), nullptr);
    {
        const juce::ScopedLock sl (limitsLock);
        juce::String lim;
        for (auto& [id, r] : limits)
            lim += id + "=" + juce::String (r.first, 6) + ":" + juce::String (r.second, 6) + ";";
        state.setProperty ("limits", lim, nullptr);
    }
    juce::MemoryOutputStream mos (dest, true);
    state.writeToStream (mos);
}

void ESXiminatorProcessor::setStateInformation (const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) size);
    if (! tree.isValid()) return;
    apvts.replaceState (tree);
    chKbd1  = (int) tree.getProperty ("chKbd1", 1);
    chKbd2  = (int) tree.getProperty ("chKbd2", 2);
    chDrum  = (int) tree.getProperty ("chDrum", 10);
    sendClock  = (bool) tree.getProperty ("sendClock", true);
    seqEnabled = (bool) tree.getProperty ("seqEnabled", false);
    auto toks = juce::StringArray::fromTokens (tree.getProperty ("seqPattern", "").toString(), ",", "");
    for (int l = 0; l < seqLanes && l < toks.size(); ++l)
        seqPattern[l] = (uint32_t) toks[l].getHexValue32();
    internalBpm = (double) tree.getProperty ("internalBpm", 120.0);
    {
        const juce::ScopedLock sl (learnLock);
        ccMap.clear();
        for (auto& tok : juce::StringArray::fromTokens (tree.getProperty ("ccMap", "").toString(), ";", ""))
            if (tok.containsChar ('='))
                ccMap[tok.upToFirstOccurrenceOf ("=", false, false).getIntValue()]
                    = tok.fromFirstOccurrenceOf ("=", false, false);
    }
    modRateHz = juce::jlimit (1, 120, (int) tree.getProperty ("modRateHz", 25));
    {
        const juce::ScopedLock sl (limitsLock);
        limits.clear();
        for (auto& tok : juce::StringArray::fromTokens (tree.getProperty ("limits", "").toString(), ";", ""))
        {
            if (! tok.containsChar ('=')) continue;
            auto id  = tok.upToFirstOccurrenceOf ("=", false, false);
            auto rng = tok.fromFirstOccurrenceOf ("=", false, false);
            if (! rng.containsChar (':')) continue;
            limits[id] = { rng.upToFirstOccurrenceOf (":", false, false).getFloatValue(),
                           rng.fromFirstOccurrenceOf (":", false, false).getFloatValue() };
        }
    }
    openMidiOutput (tree.getProperty ("midiOut", "").toString());
    openMidiInput (tree.getProperty ("midiIn", "").toString());
}

bool ESXiminatorProcessor::importXM (const juce::File& f, juce::String& report)
{
    return esx::importXMFile (f, seqPattern, seqLanes, seqSteps, report);
}

juce::AudioProcessorEditor* ESXiminatorProcessor::createEditor() { return new ESXiminatorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ESXiminatorProcessor(); }
