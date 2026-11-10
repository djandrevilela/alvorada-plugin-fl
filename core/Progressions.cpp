#include "Progressions.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace harmonia {

const std::vector<Style>& styles() {
    static const std::vector<Style> s = {
        {0, "Emocional (melódico)",
         "Sensação de subida emocional e saudade, típica de drops melódicos: alterna acordes menores (tristeza) com maiores (esperança).",
         Richness::Add9,
         {{"vi IV I V", "Começa na relativa menor (melancolia), \"chega a casa\" no I e termina no V, que pede para voltar ao início: o loop perfeito."},
          {"I iii vi IV", "Descida de terças (I, iii, vi): cada acorde partilha notas com o anterior, o que soa suave e nostálgico."},
          {"vi V IV V", "Baixo a descer por graus conjuntos (vi, V, IV) e regresso ao V: movimento melódico com tensão constante."},
          {"IV I V vi", "Começa fora de casa (IV), resolve no I, sobe ao V e cai no vi: o final inesperado deixa o ouvinte em suspenso."}},
         {{"i bVI bIII bVII", "Progressão modal clássica: i (casa), bVI e bIII (cor luminosa) e bVII (impulso para voltar ao i). Emotiva e épica."},
          {"bVI bVII i i", "Tensão a subir de bVI para bVII e libertação no i, mantido dois tempos: sensação clara de chegada."},
          {"i bIII bVII bVI", "Começa em casa, salta para a relativa maior (bIII) e desce em graus conjuntos: luminoso e nostálgico."},
          {"iv bVI bIII bVII", "Começa no iv (escuro), sobe para a zona maior (bVI, bIII) e fecha com bVII: tristeza que se transforma em esperança."}}},

        {1, "Pop energético",
         "Progressões diretas e cantáveis, com resolução clara: ideais para vozes e leads simples e memoráveis.",
         Richness::Add9,
         {{"I V vi IV", "Tónica, dominante, relativa menor e subdominante: equilíbrio entre estabilidade e emoção; o padrão mais usado na pop."},
          {"vi IV I V", "Os mesmos acordes rodados: começar no vi dá um tom mais sério antes de chegar ao I."},
          {"I vi IV V", "Descida de terças (I, vi) e cadência final forte no V que pede o regresso ao I."},
          {"I IV vi V", "Sobe ao IV, desce ao vi e termina no V: contorno ondulante que prepara o regresso ao I."}},
         {{"i bVII bVI bVII", "Baixo que desce e volta (i, bVII, bVI, bVII): vaivém hipnótico, ótimo para loops longos."},
          {"i iv bVII bIII", "Sequência de quartas ascendentes (i, iv, bVII, bIII): movimento natural e muito cantável."},
          {"i bVI bVII i", "Sobe de bVI para bVII e volta à tónica: resolução direta e satisfatória."},
          {"bIII bVII i bVI", "Começa na relativa maior (bIII), o que dá brilho, e termina em bVI: ciclo aberto, ótimo para loop."}}},

        {2, "Épico (big room)",
         "Acordes largos e dramáticos, com empréstimos modais, para build-ups e drops de festival.",
         Richness::Triad,
         {{"I bVII IV I", "Empréstimo do bVII (modo mixolídio): soa a hino e empurra de volta à tónica."},
          {"I V bVII IV", "Dominante seguida de bVII: sensação de multidão a cantar, muito aberta."},
          {"I bIII bVII IV", "Dois empréstimos modais seguidos (bIII e bVII): efeito cinematográfico e luminoso."},
          {"I bVI bVII I", "Subida bVI, bVII e resolução no I: a cadência \"épica\" típica de bandas sonoras."}},
         {{"i bVII bVI V", "Baixo a descer até à dominante maior (V): tensão máxima antes de voltar ao i."},
          {"i bVI iv V", "O i, o bVI e o iv preparam a dominante V: tudo aponta para resolver no i."},
          {"i iv bVI bVII", "Do i ao iv e subida bVI, bVII: crescimento gradual de energia."},
          {"i v bVI bVII", "Sobe do i ao v e continua em bVI, bVII: ascensão que fica \"aberta\"."}}},

        {3, "Future bass (rico)",
         "Acordes com sétimas e nonas, muito coloridos, com movimento suave entre eles.",
         Richness::Seventh,
         {{"IV V iii vi", "O V não resolve no I mas desce para iii e vi, deixando o ouvido a \"flutuar\" sem chegar a casa."},
          {"ii V I vi", "Cadência ii, V, I (a mais forte da harmonia tonal) com continuação no vi para manter o loop aberto."},
          {"IV iii ii I", "Baixo a descer em escala (IV, iii, ii, I): linha cantável, acordes ligados por notas em comum."},
          {"I iii IV iv", "Do iii ao IV e depois um iv menor emprestado: toque agridoce muito emotivo."}},
         {{"i bVI iv bVII", "Três cores diferentes sobre a mesma tónica, com o bVII a empurrar de volta ao i."},
          {"iv bVII bIII bVI", "Ciclo de quintas descendentes (iv, bVII, bIII, bVI): a forma mais natural de ligar acordes."},
          {"i bIII iv bVII", "Alterna a cor menor e a relativa maior; o iv traz tristeza suave."},
          {"bVI bVII bIII i", "Subida e queda para a tónica: sensação de chegada suave."}}},

        {4, "Progressive house (groove)",
         "Padrões repetitivos com a tónica como ponto fixo, para dar groove e espaço à melodia.",
         Richness::Add9,
         {{"I IV I V", "O I funciona como âncora e alterna com subdominante e dominante: groove e estabilidade."},
          {"I V IV I", "Sobe ao V e desce ao IV antes de voltar ao I: movimento fluido, sem tensão excessiva."},
          {"vi IV vi V", "O vi funciona como segunda \"casa\": ambiente mais introspetivo, com o V a abrir."},
          {"I iii IV V", "Subida pelos graus I, iii, IV, V: crescimento contínuo e previsível."}},
         {{"i i bVII bVI", "O i repetido funciona como pedal: o baixo fica quase fixo e a tensão vem dos acordes por cima."},
          {"i bVII i bVI", "Vai e volta ao bVII antes de descer ao bVI: groove com \"respiração\"."},
          {"i iv i bVII", "O iv dá um toque mais escuro; o regresso ao i mantém o groove."},
          {"i bVI i bVII", "bVI e bVII alternam com o i: moldura clássica de progressive."}}},

        {5, "Build-up (tensão)",
         "Progressões que terminam em dominante/tensão para antes do drop: pedem resolução.",
         Richness::Triad,
         {{"IV IV V V", "Fica em subdominante e depois acumula a dominante: tensão que só alivia quando o drop volta ao I."},
          {"vi IV V V", "Começa na relativa menor e desemboca numa dominante repetida: expectativa crescente."},
          {"ii ii V V", "Preparação clássica ii para V: a dominante repetida cria expectativa."},
          {"I IV V V", "Sobe por graus fortes (I, IV, V) e segura a dominante: quase a pedir o drop."}},
         {{"iv iv V V", "O iv escuro seguido do V maior (com sensível): tensão máxima que pede o i do drop."},
          {"bVI bVII V V", "Subida modal (bVI, bVII) até à dominante maior: build-up cinematográfico."},
          {"iv V iv V", "Alternância entre iv e V: vaivém de tensão sem resolução."},
          {"i iv V V", "Do i ao iv e fixa-se no V: tensão a crescer até ao drop."}}}};
    return s;
}

std::optional<ChordStep> parseToken(const std::string& tok) {
    if (tok.empty()) return std::nullopt;
    size_t i = 0;
    int acc = 0;
    if (tok[i] == 'b') { acc = -1; ++i; }
    else if (tok[i] == '#') { acc = 1; ++i; }
    size_t start = i;
    while (i < tok.size() && (tok[i] == 'I' || tok[i] == 'V' || tok[i] == 'i' || tok[i] == 'v')) ++i;
    if (i == start) return std::nullopt;
    std::string num = tok.substr(start, i - start);
    bool upper = std::isupper((unsigned char) num[0]) != 0;
    std::string up = num;
    std::transform(up.begin(), up.end(), up.begin(), [](unsigned char c) { return (char) std::toupper(c); });
    static const char* names[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};
    static const int majorOffsets[7] = {0, 2, 4, 5, 7, 9, 11};
    int deg = -1;
    for (int d = 0; d < 7; ++d)
        if (up == names[d]) deg = d;
    if (deg < 0) return std::nullopt;
    std::string suf = tok.substr(i);

    ChordStep s;
    s.token = tok;
    s.offset = mod12(majorOffsets[deg] + acc);
    if (suf.find("dim") != std::string::npos || suf.find("°") != std::string::npos) s.quality = Quality::Dim;
    else if (suf.find("aug") != std::string::npos || suf.find('+') != std::string::npos) s.quality = Quality::Aug;
    else if (suf.find("sus") != std::string::npos) s.quality = Quality::Sus4;
    else if (suf.find('7') != std::string::npos) s.quality = upper ? Quality::Dom : Quality::Minor;
    else if (upper) {
        // V sempre dominante; II, III, VI, VII maiúsculos sem alteração = dominantes secundárias
        bool dom = acc == 0 && (deg == 1 || deg == 2 || deg == 4 || deg == 5 || deg == 6);
        s.quality = dom ? Quality::Dom : Quality::Major;
    } else {
        s.quality = Quality::Minor;
    }
    return s;
}

std::vector<ChordStep> parseProgression(const std::string& tokens) {
    std::vector<ChordStep> out;
    std::istringstream ss(tokens);
    std::string t;
    while (ss >> t)
        if (auto s = parseToken(t)) out.push_back(*s);
    return out;
}

std::string joinTokens(const std::vector<ChordStep>& steps) {
    std::string r;
    for (size_t i = 0; i < steps.size(); ++i) {
        if (i) r += ' ';
        r += steps[i].token;
    }
    return r;
}

std::vector<Chord> realize(const std::vector<ChordStep>& steps, int tonicPc, Richness richness) {
    std::vector<Chord> out;
    for (const auto& s : steps) {
        Chord c;
        c.root = mod12(tonicPc + s.offset);
        c.quality = s.quality;
        c.richness = richness;
        out.push_back(c);
    }
    return out;
}

const std::vector<std::string>& chordPalette(bool minorMode) {
    static const std::vector<std::string> maj = {"I", "ii", "iii", "IV", "V", "vi", "viidim", "bIII", "bVI", "bVII", "iv", "II", "III", "VI"};
    static const std::vector<std::string> min = {"i", "iidim", "bIII", "iv", "v", "V", "bVI", "bVII", "IV", "bII"};
    return minorMode ? min : maj;
}

std::string prettyToken(const std::string& token) {
    std::string r = token;
    auto p = r.find("dim");
    if (p != std::string::npos) r.replace(p, 3, "°");
    return r;
}

std::string functionText(const ChordStep& s) {
    const bool dom = s.quality == Quality::Dom;
    switch (s.offset) {
        case 0: return "Tónica: o ponto de repouso, a \"casa\" da música.";
        case 1: return "bII (napolitano): cor dramática e tensão forte.";
        case 2: return dom ? "V do V: dominante secundária, cria tensão a apontar para o V."
                           : "Supertónica (ii): prepara a dominante; função de subdominante.";
        case 3: return "bIII: relativa maior (ou empréstimo modal); estável e luminoso.";
        case 4: return dom ? "III maior: dominante do vi, cria tensão a apontar para a relativa menor."
                           : "Mediante (iii): estável e suave; liga a tónica ao IV e ao vi.";
        case 5: return "Subdominante (IV/iv): afasta-se de casa e cria movimento.";
        case 6: return "Trítono (#IV/bV): tensão forte e muito instável.";
        case 7: return s.quality == Quality::Minor ? "Dominante menor (v): tensão suave, sem sensível; soa modal."
                                                   : "Dominante (V): tensão que pede resolução na tónica.";
        case 8: return "bVI: empréstimo modal; soa épico e emotivo.";
        case 9: return dom ? "VI maior: dominante do ii."
                           : "Relativa menor (vi): emoção; funciona como uma segunda tónica.";
        case 10: return "bVII: empréstimo modal; empurra de volta para a tónica.";
        case 11: return s.quality == Quality::Dim ? "Sensível (vii°): forte tendência a resolver na tónica."
                                                  : "VII maior: dominante do iii.";
    }
    return "";
}

}  // namespace harmonia
