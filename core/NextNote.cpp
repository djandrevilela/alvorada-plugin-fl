#include "NextNote.h"

#include <algorithm>
#include <cmath>

namespace harmonia {

int snapToPitchClasses(const std::array<bool, 12>& mask, int midi, SnapMode mode, int prevNote) {
    if (mask[(size_t) mod12(midi)]) return midi;
    bool any = false;
    for (bool b : mask) any = any || b;
    if (!any) return midi;
    for (int d = 1; d <= 12; ++d) {
        bool dn = mask[(size_t) mod12(midi - d)] && midi - d >= 0;
        bool up = mask[(size_t) mod12(midi + d)] && midi + d <= 127;
        if (mode == SnapMode::Down) { if (dn) return midi - d; if (up) return midi + d; continue; }
        if (mode == SnapMode::Up) { if (up) return midi + d; if (dn) return midi - d; continue; }
        if (dn && up) {
            // empate: segue a direção do movimento melódico (por defeito, sobe)
            bool goUp = prevNote < 0 || midi >= prevNote;
            return goUp ? midi + d : midi - d;
        }
        if (dn) return midi - d;
        if (up) return midi + d;
    }
    return midi;
}

int snapToScale(const Scale& scale, int midi, SnapMode mode, int prevNote) {
    return snapToPitchClasses(scale.pitchClassMask(), midi, mode, prevNote);
}

namespace {
struct Scored {
    double score = 0;
    std::vector<std::pair<double, std::string>> parts;
    void add(double v, const std::string& why) { score += v; parts.emplace_back(v, why); }
    std::string reason() const {
        auto p = parts;
        std::stable_sort(p.begin(), p.end(), [](auto& a, auto& b) { return std::abs(a.first) > std::abs(b.first); });
        std::string r;
        int n = 0;
        bool neg = score < 0;
        for (auto& x : p) {
            if ((neg && x.first < 0) || (!neg && x.first > 0.5)) {
                if (n) r += " · ";
                r += x.second;
                if (++n == 2) break;
            }
        }
        return r;
    }
};
}  // namespace

std::vector<NoteSuggestion> suggestNextNotes(const Scale& scale, const std::vector<int>& recent, const Chord* chord,
                                             int maxResults) {
    std::array<bool, 12> chordMask{};
    std::vector<int> chordPcs;
    if (chord) {
        chordPcs = chord->pitchClasses();
        for (int pc : chordPcs) chordMask[(size_t) pc] = true;
    }
    bool hasSeventh = false;
    int seventhPc = -1;
    if (chord)
        for (int iv : chord->intervals())
            if (iv == 10 || iv == 11) { hasSeventh = true; seventhPc = mod12(chord->root + iv); }

    std::vector<std::pair<NoteSuggestion, int>> all;  // sugestão, distância
    auto push = [&](int midi, const Scored& s, int dist) {
        all.push_back({NoteSuggestion{midi, s.score, s.reason()}, dist});
    };

    if (recent.empty()) {
        for (int deg : {0, 4, 2}) {
            int pc = scale.pcOfDegree(deg);
            int midi = 60 + mod12(pc - 60);
            Scored s;
            if (deg == 0) s.add(3.0, "Tónica: ponto de repouso, ótimo para começar");
            if (deg == 4) s.add(2.5, "Quinta: estável e aberta");
            if (deg == 2) s.add(2.2, "Terça: define a cor maior/menor");
            if (chord && chordMask[(size_t) pc]) s.add(1.5, "Nota do acorde atual");
            push(midi, s, 0);
        }
    } else {
        const int last = recent.back();
        const int lastPc = mod12(last);
        const int tonic = scale.root;
        for (int midi = last - 9; midi <= last + 9; ++midi) {
            if (midi < 0 || midi > 127 || !scale.contains(midi)) continue;
            const int pc = mod12(midi), iv = midi - last, ai = std::abs(iv);
            Scored s;
            if (ai == 0) {
                s.add(-0.5, "Repetição");
                if (recent.size() >= 2 && recent[recent.size() - 2] == last) s.add(-2.0, "Terceira repetição seguida");
            } else if (ai <= 2) s.add(3.0, "Grau conjunto (suave e cantável)");
            else if (ai <= 4) s.add(2.0, "Salto de terça (consonante)");
            else if (ai == 5 || ai == 7) s.add(1.0, "Salto de quarta/quinta");
            else if (ai == 6) s.add(-1.0, "Trítono: salto instável");
            else s.add(0.2, "Salto largo");

            if (recent.size() >= 2) {
                int li = last - recent[recent.size() - 2];
                if (std::abs(li) >= 5) {
                    if (ai >= 1 && ai <= 2 && (iv > 0) != (li > 0)) s.add(2.5, "Depois de um salto, volta em sentido contrário");
                    else if (ai > 0 && (iv > 0) == (li > 0)) s.add(-1.5, "Evita continuar o salto na mesma direção");
                }
            }
            // tendências melódicas
            if (lastPc == mod12(tonic + 11) && pc == tonic && iv > 0) s.add(4.0, "A sensível resolve na tónica");
            if (lastPc == mod12(tonic + 5) && ai >= 1 && ai <= 2 && iv < 0 && (pc == mod12(tonic + 4) || pc == mod12(tonic + 3)))
                s.add(1.5, "O 4.º grau tende a descer para a terça");
            if (hasSeventh && lastPc == seventhPc && iv < 0 && ai <= 2) s.add(2.5, "A sétima do acorde resolve para baixo");

            if (chord) {
                if (chordMask[(size_t) pc]) s.add(3.0, "Nota do acorde atual");
                else
                    for (int t : chordPcs)
                        if (mod12(pc - t) == 1) { s.add(-3.0, "Evitar: semitono acima de uma nota do acorde"); break; }
            } else {
                int deg = scale.degreeOfPc(pc);
                if (deg == 0 || deg == 2 || deg == 4) s.add(0.8, "Grau estável da escala");
            }
            s.add(-0.05 * ai, "");
            push(midi, s, ai);
        }
    }

    std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
        if (a.first.score != b.first.score) return a.first.score > b.first.score;
        if (a.second != b.second) return a.second < b.second;
        return a.first.midi < b.first.midi;
    });
    std::vector<NoteSuggestion> out;
    std::array<bool, 12> used{};
    for (auto& p : all) {
        int pc = mod12(p.first.midi);
        if (used[(size_t) pc]) continue;
        used[(size_t) pc] = true;
        out.push_back(p.first);
        if ((int) out.size() >= maxResults) break;
    }
    return out;
}

}  // namespace harmonia
