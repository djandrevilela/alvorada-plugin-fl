#include "PluginEditor.h"

#include "Util.h"

using namespace harmonia;

namespace {
const juce::Colour kBg(0xff14161f), kPanel(0xff1d2130), kEdge(0xff2d3348), kText(0xffe2e8f0), kMuted(0xff8a94a8);
const juce::Colour kPartColour[kParts] = {juce::Colour(0xff63b3ed), juce::Colour(0xffb794f4), juce::Colour(0xfff6ad55), juce::Colour(0xff4fd1c5)};
const char* kPartTitles[kParts] = {"Melodia", "Contra-melodia", "Arpejo", "Baixo & Sub"};

juce::String pidS(const char* base, int part) { return juce::String(base) + juce::String(part); }

void bumpParam(juce::AudioProcessorValueTreeState& a, const juce::String& id, int delta, int lo, int hi) {
    if (auto* p = a.getParameter(id)) {
        int cur = (int) std::lround(a.getRawParameterValue(id)->load());
        int next = cur + delta;
        if (next > hi) next = lo;
        if (next < lo) next = hi;
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1((float) next));
        p->endChangeGesture();
    }
}
}  // namespace

// =============================================================== MidiDragHandle
void MidiDragHandle::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1);
    g.setColour(juce::Colour(0xff4fd1c5).withAlpha(0.16f));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(juce::Colour(0xff4fd1c5));
    g.drawRoundedRectangle(r, 7.0f, 1.2f);
    g.setColour(kText);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText(text, getLocalBounds(), juce::Justification::centred);
}

void MidiDragHandle::mouseDrag(const juce::MouseEvent& e) {
    if (dragging || e.getDistanceFromDragStart() < 6 || !makeFile) return;
    auto f = makeFile();
    if (!f.existsAsFile()) return;
    dragging = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles({f.getFullPathName()}, false, this, [this] { dragging = false; });
}

// =============================================================== VariationBox
VariationBox::VariationBox(juce::AudioProcessorValueTreeState& a, const juce::String& id) : apvts(a), paramId(id) {
    addAndMakeVisible(label);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, kText);
    prev.setButtonText(TR("\xE2\x97\x82"));
    next.setButtonText(TR("\xE2\x96\xB8"));
    prev.onClick = [this] { bump(-1); };
    next.onClick = [this] { bump(+1); };
    addAndMakeVisible(prev);
    addAndMakeVisible(next);
    att = std::make_unique<juce::ParameterAttachment>(*apvts.getParameter(id), [this](float v) {
        label.setText(TR("Variação ") + juce::String((int) std::lround(v)), juce::dontSendNotification);
    });
    att->sendInitialUpdate();
}
void VariationBox::bump(int d) { bumpParam(apvts, paramId, d, 1, 16); }
void VariationBox::resized() {
    auto r = getLocalBounds();
    next.setBounds(r.removeFromRight(34));
    r.removeFromRight(4);
    prev.setBounds(r.removeFromRight(34));
    label.setBounds(r);
}

// =============================================================== PartStrip
PartStrip::PartStrip(AlvoradaProcessor& p, int idx) : proc(p), part(idx) {
    const auto col = kPartColour[idx];
    title.setText(kPartTitles[idx], juce::dontSendNotification);
    title.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, col);
    addAndMakeVisible(title);
    onTg.setButtonText("Ativa");
    addAndMakeVisible(onTg);
    bAtt.push_back(std::make_unique<BA>(proc.apvts, pidS("on", idx), onTg));

    for (auto* h : {&genHeader, &synthHeader}) {
        h->setColour(juce::Label::textColourId, kMuted);
        h->setFont(juce::FontOptions(11.5f, juce::Font::bold));
        addAndMakeVisible(*h);
    }
    genHeader.setText(TR("GERADOR (só notas que funcionam)"), juce::dontSendNotification);
    synthHeader.setText(TR("SOM"), juce::dontSendNotification);

    switch (idx) {
        case 0:
            addSlider("m_dir", TR("Descida  \xE2\x86\x94  Subida"));
            addSlider("m_cx", TR("Simples  \xE2\x86\x94  Complexa"));
            addSlider("m_pl", TR("Pluck  \xE2\x86\x94  Lead"));
            addCombo("m_oct", "Oitava (-1 / 0 / +1)");
            items.push_back({own(new VariationBox(proc.apvts, "m_var")), 28});
            break;
        case 1:
            addCombo("c_pos", TR("Posição"));
            addSlider("c_den", TR("Notas longas  \xE2\x86\x94  Mais notas"));
            items.push_back({own(new VariationBox(proc.apvts, "c_var")), 28});
            break;
        case 2: {
            addCombo("a_pat", TR("Padrão"));
            addCombo("a_rate", TR("Velocidade"));
            addCombo("a_oct", TR("Oitavas"));
            addSlider("a_gate", TR("Notas curtas  \xE2\x86\x94  Longas"));
            items.push_back({own(new VariationBox(proc.apvts, "a_var")), 28});
            auto* b = own(new juce::TextButton(TR("\xE2\x9C\xA8 Arp Future Bass (1 clique)")));
            b->onClick = [this] { proc.applyFutureBassArp(); };
            items.push_back({b, 30});
            break;
        }
        default:
            addCombo("b_pat", TR("Padrão"));
            addToggle("b_sub", TR("Camada sub (oitava abaixo)"));
            addToggle("b_low", TR("Registo grave"));
            items.push_back({own(new VariationBox(proc.apvts, "b_var")), 28});
            break;
    }

    // ---- synth: preset + 6 macros
    addAndMakeVisible(presetBox);
    juce::String lastCat;
    int i = 0;
    for (auto& pr : alvorada::presets()) {
        if (lastCat != pr.category) { presetBox.addSectionHeading(pr.category); lastCat = pr.category; }
        presetBox.addItem(pr.name, i + 1);
        ++i;
    }
    presetBox.onChange = [this] {
        int id = presetBox.getSelectedId();
        if (id > 0) proc.loadPreset(part, id - 1);
    };
    prevPre.setButtonText(TR("\xE2\x97\x82"));
    nextPre.setButtonText(TR("\xE2\x96\xB8"));
    prevPre.onClick = [this] {
        int n = (int) alvorada::presets().size();
        proc.loadPreset(part, (presetBox.getSelectedId() - 2 + n) % n);
    };
    nextPre.onClick = [this] {
        int n = (int) alvorada::presets().size();
        proc.loadPreset(part, presetBox.getSelectedId() % n);
    };
    addAndMakeVisible(prevPre);
    addAndMakeVisible(nextPre);

    const char* ids[6] = {"bri", "att", "tail", "wid", "sub", "vol"};
    const char* names[6] = {"Brilho", "Ataque", "Cauda", "Largura", "Sub", "Volume"};
    for (int k = 0; k < 6; ++k) {
        knobs[k].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knobs[k].setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        knobs[k].setColour(juce::Slider::rotarySliderFillColourId, col);
        addAndMakeVisible(knobs[k]);
        sAtt.push_back(std::make_unique<SA>(proc.apvts, pidS(ids[k], idx), knobs[k]));
        knobLabels[k].setText(names[k], juce::dontSendNotification);
        knobLabels[k].setJustificationType(juce::Justification::centred);
        knobLabels[k].setFont(juce::FontOptions(12.0f));
        knobLabels[k].setColour(juce::Label::textColourId, kMuted);
        addAndMakeVisible(knobLabels[k]);
    }
    muteTg.setButtonText("Mute");
    duckTg.setButtonText("Sidechain");
    addAndMakeVisible(muteTg);
    addAndMakeVisible(duckTg);
    bAtt.push_back(std::make_unique<BA>(proc.apvts, pidS("mute", idx), muteTg));
    bAtt.push_back(std::make_unique<BA>(proc.apvts, pidS("duck", idx), duckTg));

    drag.text = TR("Arrastar MIDI desta parte");
    drag.makeFile = [this] {
        auto f = juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile(proc.suggestedFileName(juce::String(kPartTitles[part]).replaceCharacter(' ', '_').replace("&", "e")) + ".mid");
        return proc.writeMidiFile(f, 1 << part) ? f : juce::File();
    };
    addAndMakeVisible(drag);
    syncPreset();
}

void PartStrip::addSlider(const char* id, const juce::String& label) {
    auto* s = own(new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
    auto* l = own(new juce::Label({}, label));
    l->setFont(juce::FontOptions(12.0f));
    l->setColour(juce::Label::textColourId, kMuted);
    l->attachToComponent(s, false);
    s->setColour(juce::Slider::trackColourId, kPartColour[part]);
    sAtt.push_back(std::make_unique<SA>(proc.apvts, id, *s));
    items.push_back({s, 40});
}
void PartStrip::addCombo(const char* id, const juce::String& label) {
    auto* c = own(new juce::ComboBox());
    auto* l = own(new juce::Label({}, label));
    l->setFont(juce::FontOptions(12.0f));
    l->setColour(juce::Label::textColourId, kMuted);
    l->attachToComponent(c, false);
    cAtt.push_back(std::make_unique<CA>(proc.apvts, id, *c));
    items.push_back({c, 44});
}
void PartStrip::addToggle(const char* id, const juce::String& text) {
    auto* t = own(new juce::ToggleButton(text));
    bAtt.push_back(std::make_unique<BA>(proc.apvts, id, *t));
    items.push_back({t, 26});
}

void PartStrip::syncPreset() {
    int idx = (int) proc.apvts.getRawParameterValue(pidS("pre", part))->load();
    if (presetBox.getSelectedId() != idx + 1) presetBox.setSelectedId(idx + 1, juce::dontSendNotification);
}

void PartStrip::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(2);
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(kPartColour[part].withAlpha(0.7f));
    g.drawRoundedRectangle(r, 10.0f, 1.3f);
}

void PartStrip::resized() {
    auto r = getLocalBounds().reduced(10);
    auto head = r.removeFromTop(28);
    onTg.setBounds(head.removeFromRight(70));
    title.setBounds(head);
    r.removeFromTop(4);
    genHeader.setBounds(r.removeFromTop(16));
    for (auto& it : items) {
        auto row = r.removeFromTop(it.h);
        if (dynamic_cast<juce::Slider*>(it.c) || dynamic_cast<juce::ComboBox*>(it.c)) row.removeFromTop(it.h == 40 ? 16 : 18);
        it.c->setBounds(row.withTrimmedBottom(2));
        r.removeFromTop(4);
    }
    // parte inferior: som
    auto bottom = r.removeFromBottom(getHeight() > 0 ? 330 : 0);
    synthHeader.setBounds(bottom.removeFromTop(16));
    auto pr = bottom.removeFromTop(28);
    nextPre.setBounds(pr.removeFromRight(30)); pr.removeFromRight(3);
    prevPre.setBounds(pr.removeFromRight(30)); pr.removeFromRight(4);
    presetBox.setBounds(pr);
    bottom.removeFromTop(6);
    const int kw = bottom.getWidth() / 3, kh = 84;
    for (int row = 0; row < 2; ++row) {
        auto rr = bottom.removeFromTop(kh);
        for (int c = 0; c < 3; ++c) {
            auto cell = rr.removeFromLeft(kw);
            knobLabels[row * 3 + c].setBounds(cell.removeFromBottom(16));
            knobs[row * 3 + c].setBounds(cell.reduced(6, 2));
        }
    }
    bottom.removeFromTop(4);
    auto tg = bottom.removeFromTop(24);
    muteTg.setBounds(tg.removeFromLeft(tg.getWidth() / 2));
    duckTg.setBounds(tg);
    bottom.removeFromTop(6);
    drag.setBounds(bottom.removeFromTop(30));
}

// =============================================================== ChordChip
ChordChip::ChordChip(int index, std::function<void(int, const std::string&)> onToken) : idx(index), tokenChanged(std::move(onToken)) {
    addAndMakeVisible(tokenBox);
    addAndMakeVisible(nameLabel);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    nameLabel.setInterceptsMouseClicks(false, false);
    tokenBox.onChange = [this] {
        int i = tokenBox.getSelectedItemIndex();
        if (i >= 0 && i < (int) pal.size()) tokenChanged(idx, pal[(size_t) i]);
    };
}
void ChordChip::setContent(const juce::String& name, const juce::String& roman, const std::string& token,
                           const std::vector<std::string>& palette, bool editable) {
    if (pal != palette) {
        pal = palette;
        tokenBox.clear(juce::dontSendNotification);
        int id = 1;
        for (auto& t : pal) tokenBox.addItem(U8(prettyToken(t)), id++);
    }
    auto it = std::find(pal.begin(), pal.end(), token);
    tokenBox.setSelectedItemIndex(it == pal.end() ? -1 : (int) (it - pal.begin()), juce::dontSendNotification);
    tokenBox.setEnabled(editable);
    tokenBox.setTextWhenNothingSelected(roman.isEmpty() ? juce::String("MIDI") : roman);
    nameLabel.setText(name, juce::dontSendNotification);
}
void ChordChip::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(2);
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(kEdge);
    g.drawRoundedRectangle(r, 8.0f, 1.0f);
}
void ChordChip::resized() {
    auto r = getLocalBounds().reduced(7);
    tokenBox.setBounds(r.removeFromTop(22));
    r.removeFromTop(2);
    nameLabel.setBounds(r);
}

// =============================================================== Editor
AlvoradaEditor::AlvoradaEditor(AlvoradaProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    lnf.setColourScheme(juce::LookAndFeel_V4::getMidnightColourScheme());
    setLookAndFeel(&lnf);
    setSize(1160, 810);

    auto setupCombo = [&](juce::ComboBox& cb, juce::Label& lab, const juce::String& text, const char* id) {
        addAndMakeVisible(cb);
        lab.setText(text, juce::dontSendNotification);
        lab.setColour(juce::Label::textColourId, kMuted);
        lab.setFont(juce::FontOptions(12.0f));
        lab.attachToComponent(&cb, false);
        addAndMakeVisible(lab);
        cAtt.push_back(std::make_unique<CA>(proc.apvts, id, cb));
    };
    setupCombo(keyBox, keyL, "Tom", "keyRoot");
    setupCombo(scaleBox, scaleL, "Escala", "scaleType");
    setupCombo(styleBox, styleL, TR("Estilo dos acordes"), "style");
    setupCombo(richBox, richL, "Acordes", "richness");
    setupCombo(barsBox, barsL, TR("Duração por acorde"), "bars");
    setupCombo(livePartBox, livePartL, TR("Teclado toca a parte"), "livePart");

    auto btn = [&](juce::TextButton& b, const juce::String& t, std::function<void()> fn) {
        b.setButtonText(t);
        b.onClick = std::move(fn);
        addAndMakeVisible(b);
    };
    btn(prevBtn, TR("\xE2\x97\x82 Acordes"), [this] { proc.generateProgression(-1); });
    btn(genBtn, "Gerar", [this] { proc.generateProgression(0); });
    btn(nextBtn, TR("Acordes \xE2\x96\xB8"), [this] { proc.generateProgression(+1); });
    btn(remBtn, TR("\xE2\x88\x92"), [this] { proc.removeSlot(); });
    btn(addBtn, "+", [this] { proc.addSlot(); });
    btn(exportBtn, TR("Exportar tudo (.mid)"), [this] {
        chooser = std::make_unique<juce::FileChooser>(TR("Guardar MIDI"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                                                             .getChildFile(proc.suggestedFileName("tudo") + ".mid"), "*.mid");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                                 juce::FileBrowserComponent::warnAboutOverwriting,
                             [this](const juce::FileChooser& fc) {
                                 auto f = fc.getResult();
                                 if (f != juce::File()) proc.writeMidiFile(f.withFileExtension("mid"), 0xF);
                             });
    });
    auditionTg.setButtonText(TR("\xE2\x96\xB6 Ouvir (loop)"));
    addAndMakeVisible(auditionTg);
    bAtt.push_back(std::make_unique<BA>(proc.apvts, "audition", auditionTg));
    liveLockTg.setButtonText("Scale lock no teclado");
    addAndMakeVisible(liveLockTg);
    bAtt.push_back(std::make_unique<BA>(proc.apvts, "liveLock", liveLockTg));
    duckSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    duckSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(duckSlider);
    sAtt.push_back(std::make_unique<SA>(proc.apvts, "duckAmt", duckSlider));
    duckL.setText(TR("Força do sidechain (pré-escuta)"), juce::dontSendNotification);
    duckL.setColour(juce::Label::textColourId, kMuted);
    duckL.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(duckL);

    dragAll.text = TR("Arrastar MIDI (tudo)  \xE2\x96\xB8  FL Studio");
    dragAll.makeFile = [this] {
        auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(proc.suggestedFileName("tudo") + ".mid");
        return proc.writeMidiFile(f, 0xF) ? f : juce::File();
    };
    addAndMakeVisible(dragAll);

    whyLabel.setColour(juce::Label::textColourId, kMuted);
    whyLabel.setFont(juce::FontOptions(12.5f));
    whyLabel.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(whyLabel);

    for (int i = 0; i < kParts; ++i) {
        strips.add(new PartStrip(proc, i));
        addAndMakeVisible(strips.getLast());
    }

    proc.addChangeListener(this);
    refresh();
    startTimerHz(8);
}

AlvoradaEditor::~AlvoradaEditor() {
    stopTimer();
    proc.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void AlvoradaEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kText);
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.drawText("Alvorada", 16, 8, 200, 32, juce::Justification::centredLeft);
    g.setColour(kMuted);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(TR("Melodia, contra-melodia, arpejo e baixo \xE2\x80\x94 teoria musical, sem aleatoriedade nem IA. "
                  "Larga aqui um .mid de acordes (ex.: do Harmonia) para tocar sobre eles."),
               160, 14, 800, 20, juce::Justification::centredLeft);
}

void AlvoradaEditor::resized() {
    auto r = getLocalBounds().reduced(16);
    r.removeFromTop(34);
    auto row1 = r.removeFromTop(46).withTrimmedTop(18);
    const int gap = 10, w = (row1.getWidth() - gap * 4) / 5;
    for (auto* c : {&keyBox, &scaleBox, &styleBox, &richBox, &barsBox}) {
        c->setBounds(row1.removeFromLeft(w));
        row1.removeFromLeft(gap);
    }
    r.removeFromTop(6);
    auto row2 = r.removeFromTop(30);
    prevBtn.setBounds(row2.removeFromLeft(110)); row2.removeFromLeft(6);
    genBtn.setBounds(row2.removeFromLeft(80)); row2.removeFromLeft(6);
    nextBtn.setBounds(row2.removeFromLeft(110)); row2.removeFromLeft(14);
    remBtn.setBounds(row2.removeFromLeft(32)); row2.removeFromLeft(4);
    addBtn.setBounds(row2.removeFromLeft(32)); row2.removeFromLeft(20);
    auditionTg.setBounds(row2.removeFromLeft(140));
    dragAll.setBounds(row2.removeFromRight(260)); row2.removeFromRight(8);
    exportBtn.setBounds(row2.removeFromRight(160));
    r.removeFromTop(6);

    auto chipRow = r.removeFromTop(62);
    const int cn = juce::jmax(1, chips.size());
    const int cw = juce::jmin(chipRow.getWidth() / cn, 200);
    for (auto* c : chips) c->setBounds(chipRow.removeFromLeft(cw));
    whyLabel.setBounds(r.removeFromTop(34));

    auto foot = r.removeFromBottom(34);
    livePartBox.setBounds(foot.removeFromLeft(200).withTrimmedTop(0));
    foot.removeFromLeft(220);
    liveLockTg.setBounds(foot.removeFromLeft(190));
    duckL.setBounds(foot.removeFromLeft(220));
    duckSlider.setBounds(foot.removeFromLeft(220));
    // o rótulo do teclado fica acima da caixa: reserva espaço
    r.removeFromBottom(4);

    const int sw = (r.getWidth() - 3 * 10) / kParts;
    for (auto* s : strips) {
        s->setBounds(r.removeFromLeft(sw));
        r.removeFromLeft(10);
    }
}

void AlvoradaEditor::refresh() {
    auto v = proc.getView();
    if (!v) return;
    const auto tokens = proc.getTokens();
    const auto& palette = chordPalette(proc.isMinorMode());
    const int n = (int) v->names.size();
    if (n != chips.size()) {
        chips.clear();
        for (int i = 0; i < n; ++i) {
            chips.add(new ChordChip(i, [this](int idx, const std::string& t) { proc.setToken(idx, t); }));
            addAndMakeVisible(chips.getLast());
        }
        resized();
    }
    const bool editable = !proc.usingImportedChords();
    for (int i = 0; i < n; ++i)
        chips[i]->setContent(U8(v->names[(size_t) i]), U8(v->romans[(size_t) i]),
                             i < (int) tokens.size() && editable ? tokens[(size_t) i] : std::string(), palette, editable);
    whyLabel.setText(proc.getWhyText(), juce::dontSendNotification);
}

void AlvoradaEditor::timerCallback() {
    for (auto* s : strips) s->syncPreset();
}

bool AlvoradaEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    for (auto& f : files) if (f.endsWithIgnoreCase(".mid") || f.endsWithIgnoreCase(".midi")) return true;
    return false;
}

void AlvoradaEditor::filesDropped(const juce::StringArray& files, int, int) {
    for (auto& path : files)
        if (proc.importChordsFromMidi(juce::File(path))) return;
}
