#include "Theory.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace harmonia {

const char* scaleName(ScaleType t) {
    switch (t) {
        case ScaleType::Major: return "Maior";
        case ScaleType::NaturalMinor: return "Menor natural";
        case ScaleType::HarmonicMinor: return "Menor harmónica";
        case ScaleType::MelodicMinor: return "Menor melódica";
        case ScaleType::Dorian: return "Dórico";
        case ScaleType::Phrygian: return "Frígio";
        case ScaleType::Lydian: return "Lídio";
        case ScaleType::Mixolydian: return "Mixolídio";
    }
    return "";
}

bool isMajorFamily(ScaleType t) {
    return t == ScaleType::Major || t == ScaleType::Lydian || t == ScaleType::Mixolydian;
}

std::array<int, 7> Scale::intervals() const {
    switch (type) {
        case ScaleType::Major: return {0, 2, 4, 5, 7, 9, 11};
        case ScaleType::NaturalMinor: return {0, 2, 3, 5, 7, 8, 10};
        case ScaleType::HarmonicMinor: return {0, 2, 3, 5, 7, 8, 11};
        case ScaleType::MelodicMinor: return {0, 2, 3, 5, 7, 9, 11};
        case ScaleType::Dorian: return {0, 2, 3, 5, 7, 9, 10};
        case ScaleType::Phrygian: return {0, 1, 3, 5, 7, 8, 10};
        case ScaleType::Lydian: return {0, 2, 4, 6, 7, 9, 11};
        case ScaleType::Mixolydian: return {0, 2, 4, 5, 7, 9, 10};
    }
    return {0, 2, 4, 5, 7, 9, 11};
}

std::array<bool, 12> Scale::pitchClassMask() const {
    std::array<bool, 12> m{};
    for (int iv : intervals()) m[(size_t) mod12(root + iv)] = true;
    return m;
}

bool Scale::containsPc(int pc) const { return pitchClassMask()[(size_t) mod12(pc)]; }

int Scale::degreeOfPc(int pc) const {
    auto iv = intervals();
    for (int i = 0; i < 7; ++i)
        if (mod12(root + iv[(size_t) i]) == mod12(pc)) return i;
    return -1;
}

int Scale::pcOfDegree(int degree) const {
    auto iv = intervals();
    return mod12(root + iv[(size_t) (((degree % 7) + 7) % 7)]);
}

static const char* kSharps[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char* kFlats[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

std::string noteName(int pc, bool preferFlats) { return (preferFlats ? kFlats : kSharps)[mod12(pc)]; }

bool keyPrefersFlats(int rootPc, bool majorFamily) {
    static const std::set<int> flatMajor = {5, 10, 3, 8, 1};
    if (majorFamily) return flatMajor.count(mod12(rootPc)) > 0;
    int rel = mod12(rootPc + 3);
    return flatMajor.count(rel) > 0 || rel == 6;
}

std::string midiNoteName(int midi, bool preferFlats) {
    return noteName(midi, preferFlats) + std::to_string(midi / 12 - 1);
}

std::vector<int> Chord::intervals() const {
    using R = Richness;
    switch (quality) {
        case Quality::Major:
            switch (richness) {
                case R::Triad: return {0, 4, 7};
                case R::Add9: return {0, 4, 7, 14};
                case R::Seventh: return {0, 4, 7, 11};
                case R::Ninth: return {0, 4, 7, 11, 14};
            }
            break;
        case Quality::Minor:
            switch (richness) {
                case R::Triad: return {0, 3, 7};
                case R::Add9: return {0, 3, 7, 14};
                case R::Seventh: return {0, 3, 7, 10};
                case R::Ninth: return {0, 3, 7, 10, 14};
            }
            break;
        case Quality::Dim:
            return (richness == R::Triad || richness == R::Add9) ? std::vector<int>{0, 3, 6} : std::vector<int>{0, 3, 6, 10};
        case Quality::Aug:
            return (richness == R::Triad || richness == R::Add9) ? std::vector<int>{0, 4, 8} : std::vector<int>{0, 4, 8, 10};
        case Quality::Dom:
            switch (richness) {
                case R::Triad: return {0, 4, 7};
                case R::Add9: return {0, 4, 7, 14};
                case R::Seventh: return {0, 4, 7, 10};
                case R::Ninth: return {0, 4, 7, 10, 14};
            }
            break;
        case Quality::Sus4:
            switch (richness) {
                case R::Triad: return {0, 5, 7};
                case R::Add9: return {0, 5, 7, 14};
                case R::Seventh: return {0, 5, 7, 10};
                case R::Ninth: return {0, 5, 7, 10, 14};
            }
            break;
    }
    return {0, 4, 7};
}

std::vector<int> Chord::pitchClasses() const {
    std::vector<int> out;
    for (int iv : intervals()) {
        int pc = mod12(root + iv);
        if (std::find(out.begin(), out.end(), pc) == out.end()) out.push_back(pc);
    }
    return out;
}

std::string Chord::suffix() const {
    using R = Richness;
    switch (quality) {
        case Quality::Major: return richness == R::Triad ? "" : richness == R::Add9 ? "add9" : richness == R::Seventh ? "maj7" : "maj9";
        case Quality::Minor: return richness == R::Triad ? "m" : richness == R::Add9 ? "m(add9)" : richness == R::Seventh ? "m7" : "m9";
        case Quality::Dim: return (richness == R::Triad || richness == R::Add9) ? "dim" : "m7b5";
        case Quality::Aug: return (richness == R::Triad || richness == R::Add9) ? "aug" : "7#5";
        case Quality::Dom: return richness == R::Triad ? "" : richness == R::Add9 ? "add9" : richness == R::Seventh ? "7" : "9";
        case Quality::Sus4: return richness == R::Triad ? "sus4" : richness == R::Add9 ? "sus4(add9)" : richness == R::Seventh ? "7sus4" : "9sus4";
    }
    return "";
}

std::string Chord::name(bool preferFlats) const { return noteName(root, preferFlats) + suffix(); }

namespace {
struct Pat {
    const char* suffix;
    std::vector<int> iv;
};
const std::vector<Pat>& patterns() {
    static const std::vector<Pat> p = {
        {"", {0, 4, 7}},        {"m", {0, 3, 7}},         {"dim", {0, 3, 6}},      {"aug", {0, 4, 8}},
        {"sus2", {0, 2, 7}},    {"sus4", {0, 5, 7}},      {"5", {0, 7}},           {"maj7", {0, 4, 7, 11}},
        {"m7", {0, 3, 7, 10}},  {"7", {0, 4, 7, 10}},     {"m7b5", {0, 3, 6, 10}}, {"dim7", {0, 3, 6, 9}},
        {"add9", {0, 2, 4, 7}}, {"m(add9)", {0, 2, 3, 7}}, {"maj9", {0, 2, 4, 7, 11}}, {"m9", {0, 2, 3, 7, 10}},
        {"9", {0, 2, 4, 7, 10}}, {"6", {0, 4, 7, 9}},     {"m6", {0, 3, 7, 9}},    {"7sus4", {0, 5, 7, 10}},
        {"sus4(add9)", {0, 2, 5, 7}}, {"7#5", {0, 4, 8, 10}}};
    return p;
}
}  // namespace

std::optional<ChordLabel> identifyChord(const std::vector<int>& midiNotes) {
    if (midiNotes.empty()) return std::nullopt;
    std::vector<int> sorted = midiNotes;
    std::sort(sorted.begin(), sorted.end());
    std::set<int> pcs;
    for (int n : sorted) pcs.insert(mod12(n));
    // raízes candidatas: primeiro a nota mais grave, depois as restantes por ordem ascendente
    std::vector<int> roots;
    for (int n : sorted)
        if (std::find(roots.begin(), roots.end(), mod12(n)) == roots.end()) roots.push_back(mod12(n));
    for (int r : roots) {
        std::set<int> rel;
        for (int pc : pcs) rel.insert(mod12(pc - r));
        for (const auto& p : patterns()) {
            std::set<int> ps;
            for (int iv : p.iv) ps.insert(mod12(iv));
            if (ps == rel) return ChordLabel{r, p.suffix};
        }
    }
    return std::nullopt;
}

std::string romanFor(int semis, bool lowerCase) {
    static const char* t[12] = {"I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII"};
    std::string s = t[mod12(semis)];
    if (lowerCase)
        for (auto& c : s)
            if (c == 'I' || c == 'V') c = (char) std::tolower((unsigned char) c);
    return s;
}

}  // namespace harmonia
