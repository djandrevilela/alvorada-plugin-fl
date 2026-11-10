#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <memory>
#include <optional>

#include "Generators.h"
#include "NextNote.h"
#include "Presets.h"
#include "Progressions.h"
#include "SynthEngine.h"
#include "Theory.h"

constexpr int kParts = 4;  // 0 Melodia, 1 Contra-melodia, 2 Arpejo, 3 Baixo & Sub

// Dados imutáveis partilhados com a thread de áudio.
struct Patterns {
    std::array<std::vector<alvorada::GenNote>, kParts> parts;
    std::vector<alvorada::ChordSpan> spans;
    std::vector<std::string> names, romans;
    double loopLen = 0;
    bool flats = false;
};

class AlvoradaProcessor : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster,
                          private juce::AsyncUpdater,
                          private juce::AudioProcessorValueTreeState::Listener {
public:
    AlvoradaProcessor();
    ~AlvoradaProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Alvorada"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- API do editor ----
    juce::AudioProcessorValueTreeState apvts;

    harmonia::Scale getScale() const;
    bool isMinorMode() const;
    int styleIndex() const;

    void generateProgression(int variationDelta);  // 0 = recarregar modelo e sair do modo "importado"
    std::vector<std::string> getTokens() const;
    void setToken(int slot, const std::string& token);
    void addSlot();
    void removeSlot();
    bool usingImportedChords() const { return imported.has_value(); }
    juce::String getWhyText() const;
    int progressionVariation() const { return variation; }

    void loadPreset(int part, int presetIndex);  // define o preset e repõe os macros
    void applyFutureBassArp();                   // 1 clique: arpejo típico de future bass
    bool importChordsFromMidi(const juce::File&);

    std::shared_ptr<const Patterns> getView() const;
    bool writeMidiFile(const juce::File&, int partMask) const;
    juce::String suggestedFileName(const juce::String& what) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged(const juce::String&, float) override;
    void handleAsyncUpdate() override;
    void loadTemplate();
    std::vector<alvorada::ChordSpan> buildSpans() const;
    void regenerate();
    void sendAllNotesOff(juce::MidiBuffer& out);

    float pf(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    int pi(const juce::String& id) const { return (int) std::lround(pf(id)); }
    bool pb(const juce::String& id) const { return pf(id) > 0.5f; }

    // modelo (thread de mensagens)
    juce::CriticalSection modelLock;
    std::vector<harmonia::ChordStep> steps;
    std::optional<std::vector<alvorada::ChordSpan>> imported;
    std::string whyText;
    int variation = 0;
    bool lastMinor = true;
    std::atomic<bool> repick{false};
    juce::StringArray regenListenIds;

    mutable juce::SpinLock dataLock;
    std::shared_ptr<const Patterns> published, audioData;
    std::vector<std::shared_ptr<const Patterns>> graveyard;

    // áudio (ponteiros para parâmetros em cache: a thread de áudio não pode alocar Strings)
    struct PartParams { std::atomic<float> *preset, *bright, *attack, *tail, *width, *sub, *vol, *mute, *duck; };
    std::array<PartParams, kParts> pp{};
    std::atomic<float> *pAudition = nullptr, *pDuckAmt = nullptr, *pLiveLock = nullptr, *pLivePart = nullptr, *pKey = nullptr, *pScale = nullptr;
    std::array<PartState, kParts> partStates;
    std::array<juce::Synthesiser, kParts> synths;
    std::array<juce::AudioBuffer<float>, kParts> tmp;
    std::array<juce::MidiBuffer, kParts> partMidi;
    juce::MidiBuffer outMidi;

    struct Active { int part; int note; double offBeat; };
    std::vector<Active> active;
    bool seqRunning = false, lastAudition = false;
    double expectedP0 = 0.0, auditionBeat = 0.0, internalBeat = 0.0;
    std::array<std::array<int, 128>, 16> mapped{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlvoradaProcessor)
};
