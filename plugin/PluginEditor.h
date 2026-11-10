#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <memory>

#include "PluginProcessor.h"

class MidiDragHandle : public juce::Component {
public:
    std::function<juce::File()> makeFile;
    juce::String text;
    void paint(juce::Graphics&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override { dragging = false; }

private:
    bool dragging = false;
};

// Controlo "Variação n  ◂ ▸" ligado a um parâmetro inteiro.
class VariationBox : public juce::Component {
public:
    VariationBox(juce::AudioProcessorValueTreeState&, const juce::String& id);
    void resized() override;

private:
    void bump(int delta);
    juce::AudioProcessorValueTreeState& apvts;
    juce::String paramId;
    juce::Label label;
    juce::TextButton prev, next;
    std::unique_ptr<juce::ParameterAttachment> att;
};

class PartStrip : public juce::Component {
public:
    PartStrip(AlvoradaProcessor&, int partIndex);
    void paint(juce::Graphics&) override;
    void resized() override;
    void syncPreset();

private:
    struct Item { juce::Component* c; int h; };
    template <typename T> T* own(T* c) { owned.add(c); addAndMakeVisible(c); return c; }
    void addSlider(const char* id, const juce::String& label);
    void addCombo(const char* id, const juce::String& label);
    void addToggle(const char* id, const juce::String& text);

    AlvoradaProcessor& proc;
    int part;
    juce::OwnedArray<juce::Component> owned;
    std::vector<Item> items;
    juce::Label title;
    juce::ToggleButton onTg, muteTg, duckTg;
    juce::ComboBox presetBox;
    juce::TextButton prevPre, nextPre;
    juce::Slider knobs[6];
    juce::Label knobLabels[6];
    MidiDragHandle drag;
    juce::Label genHeader, synthHeader;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sAtt;
    std::vector<std::unique_ptr<CA>> cAtt;
    std::vector<std::unique_ptr<BA>> bAtt;
};

class ChordChip : public juce::Component {
public:
    ChordChip(int index, std::function<void(int, const std::string&)> onToken);
    void paint(juce::Graphics&) override;
    void resized() override;
    void setContent(const juce::String& name, const juce::String& roman, const std::string& token,
                    const std::vector<std::string>& palette, bool editable);

    juce::ComboBox tokenBox;
    juce::Label nameLabel;

private:
    int idx;
    std::vector<std::string> pal;
    std::function<void(int, const std::string&)> tokenChanged;
};

class AlvoradaEditor : public juce::AudioProcessorEditor,
                       private juce::ChangeListener,
                       private juce::Timer,
                       public juce::FileDragAndDropTarget {
public:
    explicit AlvoradaEditor(AlvoradaProcessor&);
    ~AlvoradaEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override;
    void refresh();

    AlvoradaProcessor& proc;
    juce::LookAndFeel_V4 lnf;

    juce::ComboBox keyBox, scaleBox, styleBox, richBox, barsBox, livePartBox;
    juce::Label keyL, scaleL, styleL, richL, barsL, livePartL;
    juce::TextButton prevBtn, genBtn, nextBtn, addBtn, remBtn, exportBtn;
    juce::ToggleButton auditionTg, liveLockTg;
    juce::Slider duckSlider;
    juce::Label duckL, whyLabel;
    MidiDragHandle dragAll;
    juce::OwnedArray<ChordChip> chips;
    juce::OwnedArray<PartStrip> strips;
    std::unique_ptr<juce::FileChooser> chooser;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<CA>> cAtt;
    std::vector<std::unique_ptr<BA>> bAtt;
    std::vector<std::unique_ptr<SA>> sAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlvoradaEditor)
};
