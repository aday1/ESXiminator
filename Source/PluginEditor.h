#pragma once
#include "PluginProcessor.h"

//==============================================================================
struct ESXLookAndFeel : public juce::LookAndFeel_V4
{
    ESXLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
};

//==============================================================================
class TopStrip;
class PartMatrix;
class PartEdit;
class FxRack;
class StepKeys;
class ModPanel;
class SetupPanel;

class ESXiminatorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ESXiminatorEditor (ESXiminatorProcessor&);
    ~ESXiminatorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    ESXiminatorProcessor& proc;
private:
    void showOverlay (juce::Component* c);   // MOD / SETUP live off-panel; the hardware has no such page

    ESXLookAndFeel lnf;
    std::unique_ptr<TopStrip>   top;
    std::unique_ptr<PartMatrix> matrix;
    std::unique_ptr<PartEdit>   edit;
    std::unique_ptr<FxRack>     fx;
    std::unique_ptr<StepKeys>   steps;
    std::unique_ptr<ModPanel>   modPage;
    std::unique_ptr<SetupPanel> setupPage;
    juce::TextButton closeOverlay { "CLOSE" };
    juce::Component* overlay = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ESXiminatorEditor)
};
