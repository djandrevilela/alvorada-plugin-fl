#pragma once
// Biblioteca de presets do synth do Alvorada (dados puros). Nomes genéricos, sem marcas.
// Cada preset define o motor, parâmetros internos e os valores iniciais dos 6 macros (0..1):
// Brilho, Ataque, Cauda, Largura, Sub, Volume.
#include <cmath>
#include <string>
#include <vector>

namespace alvorada {

enum class Engine { Subtractive, FM, String };
enum class Wave { Saw, Square, Tri, Sine };

struct SynthPreset {
    const char* category;
    const char* name;
    Engine engine;
    Wave w1, w2;
    float semi2;      // osc 2: semitons
    float mix2;       // osc 2: mistura 0..1
    int unison;       // 1..7
    float maxDetune;  // cents (com Largura = 1)
    float resonance;  // 0..1
    float fenv;       // profundidade da envolvente do filtro (0..1 => até 4 oitavas)
    float fdecay;     // base do decay do filtro (s)
    float sustain;    // 0..1
    float fmRatio, fmIndex, fmDecay;
    // macros (valores iniciais)
    float bright, attack, tail, width, sub, vol;
};

const std::vector<SynthPreset>& presets();
int presetIndexByName(const std::string& name);

// Mapeamento dos macros para valores físicos
inline float macroAttackSec(float a) { return 0.001f * std::pow(2000.f, a); }
inline float macroDecaySec(float t) { return 0.04f * std::pow(150.f, t); }
inline float macroReleaseSec(float t) { return 0.03f * std::pow(100.f, t); }
inline float macroCutoffHz(float b) { return 40.f * std::pow(2.f, b * 9.f); }

}  // namespace alvorada
