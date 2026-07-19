#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "ESXDef.h"
#include "ModMatrix.h"

class ESXiminatorProcessor : public juce::AudioProcessor,
                             public juce::AudioProcessorValueTreeState::Listener,
                             public juce::MidiInputCallback
{
public:
    ESXiminatorProcessor();
    ~ESXiminatorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true; }
    const juce::String getName() const override            { return "ESXiminator"; }
    bool acceptsMidi() const override                      { return true; }
    bool producesMidi() const override                     { return true; }
    bool isMidiEffect() const override                     { return true; }
    double getTailLengthSeconds() const override           { return 0.0; }
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override       { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void parameterChanged (const juce::String& id, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

    juce::StringArray getMidiOutputNames() const;
    void openMidiOutput (const juce::String& name);
    juce::String getMidiOutputName() const            { return midiOutName; }

    juce::StringArray getMidiInputNames() const;
    void openMidiInput (const juce::String& name);
    juce::String getMidiInputName() const             { return midiInName; }
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;

    void armLearn (const juce::String& paramId);
    void clearMapping (const juce::String& paramId);
    juce::String mappingDescription (const juce::String& paramId) const;

    // ---- safe limits: clamp every outgoing value for a param, manual moves included ----
    void setParamLimits (const juce::String& paramId, float lo01, float hi01);
    void clearParamLimits (const juce::String& paramId);
    bool hasParamLimits (const juce::String& paramId) const;
    std::pair<float, float> getParamLimits (const juce::String& paramId) const;   // normalised 0-1
    juce::String limitsDescription (const juce::String& paramId) const;           // native units, for menus
    float applyLimits (const juce::String& paramId, float norm01) const;

    // ---- modulator targets ----
    const juce::StringArray& getTargetNames() const { return targetNames; }
    const juce::StringArray& getTargetIDs()   const { return targetIDs; }
    static juce::StringArray buildTargetNames();
    static juce::StringArray buildTargetIDs();

    std::atomic<float> modOut[esx::numModSlots];   // last value sent, normalised; -1 = slot idle
    std::atomic<int>   modRateHz { 25 };           // per-slot send rate cap (MIDI bandwidth guard)

    std::atomic<bool>   internalPlay { false };
    std::atomic<double> internalBpm  { 120.0 };

    void triggerPart (int part, bool on);
    void sendAllParameters();
    void sendPatternChange (int bankCD, int prog);

    std::atomic<int> chKbd1 { 1 }, chKbd2 { 2 }, chDrum { 10 };
    std::atomic<bool> sendClock { true };

    static constexpr int seqLanes = 13;
    static constexpr int seqSteps = 16;
    std::atomic<uint32_t> seqPattern[seqLanes];
    std::atomic<bool> seqEnabled { false };
    std::atomic<int>  seqCurrentStep { -1 };
    std::atomic<int>  seqVelocity { 110 };

    bool importXM (const juce::File& f, juce::String& report);

private:
    void queueMsg (const juce::MidiMessage& m);
    void queueCC (int ch1, int cc, int val);
    void queueNRPN (int ch1, int nrpn14, int valueMSB, int valueLSB = -1);
    void sendParamNow (int part, int slot, float normValue);
    void sendMutesNow();
    void sendFxAndGlobalNow();

    // ---- single choke point for every outgoing param value (applies safe limits) ----
    void sendParamMidi (const juce::String& paramId, float norm01);
    void sendPartSlotMidi (int part, int slot, float norm01);

    // where a param id transmits to
    enum TargetKind { tPartSlot = 0, tFx, tFxChain, tAccentLevel, tAccentMSeq, tSwing, tRollType };
    struct SendTarget { int kind, a, b; };
    std::map<juce::String, SendTarget> sendTargets;
    void buildSendTargets();

    juce::StringArray targetNames, targetIDs;

    mutable juce::CriticalSection limitsLock;
    std::map<juce::String, std::pair<float, float>> limits;

    // ---- modulator engine ----
    void processModulators (double bpm, double ppq, bool playing, int numSamples, double sr);
    double modPhase[esx::numModSlots] {};      // free-run phase accumulator
    double modLastPhase[esx::numModSlots] {};  // wrap detection for S&H / smooth random
    float  modSH[esx::numModSlots] {};
    float  modSHPrev[esx::numModSlots] {};
    double modSendTimer[esx::numModSlots] {};
    int    lastModSent[esx::numModSlots];
    double modFreePpq = 0.0;                   // keeps synced mods moving when transport is stopped
    juce::Random modRng;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static BusesProperties makeBuses();

    std::unique_ptr<juce::MidiOutput> midiOut;
    juce::String midiOutName;
    juce::CriticalSection midiOutLock;

    std::unique_ptr<juce::MidiInput> midiIn;
    juce::String midiInName;
    juce::AbstractFifo inFifo { 512 };
    juce::MidiMessage inFifoMsgs[512];

    mutable juce::CriticalSection learnLock;
    juce::String learnArmed;
    std::map<int, juce::String> ccMap;
    void handleControlIn (const juce::MidiMessage& m);

    juce::AbstractFifo fifo { 2048 };
    juce::MidiMessage fifoMsgs[2048];

    double lastPpq = -1.0; bool wasPlaying = false;
    double clockPhase = 0.0, internalPpq = 0.0;
    int pendingOffs[seqLanes];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ESXiminatorProcessor)
};
