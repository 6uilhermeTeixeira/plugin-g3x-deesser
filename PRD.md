# G3X DeEsser — Product Requirements Document

**Versão:** 0.1.0  
**Status:** proposta para confirmação  
**Target inicial:** C++20, JUCE fixado, CMake  
**Entrega inicial:** VST3 64-bit para Windows; Standalone para desenvolvimento

## 1. Visão do produto

G3X DeEsser é um processador vocal para controlar sibilância com poucos
controles, leitura visual imediata e resultado transparente. O usuário escolhe
a região problemática, ajusta a sensibilidade e limita a redução máxima sem
precisar montar um compressor multibanda.

A ergonomia é informada pelo Waves DeEsser: seleção entre processamento de
banda dividida e sinal completo, detector passa-altas ou passa-faixa,
monitoração do sidechain, threshold e medição clara da atenuação. A
implementação e a identidade visual serão próprias.

## 2. Objetivos

- Reduzir sons como “s”, “sh” e “ch” sem tornar a voz opaca ou artificial.
- Permitir localizar rapidamente a sibilância ouvindo o sidechain.
- Oferecer modos Split e Wideband com transição sem clique.
- Manter baixa latência e uso previsível de CPU para gravação e mixagem.
- Preservar automação e sessões por meio de IDs de parâmetros estáveis.
- Operar corretamente em mono, mono-to-stereo e estéreo linkado.

## 3. Usuário e casos de uso

- Vocais cantados, voz falada, podcast e locução.
- Redução de aspereza em pratos, overheads e outros materiais brilhantes.
- Correção rápida durante gravação e ajuste mais preciso durante mixagem.

O primeiro release não promete restauração de áudio severamente distorcido nem
separação fonética baseada em inteligência artificial.

## 4. Controles públicos propostos

### 4.1 Frequency (`frequencyHz`)

- Faixa inicial: 2 kHz a 16 kHz, escala logarítmica.
- Padrão inicial: 5.5 kHz.
- Define a frequência do filtro do detector e o crossover do modo Split.
- Exibição em Hz/kHz e edição direta pelo teclado.

### 4.2 Threshold (`thresholdDb`)

- Faixa inicial: -80 a 0 dBFS.
- Padrão inicial: -30 dBFS.
- Determina quando o envelope do sidechain inicia a redução.
- Suavização obrigatória para automação sem zipper noise.

### 4.3 Range (`rangeDb`)

- Faixa: 0 a 24 dB de atenuação máxima.
- Padrão inicial: 8 dB.
- Diferencial próprio em relação à referência: impede redução excessiva e torna
  o resultado mais previsível.

### 4.4 Detector (`detectorMode`)

- `HighPass`: reage ao conteúdo acima da frequência selecionada; apropriado
  para sibilância distribuída numa região ampla.
- `BandPass`: concentra a detecção ao redor da frequência escolhida; apropriado
  para ressonâncias específicas.

### 4.5 Processing (`processingMode`)

- `Split`: atenua somente a banda alta separada pelo crossover.
- `Wideband`: reduz o sinal completo quando o detector dispara.

### 4.6 Monitor (`monitorMode`)

- `Audio`: saída processada normal.
- `Sidechain`: escuta somente o sinal que alimenta o detector.
- A troca deve ser suavizada e o estado deve ser salvo com a sessão.

### 4.7 Parâmetros auxiliares

- `bypass`: bypass interno com fade curto e sem clique.
- Link estéreo permanente no primeiro release; opção de link configurável fica
  para avaliação posterior.

Os IDs acima são contratos de compatibilidade após o primeiro beta público.

## 5. Processamento proposto

```text
Input
  -> segurança contra denormals/NaN/Inf
  -> sidechain HP ou BP
  -> detector de envelope
  -> curva dinâmica hard-knee inicial
  -> redução limitada por Range
  -> ganho Wideband ou ganho da banda alta do crossover
  -> Output
```

### 5.1 Detector

- Envelope rápido, com attack inicial entre 0.1 e 1 ms.
- Release inicial entre 40 e 120 ms, a validar por testes auditivos.
- Coeficientes derivados do sample rate.
- Detecção estéreo linkada pelo maior envelope para preservar a imagem.
- O filtro do sidechain não deve colorir o caminho de áudio em Wideband.

### 5.2 Curva de redução

- Protótipo inicia com hard knee, coerente com o fluxo simples estudado.
- Ratio efetivo e constantes temporais deverão ser escolhidos por testes, não
  por tentativa de clonagem do comportamento proprietário da Waves.
- A redução calculada deve respeitar `rangeDb` em todos os níveis de entrada.

### 5.3 Modo Split

- Crossover complementar e reconstrução com erro mínimo na região de passagem.
- A redução atua somente na banda alta.
- Mudanças de frequência e modo devem ser suavizadas.
- Latência reportada ao host deve permanecer correta; alvo inicial de zero
  samples se a topologia escolhida permitir.

## 6. Interface proposta

- Janela compacta, redimensionável e com leitura HiDPI.
- Coluna esquerda: Processing, Frequency, Detector e Monitor.
- Área central dominante: Threshold, nível do detector e redução de ganho.
- Área direita: Range e medidores de saída estéreo.
- Controles com valor numérico, unidade, reset por duplo clique e edição direta.
- Medidores atualizados entre 30 e 60 Hz, desacoplados da thread de áudio.
- Paleta, tipografia, ícones, espaçamento e componentes devem ser originais e
  claramente distintos da captura de referência.
- Navegação por teclado, foco visível e nomes acessíveis para leitores de tela.

## 7. Medição e feedback

- Medidor de nível do detector/threshold.
- Medidor de redução com pico retido curto e escala em dB.
- Medidor de saída mono ou estéreo.
- Indicador de atividade quando a redução ultrapassar 0.1 dB.
- Transferência DSP–UI por atomics ou FIFO lock-free.

## 8. Requisitos de tempo real

- Nenhuma alocação, mutex, I/O, logging ou chamada de UI em `processBlock`.
- Saída finita para silêncio, impulsos e níveis extremos.
- Funcionamento inicial de 44.1 a 192 kHz e buffers de 16 a 2048 samples.
- Estado serializado, versionado e compatível entre versões.
- Parâmetros e mudanças de topologia sem cliques.
- Processamento determinístico e coberto por testes automatizados.

## 9. Presets iniciais

- Vocal Female — Gentle
- Vocal Female — Focused
- Vocal Male — Gentle
- Vocal Male — Focused
- Speech
- Cymbal Tamer

Os presets serão criados do zero e deverão apenas servir como pontos de
partida; não serão copiados de produtos comerciais.

## 10. Critérios de aceitação do protótipo

- Build Debug e Release em Linux; build Release VST3 em Windows CI com MSVC.
- Testes das respostas dos filtros, curva estática, Range e link estéreo.
- Modo Split reconstrói sinal sem redução dentro da tolerância definida.
- Nenhuma descontinuidade audível em automação e troca de monitoração.
- Estado recuperado após salvar e reabrir o host.
- Plugin reconhecido como efeito e aprovado no pluginval/VST3 Validator antes
  do beta.
- Validação manual no FL Studio em Windows, 44.1/48 kHz e buffers de
  64/256/1024 samples.

## 11. Fora do escopo inicial

- Reprodução exata ou engenharia reversa do Waves DeEsser.
- Uso de marca, código, assets, presets ou trade dress da Waves.
- Detecção por IA, processamento mid/side e sidechain externo.
- AAX, formatos nativos de DAWs, iOS e versão final para macOS.

## 12. Marcos propostos

1. **M0 — Fundação:** PRD aprovado, naming, licença e arquitetura.
2. **M1 — Detector:** filtros, envelope e testes unitários.
3. **M2 — DSP:** Wideband, Split, Range e link estéreo.
4. **M3 — Interface:** controles, medidores, presets e acessibilidade.
5. **M4 — Windows Alpha:** CI MSVC, artefato VST3 e validação no FL Studio.
6. **M5 — Beta:** validadores, regressão, documentação e empacotamento.

## 13. Decisões que precisam de confirmação

- Nome público: `G3X DeEsser` ou outra marca.
- Manter `Range` visível ou buscar uma interface ainda mais minimalista.
- Priorizar transparência ou caráter mais agressivo como padrão.
- Disponibilizar Standalone ao usuário ou mantê-lo apenas para desenvolvimento.
- Modelo de licença do produto e compatibilidade com a licença do JUCE.

## 14. Fontes de pesquisa

- [Página oficial do Waves DeEsser](https://www.waves.com/plugins/deesser)
- [Manual oficial do Waves DeEsser (PDF)](https://assets.wavescdn.com/pdf/plugins/deesser.pdf)
- [Imagem oficial do produto](https://media.wavescdn.com/images/products/plugins/600/deesser.png)

Consulta realizada em 4 de setembro de 2026. As fontes são documentação de
referência; não constituem especificação de clonagem.

