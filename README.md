# Alvorada (VST3) — melodia, contra-melodia, arpejo, baixo e synth para FL Studio

Plugin irmão do **Harmonia** (acordes). Tudo é **determinístico e baseado em teoria** (sem aleatoriedade, sem IA):
tempos fortes = notas do acorde, resto = notas da escala, notas a evitar substituídas, movimento contrário na contra-melodia.
Sem melodias nem baixos copiados de músicas; os presets têm nomes genéricos.

## As 4 partes (cada uma com o seu synth)
| Parte | Controlos principais |
|---|---|
| **Melodia** | Descida ↔ Subida, Simples ↔ Complexa, Pluck ↔ Lead, oitava, variação |
| **Contra-melodia** | Abaixo/acima da melodia, notas longas ↔ mais notas, variação (terças/sextas, movimento contrário, sem paralelas) |
| **Arpejo** | Subir, Descer, Subir/Descer, Alternado, **Future Bass**; 1/8, 1/16, 1/16 tercina; 1–3 oitavas; duração; botão **Arp Future Bass (1 clique)** |
| **Baixo & Sub** | Offbeat, Rolling, Sidechain-ready, Oitavas (bounce), Sub longo, Sincopado 3+3+2; camada sub; registo grave; variação (nota de aproximação ao acorde seguinte) |

"Variação" escolhe outro modelo rítmico/contorno de uma lista curada (nunca é sorteio).

## Acordes
Escolhe tom, escala e estilo (os mesmos modelos do Harmonia) e podes trocar qualquer acorde. Ou **larga um ficheiro .mid de acordes**
(ex.: exportado do Harmonia) sobre o plugin: os acordes são identificados e as 4 partes são geradas sobre eles.

## Synth fácil
Cada parte tem um **preset** (26 na biblioteca: Lead, Pluck, Pad, Teclas, Baixo, Guitarra, Arp) e só **6 botões**:
Brilho, Ataque, Cauda, Largura, Sub, Volume. Ao trocar de preset, os botões mudam para os valores desse preset (são o ponto de partida,
fáceis de alterar). Motores: subtrativo (osciladores + unison + filtro), FM (pianos/sinos) e cordas (guitarras/harpa).
Há ainda Mute e Sidechain interno por parte (para ouvires o "pumping"; no baixo vem ligado).

## Usar no FL Studio
1. Copia `Alvorada.vst3` para `C:\Program Files\Common Files\VST3\` e faz *Find installed plugins*.
2. Adiciona o Alvorada como gerador. Carrega em **▶ Ouvir (loop)** para ouvir as 4 partes sem teres de dar play ao FL
   (com o FL a tocar, o plugin segue o transporte).
3. **Arrastar MIDI**: cada parte tem o seu botão para arrastar o MIDI para o piano roll de outro instrumento (ou usa o
   *Arrastar MIDI (tudo)*, que leva as 4 partes em pistas/canais 1–4).
4. **Teclado ao vivo**: o MIDI que tocas no FL toca a parte escolhida no rodapé, com scale lock opcional.
5. A saída MIDI do plugin leva as notas das partes nos canais 1–4 (para roteares para outros instrumentos pelo Wrapper do FL).

## Compilar
- **GitHub Actions (sem instalar nada)**: faz push da pasta para um repositório, separador *Actions* → artefacto `Alvorada-VST3`.
- **Windows local**: VS 2022 + CMake + Git:
  ```
  cmake -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release --target Alvorada_VST3
  ```
  Resultado: `build/Alvorada_artefacts/Release/VST3/Alvorada.vst3`
- **Só motor + testes**: `cmake -B build -DALVORADA_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`
- **Teste de áudio offline** (opcional): `-DALVORADA_BUILD_RENDER_TEST=ON` e correr `AlvoradaRenderTest`.

## Estado e limites desta versão
- Motor testado (576 combinações estilo/tom/duração: tudo dentro da harmonia, determinístico). Compila em Linux e o teste de áudio
  offline passa (sem NaN/clipping nos 26 presets). **Não foi ouvido nem testado dentro do FL Studio**: o carácter dos sons
  (sobretudo piano FM e guitarras) é uma aproximação e vai precisar de afinação com os teus ouvidos.
- Os modos de sidechain são apenas de pré-escuta; no mix real usa o teu compressor com sidechain.
- JUCE é AGPLv3 ou comercial. O nome "Alvorada" é provisório: confirma que está livre antes de lançar.
