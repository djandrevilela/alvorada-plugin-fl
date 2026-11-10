#pragma once
// Harmonia core — teoria musical básica (C++17 puro, determinístico).
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace harmonia {

inline int mod12(int x) { return ((x % 12) + 12) % 12; }

enum class ScaleType { Major, NaturalMinor, HarmonicMinor, MelodicMinor, Dorian, Phrygian, Lydian, Mixolydian };
constexpr int kNumScaleTypes = 8;

const char* scaleName(ScaleType t);
bool isMajorFamily(ScaleType t);  // Maior, Lídio, Mixolídio

struct Scale {
    int root = 0;  // pitch class 0..11
    ScaleType type = ScaleType::Major;

    std::array<int, 7> intervals() const;
    std::array<bool, 12> pitchClassMask() const;
    bool containsPc(int pc) const;
    bool contains(int midi) const { return containsPc(mod12(midi)); }
    int degreeOfPc(int pc) const;  // 0..6, ou -1
    int pcOfDegree(int degree) const;
};

std::string noteName(int pc, bool preferFlats);
bool keyPrefersFlats(int rootPc, bool majorFamily);
std::string midiNoteName(int midi, bool preferFlats);  // C4 = 60

enum class Quality { Major, Minor, Dim, Aug, Dom, Sus4 };
enum class Richness { Triad, Add9, Seventh, Ninth };

struct Chord {
    int root = 0;
    Quality quality = Quality::Major;
    Richness richness = Richness::Triad;

    std::vector<int> intervals() const;      // a partir da raiz, ascendente (pode incluir 14 = nona)
    std::vector<int> pitchClasses() const;   // únicas, pela ordem do acorde
    std::string suffix() const;
    std::string name(bool preferFlats) const;
};

struct ChordLabel {
    int root = 0;
    std::string suffix;
    std::string name(bool preferFlats) const { return noteName(root, preferFlats) + suffix; }
};

// Identifica um acorde a partir de notas MIDI simultâneas (correspondência exata de classes de altura).
std::optional<ChordLabel> identifyChord(const std::vector<int>& midiNotes);

// Numeral romano relativo à tónica ("I", "bVII", "vi"...).
std::string romanFor(int semitonesAboveTonic, bool lowerCase);

}  // namespace harmonia
