// Teste de áudio offline (opcional): renderiza todos os presets e o sequenciador e verifica NaN, silêncio e clipping.
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"

static int fails = 0;

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    AlvoradaProcessor proc;
    const double sr = 44100.0;
    const int bs = 512;
    proc.setPlayConfigDetails(0, 2, sr, bs);
    proc.prepareToPlay(sr, bs);
    juce::AudioBuffer<float> buf(2, bs);

    std::printf("%-22s %8s %8s\n", "preset", "pico", "rms");
    const int N = (int) alvorada::presets().size();
    for (int i = 0; i < N; ++i) {
        proc.loadPreset(0, i);
        double sumSq = 0, peak = 0;
        long count = 0;
        bool bad = false;
        for (int b = 0; b < 220; ++b) {
            juce::MidiBuffer mb;
            if (b == 0) for (int n : {48, 60, 67}) mb.addEvent(juce::MidiMessage::noteOn(1, n, 0.8f), 0);
            if (b == 90) for (int n : {48, 60, 67}) mb.addEvent(juce::MidiMessage::noteOff(1, n, 0.f), 0);
            buf.clear();
            proc.processBlock(buf, mb);
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < bs; ++s) {
                    float x = buf.getSample(ch, s);
                    if (!std::isfinite(x)) bad = true;
                    peak = std::max(peak, (double) std::abs(x));
                    sumSq += x * x;
                    ++count;
                }
        }
        double rms = std::sqrt(sumSq / (double) count);
        bool ok = !bad && peak > 0.01 && peak < 1.0 && rms > 0.001;
        std::printf("%-22s %8.3f %8.4f %s\n", alvorada::presets()[(size_t) i].name, peak, rms, ok ? "" : "  <-- PROBLEMA");
        if (!ok) ++fails;
        proc.loadPreset(0, alvorada::presetIndexByName("Supersaw Lead"));
    }

    // sequenciador interno ("Ouvir"), todas as partes ligadas
    {
        proc.loadPreset(0, alvorada::presetIndexByName("Future Pluck"));
        for (int p = 0; p < kParts; ++p)
            if (auto* q = proc.apvts.getParameter("on" + juce::String(p))) q->setValueNotifyingHost(1.f);
        if (auto* q = proc.apvts.getParameter("audition")) q->setValueNotifyingHost(1.f);
        proc.applyFutureBassArp();
        // regenera sincronamente (o AsyncUpdater não corre sem message loop)
        proc.generateProgression(0);
        int noteOns[16] = {0};
        double peak = 0;
        bool bad = false;
        for (int b = 0; b < 1800; ++b) {  // ~20 s
            juce::MidiBuffer mb;
            buf.clear();
            proc.processBlock(buf, mb);
            for (const auto meta : mb) {
                auto m = meta.getMessage();
                if (m.isNoteOn()) ++noteOns[m.getChannel() - 1];
            }
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < bs; ++s) {
                    float x = buf.getSample(ch, s);
                    if (!std::isfinite(x)) bad = true;
                    peak = std::max(peak, (double) std::abs(x));
                }
        }
        std::printf("\nSequenciador (20 s): notas MIDI por parte  melodia=%d contra=%d arp=%d baixo=%d  pico=%.3f\n",
                    noteOns[0], noteOns[1], noteOns[2], noteOns[3], peak);
        if (bad || peak > 1.0 || peak < 0.01) ++fails;
        if (noteOns[0] == 0 || noteOns[2] == 0 || noteOns[3] == 0) ++fails;

        // exportação MIDI
        auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("alv_test.mid");
        bool wrote = proc.writeMidiFile(f, 0xF);
        std::printf("Exportar MIDI: %s (%lld bytes)\n", wrote ? "ok" : "FALHOU", (long long) f.getSize());
        if (!wrote) ++fails;
        // importar de volta como acordes (a faixa do baixo+melodia não é um acorde, mas não deve falhar nem crashar)
        proc.importChordsFromMidi(f);
    }

    std::printf(fails == 0 ? "\nTudo OK.\n" : "\n%d problema(s).\n", fails);
    return fails == 0 ? 0 : 1;
}
