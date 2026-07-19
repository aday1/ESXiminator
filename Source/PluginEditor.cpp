#include "PluginEditor.h"
#include "HardwarePanel.h"
#include "UpdateChecker.h"
#include "Version.h"
#include <thread>
using namespace esx;

// ================= ESX red/metal look =================
namespace col
{
    const juce::Colour body      (0xff8f1a12);
    const juce::Colour bodyDark  (0xff5c0e09);
    const juce::Colour metal     (0xffb9b4ae);
    const juce::Colour panel     (0xff232022);
    const juce::Colour panelHi   (0xff37322f);
    const juce::Colour led       (0xffff5533);
    const juce::Colour ledOff    (0xff4a2320);
    const juce::Colour text      (0xffe8e2d8);
    const juce::Colour amber     (0xffffb14a);
    const juce::Colour panelBlk  (0xff1b1a1c);   // dark panel the sections are printed on
    const juce::Colour silk      (0xffd8d2c8);   // silkscreen lettering
    const juce::Colour keyWhite  (0xffe6e1d8);   // step key caps
}

ESXLookAndFeel::ESXLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::panel);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::ComboBox::backgroundColourId, col::panelHi);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::outlineColourId, col::bodyDark);
    setColour (juce::PopupMenu::backgroundColourId, col::panelHi);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::body);
    setColour (juce::TextButton::buttonColourId, col::panelHi);
    setColour (juce::TextButton::buttonOnColourId, col::body);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::tabTextColourId, col::text.withAlpha (0.6f));
    setColour (juce::TabbedButtonBar::frontTextColourId, juce::Colours::white);
}

void ESXLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float a0, float a1, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    auto d = juce::jmin (r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    auto knob = juce::Rectangle<float> (d, d).withCentre (c).reduced (d * 0.16f);

    juce::Path arc;
    arc.addCentredArc (c.x, c.y, d * 0.46f, d * 0.46f, 0.0f, a0, a0 + (a1 - a0) * pos, true);
    g.setColour (col::led);
    g.strokePath (arc, juce::PathStrokeType (d * 0.055f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path track;
    track.addCentredArc (c.x, c.y, d * 0.46f, d * 0.46f, 0.0f, a0 + (a1 - a0) * pos, a1, true);
    g.setColour (col::ledOff);
    g.strokePath (track, juce::PathStrokeType (d * 0.055f));

    juce::ColourGradient grad (col::metal.brighter (0.25f), knob.getX(), knob.getY(),
                               col::metal.darker (0.65f), knob.getRight(), knob.getBottom(), false);
    g.setGradientFill (grad);
    g.fillEllipse (knob);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (knob, 1.2f);
    g.setColour (col::panel.withAlpha (0.35f));
    g.fillEllipse (knob.reduced (d * 0.16f));

    auto angle = a0 + (a1 - a0) * pos;
    juce::Path ptr;
    ptr.addRoundedRectangle (-d * 0.025f, -knob.getWidth() / 2.0f + 2.0f, d * 0.05f, knob.getWidth() * 0.32f, d * 0.02f);
    g.setColour (col::led);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c));
}

void ESXLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    bool on = b.getToggleState();
    g.setColour (on ? col::body : col::panelHi.brighter (over ? 0.15f : 0.0f));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    // no caption -> the label is silkscreened above, so centre the LED in the cap
    const bool labelled = b.getButtonText().isNotEmpty();
    auto ledArea = labelled ? r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.28f)
                            : juce::Rectangle<float> (r.getHeight() * 0.44f, r.getHeight() * 0.44f).withCentre (r.getCentre());
    g.setColour (on ? col::led : col::ledOff);
    g.fillEllipse (ledArea);
    if (on) { g.setColour (col::led.withAlpha (0.35f)); g.fillEllipse (ledArea.expanded (3.0f)); }
    if (! labelled) return;
    g.setColour (on ? juce::Colours::white : col::text);
    g.setFont (juce::Font (juce::jmin (13.0f, r.getHeight() * 0.6f), juce::Font::bold));
    g.drawText (b.getButtonText(), r.reduced (2.0f, 0.0f), juce::Justification::centredLeft);
}

// ================= MIDI learn / forget, shared by every control =================
// withRange: only continuous controls get safe-limit entries (a choice or switch has none)
static void showLearnMenu (ESXiminatorProcessor& proc, const juce::String& pid,
                           juce::Component* target, bool withRange,
                           std::function<void()> onChanged = {})
{
    juce::PopupMenu m;
    auto desc = proc.mappingDescription (pid);
    m.addSectionHeader (desc.isEmpty() ? "MIDI: unmapped" : "MIDI: " + desc);
    m.addItem (1, "MIDI learn (move a controller knob)");
    m.addItem (2, "MIDI forget" + (desc.isEmpty() ? juce::String() : " (" + desc + ")"), desc.isNotEmpty());

    if (withRange)
    {
        auto lim = proc.limitsDescription (pid);
        m.addSeparator();
        m.addSectionHeader (lim.isEmpty() ? "Safe range: off" : "Safe range: " + lim);
        m.addItem (3, "Set safe MIN to current value");
        m.addItem (4, "Set safe MAX to current value");
        m.addItem (5, "Clear safe range", lim.isNotEmpty());
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
        [&proc, pid, onChanged] (int r)
        {
            auto* p = proc.apvts.getParameter (pid);
            auto cur = p != nullptr ? p->getValue() : 0.0f;
            auto rng = proc.getParamLimits (pid);
            if      (r == 1) proc.armLearn (pid);
            else if (r == 2) proc.clearMapping (pid);
            else if (r == 3) proc.setParamLimits (pid, cur, juce::jmax (cur, rng.second));
            else if (r == 4) proc.setParamLimits (pid, juce::jmin (cur, rng.first), cur);
            else if (r == 5) proc.clearParamLimits (pid);
            if (r >= 3 && onChanged) onChanged();
        });
}

class LearnSlider : public juce::Slider
{
public:
    LearnSlider (ESXiminatorProcessor& p, juce::String paramId)
        : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow),
          proc (p), pid (std::move (paramId)) {}

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            showLearnMenu (proc, pid, this, true, [this] { refreshLimitTint(); });
            return;
        }
        juce::Slider::mouseDown (e);
    }

    // amber read-out = this knob is range-limited
    void refreshLimitTint()
    {
        setColour (juce::Slider::textBoxTextColourId, proc.hasParamLimits (pid) ? col::amber : col::text);
        repaint();
    }

    void parentHierarchyChanged() override { refreshLimitTint(); }

private:
    ESXiminatorProcessor& proc;
    juce::String pid;
};

class LearnCombo : public juce::ComboBox
{
public:
    LearnCombo (ESXiminatorProcessor& p, juce::String paramId) : proc (p), pid (std::move (paramId)) {}
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showLearnMenu (proc, pid, this, false); return; }
        juce::ComboBox::mouseDown (e);
    }
private:
    ESXiminatorProcessor& proc;
    juce::String pid;
};

class LearnToggle : public juce::ToggleButton
{
public:
    LearnToggle (ESXiminatorProcessor& p, juce::String paramId, const juce::String& text)
        : juce::ToggleButton (text), proc (p), pid (std::move (paramId)) {}
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showLearnMenu (proc, pid, this, false); return; }
        juce::ToggleButton::mouseDown (e);
    }
private:
    ESXiminatorProcessor& proc;
    juce::String pid;
};

// ================= small helpers =================
static juce::Label* makeCaption (juce::Component& parent, juce::OwnedArray<juce::Label>& store, const juce::String& t)
{
    auto* l = store.add (new juce::Label ({}, t));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (11.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, col::text.withAlpha (0.85f));
    parent.addAndMakeVisible (l);
    return l;
}

// ================= silkscreen helpers =================
static void drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto f = r.toFloat();
    g.setColour (col::panelBlk);
    g.fillRoundedRectangle (f, 5.0f);
    g.setColour (col::silk.withAlpha (0.30f));
    g.drawRoundedRectangle (f.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (col::silk.withAlpha (0.75f));
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText (title, r.reduced (8, 3).removeFromTop (12), juce::Justification::centredLeft);
}

// ================= PART EDIT (per-part NRPN block +00..+17) =================
class PartEdit : public juce::Component
{
public:
    explicit PartEdit (ESXiminatorProcessor& p) : proc (p) { build (0); }

    void build (int part)
    {
        selected = part;
        knobAtts.clear(); comboAtts.clear(); toggleAtts.clear();
        knobs.clear(); combos.clear(); toggles.clear(); captions.clear();
        cells.clear();

        for (auto& grp : editGroups())
            for (int slot : grp.slots)
            {
                if (! partHasSlot (part, slot)) continue;
                const auto& sd = slots[slot];
                auto id = paramID (part, slot);

                if (sd.type == 1)
                {
                    // silkscreen label above an LED button, as on the hardware
                    auto* t = toggles.add (new LearnToggle (proc, id, {}));
                    toggleAtts.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (proc.apvts, id, *t));
                    addAndMakeVisible (t);
                    cells.push_back ({ slot, t, makeCaption (*this, captions, sd.name) });
                }
                else if (sd.type == 2)
                {
                    auto* c = combos.add (new LearnCombo (proc, id));
                    if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
                        c->addItemList (pc->choices, 1);
                    comboAtts.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (proc.apvts, id, *c));
                    addAndMakeVisible (c);
                    cells.push_back ({ slot, c, makeCaption (*this, captions, sd.name) });
                }
                else
                {
                    auto* k = knobs.add (new LearnSlider (proc, id));
                    k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
                    knobAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, id, *k));
                    addAndMakeVisible (k);
                    cells.push_back ({ slot, k, makeCaption (*this, captions, sd.name) });
                }
            }
        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        for (auto& gb : groupBounds)
            drawSection (g, gb.second, gb.first);
    }

    void resized() override
    {
        groupBounds.clear();
        auto r = getLocalBounds();
        const int cellW = 78;
        const int rowH  = juce::jmax (92, r.getHeight() / 4);   // fill the panel height

        auto place = [&] (const EditGroup& grp, juce::Rectangle<int> box)
        {
            groupBounds.push_back ({ grp.title, box });
            auto inner = box.reduced (8, 0).withTrimmedTop (16).withTrimmedBottom (4);
            for (int slot : grp.slots)
            {
                auto* cell = find (slot);
                if (cell == nullptr) continue;
                auto cb = inner.removeFromLeft (cellW);
                cell->caption->setBounds (cb.removeFromTop (13));
                if (dynamic_cast<juce::ComboBox*> (cell->control) != nullptr)
                    cell->control->setBounds (cb.removeFromTop (26).reduced (2, 0));
                else if (dynamic_cast<juce::ToggleButton*> (cell->control) != nullptr)
                    cell->control->setBounds (cb.removeFromTop (30).reduced (12, 2));
                else
                    cell->control->setBounds (cb.reduced (2, 0));
            }
        };

        const auto& gs = editGroups();
        place (gs[0], r.removeFromTop (rowH).reduced (0, 2));
        place (gs[1], r.removeFromTop (rowH).reduced (0, 2));
        place (gs[2], r.removeFromTop (rowH).reduced (0, 2));
        auto bottom = r.removeFromTop (rowH).reduced (0, 2);
        place (gs[3], bottom.removeFromLeft (340));
        bottom.removeFromLeft (6);
        place (gs[4], bottom);
    }

private:
    struct Cell { int slot; juce::Component* control; juce::Label* caption; };
    Cell* find (int slot)
    {
        for (auto& c : cells) if (c.slot == slot) return &c;
        return nullptr;
    }

    ESXiminatorProcessor& proc;
    int selected = 0;
    std::vector<Cell> cells;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> groupBounds;
    juce::OwnedArray<LearnSlider> knobs;
    juce::OwnedArray<LearnCombo> combos;
    juce::OwnedArray<LearnToggle> toggles;
    juce::OwnedArray<juce::Label> captions;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAtts;
};

// ================= PART matrix (select / trigger / mute) =================
class PartMatrix : public juce::Component
{
public:
    PartMatrix (ESXiminatorProcessor& p, std::function<void(int)> onSelect)
        : proc (p), selectCb (std::move (onSelect))
    {
        for (int i = 0; i < NumParts; ++i)
        {
            auto* b = partBtns.add (new juce::TextButton (parts[i].name));
            b->setClickingTogglesState (true);
            b->setRadioGroupId (901);
            b->onClick = [this, i] { if (selectCb) selectCb (i); };
            addAndMakeVisible (b);

            auto* t = trigBtns.add (new juce::TextButton ("|>"));
            t->onStateChange = [this, t, i]
            {
                bool down = t->isDown();
                if (down != trigHeld[i]) { trigHeld[i] = down; proc.triggerPart (i, down); }
            };
            addAndMakeVisible (t);

            auto* m = muteBtns.add (new LearnToggle (proc, juce::String (parts[i].id) + "_mute", {}));
            muteAtts.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (
                proc.apvts, juce::String (parts[i].id) + "_mute", *m));
            addAndMakeVisible (m);
        }
        partBtns[0]->setToggleState (true, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override { drawSection (g, getLocalBounds(), "PART / MUTE"); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 0).withTrimmedTop (18).withTrimmedBottom (4);
        const int rowH = juce::jmax (18, r.getHeight() / NumParts);
        for (int i = 0; i < NumParts; ++i)
        {
            auto row = r.removeFromTop (rowH).reduced (0, 1);
            muteBtns[i]->setBounds (row.removeFromRight (26));
            trigBtns[i]->setBounds (row.removeFromRight (30).reduced (1, 0));
            partBtns[i]->setBounds (row);
        }
    }

private:
    ESXiminatorProcessor& proc;
    std::function<void(int)> selectCb;
    bool trigHeld[NumParts] = {};
    juce::OwnedArray<juce::TextButton> partBtns, trigBtns;
    juce::OwnedArray<LearnToggle> muteBtns;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAtts;
};

// ================= FX rack (FX1-3 type/edit1/edit2/mseq + chain) =================
class FxRack : public juce::Component
{
public:
    explicit FxRack (ESXiminatorProcessor& p) : proc (p)
    {
        for (int f = 0; f < 3; ++f)
        {
            auto pfx = "fx" + juce::String (f + 1);
            auto* c = combos.add (new LearnCombo (proc, pfx + "_type"));
            if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (pfx + "_type")))
                c->addItemList (pc->choices, 1);
            comboAtts.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (proc.apvts, pfx + "_type", *c));
            addAndMakeVisible (c);

            for (auto* nm : { "_edit1", "_edit2" })
            {
                auto* k = knobs.add (new LearnSlider (proc, pfx + nm));
                k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 14);
                knobAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, pfx + nm, *k));
                addAndMakeVisible (k);
                makeCaption (*this, captions, juce::String (nm).replace ("_edit", "EDIT "));
            }
            auto* t = toggles.add (new LearnToggle (proc, pfx + "_mseq", "M.SEQ"));
            toggleAtts.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (proc.apvts, pfx + "_mseq", *t));
            addAndMakeVisible (t);
        }
        chainBox = std::make_unique<LearnCombo> (proc, "fx_chain");
        if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("fx_chain")))
            chainBox->addItemList (pc->choices, 1);
        chainAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "fx_chain", *chainBox);
        addAndMakeVisible (*chainBox);
    }

    void paint (juce::Graphics& g) override
    {
        for (int f = 0; f < 3; ++f) drawSection (g, fxBounds[f], "FX" + juce::String (f + 1));
        drawSection (g, chainBounds, "FX CHAIN");
    }

    void resized() override
    {
        auto r = getLocalBounds();
        for (int f = 0; f < 3; ++f)
        {
            auto box = r.removeFromTop (108).reduced (0, 2);
            fxBounds[f] = box;
            auto in = box.reduced (8, 0).withTrimmedTop (17);
            combos[f]->setBounds (in.removeFromTop (24));
            in.removeFromTop (2);
            auto e1 = in.removeFromLeft (66);
            captions[f * 2]->setBounds (e1.removeFromTop (12));
            knobs[f * 2]->setBounds (e1);
            auto e2 = in.removeFromLeft (66);
            captions[f * 2 + 1]->setBounds (e2.removeFromTop (12));
            knobs[f * 2 + 1]->setBounds (e2);
            toggles[f]->setBounds (in.removeFromLeft (76).withSizeKeepingCentre (72, 22));
        }
        chainBounds = r.removeFromTop (48).reduced (0, 2);
        auto ci = chainBounds.reduced (8, 0).withTrimmedTop (18);
        chainBox->setBounds (ci.removeFromTop (24));
    }

private:
    ESXiminatorProcessor& proc;
    juce::OwnedArray<LearnSlider> knobs;
    juce::OwnedArray<LearnCombo> combos;
    juce::OwnedArray<LearnToggle> toggles;
    juce::OwnedArray<juce::Label> captions;
    std::unique_ptr<LearnCombo> chainBox;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAtts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chainAtt;
    juce::Rectangle<int> fxBounds[3], chainBounds;
};

// ================= 16 step keys (drives note triggers for the selected part) =================
class StepKeys : public juce::Component, private juce::Timer
{
public:
    explicit StepKeys (ESXiminatorProcessor& p) : proc (p)
    {
        run.setButtonText ("SEQ RUN");
        run.setToggleState (proc.seqEnabled.load(), juce::dontSendNotification);
        run.onClick = [this] { proc.seqEnabled = run.getToggleState(); };
        addAndMakeVisible (run);

        vel.setRange (1, 127, 1);
        vel.setValue (proc.seqVelocity.load(), juce::dontSendNotification);
        vel.setSliderStyle (juce::Slider::LinearHorizontal);
        vel.setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 16);
        vel.onValueChange = [this] { proc.seqVelocity = (int) vel.getValue(); };
        addAndMakeVisible (vel);
        velLabel = makeCaption (*this, captions, "VELOCITY");

        importBtn.setButtonText ("IMPORT .XM");
        importBtn.onClick = [this] { doImport(); };
        addAndMakeVisible (importBtn);

        clearBtn.setButtonText ("CLEAR");
        clearBtn.onClick = [this] { for (int l = 0; l < proc.seqLanes; ++l) proc.seqPattern[l] = 0; repaint(); };
        addAndMakeVisible (clearBtn);

        status.setColour (juce::Label::textColourId, col::amber);
        status.setFont (juce::Font (11.0f));
        addAndMakeVisible (status);
        startTimerHz (20);
    }

    void setPart (int p) { part = p; repaint(); }

    void doImport()
    {
        chooser = std::make_unique<juce::FileChooser> ("Import XM module", juce::File(), "*.xm");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                juce::String rep;
                proc.importXM (f, rep);
                status.setText (rep, juce::dontSendNotification);
                repaint();
            });
    }

    void paint (juce::Graphics& g) override
    {
        drawSection (g, getLocalBounds(), "STEP KEYS - " + juce::String (parts[juce::jlimit (0, NumParts - 1, part)].name)
                                          + (hasLane() ? "" : "   (no sequencer lane for this part)"));
        const int playStep = proc.seqCurrentStep.load();
        const uint32_t bits = hasLane() ? proc.seqPattern[part].load() : 0;

        for (int s = 0; s < 16; ++s)
        {
            auto k = keyRect (s);
            const bool on = (bits & (1u << s)) != 0;
            g.setColour (! hasLane() ? col::panelHi.withAlpha (0.4f)
                                     : (on ? col::led : (s % 4 == 0 ? col::keyWhite : col::keyWhite.darker (0.22f))));
            g.fillRoundedRectangle (k.toFloat(), 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (k.toFloat().reduced (0.5f), 3.0f, 1.0f);

            auto led = juce::Rectangle<float> ((float) k.getCentreX() - 3.0f, (float) k.getY() - 9.0f, 6.0f, 6.0f);
            g.setColour (s == playStep ? col::amber : col::ledOff);
            g.fillEllipse (led);

            g.setColour (on ? juce::Colours::white : juce::Colours::black.withAlpha (0.7f));
            g.setFont (juce::Font (10.0f, juce::Font::bold));
            g.drawText (juce::String (s + 1), k, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! hasLane()) return;
        for (int s = 0; s < 16; ++s)
            if (keyRect (s).contains (e.getPosition()))
            {
                proc.seqPattern[part].fetch_xor (1u << s);
                repaint();
                return;
            }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 0).withTrimmedTop (18).withTrimmedBottom (6);
        auto ctl = r.removeFromBottom (24);
        run.setBounds (ctl.removeFromLeft (96));
        ctl.removeFromLeft (8);
        velLabel->setBounds (ctl.removeFromLeft (58));
        vel.setBounds (ctl.removeFromLeft (170).reduced (0, 2));
        ctl.removeFromLeft (10);
        importBtn.setBounds (ctl.removeFromLeft (100).reduced (0, 1));
        clearBtn.setBounds (ctl.removeFromLeft (64).reduced (2, 1));
        status.setBounds (ctl.reduced (8, 0));
        r.removeFromTop (10);
        keyRow = r;
    }

private:
    bool hasLane() const { return part >= 0 && part < proc.seqLanes; }
    juce::Rectangle<int> keyRect (int s) const
    {
        const float w = keyRow.getWidth() / 16.0f;
        return juce::Rectangle<float> (keyRow.getX() + s * w, (float) keyRow.getY(), w, (float) keyRow.getHeight())
                 .reduced (3.0f, 2.0f).toNearestInt();
    }
    void timerCallback() override
    {
        int s = proc.seqCurrentStep.load();
        if (s != lastShown) { lastShown = s; repaint(); }
    }

    ESXiminatorProcessor& proc;
    int part = 0, lastShown = -2;
    juce::ToggleButton run;
    juce::Slider vel;
    juce::Label* velLabel = nullptr;
    juce::Label status;
    juce::OwnedArray<juce::Label> captions;
    juce::TextButton importBtn, clearBtn;
    juce::Rectangle<int> keyRow;
    std::unique_ptr<juce::FileChooser> chooser;
};

// ================= top strip: pattern select, transport, global NRPNs =================
class TopStrip : public juce::Component
{
public:
    TopStrip (ESXiminatorProcessor& p, std::function<void()> openMod, std::function<void()> openSetup)
        : proc (p)
    {
        for (auto* n : { "A", "B", "C", "D" }) bankBox.addItem (n, bankBox.getNumItems() + 1);
        bankBox.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (bankBox);
        for (int i = 1; i <= 64; ++i) patBox.addItem (juce::String (i), i);
        patBox.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (patBox);
        patBtn.setButtonText ("LOAD");
        patBtn.onClick = [this]
        {
            const int bank = bankBox.getSelectedId() - 1;      // A/B share LSB 0, C/D share LSB 1
            const int prog = patBox.getSelectedId() - 1 + ((bank == 1 || bank == 3) ? 64 : 0);
            proc.sendPatternChange (bank >= 2 ? 1 : 0, prog);
        };
        addAndMakeVisible (patBtn);

        playBtn.setButtonText ("PLAY");
        playBtn.setClickingTogglesState (true);
        playBtn.setToggleState (proc.internalPlay.load(), juce::dontSendNotification);
        playBtn.onClick = [this] { proc.internalPlay = playBtn.getToggleState(); };
        addAndMakeVisible (playBtn);

        bpm.setRange (40.0, 300.0, 0.1);
        bpm.setValue (proc.internalBpm.load(), juce::dontSendNotification);
        bpm.setSliderStyle (juce::Slider::LinearHorizontal);
        bpm.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 16);
        bpm.onValueChange = [this] { proc.internalBpm = bpm.getValue(); };
        addAndMakeVisible (bpm);

        for (auto def : { std::pair<const char*, const char*> { "accent_level", "ACCENT" },
                          { "swing", "SWING" } })
        {
            auto* k = knobs.add (new LearnSlider (proc, def.first));
            k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 44, 13);
            knobAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, def.first, *k));
            addAndMakeVisible (k);
            makeCaption (*this, captions, def.second);
        }
        accentMSeq = std::make_unique<LearnToggle> (proc, "accent_mseq", "ACC M.SEQ");
        accentAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "accent_mseq", *accentMSeq);
        addAndMakeVisible (*accentMSeq);

        rollBox = std::make_unique<LearnCombo> (proc, "roll_type");
        if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("roll_type")))
            rollBox->addItemList (pc->choices, 1);
        rollAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "roll_type", *rollBox);
        addAndMakeVisible (*rollBox);
        rollCaption = makeCaption (*this, captions, "ROLL TYPE");

        modBtn.setButtonText ("MOD");
        modBtn.onClick = std::move (openMod);
        addAndMakeVisible (modBtn);
        setupBtn.setButtonText ("SETUP");
        setupBtn.onClick = std::move (openSetup);
        addAndMakeVisible (setupBtn);
        syncBtn.setButtonText ("SYNC ALL");
        syncBtn.onClick = [this] { proc.sendAllParameters(); };
        addAndMakeVisible (syncBtn);
    }

    void paint (juce::Graphics& g) override
    {
        drawSection (g, patternBounds, "PATTERN");
        drawSection (g, tempoBounds, "TEMPO / TRANSPORT");
        drawSection (g, globalBounds, "GLOBAL");
        drawSection (g, utilBounds, "UTILITY");
    }

    void resized() override
    {
        auto r = getLocalBounds();
        patternBounds = r.removeFromLeft (222).reduced (2, 0);
        auto pi = patternBounds.reduced (8, 0).withTrimmedTop (18);
        auto prow = pi.removeFromTop (26);
        bankBox.setBounds (prow.removeFromLeft (52));
        prow.removeFromLeft (4);
        patBox.setBounds (prow.removeFromLeft (66));
        prow.removeFromLeft (4);
        patBtn.setBounds (prow.removeFromLeft (62));

        tempoBounds = r.removeFromLeft (268).reduced (2, 0);
        auto ti = tempoBounds.reduced (8, 0).withTrimmedTop (18);
        auto trow = ti.removeFromTop (26);
        playBtn.setBounds (trow.removeFromLeft (66));
        trow.removeFromLeft (6);
        bpm.setBounds (trow.reduced (0, 2));

        globalBounds = r.removeFromLeft (330).reduced (2, 0);
        auto gi = globalBounds.reduced (8, 0).withTrimmedTop (15).withTrimmedBottom (3);
        for (int i = 0; i < 2; ++i)
        {
            auto cb = gi.removeFromLeft (54);
            captions[i]->setBounds (cb.removeFromTop (12));
            knobs[i]->setBounds (cb.reduced (1, 0));
        }
        gi.removeFromLeft (4);
        auto rc = gi.removeFromLeft (84);
        rollCaption->setBounds (rc.removeFromTop (12));
        rollBox->setBounds (rc.removeFromTop (24));
        gi.removeFromLeft (4);
        accentMSeq->setBounds (gi.removeFromLeft (96).withSizeKeepingCentre (92, 22));

        utilBounds = r.reduced (2, 0);
        auto ui = utilBounds.reduced (8, 0).withTrimmedTop (18);
        auto urow = ui.removeFromTop (26);
        modBtn.setBounds (urow.removeFromLeft (54));
        urow.removeFromLeft (4);
        setupBtn.setBounds (urow.removeFromLeft (62));
        urow.removeFromLeft (4);
        syncBtn.setBounds (urow.removeFromLeft (84));
    }

private:
    ESXiminatorProcessor& proc;
    juce::ComboBox bankBox, patBox;
    juce::TextButton patBtn, playBtn, modBtn, setupBtn, syncBtn;
    juce::Slider bpm;
    juce::OwnedArray<LearnSlider> knobs;
    juce::OwnedArray<juce::Label> captions;
    juce::Label* rollCaption = nullptr;
    std::unique_ptr<LearnToggle> accentMSeq;
    std::unique_ptr<LearnCombo> rollBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> accentAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rollAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtts;
    juce::Rectangle<int> patternBounds, tempoBounds, globalBounds, utilBounds;
};

// ================= SETUP tab =================
class SetupPanel : public juce::Component
{
public:
    explicit SetupPanel (ESXiminatorProcessor& p) : proc (p)
    {
        refreshDevices();
        midiBox.onChange = [this]
        {
            proc.openMidiOutput (midiBox.getSelectedId() > 1 ? midiBox.getText() : juce::String());
        };
        addAndMakeVisible (midiBox);
        midiInBox.onChange = [this]
        {
            proc.openMidiInput (midiInBox.getSelectedId() > 1 ? midiInBox.getText() : juce::String());
        };
        addAndMakeVisible (midiInBox);
        refreshBtn.setButtonText ("RESCAN");
        refreshBtn.onClick = [this] { refreshDevices(); };
        addAndMakeVisible (refreshBtn);

        auto setupCh = [this] (juce::ComboBox& b, std::atomic<int>& target)
        {
            for (int i = 1; i <= 16; ++i) b.addItem (juce::String (i), i);
            b.setSelectedId (target.load(), juce::dontSendNotification);
            b.onChange = [&b, &target] { target = b.getSelectedId(); };
            addAndMakeVisible (b);
        };
        setupCh (chK1, proc.chKbd1); setupCh (chK2, proc.chKbd2); setupCh (chDr, proc.chDrum);

        clockToggle.setButtonText ("SEND MIDI CLOCK / START / STOP (tempo-sync the ESX)");
        clockToggle.setToggleState (proc.sendClock.load(), juce::dontSendNotification);
        clockToggle.onClick = [this] { proc.sendClock = clockToggle.getToggleState(); };
        addAndMakeVisible (clockToggle);

        syncBtn.setButtonText ("SYNC ALL PARAMETERS TO ESX");
        syncBtn.setColour (juce::TextButton::buttonColourId, col::body);
        syncBtn.onClick = [this] { proc.sendAllParameters(); };
        addAndMakeVisible (syncBtn);

        updateToggle.setButtonText ("CHECK GITHUB FOR UPDATES (opt-in)");
        updateToggle.setToggleState (UpdatePrefs::getCheckEnabled(), juce::dontSendNotification);
        updateToggle.onClick = [this]
        {
            UpdatePrefs::setCheckEnabled (updateToggle.getToggleState());
            if (updateToggle.getToggleState())
                startUpdateCheck (false);
        };
        addAndMakeVisible (updateToggle);

        updateBtn.setButtonText ("CHECK NOW");
        updateBtn.onClick = [this] { startUpdateCheck (true); };
        addAndMakeVisible (updateBtn);

        updateStatus.setText ("Updates: off (v" + juce::String (ESXIMINATOR_VERSION_STRING) + ")",
                              juce::dontSendNotification);
        updateStatus.setColour (juce::Label::textColourId, col::text.withAlpha (0.75f));
        updateStatus.setFont (juce::Font (12.0f));
        addAndMakeVisible (updateStatus);

        if (UpdatePrefs::getCheckEnabled())
            juce::Timer::callAfterDelay (1500, [safe = juce::Component::SafePointer<SetupPanel> (this)]
            {
                if (safe != nullptr) safe->startUpdateCheck (false);
            });

        for (auto* b : { "A", "B", "C", "D" }) bankBox.addItem (b, bankBox.getNumItems() + 1);
        bankBox.setSelectedId (1);
        addAndMakeVisible (bankBox);
        for (int i = 1; i <= 64; ++i) patBox.addItem (juce::String (i), i);
        patBox.setSelectedId (1);
        addAndMakeVisible (patBox);
        patBtn.setButtonText ("LOAD PATTERN");
        patBtn.onClick = [this]
        {
            int bank = bankBox.getSelectedId() - 1;
            int prog = (bank % 2) * 64 + (patBox.getSelectedId() - 1);
            proc.sendPatternChange (bank / 2, prog);
        };
        addAndMakeVisible (patBtn);

        for (auto def : { std::pair<juce::Label*, const char*> { &lDev, "MIDI OUTPUT (direct to your USB-MIDI interface)" },
                          { &lDevIn, "MIDI INPUT (external keyboard / controller; right-click any knob for MIDI learn)" },
                          { &lK1, "KEYS 1 / GLOBAL CH" }, { &lK2, "KEYS 2 CH" }, { &lDr, "DRUM PARTS CH" },
                          { &lPat, "PATTERN SELECT" }, { &lHelp, "" } })
        {
            def.first->setText (def.second, juce::dontSendNotification);
            def.first->setFont (juce::Font (12.0f, juce::Font::bold));
            addAndMakeVisible (def.first);
        }
        lHelp.setFont (juce::Font (12.0f));
        lHelp.setColour (juce::Label::textColourId, col::text.withAlpha (0.7f));
        lHelp.setText ("On the ESX-1 (Global mode): set MIDI FILTER P/C/E/N all enabled, CLOCK = EXT or AUTO, "
                       "and keep the CC Assign map at factory defaults. Channels here must match the ESX Global channels.",
                       juce::dontSendNotification);
        lHelp.setJustificationType (juce::Justification::topLeft);
    }

    void refreshDevices()
    {
        auto fill = [] (juce::ComboBox& box, const juce::StringArray& names,
                        const juce::String& cur, const char* noneLabel)
        {
            box.clear();
            box.addItem (noneLabel, 1);
            int id = 2;
            for (auto& n : names) box.addItem (n, id++);
            box.setSelectedId (1, juce::dontSendNotification);
            for (int i = 0; i < box.getNumItems(); ++i)
                if (cur.isNotEmpty() && box.getItemText (i) == cur)
                    box.setSelectedId (box.getItemId (i), juce::dontSendNotification);
        };
        fill (midiBox,   proc.getMidiOutputNames(), proc.getMidiOutputName(), "(host output only)");
        fill (midiInBox, proc.getMidiInputNames(),  proc.getMidiInputName(),  "(host input only)");
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (16);
        lDev.setBounds (r.removeFromTop (18));
        auto devRow = r.removeFromTop (30);
        refreshBtn.setBounds (devRow.removeFromRight (90).reduced (4, 2));
        midiBox.setBounds (devRow.reduced (0, 2));
        r.removeFromTop (6);
        lDevIn.setBounds (r.removeFromTop (18));
        midiInBox.setBounds (r.removeFromTop (30).withTrimmedRight (98).reduced (0, 2));
        r.removeFromTop (10);
        auto chRow = r.removeFromTop (48);
        auto third = chRow.getWidth() / 3;
        auto c1 = chRow.removeFromLeft (third).reduced (4);
        lK1.setBounds (c1.removeFromTop (18)); chK1.setBounds (c1.removeFromTop (26).withWidth (80));
        auto c2 = chRow.removeFromLeft (third).reduced (4);
        lK2.setBounds (c2.removeFromTop (18)); chK2.setBounds (c2.removeFromTop (26).withWidth (80));
        auto c3 = chRow.reduced (4);
        lDr.setBounds (c3.removeFromTop (18)); chDr.setBounds (c3.removeFromTop (26).withWidth (80));
        r.removeFromTop (10);
        clockToggle.setBounds (r.removeFromTop (28));
        r.removeFromTop (10);
        lPat.setBounds (r.removeFromTop (18));
        auto patRow = r.removeFromTop (30);
        bankBox.setBounds (patRow.removeFromLeft (70).reduced (2));
        patBox.setBounds (patRow.removeFromLeft (70).reduced (2));
        patBtn.setBounds (patRow.removeFromLeft (140).reduced (2));
        r.removeFromTop (14);
        syncBtn.setBounds (r.removeFromTop (40).withWidth (320));
        r.removeFromTop (12);
        updateToggle.setBounds (r.removeFromTop (28));
        auto updRow = r.removeFromTop (30);
        updateBtn.setBounds (updRow.removeFromLeft (120).reduced (0, 2));
        updateStatus.setBounds (updRow.reduced (8, 0));
        r.removeFromTop (10);
        lHelp.setBounds (r);
    }

    void startUpdateCheck (bool interactive)
    {
        updateStatus.setText ("Checking GitHub...", juce::dontSendNotification);
        updateBtn.setEnabled (false);
        juce::Component::SafePointer<SetupPanel> safe (this);
        std::thread ([safe, interactive]
        {
            auto result = UpdateChecker::checkLatest();
            juce::MessageManager::callAsync ([safe, interactive, result]
            {
                if (safe == nullptr) return;
                safe->updateBtn.setEnabled (true);
                safe->updateStatus.setText (result.message, juce::dontSendNotification);
                if (result.updateAvailable)
                {
                    safe->updateStatus.setColour (juce::Label::textColourId, col::led);
                    juce::AlertWindow::showAsync (
                        juce::MessageBoxOptions()
                            .withIconType (juce::MessageBoxIconType::InfoIcon)
                            .withTitle ("ESXiminator update")
                            .withMessage (result.message + "\n\nOpen the download page?")
                            .withButton ("Download")
                            .withButton ("Later"),
                        [result] (int btn)
                        {
                            if (btn == 1) UpdateChecker::openDownloadPage (result);
                        });
                }
                else
                {
                    safe->updateStatus.setColour (juce::Label::textColourId, col::amber);
                    if (interactive && result.ok)
                        juce::AlertWindow::showMessageBoxAsync (
                            juce::MessageBoxIconType::InfoIcon, "ESXiminator", result.message);
                    else if (interactive && ! result.ok)
                        juce::AlertWindow::showMessageBoxAsync (
                            juce::MessageBoxIconType::WarningIcon, "ESXiminator", result.message);
                }
            });
        }).detach();
    }

    ESXiminatorProcessor& proc;
    juce::ComboBox midiBox, midiInBox, chK1, chK2, chDr, bankBox, patBox;
    juce::TextButton refreshBtn, syncBtn, patBtn, updateBtn;
    juce::ToggleButton clockToggle, updateToggle;
    juce::Label lDev, lDevIn, lK1, lK2, lDr, lPat, lHelp, updateStatus;
};

// ================= MOD tab (automation effects) =================
class ModRow : public juce::Component, private juce::Timer
{
public:
    ModRow (ESXiminatorProcessor& p, int slotIndex) : proc (p), idx (slotIndex)
    {
        using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
        using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
        using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

        onBtn.setButtonText (juce::String (idx + 1));
        addAndMakeVisible (onBtn);
        onAtt = std::make_unique<BA> (proc.apvts, modID (idx, "on"), onBtn);

        auto fillBox = [this] (juce::ComboBox& b, const juce::StringArray& items)
        {
            for (int i = 0; i < items.size(); ++i) b.addItem (items[i], i + 1);
            addAndMakeVisible (b);
        };
        fillBox (targetBox, proc.getTargetNames());
        targetAtt = std::make_unique<CA> (proc.apvts, modID (idx, "target"), targetBox);

        juce::StringArray waves; for (auto* w : modWaves) waves.add (w);
        fillBox (waveBox, waves);
        waveAtt = std::make_unique<CA> (proc.apvts, modID (idx, "wave"), waveBox);

        syncBtn.setButtonText ("SYNC");
        addAndMakeVisible (syncBtn);
        syncAtt = std::make_unique<BA> (proc.apvts, modID (idx, "sync"), syncBtn);
        syncBtn.onStateChange = [this] { updateRateEnablement(); };

        juce::StringArray divs; for (auto& d : modDivs) divs.add (d.name);
        fillBox (divBox, divs);
        divAtt = std::make_unique<CA> (proc.apvts, modID (idx, "div"), divBox);

        auto setupSlider = [this] (juce::Slider& s, const juce::String& suffix)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 44, 13);
            s.setTextValueSuffix (suffix);
            addAndMakeVisible (s);
        };
        setupSlider (hzSlider, " Hz");
        setupSlider (depthSlider, "%");
        setupSlider (loSlider, {});
        setupSlider (hiSlider, {});
        setupSlider (phaseSlider, {});
        hzAtt    = std::make_unique<SA> (proc.apvts, modID (idx, "hz"), hzSlider);
        depthAtt = std::make_unique<SA> (proc.apvts, modID (idx, "depth"), depthSlider);
        loAtt    = std::make_unique<SA> (proc.apvts, modID (idx, "lo"), loSlider);
        hiAtt    = std::make_unique<SA> (proc.apvts, modID (idx, "hi"), hiSlider);
        phaseAtt = std::make_unique<SA> (proc.apvts, modID (idx, "phase"), phaseSlider);

        updateRateEnablement();
        startTimerHz (25);
    }

    void updateRateEnablement()
    {
        const bool sync = syncBtn.getToggleState();
        divBox.setEnabled (sync);
        hzSlider.setEnabled (! sync);
        divBox.setAlpha (sync ? 1.0f : 0.4f);
        hzSlider.setAlpha (sync ? 0.4f : 1.0f);
    }

    void timerCallback() override
    {
        auto v = proc.modOut[idx].load();
        if (std::abs (v - lastMeter) > 0.002f) { lastMeter = v; repaint (meterArea); }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::panelHi.withAlpha (0.45f));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f);

        auto m = meterArea.toFloat().reduced (2.0f);
        g.setColour (col::ledOff);
        g.fillRoundedRectangle (m, 2.0f);
        if (lastMeter >= 0.0f)
        {
            g.setColour (col::led);
            g.fillRoundedRectangle (m.removeFromBottom (m.getHeight() * lastMeter), 2.0f);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (4, 2);
        onBtn.setBounds (r.removeFromLeft (30).reduced (1, 14));
        targetBox.setBounds (r.removeFromLeft (260).reduced (2, 14));
        waveBox.setBounds (r.removeFromLeft (92).reduced (2, 14));
        syncBtn.setBounds (r.removeFromLeft (86).reduced (2, 14));
        divBox.setBounds (r.removeFromLeft (74).reduced (2, 14));
        hzSlider.setBounds (r.removeFromLeft (58));
        depthSlider.setBounds (r.removeFromLeft (58));
        loSlider.setBounds (r.removeFromLeft (58));
        hiSlider.setBounds (r.removeFromLeft (58));
        phaseSlider.setBounds (r.removeFromLeft (58));
        r.removeFromLeft (8);
        meterArea = r.removeFromLeft (20);
    }

private:
    ESXiminatorProcessor& proc;
    int idx;
    juce::ToggleButton onBtn, syncBtn;
    juce::ComboBox targetBox, waveBox, divBox;
    juce::Slider hzSlider, depthSlider, loSlider, hiSlider, phaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   onAtt, syncAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAtt, waveAtt, divAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   hzAtt, depthAtt, loAtt, hiAtt, phaseAtt;
    juce::Rectangle<int> meterArea;
    float lastMeter = -1.0f;
};

class ModPanel : public juce::Component
{
public:
    explicit ModPanel (ESXiminatorProcessor& p) : proc (p)
    {
        static const char* const hdrs[] = { "ON", "TARGET", "WAVE", "RATE MODE", "DIVISION",
                                            "FREE Hz", "DEPTH", "LO", "HI", "PHASE", "OUT" };
        static constexpr int hdrX[]     = { 6, 38, 298, 392, 478, 554, 612, 670, 728, 784, 844 };
        for (int i = 0; i < 11; ++i)
        {
            auto* l = headers.add (new juce::Label ({}, hdrs[i]));
            l->setFont (juce::Font (10.0f, juce::Font::bold));
            l->setColour (juce::Label::textColourId, col::text.withAlpha (0.6f));
            l->setJustificationType (juce::Justification::centredLeft);
            l->getProperties().set ("x", hdrX[i]);
            addAndMakeVisible (l);
        }
        for (int i = 0; i < numModSlots; ++i)
            addAndMakeVisible (rows.add (new ModRow (proc, i)));

        rateLabel.setText ("SEND RATE - MIDI DIN saturates near 260 NRPN/sec across all slots", juce::dontSendNotification);
        rateLabel.setFont (juce::Font (11.0f, juce::Font::bold));
        rateLabel.setColour (juce::Label::textColourId, col::text.withAlpha (0.85f));
        addAndMakeVisible (rateLabel);

        rateSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        rateSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 18);
        rateSlider.setRange (5.0, 60.0, 1.0);
        rateSlider.setTextValueSuffix (" Hz/slot");
        rateSlider.setValue (proc.modRateHz.load(), juce::dontSendNotification);
        rateSlider.onValueChange = [this] { proc.modRateHz = (int) rateSlider.getValue(); };
        addAndMakeVisible (rateSlider);

        help.setText ("Each slot sweeps its target between LO and HI. DEPTH blends against the knob's own value "
                      "(0% = knob only, 100% = full sweep). Safe ranges (right-click any knob) still clamp the "
                      "result, so a modulated cutoff can never pass the limit you set.",
                      juce::dontSendNotification);
        help.setFont (juce::Font (11.0f));
        help.setColour (juce::Label::textColourId, col::text.withAlpha (0.55f));
        help.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (help);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 6);
        auto hdr = r.removeFromTop (14);
        for (auto* l : headers)
            l->setBounds ((int) l->getProperties()["x"], hdr.getY(), 80, 14);
        for (auto* row : rows)
            row->setBounds (r.removeFromTop (56));
        r.removeFromTop (6);
        rateLabel.setBounds (r.removeFromTop (16));
        rateSlider.setBounds (r.removeFromTop (22).withWidth (430));
        r.removeFromTop (4);
        help.setBounds (r);
    }

private:
    ESXiminatorProcessor& proc;
    juce::OwnedArray<ModRow> rows;
    juce::OwnedArray<juce::Label> headers;
    juce::Label rateLabel, help;
    juce::Slider rateSlider;
};

// ================= Editor shell =================
ESXiminatorEditor::ESXiminatorEditor (ESXiminatorProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    top = std::make_unique<TopStrip> (proc,
            [this] { showOverlay (modPage.get()); },
            [this] { showOverlay (setupPage.get()); });
    edit   = std::make_unique<PartEdit> (proc);
    steps  = std::make_unique<StepKeys> (proc);
    matrix = std::make_unique<PartMatrix> (proc, [this] (int part)
             {
                 edit->build (part);
                 steps->setPart (part);
             });
    fx     = std::make_unique<FxRack> (proc);

    for (auto* c : std::initializer_list<juce::Component*> { top.get(), matrix.get(), edit.get(), fx.get(), steps.get() })
        addAndMakeVisible (c);

    modPage   = std::make_unique<ModPanel> (proc);
    setupPage = std::make_unique<SetupPanel> (proc);
    addChildComponent (*modPage);
    addChildComponent (*setupPage);
    closeOverlay.onClick = [this] { showOverlay (nullptr); };
    addChildComponent (closeOverlay);

    setSize (1180, 726);
}

ESXiminatorEditor::~ESXiminatorEditor() { setLookAndFeel (nullptr); }

void ESXiminatorEditor::showOverlay (juce::Component* c)
{
    if (overlay != nullptr) overlay->setVisible (false);
    overlay = c;
    if (overlay != nullptr) { overlay->setVisible (true); overlay->toFront (true); }
    closeOverlay.setVisible (overlay != nullptr);
    closeOverlay.toFront (false);
    resized();
}

void ESXiminatorEditor::paint (juce::Graphics& g)
{
    // red chassis
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient body (col::body.brighter (0.10f), 0, 0, col::bodyDark, 0, r.getBottom(), false);
    g.setGradientFill (body);
    g.fillAll();

    // brushed strip across the top, as on the hardware
    auto strip = r.removeFromTop (52.0f);
    juce::ColourGradient metal (col::metal.brighter (0.15f), 0, strip.getY(),
                                col::metal.darker (0.5f), 0, strip.getBottom(), false);
    g.setGradientFill (metal);
    g.fillRect (strip);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (0, strip.getBottom(), r.getRight(), strip.getBottom(), 1.5f);

    g.setColour (col::bodyDark);
    g.setFont (juce::Font (26.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("ESXiminator", 16, 8, 320, 34, juce::Justification::centredLeft);

    // valve window - decoration only: Valve Force is not reachable over MIDI
    auto tubes = juce::Rectangle<float> (r.getRight() - 296.0f, 6.0f, 282.0f, 40.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (tubes, 4.0f);
    for (int i = 0; i < 2; ++i)
    {
        auto t = juce::Rectangle<float> (tubes.getX() + 16.0f + i * 40.0f, tubes.getY() + 7.0f, 26.0f, 26.0f);
        g.setColour (col::amber.withAlpha (0.30f)); g.fillEllipse (t.expanded (4.0f));
        g.setColour (col::amber.withAlpha (0.85f)); g.fillEllipse (t);
        g.setColour (col::led);                     g.fillEllipse (t.reduced (7.0f));
    }
    g.setColour (col::silk.withAlpha (0.75f));
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText ("VALVE FORCE (not MIDI controllable)", tubes.withTrimmedLeft (104.0f),
                juce::Justification::centredLeft);
}

void ESXiminatorEditor::resized()
{
    auto r = getLocalBounds().withTrimmedTop (52).reduced (8, 6);

    top->setBounds (r.removeFromTop (86));
    r.removeFromTop (6);
    steps->setBounds (r.removeFromBottom (128));
    r.removeFromBottom (6);

    matrix->setBounds (r.removeFromLeft (188));
    r.removeFromLeft (6);
    fx->setBounds (r.removeFromRight (326));
    r.removeFromLeft (6);
    edit->setBounds (r);

    if (overlay != nullptr)
    {
        auto ov = getLocalBounds().withTrimmedTop (52).reduced (8, 6);
        closeOverlay.setBounds (ov.removeFromTop (24).removeFromRight (80));
        overlay->setBounds (ov);
    }
}
