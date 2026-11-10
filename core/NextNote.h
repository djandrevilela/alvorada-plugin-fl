#pragma once
// Scale lock / correção e recomendação de próxima nota (regras de teoria, sem aleatoriedade).
#include <array>
#include <string>
#include <vector>

#include "Theory.h"

namespace harmonia {

enum class SnapMode { Nearest, Down, Up };

// Ajusta uma nota para a escala. Em empate (nota entre duas da escala), usa a direção do movimento (prevNote).
int snapToScale(const Scale& scale, int midi, SnapMode mode, int prevNote = -1);
int snapToPitchClasses(const std::array<bool, 12>& mask, int midi, SnapMode mode, int prevNote = -1);

struct NoteSuggestion {
    int midi = 0;
    double score = 0;
    std::string reason;  // pt-PT
};

// recent: últimas notas tocadas (a última é a mais recente). chord: acorde atual (pode ser nullptr).
std::vector<NoteSuggestion> suggestNextNotes(const Scale& scale, const std::vector<int>& recent,
                                             const Chord* chord, int maxResults = 5);

}  // namespace harmonia
