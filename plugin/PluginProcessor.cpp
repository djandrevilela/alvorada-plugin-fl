#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "Util.h"

using namespace harmonia;
using namespace alvorada;

namespace {
const char* kKeyNames[12] = {"C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
const double kBarsValues[4] = {0.5, 1.0, 2.0, 4.0};
const char* kPartNames[kParts] = {"Melodia", "Contra-melodia", "Arpejo", "Baixo & Sub"};

juce::String pid(const char* base, int part) { return juce::String(base) + juce::String(part); }

void setParam(juce::AudioProcessorValueTreeState& a, const juce::String& id, float realValue) {
    if (auto* p = a.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(realValue));
        p->endChangeGesture();
    }
}

float duckCurve(double beat, float amt) {
    double ph = beat - std::floor(beat);
    float x = (float) std::min(1.0, ph / 0.5);
    float s = x * x * (3.f - 2.f * x);
    return 1.f - amt * (1.f - s);
}
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout AlvoradaProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto choice = [&](const String& id, const String& name, StringArray items, int def) {
        l.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, items, def));
    };
    auto flt = [&](const String& id, const String& name, float lo, float hi, float def) {
        l.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name, NormalisableRange<float>(lo, hi), def));
    };
    auto boo = [&](const String& id, const String& name, bool def) { l.add(std::make_unique<AudioParameterBool>(ParameterID{id, 1}, name, def)); };
    auto intp = [&](const String& id, const String& name, int lo, int hi, int def) {
        l.add(std::make_unique<AudioParameterInt>(ParameterID{id, 1}, name, lo, hi, def));
    };

    StringArray keys, scales, stys, presetNames;
    for (auto* k : kKeyNames) keys.add(k);
    for (int i = 0; i < kNumScaleTypes; ++i) scales.add(TR(scaleName((ScaleType) i)));
    for (auto& s : harmonia::styles()) stys.add(TR(s.name));
    for (auto& p : presets()) presetNames.add(String(p.category) + " - " + p.name);

    choice("keyRoot", TR("Tom"), keys, 9);
    choice("scaleType", TR("Escala"), scales, 1);
    choice("style", TR("Estilo"), stys, 0);
    choice("richness", TR("Acordes"), StringArray{TR("Automático (do estilo)"), TR("Tríades"), "Add9", TR("Sétimas"), "Nonas"}, 0);
    choice("bars", TR("Duração por acorde"), StringArray{TR("1/2 compasso"), TR("1 compasso"), TR("2 compassos"), TR("4 compassos")}, 1);
    boo("audition", TR("Ouvir (loop)"), false);
    flt("duckAmt", TR("Sidechain (força)"), 0.f, 1.f, 0.8f);
    boo("liveLock", TR("Scale lock no teclado"), false);
    choice("livePart", TR("Teclado toca a parte"), StringArray{"Melodia", "Contra-melodia", "Arpejo", "Baixo & Sub"}, 0);

    // melodia
    flt("m_dir", TR("Descida/Subida"), -1.f, 1.f, 0.3f);
    flt("m_cx", TR("Simples/Complexa"), 0.f, 1.f, 0.4f);
    flt("m_pl", TR("Pluck/Lead"), 0.f, 1.f, 0.3f);
    choice("m_oct", TR("Oitava da melodia"), StringArray{"-1", "0", "+1"}, 1);
    intp("m_var", TR("Variação da melodia"), 1, 16, 1);
    // contra-melodia
    choice("c_pos", TR("Posição"), StringArray{"Abaixo da melodia", "Acima da melodia"}, 0);
    flt("c_den", TR("Densidade"), 0.f, 1.f, 0.5f);
    intp("c_var", TR("Variação da contra-melodia"), 1, 16, 1);
    // arpejo
    choice("a_pat", TR("Padrão do arpejo"), StringArray{"Subir", "Descer", "Subir/Descer", "Alternado", "Future Bass"}, 4);
    choice("a_rate", TR("Velocidade"), StringArray{"1/8", "1/16", "1/16 tercina"}, 1);
    choice("a_oct", TR("Oitavas"), StringArray{"1", "2", "3"}, 1);
    flt("a_gate", TR("Duração das notas"), 0.1f, 1.f, 0.55f);
    intp("a_var", TR("Variação do arpejo"), 1, 16, 1);
    // baixo
    choice("b_pat", TR("Padrão do baixo"),
           StringArray{"Offbeat", "Rolling", "Sidechain-ready", TR("Oitavas (bounce)"), "Sub longo", TR("Sincopado 3+3+2")}, 0);
    boo("b_sub", TR("Camada sub"), false);
    boo("b_low", TR("Oitava grave"), false);
    intp("b_var", TR("Variação do baixo"), 1, 16, 1);

    const char* defaults[kParts] = {"Supersaw Lead", "Warm Pad", "Arp Pluck", "Saw Bass"};
    for (int p = 0; p < kParts; ++p) {
        const int di = std::max(0, presetIndexByName(defaults[p]));
        const auto& d = presets()[(size_t) di];
        const String nm = kPartNames[p];
        boo(pid("on", p), TR("Ativar ") + nm, p != 1);
        choice(pid("pre", p), TR("Preset ") + nm, presetNames, di);
        flt(pid("bri", p), TR("Brilho ") + nm, 0.f, 1.f, d.bright);
        flt(pid("att", p), TR("Ataque ") + nm, 0.f, 1.f, d.attack);
        flt(pid("tail", p), TR("Cauda ") + nm, 0.f, 1.f, d.tail);
        flt(pid("wid", p), TR("Largura ") + nm, 0.f, 1.f, d.width);
        flt(pid("sub", p), TR("Sub ") + nm, 0.f, 1.f, d.sub);
        flt(pid("vol", p), TR("Volume ") + nm, 0.f, 1.f, d.vol);
        boo(pid("mute", p), TR("Mute ") + nm, false);
        boo(pid("duck", p), "Sidechain " + nm, p == 3);
    }
    return l;
}

AlvoradaProcessor::AlvoradaProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "ALVORADA", createLayout()) {
    for (auto& row : mapped) row.fill(-1);
    active.reserve(1024);
    for (int p = 0; p < kParts; ++p) {
        synths[(size_t) p].addSound(new AlvSound());
        for (int v = 0; v < 8; ++v) synths[(size_t) p].addVoice(new AlvVoice(partStates[(size_t) p]));
        auto raw = [&](const char* b) { return apvts.getRawParameterValue(pid(b, p)); };
        pp[(size_t) p] = {raw("pre"), raw("bri"), raw("att"), raw("tail"), raw("wid"), raw("sub"), raw("vol"), raw("mute"), raw("duck")};
    }
    pAudition = apvts.getRawParameterValue("audition");
    pDuckAmt = apvts.getRawParameterValue("duckAmt");
    pLiveLock = apvts.getRawParameterValue("liveLock");
    pLivePart = apvts.getRawParameterValue("livePart");
    pKey = apvts.getRawParameterValue("keyRoot");
    pScale = apvts.getRawParameterValue("scaleType");

    juce::StringArray regenIds{"keyRoot", "scaleType", "style", "richness", "bars", "m_dir", "m_cx", "m_pl", "m_oct", "m_var",
                               "c_pos", "c_den", "c_var", "a_pat", "a_rate", "a_oct", "a_gate", "a_var", "b_pat", "b_sub", "b_low", "b_var"};
    for (int p = 0; p < kParts; ++p) regenIds.add(pid("on", p));
    for (auto& id : regenIds) apvts.addParameterListener(id, this);
    regenListenIds = regenIds;

    lastMinor = isMinorMode();
    loadTemplate();
    regenerate();
}

AlvoradaProcessor::~AlvoradaProcessor() {
    cancelPendingUpdate();
    for (auto& id : regenListenIds) apvts.removeParameterListener(id, this);
}

// ------------------------------------------------------------ parâmetros / modelo
Scale AlvoradaProcessor::getScale() const {
    Scale s;
    s.root = juce::jlimit(0, 11, (int) pKey->load());
    s.type = (ScaleType) juce::jlimit(0, kNumScaleTypes - 1, (int) pScale->load());
    return s;
}
bool AlvoradaProcessor::isMinorMode() const { return !isMajorFamily(getScale().type); }
int AlvoradaProcessor::styleIndex() const { return juce::jlimit(0, (int) harmonia::styles().size() - 1, pi("style")); }

void AlvoradaProcessor::parameterChanged(const juce::String& id, float) {
    if (id == "style") repick = true;
    triggerAsyncUpdate();
}

void AlvoradaProcessor::handleAsyncUpdate() {
    bool minorNow = isMinorMode();
    if (repick.exchange(false) || minorNow != lastMinor) {
        lastMinor = minorNow;
        imported.reset();
        loadTemplate();
    }
    regenerate();
}

void AlvoradaProcessor::loadTemplate() {
    const auto& st = harmonia::styles()[(size_t) styleIndex()];
    const auto& tpls = isMinorMode() ? st.minor : st.major;
    int n = (int) tpls.size();
    variation = ((variation % n) + n) % n;
    juce::ScopedLock sl(modelLock);
    steps = parseProgression(tpls[(size_t) variation].tokens);
    whyText = tpls[(size_t) variation].why;
}

void AlvoradaProcessor::generateProgression(int delta) {
    imported.reset();
    variation += delta;
    loadTemplate();
    regenerate();
}

std::vector<std::string> AlvoradaProcessor::getTokens() const {
    juce::ScopedLock sl(modelLock);
    std::vector<std::string> t;
    for (auto& s : steps) t.push_back(s.token);
    return t;
}

void AlvoradaProcessor::setToken(int slot, const std::string& token) {
    auto p = parseToken(token);
    if (!p) return;
    {
        juce::ScopedLock sl(modelLock);
        if (slot < 0 || slot >= (int) steps.size()) return;
        steps[(size_t) slot] = *p;
        imported.reset();
        whyText = "Progressão editada por ti: as melodias e o baixo foram recalculados sobre os novos acordes.";
    }
    regenerate();
}

void AlvoradaProcessor::addSlot() {
    {
        juce::ScopedLock sl(modelLock);
        if (steps.size() >= 8) return;
        steps.push_back(*parseToken(isMinorMode() ? "i" : "I"));
        imported.reset();
    }
    regenerate();
}

void AlvoradaProcessor::removeSlot() {
    {
        juce::ScopedLock sl(modelLock);
        if (steps.size() <= 1) return;
        steps.pop_back();
        imported.reset();
    }
    regenerate();
}

juce::String AlvoradaProcessor::getWhyText() const {
    juce::ScopedLock sl(modelLock);
    if (imported) return TR("Acordes importados de um ficheiro MIDI. Carrega em \"Gerar\" para voltar aos modelos do estilo.");
    return U8(whyText);
}

std::vector<ChordSpan> AlvoradaProcessor::buildSpans() const {
    juce::ScopedLock sl(modelLock);
    if (imported) return *imported;
    std::vector<ChordSpan> out;
    Richness r;
    switch (pi("richness")) {
        case 1: r = Richness::Triad; break;
        case 2: r = Richness::Add9; break;
        case 3: r = Richness::Seventh; break;
        case 4: r = Richness::Ninth; break;
        default: r = harmonia::styles()[(size_t) styleIndex()].defaultRichness;
    }
    const double bpc = kBarsValues[juce::jlimit(0, 3, pi("bars"))] * 4.0;
    auto chords = realize(steps, getScale().root, r);
    for (size_t i = 0; i < chords.size(); ++i) out.push_back({chords[i], (double) i * bpc, bpc});
    return out;
}

void AlvoradaProcessor::regenerate() {
    auto d = std::make_shared<Patterns>();
    const Scale sc = getScale();
    d->flats = keyPrefersFlats(sc.root, isMajorFamily(sc.type));
    d->spans = buildSpans();
    d->loopLen = totalLength(d->spans);
    {
        juce::ScopedLock sl(modelLock);
        for (size_t i = 0; i < d->spans.size(); ++i) {
            d->names.push_back(d->spans[i].chord.name(d->flats));
            d->romans.push_back(!imported && i < steps.size() ? prettyToken(steps[i].token) : "");
        }
    }
    MelodyParams mp;
    mp.direction = pf("m_dir"); mp.complexity = pf("m_cx"); mp.pluckLead = pf("m_pl");
    mp.octave = pi("m_oct") - 1; mp.variation = pi("m_var") - 1;
    auto mel = generateMelody(sc, d->spans, mp);
    if (pb("on0")) d->parts[0] = mel;

    if (pb("on1")) {
        CounterParams cp;
        cp.above = pi("c_pos") == 1; cp.density = pf("c_den"); cp.variation = pi("c_var") - 1;
        d->parts[1] = generateCounter(sc, d->spans, mel, cp);
    }
    if (pb("on2")) {
        ArpParams ap;
        ap.pattern = (ArpPattern) juce::jlimit(0, kNumArpPatterns - 1, pi("a_pat"));
        ap.rate = (ArpRate) juce::jlimit(0, 2, pi("a_rate"));
        ap.octaves = pi("a_oct") + 1; ap.gate = pf("a_gate"); ap.variation = pi("a_var") - 1;
        d->parts[2] = generateArp(d->spans, ap);
    }
    if (pb("on3")) {
        BassParams bp;
        bp.pattern = (BassPattern) juce::jlimit(0, kNumBassPatterns - 1, pi("b_pat"));
        bp.subLayer = pb("b_sub"); bp.lowOctave = pb("b_low"); bp.variation = pi("b_var") - 1;
        d->parts[3] = generateBass(sc, d->spans, bp);
    }
    {
        juce::SpinLock::ScopedLockType sl(dataLock);
        graveyard.push_back(published);
        if (graveyard.size() > 4) graveyard.erase(graveyard.begin());
        published = d;
    }
    sendChangeMessage();
}

std::shared_ptr<const Patterns> AlvoradaProcessor::getView() const {
    juce::SpinLock::ScopedLockType sl(dataLock);
    return published;
}

void AlvoradaProcessor::loadPreset(int part, int idx) {
    if (part < 0 || part >= kParts || idx < 0 || idx >= (int) presets().size()) return;
    const auto& d = presets()[(size_t) idx];
    setParam(apvts, pid("pre", part), (float) idx);
    setParam(apvts, pid("bri", part), d.bright);
    setParam(apvts, pid("att", part), d.attack);
    setParam(apvts, pid("tail", part), d.tail);
    setParam(apvts, pid("wid", part), d.width);
    setParam(apvts, pid("sub", part), d.sub);
    setParam(apvts, pid("vol", part), d.vol);
}

void AlvoradaProcessor::applyFutureBassArp() {
    setParam(apvts, "a_pat", (float) (int) ArpPattern::FutureBass);
    setParam(apvts, "a_rate", (float) (int) ArpRate::Sixteenth);
    setParam(apvts, "a_oct", 1.f);
    setParam(apvts, "a_gate", 0.55f);
    setParam(apvts, "a_var", 1.f);
    setParam(apvts, "on2", 1.f);
    setParam(apvts, "mute2", 0.f);
    loadPreset(2, presetIndexByName("Future Pluck"));
    triggerAsyncUpdate();
}

bool AlvoradaProcessor::importChordsFromMidi(const juce::File& f) {
    juce::FileInputStream in(f);
    if (!in.openedOk()) return false;
    juce::MidiFile mf;
    if (!mf.readFrom(in) || mf.getTimeFormat() <= 0) return false;
    const double tpq = mf.getTimeFormat();
    std::vector<GenNote> notes;
    for (int t = 0; t < mf.getNumTracks(); ++t) {
        juce::MidiMessageSequence seq(*mf.getTrack(t));
        seq.updateMatchedPairs();
        for (int i = 0; i < seq.getNumEvents(); ++i) {
            auto* e = seq.getEventPointer(i);
            if (!e->message.isNoteOn()) continue;
            double s = e->message.getTimeStamp() / tpq;
            double en = e->noteOffObject ? e->noteOffObject->message.getTimeStamp() / tpq : s + 1.0;
            notes.push_back({e->message.getNoteNumber(), s, std::max(0.05, en - s), 90});
        }
    }
    auto spans = chordsFromNotes(notes);
    if (spans.empty()) return false;
    {
        juce::ScopedLock sl(modelLock);
        imported = spans;
    }
    regenerate();
    return true;
}

// ------------------------------------------------------------ áudio
void AlvoradaProcessor::prepareToPlay(double sr, int block) {
    for (int p = 0; p < kParts; ++p) {
        synths[(size_t) p].setCurrentPlaybackSampleRate(sr);
        tmp[(size_t) p].setSize(2, juce::jmax(block, 1024));
        partMidi[(size_t) p].ensureSize(4096);
    }
    outMidi.ensureSize(8192);
    seqRunning = false;
    lastAudition = false;
    internalBeat = 0.0;
}

void AlvoradaProcessor::sendAllNotesOff(juce::MidiBuffer& out) {
    for (auto& a : active) {
        auto m = juce::MidiMessage::noteOff(a.part + 1, a.note);
        partMidi[(size_t) a.part].addEvent(m, 0);
        out.addEvent(m, 0);
    }
    active.clear();
}

void AlvoradaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    {
        juce::SpinLock::ScopedTryLockType tl(dataLock);
        if (tl.isLocked()) audioData = published;
    }
    for (int p = 0; p < kParts; ++p) {
        auto& s = partStates[(size_t) p];
        const auto& q = pp[(size_t) p];
        s.preset = (int) q.preset->load();
        s.bright = q.bright->load(); s.attack = q.attack->load(); s.tail = q.tail->load();
        s.width = q.width->load(); s.sub = q.sub->load();
    }

    bool playing = false, hasPpq = false;
    double ppq = 0.0, bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            playing = pos->getIsPlaying();
            if (auto pp_ = pos->getPpqPosition()) { ppq = *pp_; hasPpq = true; }
            if (auto b = pos->getBpm()) bpm = *b;
        }
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const double bps = bpm / 60.0 / sr;

    outMidi.clear();
    for (auto& m : partMidi) m.clear();

    // relógio do sequenciador: anfitrião a tocar, ou "Ouvir (loop)" interno
    const bool audition = pAudition->load() > 0.5f;
    bool useClock = false;
    double p0 = 0.0;
    if (playing && hasPpq) { useClock = true; p0 = ppq; }
    else if (audition) {
        if (!lastAudition) auditionBeat = 0.0;
        useClock = true;
        p0 = auditionBeat;
        auditionBeat += n * bps;
    }
    lastAudition = audition;
    const double p1 = p0 + n * bps;
    double from = p0;
    if (useClock) {
        const bool cont = seqRunning && std::abs(p0 - expectedP0) <= 0.05;
        if (cont) from = expectedP0;
        else sendAllNotesOff(outMidi);
        expectedP0 = p1;
        seqRunning = true;
    } else if (seqRunning) {
        sendAllNotesOff(outMidi);
        seqRunning = false;
    }

    if (useClock && audioData && audioData->loopLen > 0.0) {
        const double L = audioData->loopLen;
        for (size_t i = 0; i < active.size();) {
            auto& a = active[i];
            if (a.offBeat < p1) {
                int off = juce::jlimit(0, n - 1, (int) ((a.offBeat - p0) / bps));
                auto m = juce::MidiMessage::noteOff(a.part + 1, a.note);
                partMidi[(size_t) a.part].addEvent(m, off);
                outMidi.addEvent(m, off);
                active[i] = active.back();
                active.pop_back();
            } else ++i;
        }
        const long long k0 = (long long) std::floor(from / L);
        for (long long k = k0; k <= k0 + 1; ++k)
            for (int p = 0; p < kParts; ++p) {
                for (const auto& gn : audioData->parts[(size_t) p]) {
                    const double abs = (double) k * L + gn.start;
                    if (abs < from || abs >= p1) continue;
                    int off = juce::jlimit(0, n - 1, (int) ((abs - p0) / bps));
                    auto m = juce::MidiMessage::noteOn(p + 1, gn.pitch, (juce::uint8) gn.velocity);
                    partMidi[(size_t) p].addEvent(m, off);
                    outMidi.addEvent(m, off);
                    if (active.size() < active.capacity()) active.push_back({p, gn.pitch, abs + gn.length});
                }
            }
    }

    // teclado ao vivo -> parte escolhida (com scale lock opcional)
    const int livePart = juce::jlimit(0, kParts - 1, (int) pLivePart->load());
    const bool lock = pLiveLock->load() > 0.5f;
    const Scale scale = getScale();
    for (const auto meta : midi) {
        auto m = meta.getMessage();
        const int pos = meta.samplePosition, ch = juce::jlimit(1, 16, m.getChannel());
        if (m.isNoteOn()) {
            int note = m.getNoteNumber();
            int out = lock ? snapToScale(scale, note, SnapMode::Nearest, -1) : note;
            out = juce::jlimit(0, 127, out);
            mapped[(size_t) ch - 1][(size_t) note] = out;
            auto mm = juce::MidiMessage::noteOn(ch, out, m.getVelocity());
            partMidi[(size_t) livePart].addEvent(mm, pos);
            outMidi.addEvent(mm, pos);
        } else if (m.isNoteOff()) {
            int note = m.getNoteNumber();
            int out = mapped[(size_t) ch - 1][(size_t) note];
            if (out < 0) out = note;
            mapped[(size_t) ch - 1][(size_t) note] = -1;
            auto mm = juce::MidiMessage::noteOff(ch, out, m.getVelocity());
            partMidi[(size_t) livePart].addEvent(mm, pos);
            outMidi.addEvent(mm, pos);
        } else {
            partMidi[(size_t) livePart].addEvent(m, pos);
            outMidi.addEvent(m, pos);
        }
    }

    // síntese por parte, com volume, mute e sidechain interno (pré-escuta)
    const double duckBeat0 = useClock ? p0 : (hasPpq ? ppq : internalBeat);
    internalBeat += n * bps;
    const float duckAmt = pDuckAmt->load();
    for (int p = 0; p < kParts; ++p) {
        auto& tb = tmp[(size_t) p];
        if (tb.getNumSamples() < n) tb.setSize(2, n, false, false, true);
        tb.clear(0, n);
        synths[(size_t) p].renderNextBlock(tb, partMidi[(size_t) p], 0, n);
        const float vol = pp[(size_t) p].vol->load();
        const float g = pp[(size_t) p].mute->load() > 0.5f ? 0.f : vol * vol * 1.3f;
        const bool duck = pp[(size_t) p].duck->load() > 0.5f && duckAmt > 0.f;
        for (int ch = 0; ch < 2 && ch < buffer.getNumChannels(); ++ch) {
            const float* src = tb.getReadPointer(ch);
            float* dst = buffer.getWritePointer(ch);
            for (int i = 0; i < n; ++i) dst[i] += src[i] * g * (duck ? duckCurve(duckBeat0 + i * bps, duckAmt) : 1.f);
        }
    }
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i) d[i] = std::tanh(d[i]);  // limitador suave de segurança
    }
    midi.swapWith(outMidi);
}

// ------------------------------------------------------------ MIDI export
juce::String AlvoradaProcessor::suggestedFileName(const juce::String& what) const {
    auto sc = getScale();
    return "Alvorada_" + what + "_" + U8(noteName(sc.root, keyPrefersFlats(sc.root, isMajorFamily(sc.type)))) +
           (isMajorFamily(sc.type) ? "maj" : "min");
}

bool AlvoradaProcessor::writeMidiFile(const juce::File& f, int partMask) const {
    auto d = getView();
    if (!d) return false;
    const int tpq = 960;
    juce::MidiMessageSequence conductor;
    conductor.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4));
    conductor.addEvent(juce::MidiMessage::tempoMetaEvent(500000));
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(tpq);
    mf.addTrack(conductor);
    bool any = false;
    for (int p = 0; p < kParts; ++p) {
        if (!(partMask & (1 << p)) || d->parts[(size_t) p].empty()) continue;
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::textMetaEvent(3, kPartNames[p]));
        for (const auto& n : d->parts[(size_t) p]) {
            seq.addEvent(juce::MidiMessage::noteOn(p + 1, n.pitch, (juce::uint8) n.velocity), n.start * tpq);
            seq.addEvent(juce::MidiMessage::noteOff(p + 1, n.pitch), (n.start + n.length) * tpq - 2);
        }
        seq.updateMatchedPairs();
        mf.addTrack(seq);
        any = true;
    }
    if (!any) return false;
    f.deleteFile();
    juce::FileOutputStream os(f);
    return os.openedOk() && mf.writeTo(os);
}

// ------------------------------------------------------------ estado
void AlvoradaProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = apvts.copyState();
    std::vector<ChordStep> st;
    std::optional<std::vector<ChordSpan>> imp;
    {
        juce::ScopedLock sl(modelLock);
        st = steps;
        imp = imported;
    }
    state.setProperty("progression", U8(joinTokens(st)), nullptr);
    state.setProperty("variation", variation, nullptr);
    juce::String impStr;
    if (imp)
        for (auto& s : *imp)
            impStr += juce::String(s.chord.root) + "," + juce::String((int) s.chord.quality) + "," + juce::String((int) s.chord.richness) + "," +
                      juce::String(s.start, 4) + "," + juce::String(s.length, 4) + ";";
    state.setProperty("imported", impStr, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void AlvoradaProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(apvts.state.getType())) return;
    auto vt = juce::ValueTree::fromXml(*xml);
    apvts.replaceState(vt);
    variation = (int) vt.getProperty("variation", 0);
    auto parsed = parseProgression(vt.getProperty("progression", "").toString().toStdString());
    std::vector<ChordSpan> imp;
    for (auto& rec : juce::StringArray::fromTokens(vt.getProperty("imported", "").toString(), ";", "")) {
        auto f = juce::StringArray::fromTokens(rec, ",", "");
        if (f.size() != 5) continue;
        ChordSpan s;
        s.chord.root = f[0].getIntValue();
        s.chord.quality = (Quality) f[1].getIntValue();
        s.chord.richness = (Richness) f[2].getIntValue();
        s.start = f[3].getDoubleValue();
        s.length = f[4].getDoubleValue();
        imp.push_back(s);
    }
    repick = false;
    lastMinor = isMinorMode();
    {
        juce::ScopedLock sl(modelLock);
        if (parsed.empty()) { /* modelo por omissão */ }
        else steps = parsed;
        imported = imp.empty() ? std::nullopt : std::optional<std::vector<ChordSpan>>(imp);
    }
    if (parsed.empty()) loadTemplate();
    regenerate();
}

juce::AudioProcessorEditor* AlvoradaProcessor::createEditor() { return new AlvoradaEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AlvoradaProcessor(); }
