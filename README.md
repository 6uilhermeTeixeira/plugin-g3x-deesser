# G3X DeEsser

G3X DeEsser é um plugin de redução de sibilância em desenvolvimento. A
primeira entrega será um VST3 64-bit para Windows, desenvolvido em C++20 com
JUCE e CMake, seguindo o processo de produto e validação do DarkVox.

## Estado

**M3 — interface em validação.** O projeto já possui plugin JUCE compilável,
parâmetros automatizáveis, processamento Wideband/Split, Range, bypass,
monitoração do sidechain, transições suavizadas, medição lock-free e testes de
DSP. A interface original G3X inclui layout redimensionável, medidores a 40 Hz,
seis presets de fábrica, edição numérica e navegação por teclado.

- [PRD](PRD.md)
- [Referência visual e fontes](docs/references/README.md)
- [Captura da interface de referência](docs/references/waves-deesser-interface.png)
- [Marcos de desenvolvimento](docs/MILESTONES.md)

![Interface do Waves DeEsser usada como referência de ergonomia](docs/references/waves-deesser-interface.png)

## Limites da referência

O Waves DeEsser foi estudado somente para entender fluxo de trabalho,
hierarquia de controles e terminologia comum. O G3X DeEsser deverá possuir DSP,
marca, código, interface, textos, componentes gráficos e presets originais.

Nenhum ativo da Waves será incorporado ao produto final.

## Build rápido

```bash
cmake --preset debug
cmake --build --preset debug --parallel 2
ctest --preset debug
```
