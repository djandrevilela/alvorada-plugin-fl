#pragma once
// Estilos e modelos de progressão. Tudo determinístico: o utilizador escolhe o estilo e a variação.
// Os modelos são padrões harmónicos genéricos (funções tonais), não cópias de músicas.
#include "Theory.h"

namespace harmonia {

struct ChordStep {
    int offset = 0;  // semitons acima da tónica
    Quality quality = Quality::Major;
    std::string token;  // ex.: "vi", "bVII", "viidim"
};

struct ProgressionTemplate {
    const char* tokens;
    const char* why;  // explicação em pt-PT
};

struct Style {
    int id;
    const char* name;
    const char* description;
    Richness defaultRichness;
    std::vector<ProgressionTemplate> major;
    std::vector<ProgressionTemplate> minor;
};

const std::vector<Style>& styles();

std::optional<ChordStep> parseToken(const std::string& token);
std::vector<ChordStep> parseProgression(const std::string& tokens);
std::string joinTokens(const std::vector<ChordStep>& steps);

std::vector<Chord> realize(const std::vector<ChordStep>& steps, int tonicPc, Richness richness);

const std::vector<std::string>& chordPalette(bool minorMode);
std::string prettyToken(const std::string& token);  // "viidim" -> "vii°"
std::string functionText(const ChordStep& step);    // explicação da função harmónica (pt-PT)

}  // namespace harmonia
