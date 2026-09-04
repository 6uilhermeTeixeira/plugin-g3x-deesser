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

## Download e instalação — Windows x64

1. Abra [Actions](https://github.com/6uilhermeTeixeira/plugin-g3x-deesser/actions) e selecione uma execução bem-sucedida da branch `main`.
2. Em **Artifacts**, baixe `G3X-DeEsser-Windows-x64-<commit>`. O download fica disponível por 30 dias; **Run workflow** permite gerar um novo build.
3. Extraia o ZIP. A raiz contém somente `SHA256SUMS.txt` e a pasta `G3X DeEsser.vst3`, com todos os arquivos internos do plugin.
4. Na pasta extraída, abra o PowerShell e verifique o binário:

```powershell
$expected, $relativePath = (Get-Content -LiteralPath .\SHA256SUMS.txt -Raw).Trim() -split '  ', 2
$actual = (Get-FileHash -LiteralPath $relativePath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $expected) { throw "SHA-256 divergente; baixe o artifact novamente." }
"SHA-256 confirmado."
```

5. Copie a pasta **`G3X DeEsser.vst3` inteira** para `C:\Program Files\Common Files\VST3` e atualize a busca de plugins da DAW. A cópia pode solicitar permissão de administrador.

O SHA-256 verifica o binário Windows x64 dentro do bundle; não é o hash do ZIP ou dos recursos. O artifact contém o VST3 Release; o aplicativo Standalone continua disponível como alvo de compilação, mas não é incluído no download.
