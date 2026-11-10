#include "Generators.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace alvorada {

using namespace harmonia;

double totalLength(const std::vector<ChordSpan>& spans) {
    double e = 0;
    for (auto& c : spans) e = std::max(e, c.start + c.length);
    return e;
}

const Chord* chordAt(const std::vector<ChordSpan>& spans, double t) {
    for (auto& c : spans)
        if (t >= c.start - 1e-9 && t < c.start + c.length - 1e-9) return &c.chord;
    return nullptr;
}

namespace {

const ChordSpan* spanAt(const std::vector<ChordSpan>& spans, double t) {
    for (auto& c : spans)
        if (t >= c.start - 1e-9 && t < c.start + c.length - 1e-9) return &c;
    return nullptr;
}

bool isChordStart(const std::vector<ChordSpan>& spans, double t) {
    for (auto& c : spans)
        if (std::abs(c.start - t) < 1e-9) return true;
    return false;
}

std::array<bool, 12> maskOf(const Chord& c) {
    std::array<bool, 12> m{};
    for (int pc : c.pitchClasses()) m[(size_t) pc] = true;
    return m;
}

int mod4(int x) { return ((x % 4) + 4) % 4; }

struct R { int slot; int len; };
using Rhythm = std::vector<R>;

// Modelos rítmicos de 1 compasso (16 semicolcheias). Curados à mão: simples, médio, complexo.
const std::vector<std::vector<Rhythm>>& rhythmBank() {
    static const std::vector<std::vector<Rhythm>> b = {
        {Rhythm{{0, 8}, {8, 4}, {12, 4}}, Rhythm{{0, 4}, {4, 4}, {8, 8}}, Rhythm{{0, 6}, {6, 2}, {8, 4}, {12, 4}}, Rhythm{{0, 12}, {12, 4}}},
        {Rhythm{{0, 4}, {4, 2}, {6, 2}, {8, 4}, {12, 2}, {14, 2}}, Rhythm{{0, 6}, {6, 2}, {8, 2}, {10, 2}, {12, 4}},
         Rhythm{{0, 2}, {2, 2}, {4, 4}, {8, 6}, {14, 2}}, Rhythm{{0, 4}, {4, 4}, {8, 2}, {10, 2}, {12, 4}}},
        {Rhythm{{0, 3}, {3, 3}, {6, 2}, {8, 3}, {11, 3}, {14, 2}}, Rhythm{{0, 2}, {2, 2}, {4, 2}, {6, 2}, {8, 4}, {12, 2}, {14, 2}},
         Rhythm{{0, 3}, {3, 3}, {6, 4}, {10, 2}, {12, 2}, {14, 2}}, Rhythm{{0, 2}, {2, 2}, {4, 4}, {8, 2}, {10, 2}, {12, 3}, {15, 1}}}};
    return b;
}

}  // namespace

// ============================================================ MELODIA
std::vector<GenNote> generateMelody(const Scale& sc, const std::vector<ChordSpan>& spans, const MelodyParams& p) {
    std::vector<GenNote> out;
    if (spans.empty()) return out;
    const double total = totalLength(spans);
    const int bars = (int) std::ceil(total / 4.0 - 1e-9);
    const int lo = 60 + 12 * std::max(-1, std::min(1, p.octave)), hi = lo + 19;
    const double mid = 0.5 * (lo + hi), range = hi - lo;
    const double d = std::max(-1.0, std::min(1.0, p.direction));
    const double cxRaw = std::max(0.0, std::min(1.0, p.complexity));
    const double pl = std::max(0.0, std::min(1.0, p.pluckLead));
    const double cx = std::max(0.0, std::min(1.0, cxRaw + (0.5 - pl) * 0.2));  // pluck tende a ser mais denso
    const int bucket = cx < 0.34 ? 0 : cx < 0.67 ? 1 : 2;
    static const int phraseMap[4] = {0, 0, 1, 2};  // frases A A B C

    struct Sk { double start, len; bool anchor; bool barStart; int pitch; };
    std::vector<Sk> sk;
    for (int b = 0; b < bars; ++b) {
        const auto& rh = rhythmBank()[(size_t) bucket][(size_t) mod4(p.variation + phraseMap[b % 4] + b / 4)];
        for (const auto& r : rh) {
            double s = 4.0 * b + r.slot * 0.25;
            if (s >= total - 1e-9) continue;
            const ChordSpan* cs = spanAt(spans, s);
            if (!cs) continue;
            double e = std::min({s + r.len * 0.25, cs->start + cs->length, total});
            if (e - s < 0.0625) continue;
            sk.push_back({s, e - s, (r.slot % 4 == 0) || isChordStart(spans, s), r.slot == 0, 0});
        }
    }
    if (sk.empty()) return out;
    sk[0].anchor = true;

    // 1) âncoras: notas do acorde nos tempos fortes, perto do contorno-alvo
    const double phraseLen = std::min(16.0, total);
    int prev = -1;
    for (auto& n : sk) {
        if (!n.anchor) continue;
        const Chord& ch = *chordAt(spans, n.start);
        auto mask = maskOf(ch);
        const double u = std::fmod(n.start, phraseLen) / phraseLen;
        const double tgt = mid + d * (u - 0.5) * range * 0.9 + (1.0 - std::abs(d)) * (std::sin(M_PI * u) - 0.5) * range * 0.3;
        double best = -1e9;
        int bestP = lo;
        for (int m = lo; m <= hi; ++m) {
            if (!mask[(size_t) mod12(m)]) continue;
            double s = -std::abs(m - tgt);
            if (prev >= 0) {
                int leap = std::abs(m - prev);
                if (leap > 7) s -= (leap - 7) * 1.5 * (1.2 - cx);
                if (leap == 0) s -= 2.5;
                else if (leap <= 4) s += 2.0;
            }
            int iv = mod12(m - ch.root);
            if (iv == 0) s += 0.0;
            else if (iv == 3 || iv == 4) s += 0.6;
            else if (iv == 7) s += 0.3;
            else s += 0.2 * cx;
            if (&n == &sk[0] && (iv == 3 || iv == 4 || iv == 7)) s += 0.8;
            if (s > best + 1e-12) { best = s; bestP = m; }
        }
        n.pitch = bestP;
        prev = bestP;
    }

    // 2) preenchimento: notas de passagem e vizinhas da escala entre âncoras
    std::vector<int> ladder;
    for (int m = lo - 12; m <= hi + 12; ++m)
        if (sc.contains(m)) ladder.push_back(m);
    auto idxOf = [&](int pitch) {
        int bi = 0, bd = 1000;
        for (size_t i = 0; i < ladder.size(); ++i) {
            int dd = std::abs(ladder[i] - pitch);
            if (dd < bd) { bd = dd; bi = (int) i; }
        }
        return bi;
    };
    bool flip = false;
    for (size_t i = 0; i < sk.size();) {
        if (sk[i].anchor) { ++i; continue; }
        size_t j = i;
        while (j < sk.size() && !sk[j].anchor) ++j;  // run de fillers [i, j)
        int A = i > 0 ? sk[i - 1].pitch : -1;
        int B = j < sk.size() ? sk[j].pitch : -1;
        if (A < 0) A = B;
        if (B < 0) B = A;
        int ia = idxOf(A), ib = idxOf(B);
        const int m = (int) (j - i);
        int lastPitch = A;
        for (int k = 1; k <= m; ++k) {
            int idx = ia + (int) std::lround((double) (ib - ia) * k / (m + 1));
            int pitch = ladder[(size_t) std::max(0, std::min((int) ladder.size() - 1, idx))];
            if (pitch == lastPitch) {  // evita repetições: nota vizinha no sentido do contorno
                int dir = (d >= 0 ? 1 : -1) * (flip ? -1 : 1);
                flip = !flip;
                idx = std::max(0, std::min((int) ladder.size() - 1, idx + dir));
                pitch = ladder[(size_t) idx];
            }
            sk[i + (size_t) k - 1].pitch = pitch;
            lastPitch = pitch;
        }
        i = j;
    }

    // 3) nota a evitar: semitono acima de nota do acorde em notas ≥ 1/2 tempo => nota do acorde mais próxima
    for (auto& n : sk) {
        if (n.anchor || n.len < 0.5) continue;
        const Chord& ch = *chordAt(spans, n.start);
        auto mask = maskOf(ch);
        if (mask[(size_t) mod12(n.pitch)]) continue;
        bool avoid = false;
        for (int pc : ch.pitchClasses())
            if (mod12(n.pitch - pc) == 1) avoid = true;
        if (!avoid) continue;
        int best = n.pitch, bd = 1000;
        for (int m = lo - 2; m <= hi + 2; ++m)
            if (mask[(size_t) mod12(m)] && std::abs(m - n.pitch) < bd) { bd = std::abs(m - n.pitch); best = m; }
        n.pitch = best;
    }

    // 4) final: termina numa nota do acorde (raiz ou terça), longa
    {
        Sk& last = sk.back();
        const Chord& ch = *chordAt(spans, last.start);
        auto mask = maskOf(ch);
        int ref = sk.size() > 1 ? sk[sk.size() - 2].pitch : last.pitch;
        int best = last.pitch, bs = -1000;
        for (int m = lo; m <= hi; ++m) {
            if (!mask[(size_t) mod12(m)]) continue;
            int iv = mod12(m - ch.root);
            int s = -std::abs(m - ref) + ((iv == 0 || iv == 3 || iv == 4) ? 3 : 0);
            if (s > bs) { bs = s; best = m; }
        }
        last.pitch = best;
        last.anchor = true;
    }

    const double gate = 0.35 + 0.63 * pl;
    for (size_t i = 0; i < sk.size(); ++i) {
        const auto& n = sk[i];
        double len = n.len * gate;
        if (i + 1 == sk.size() && pl > 0.5) {
            const ChordSpan* cs = spanAt(spans, n.start);
            len = (cs ? cs->start + cs->length : total) - n.start;
            len = std::min(len, total - n.start) * 0.98;
        }
        len = std::max(0.0625, std::min(len, n.len));
        if (i + 1 == sk.size() && pl > 0.5) len = std::max(len, 0.0625);
        int vel = n.anchor ? 100 : 82;
        if (n.barStart) vel = std::min(127, vel + 6);
        out.push_back({n.pitch, n.start, len, vel});
    }
    return out;
}

// ============================================================ CONTRA-MELODIA
namespace {
int melodyPitchAt(const std::vector<GenNote>& mel, double t) {
    int p = mel.front().pitch;
    for (auto& n : mel) {
        if (n.start <= t + 1e-9) p = n.pitch;
        else break;
    }
    return p;
}
}  // namespace

std::vector<GenNote> generateCounter(const Scale& sc, const std::vector<ChordSpan>& spans, const std::vector<GenNote>& melody,
                                     const CounterParams& p) {
    std::vector<GenNote> out;
    if (spans.empty() || melody.empty()) return out;
    const double total = totalLength(spans);
    const int bars = (int) std::ceil(total / 4.0 - 1e-9);
    const double dens = std::max(0.0, std::min(1.0, p.density));
    std::vector<int> slots;
    if (dens < 0.34) slots = {0};
    else if (dens < 0.67) slots = {0, 8};
    else slots = {0, 4, 8, 12};
    const int off = mod4(p.variation) >= 2 ? 2 : 0;  // desloca o ataque 1 colcheia nas variações 2 e 3
    const bool sixths = (p.variation % 2) != 0;

    std::vector<std::pair<double, int>> onsets;
    for (int b = 0; b < bars; ++b)
        for (int s : slots) {
            double t = 4.0 * b + (s + (s == 0 ? 0 : off)) * 0.25;
            if (t < total - 1e-9 && spanAt(spans, t)) onsets.push_back({t, s});
        }
    // cada chegada a um novo acorde é um ataque
    for (auto& c : spans) {
        bool has = false;
        for (auto& o : onsets) if (std::abs(o.first - c.start) < 1e-9) has = true;
        if (!has && c.start < total - 1e-9) onsets.push_back({c.start, 0});
    }
    std::sort(onsets.begin(), onsets.end());

    int prevC = -1, prevM = -1;
    for (size_t k = 0; k < onsets.size(); ++k) {
        const double t = onsets[k].first;
        const ChordSpan* cs = spanAt(spans, t);
        const Chord& ch = cs->chord;
        auto mask = maskOf(ch);
        const int m = melodyPitchAt(melody, t);
        const int lo = p.above ? m + 3 : m - 16, hi = p.above ? m + 16 : m - 3;
        double best = -1e9;
        int bestP = -1;
        for (int c = std::max(40, lo); c <= std::min(100, hi); ++c) {
            if (!sc.contains(c) && !mask[(size_t) mod12(c)]) continue;
            int iv = std::abs(m - c) % 12;
            double s = 0;
            if (iv == 3 || iv == 4) s += sixths ? 2.0 : 3.0;
            else if (iv == 8 || iv == 9) s += sixths ? 3.0 : 2.0;
            else if (iv == 7) s += 1.0;
            else if (iv == 0) s += 0.5;
            else if (iv == 5) s -= 1.5;
            else if (iv == 6) s -= 2.0;
            else s -= 3.0;  // segundas, sétimas
            bool strong = onsets[k].second % 4 == 0;
            if (mask[(size_t) mod12(c)]) s += strong ? 2.5 : 2.0;
            if (prevC >= 0) {
                int leap = std::abs(c - prevC);
                s -= 0.35 * leap;
                if (leap <= 2) s += 1.5;
                int dm = prevM >= 0 ? (m - prevM) : 0, dc = c - prevC;
                if (dm != 0 && dc != 0) {
                    if ((dm > 0) != (dc > 0)) s += 1.5;  // movimento contrário
                    else s -= 0.5;
                    int ivPrev = std::abs(prevM - prevC) % 12;
                    if ((dm > 0) == (dc > 0) && ivPrev == iv && (iv == 7 || iv == 0)) s -= 6.0;  // quintas/oitavas paralelas
                }
            }
            if (s > best + 1e-12) { best = s; bestP = c; }
        }
        if (bestP < 0) continue;
        double end = (k + 1 < onsets.size()) ? onsets[k + 1].first : total;
        end = std::min(end, cs->start + cs->length);
        double len = (end - t) * 0.95;
        if (len < 0.0625) continue;
        out.push_back({bestP, t, len, 72});
        prevC = bestP;
        prevM = m;
    }
    return out;
}

// ============================================================ BAIXO
std::vector<GenNote> generateBass(const Scale& sc, const std::vector<ChordSpan>& spans, const BassParams& p) {
    std::vector<GenNote> out;
    if (spans.empty()) return out;
    const double total = totalLength(spans);
    const int bars = (int) std::ceil(total / 4.0 - 1e-9);
    const int base = p.lowOctave ? 28 : 36;
    auto rootAt = [&](const Chord& c, int b) { return b + mod12(c.root - b); };

    if (p.pattern == BassPattern::SubLong) {
        for (auto& cs : spans) out.push_back({rootAt(cs.chord, 24), cs.start, cs.length * 0.98, 100});
        return out;
    }

    struct T { int slot, len, oct; int vel; };
    std::vector<T> tpl;
    switch (p.pattern) {
        case BassPattern::Offbeat:
            for (int s : {2, 6, 10, 14}) tpl.push_back({s, 2, 0, 100});
            break;
        case BassPattern::Rolling:
            for (int b = 0; b < 4; ++b)
                for (int k = 0; k < 4; ++k) tpl.push_back({b * 4 + k, 1, k == 2 ? 12 : 0, k == 0 ? 105 : 80});
            break;
        case BassPattern::SidechainReady:
            for (int s : {1, 5, 9, 13}) tpl.push_back({s, 3, 0, 100});
            break;
        case BassPattern::OctaveBounce:
            for (int i = 0; i < 8; ++i) tpl.push_back({i * 2, 2, (i % 2) * 12, i % 2 ? 85 : 100});
            break;
        case BassPattern::Syncopated:
            tpl = {{0, 3, 0, 105}, {3, 3, 0, 90}, {6, 2, 12, 85}, {8, 3, 0, 100}, {11, 3, 0, 90}, {14, 2, 12, 85}};
            break;
        default: break;
    }
    for (int b = 0; b < bars; ++b)
        for (size_t i = 0; i < tpl.size(); ++i) {
            const auto& t = tpl[i];
            double s = 4.0 * b + t.slot * 0.25;
            if (s >= total - 1e-9) continue;
            const ChordSpan* cs = spanAt(spans, s);
            if (!cs) continue;
            double e = std::min({s + t.len * 0.25, cs->start + cs->length, total});
            double len = (e - s) * (p.pattern == BassPattern::Rolling ? 0.9 : 0.92);
            if (len < 0.0625) continue;
            int pitch = rootAt(cs->chord, base) + t.oct;

            // variação ímpar: última nota do compasso aproxima-se por grau conjunto da raiz do acorde seguinte
            bool lastOfBar = i + 1 == tpl.size();
            if ((p.variation % 2) != 0 && lastOfBar && t.len <= 2) {
                const Chord* nx = chordAt(spans, 4.0 * b + 4.0);
                if (nx && nx->root != cs->chord.root) {
                    int R = rootAt(*nx, base);
                    int below = R - 1, above = R + 1;
                    while (below > 0 && !sc.contains(below)) --below;
                    while (above < 127 && !sc.contains(above)) ++above;
                    pitch = std::abs(pitch - below) <= std::abs(pitch - above) ? below : above;
                }
            }
            out.push_back({pitch, s, len, t.vel});
        }
    if (p.subLayer) {
        size_t n = out.size();
        for (size_t i = 0; i < n; ++i)
            if (out[i].pitch - 12 >= 12) out.push_back({out[i].pitch - 12, out[i].start, out[i].length, std::max(1, out[i].velocity - 10)});
    }
    std::stable_sort(out.begin(), out.end(), [](const GenNote& a, const GenNote& b) { return a.start < b.start; });
    return out;
}

// ============================================================ ARPEJO
namespace {
int foldIdx(int x, int n) {
    if (n <= 1) return 0;
    int per = 2 * n - 2;
    int m = ((x % per) + per) % per;
    return m < n ? m : per - m;
}
}  // namespace

std::vector<GenNote> generateArp(const std::vector<ChordSpan>& spans, const ArpParams& p) {
    std::vector<GenNote> out;
    const double step = p.rate == ArpRate::Eighth ? 0.5 : p.rate == ArpRate::Sixteenth ? 0.25 : 1.0 / 6.0;
    const int octs = std::max(1, std::min(3, p.octaves));
    const double gate = std::max(0.1, std::min(1.0, p.gate));
    for (auto& cs : spans) {
        // escada de notas do acorde, de baixo para cima
        auto pcs = cs.chord.pitchClasses();
        std::sort(pcs.begin(), pcs.end(), [&](int a, int b) { return mod12(a - cs.chord.root) < mod12(b - cs.chord.root); });
        const int startPitch = 55 + mod12(cs.chord.root - 55);
        std::vector<int> ladder;
        for (int o = 0; o < octs; ++o)
            for (int pc : pcs) ladder.push_back(startPitch + 12 * o + mod12(pc - cs.chord.root));
        ladder.push_back(startPitch + 12 * octs);
        const int N = (int) ladder.size();
        const int phase = (p.variation % 2) != 0 ? 2 : 0;

        int steps = (int) std::floor(cs.length / step + 1e-6);
        for (int i = 0; i < steps; ++i) {
            const double t = cs.start + i * step;
            int k = i + phase, idx = 0;
            switch (p.pattern) {
                case ArpPattern::Up: idx = k % N; break;
                case ArpPattern::Down: idx = N - 1 - (k % N); break;
                case ArpPattern::UpDown: idx = foldIdx(k, N); break;
                case ArpPattern::Alternate: idx = foldIdx(k / 2 + ((k % 2) ? 2 : 0), N); break;
                case ArpPattern::FutureBass: idx = foldIdx(k, std::min(N, 6)); break;
            }
            // acentos: tempo > contratempo > resto
            int vel;
            const double inBeat = std::fmod(t - cs.start, 1.0);
            if (inBeat < 1e-6) vel = p.pattern == ArpPattern::FutureBass ? 108 : 100;
            else if (std::abs(inBeat - 0.5) < 1e-6) vel = p.pattern == ArpPattern::FutureBass ? 92 : 86;
            else vel = p.pattern == ArpPattern::FutureBass ? 74 : 80;
            double len = std::min(step * gate, cs.start + cs.length - t);
            if (len < 0.03) continue;
            out.push_back({ladder[(size_t) idx], t, len, vel});
        }
    }
    return out;
}

// ============================================================ IMPORTAR ACORDES
namespace {
Chord chordFromLabel(const ChordLabel& l) {
    Chord c;
    c.root = l.root;
    const std::string& s = l.suffix;
    auto set = [&](Quality q, Richness r) { c.quality = q; c.richness = r; };
    if (s == "m") set(Quality::Minor, Richness::Triad);
    else if (s == "dim") set(Quality::Dim, Richness::Triad);
    else if (s == "aug") set(Quality::Aug, Richness::Triad);
    else if (s == "sus4" || s == "sus2") set(Quality::Sus4, Richness::Triad);
    else if (s == "maj7") set(Quality::Major, Richness::Seventh);
    else if (s == "m7") set(Quality::Minor, Richness::Seventh);
    else if (s == "7") set(Quality::Dom, Richness::Seventh);
    else if (s == "m7b5" || s == "dim7") set(Quality::Dim, Richness::Seventh);
    else if (s == "add9") set(Quality::Major, Richness::Add9);
    else if (s == "m(add9)") set(Quality::Minor, Richness::Add9);
    else if (s == "maj9") set(Quality::Major, Richness::Ninth);
    else if (s == "m9") set(Quality::Minor, Richness::Ninth);
    else if (s == "9") set(Quality::Dom, Richness::Ninth);
    else if (s == "m6") set(Quality::Minor, Richness::Triad);
    else if (s == "7sus4") set(Quality::Sus4, Richness::Seventh);
    else if (s == "sus4(add9)") set(Quality::Sus4, Richness::Add9);
    else if (s == "7#5") set(Quality::Aug, Richness::Seventh);
    else set(Quality::Major, Richness::Triad);  // "", "5", "6"
    return c;
}
}  // namespace

std::vector<ChordSpan> chordsFromNotes(const std::vector<GenNote>& notes) {
    std::vector<ChordSpan> out;
    if (notes.empty()) return out;
    std::vector<GenNote> sorted = notes;
    std::stable_sort(sorted.begin(), sorted.end(), [](const GenNote& a, const GenNote& b) { return a.start < b.start; });
    std::vector<double> onsets;
    for (auto& n : sorted)
        if (onsets.empty() || n.start - onsets.back() > 0.06) onsets.push_back(n.start);
    double end = 0;
    for (auto& n : sorted) end = std::max(end, n.start + n.length);
    for (size_t i = 0; i < onsets.size() && out.size() < 8; ++i) {
        const double t0 = onsets[i], t1 = i + 1 < onsets.size() ? onsets[i + 1] : end;
        std::vector<int> pitches;
        for (auto& n : sorted)
            if (n.start <= t0 + 0.06 && n.start + n.length > t0 + 0.06) pitches.push_back(n.pitch);
        if (pitches.size() < 2 || t1 - t0 < 0.25) continue;
        Chord c;
        if (auto id = identifyChord(pitches)) c = chordFromLabel(*id);
        else {
            int low = *std::min_element(pitches.begin(), pitches.end());
            c.root = mod12(low);
            bool minor3 = false;
            for (int p : pitches) if (mod12(p - low) == 3) minor3 = true;
            c.quality = minor3 ? Quality::Minor : Quality::Major;
        }
        out.push_back({c, t0 - onsets[0], t1 - t0});
    }
    return out;
}

}  // namespace alvorada
