# Alvorada — princípios do projeto (ler sempre antes de mexer no código)

Plugin VST3 (JUCE) de melodia, baixo, arpejos e synth para produtores de EDM sem muita teoria musical.
Plugin irmão do Harmonia (acordes). Partilha o motor de teoria (`core/Theory`, `Progressions`, `NextNote`).

## Regras inegociáveis (definidas pelo autor)
1. **Nada de geração aleatória nem IA.** Tudo é determinístico e derivado de regras de teoria musical
   (acordes, escalas, condução melódica, movimento contrário, notas a evitar). Mesmas entradas => mesmas saídas.
   "Variação" = escolher outro modelo rítmico/contorno de uma lista curada, nunca sorteio.
   Proibido `rand`, `std::random_*`, modelos ML. (Ruído de excitação no synth usa LCG fixo: é DSP, não geração musical.)
2. **Só notas que funcionam**: tempos fortes = notas do acorde; resto = notas da escala; notas a evitar substituídas.
3. **Sem infringir copyright**: sem melodias/baixos/arpejos copiados de músicas, sem nomes de artistas nem de produtos
   comerciais nos presets. Só padrões genéricos (não protegíveis).
4. Interface e textos em português europeu (pt-PT).
5. `core/` é C++17 puro, testado em `tests/`. `plugin/` só orquestra (JUCE).

## Estrutura
- `core/Generators`  melodia, contra-melodia, arpejo, baixo + importação de acordes de MIDI
- `core/Presets`     biblioteca de presets do synth (dados puros) e mapeamento dos macros
- `plugin/SynthEngine.h`  voz (subtrativo, FM, cordas Karplus-Strong)
- `plugin/PluginProcessor` 4 partes (melodia, contra-melodia, arpejo, baixo), sequenciador sincronizado com o anfitrião
- `plugin/PluginEditor`    interface
