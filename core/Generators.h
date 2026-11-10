#pragma once
// Geradores determinísticos de melodia, contra-melodia, arpejo e baixo (teoria musical, sem aleatoriedade).
// Tempo medido em tempos (beats) de 4/4.
#include <vector>

#include "Theory.h"

namespace alvorada {

struct ChordSpan {
    harmonia::Chord chord;
    double start = 0;
    double length = 4;
};

struct GenNote {
    int pitch = 60;
    double start = 0;
    double length = 1;
    int velocity = 90;
};

double totalLength(const std::vector<ChordSpan>& spans);
const harmonia::Chord* chordAt(const std::vector<ChordSpan>& spans, double t);

// ---- Melodia ----
struct MelodyParams {
    double direction = 0.3;    // -1 (desce) .. +1 (sobe)
    double complexity = 0.4;   // 0 simples .. 1 complexa (ritmo e saltos)
    double pluckLead = 0.3;    // 0 pluck (curto) .. 1 lead (ligado)
    int octave = 0;            // -1, 0, +1
    int variation = 0;         // escolhe outro conjunto de modelos rítmicos (lista curada)
};
std::vector<GenNote> generateMelody(const harmonia::Scale&, const std::vector<ChordSpan>&, const MelodyParams&);

// ---- Contra-melodia ----
struct CounterParams {
    bool above = false;        // false: harmonia abaixo da melodia; true: descante acima
    double density = 0.5;      // 0 notas longas .. 1 mais notas
    int variation = 0;         // alterna preferência terças/sextas
};
std::vector<GenNote> generateCounter(const harmonia::Scale&, const std::vector<ChordSpan>&,
                                     const std::vector<GenNote>& melody, const CounterParams&);

// ---- Baixo ----
enum class BassPattern { Offbeat, Rolling, SidechainReady, OctaveBounce, SubLong, Syncopated };
constexpr int kNumBassPatterns = 6;
struct BassParams {
    BassPattern pattern = BassPattern::Offbeat;
    bool subLayer = false;     // acrescenta a oitava abaixo
    bool lowOctave = false;    // registo mais grave
    int variation = 0;         // ímpar: nota de aproximação ao acorde seguinte
};
std::vector<GenNote> generateBass(const harmonia::Scale&, const std::vector<ChordSpan>&, const BassParams&);

// ---- Arpejo ----
enum class ArpPattern { Up, Down, UpDown, Alternate, FutureBass };
constexpr int kNumArpPatterns = 5;
enum class ArpRate { Eighth, Sixteenth, SixteenthTriplet };
struct ArpParams {
    ArpPattern pattern = ArpPattern::FutureBass;
    ArpRate rate = ArpRate::Sixteenth;
    int octaves = 2;           // 1..3
    double gate = 0.55;        // 0.1..1
    int variation = 0;         // par/ímpar: fase inicial do padrão
};
std::vector<GenNote> generateArp(const std::vector<ChordSpan>&, const ArpParams&);

// ---- Importar acordes de notas MIDI (ex.: ficheiro exportado do Harmonia) ----
std::vector<ChordSpan> chordsFromNotes(const std::vector<GenNote>& notes);

}  // namespace alvorada
