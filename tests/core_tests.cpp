#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

#include "Generators.h"
#include "Presets.h"
#include "Progressions.h"

using namespace harmonia;
using namespace alvorada;

static int g_fail = 0;
#define CHECK(cond)                                                                                  \
    do {                                                                                             \
        if (!(cond)) { std::printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_fail; }     \
    } while (0)

static std::vector<ChordSpan> spansFor(const std::string& tokens, int tonic, Richness r, double bpc) {
    auto chords = realize(parseProgression(tokens), tonic, r);
    std::vector<ChordSpan> s;
    for (size_t i = 0; i < chords.size(); ++i) s.push_back({chords[i], i * bpc, bpc});
    return s;
}

static bool inHarmony(const Scale& sc, const std::vector<ChordSpan>& sp, const GenNote& n) {
    const Chord* c = chordAt(sp, n.start);
    if (!c) return false;
    if (sc.contains(n.pitch)) return true;
    for (int pc : c->pitchClasses()) if (pc == mod12(n.pitch)) return true;
    return false;
}

int main() {
    int combos = 0;
    for (const auto& st : styles())
        for (int minor = 0; minor < 2; ++minor)
            for (const auto& t : (minor ? st.minor : st.major))
                for (int tonic : {0, 5, 9, 10})
                    for (double bpc : {2.0, 4.0, 8.0}) {
                        Scale sc{tonic, minor ? ScaleType::NaturalMinor : ScaleType::Major};
                        auto sp = spansFor(t.tokens, tonic, st.defaultRichness, bpc);
                        ++combos;

                        // ---- melodia
                        for (double cx : {0.1, 0.5, 0.9})
                            for (double dir : {-1.0, 0.0, 1.0})
                                for (double pl : {0.0, 1.0})
                                    for (int var : {0, 1, 2, 3}) {
                                        MelodyParams mp;
                                        mp.complexity = cx; mp.direction = dir; mp.pluckLead = pl; mp.variation = var;
                                        auto mel = generateMelody(sc, sp, mp);
                                        CHECK(!mel.empty());
                                        double total = totalLength(sp);
                                        for (size_t i = 0; i < mel.size(); ++i) {
                                            CHECK(inHarmony(sc, sp, mel[i]));
                                            CHECK(mel[i].pitch >= 50 && mel[i].pitch <= 100);
                                            CHECK(mel[i].start >= 0 && mel[i].start + mel[i].length <= total + 1e-6);
                                            if (i + 1 < mel.size()) CHECK(mel[i].start + mel[i].length <= mel[i + 1].start + 1e-9);  // monofónica
                                            // tempos fortes (inteiros) = notas do acorde
                                            if (std::abs(mel[i].start - std::round(mel[i].start)) < 1e-9 && std::fmod(mel[i].start * 4, 4) < 1e-9) {
                                                const Chord* c = chordAt(sp, mel[i].start);
                                                bool ct = false;
                                                for (int pc : c->pitchClasses()) if (pc == mod12(mel[i].pitch)) ct = true;
                                                CHECK(ct);
                                            }
                                        }
                                        auto again = generateMelody(sc, sp, mp);
                                        CHECK(again.size() == mel.size());
                                        for (size_t i = 0; i < mel.size() && i < again.size(); ++i) CHECK(again[i].pitch == mel[i].pitch && again[i].start == mel[i].start);

                                        // ---- contra-melodia sobre esta melodia
                                        for (bool above : {false, true}) {
                                            CounterParams cp; cp.above = above; cp.density = cx; cp.variation = var;
                                            auto ctr = generateCounter(sc, sp, mel, cp);
                                            CHECK(!ctr.empty());
                                            for (auto& n : ctr) CHECK(inHarmony(sc, sp, n));
                                        }
                                    }

                        // ---- baixo
                        for (int pat = 0; pat < kNumBassPatterns; ++pat)
                            for (bool sub : {false, true})
                                for (int var : {0, 1}) {
                                    BassParams bp; bp.pattern = (BassPattern) pat; bp.subLayer = sub; bp.variation = var;
                                    auto bass = generateBass(sc, sp, bp);
                                    CHECK(!bass.empty());
                                    for (auto& n : bass) {
                                        CHECK(n.pitch >= 12 && n.pitch <= 60);
                                        CHECK(sc.contains(n.pitch) || inHarmony(sc, sp, n));
                                    }
                                    // notas de raiz nos inícios de acorde (sem aproximações)
                                    if (var == 0 && !sub)
                                        for (auto& n : bass) {
                                            const Chord* c = chordAt(sp, n.start);
                                            CHECK(c && mod12(n.pitch) == c->root);
                                        }
                                }

                        // ---- arpejos: só notas do acorde
                        for (int pat = 0; pat < kNumArpPatterns; ++pat)
                            for (int rate = 0; rate < 3; ++rate)
                                for (int oct : {1, 2, 3}) {
                                    ArpParams ap; ap.pattern = (ArpPattern) pat; ap.rate = (ArpRate) rate; ap.octaves = oct;
                                    auto arp = generateArp(sp, ap);
                                    CHECK(!arp.empty());
                                    for (auto& n : arp) {
                                        const Chord* c = chordAt(sp, n.start);
                                        bool ct = false;
                                        for (int pc : c->pitchClasses()) if (pc == mod12(n.pitch)) ct = true;
                                        CHECK(ct);
                                    }
                                }
                    }

    // direção: "subida" tem de acabar mais alta que "descida" em média
    {
        Scale sc{0, ScaleType::Major};
        auto sp = spansFor("I V vi IV", 0, Richness::Triad, 4.0);
        MelodyParams up, down;
        up.direction = 1; down.direction = -1;
        auto mu = generateMelody(sc, sp, up), md = generateMelody(sc, sp, down);
        auto avg = [](const std::vector<GenNote>& m, size_t from, size_t to) { double s = 0; for (size_t i = from; i < to; ++i) s += m[i].pitch; return s / (double) (to - from); };
        CHECK(avg(mu, mu.size() / 2, mu.size()) - avg(mu, 0, mu.size() / 2) > avg(md, md.size() / 2, md.size()) - avg(md, 0, md.size() / 2));
        // variação muda o resultado
        MelodyParams v1 = up; v1.variation = 1;
        auto mv = generateMelody(sc, sp, v1);
        bool diff = mv.size() != mu.size();
        for (size_t i = 0; !diff && i < mu.size(); ++i) diff = mu[i].start != mv[i].start || mu[i].pitch != mv[i].pitch;
        CHECK(diff);
        // complexidade aumenta o número de notas
        MelodyParams simple, complex;
        simple.complexity = 0.05; complex.complexity = 0.95;
        CHECK(generateMelody(sc, sp, complex).size() > generateMelody(sc, sp, simple).size());
        // pluck mais curto que lead
        MelodyParams pk, ld; pk.pluckLead = 0; ld.pluckLead = 1;
        CHECK(generateMelody(sc, sp, pk)[0].length < generateMelody(sc, sp, ld)[0].length);
    }

    // arp future bass: 16 passos por compasso, em semicolcheias
    {
        auto sp = spansFor("I", 0, Richness::Triad, 4.0);
        auto arp = generateArp(sp, ArpParams{});
        CHECK(arp.size() == 16);
        CHECK(arp[0].start == 0 && std::abs(arp[1].start - 0.25) < 1e-9);
    }

    // baixo offbeat: 4 notas por compasso nos contratempos
    {
        Scale sc{0, ScaleType::Major};
        auto sp = spansFor("I", 0, Richness::Triad, 4.0);
        auto b = generateBass(sc, sp, BassParams{});
        CHECK(b.size() == 4 && std::abs(b[0].start - 0.5) < 1e-9);
        BassParams sl; sl.pattern = BassPattern::SidechainReady;
        auto s = generateBass(sc, sp, sl);
        CHECK(s.size() == 4 && std::abs(s[0].start - 0.25) < 1e-9);  // deixa o ataque do bumbo livre
    }

    // importar acordes
    {
        std::vector<GenNote> n = {{36, 0, 4, 90}, {60, 0, 4, 90}, {64, 0, 4, 90}, {67, 0, 4, 90},
                                  {43, 4, 4, 90}, {59, 4, 4, 90}, {62, 4, 4, 90}, {67, 4, 4, 90}};
        auto ch = chordsFromNotes(n);
        CHECK(ch.size() == 2 && ch[0].chord.root == 0 && ch[1].chord.root == 7);
        CHECK(std::abs(ch[1].start - 4.0) < 1e-9 && std::abs(ch[1].length - 4.0) < 1e-9);
    }

    // biblioteca de presets
    {
        const auto& ps = presets();
        CHECK(ps.size() >= 24);
        std::set<std::string> names;
        for (auto& p : ps) {
            CHECK(names.insert(std::string(p.name)).second);
            for (float v : {p.bright, p.attack, p.tail, p.width, p.sub, p.vol}) CHECK(v >= 0 && v <= 1);
        }
        CHECK(presetIndexByName("Future Pluck") >= 0);
    }

    std::printf("%d combinações de estilo/tom/duração testadas.\n", combos);
    if (g_fail == 0) std::printf("Todos os testes passaram.\n");
    return g_fail == 0 ? 0 : 1;
}
