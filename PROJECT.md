# MP3Gain Revival by Jair Lima

## Descrição
Fork e continuação do MP3Gain original (abandonado), mantido sob LGPL 2.1 ou posterior.
Objetivo: modernizar o CLI em C para um projeto cross-platform mantível.

## Stack e Dependências
- **Linguagem:** C99
- **Build system:** CMake 3.16+
- **Compilador testado:** MSVC 19.50 (VS BuildTools 2026)
- **Dependência crítica:** libmpg123
  - Build local em: `vcpkg_installed/vcpkg/blds/mpg123/x64-windows-rel/src/libmpg123/`
  - Header: `vcpkg_installed/vcpkg/blds/mpg123/src/-0852196a3c.clean/src/include/mpg123.h`
  - DLL de runtime copiada para: `build/Release/mpg123.dll`

## Estrutura de Pastas
```
project/          <- Código ativo (tree principal de trabalho)
  mp3gain.c       <- Entry point, buffers, gain I/O, funções de baixo nível
  process.c/h     <- Orquestração principal: batch, análise, decoder mpg123
  cli.c/h         <- Parsing de argumentos CLI
  prep.c/h        <- Preparação por arquivo, flags de recálculo
  exec.c/h        <- Ações simples (undo, direct gain, delete tag)
  gain_analysis.c/h  <- Algoritmo ReplayGain 1.0
  apetag.c/h      <- Leitura/escrita de tags APE
  id3tag.c/h      <- Leitura/escrita de tags ID3v2
  rg_error.c/h    <- Tratamento de erros
  CMakeLists.txt  <- Build moderno (substitui .sln/.dsp)
docs/             <- BUILD.md, ARCHITECTURE.md
archive/          <- Material histórico preservado (não modificar)
build/            <- Gerado pelo CMake (ignorar em buscas)
vcpkg_installed/  <- Dependências (ignorar em buscas)
```

## Comandos Essenciais

### Configurar build (primeira vez ou após --fresh)
```powershell
cmake -S project -B build `
  -DMPG123_INCLUDE_DIR="vcpkg_installed/vcpkg/blds/mpg123/src/-0852196a3c.clean/src/include" `
  -DMPG123_LIBRARY="vcpkg_installed/vcpkg/blds/mpg123/x64-windows-rel/src/libmpg123/mpg123.lib"
```

### Compilar
```powershell
cmake --build build --config Release
```

### Testar
```powershell
powershell -Command "& 'build/Release/mp3gain.exe' '-v'"
powershell -Command "& 'build/Release/mp3gain.exe' '-h'"
```

### Analisar arquivo MP3
```powershell
build\Release\mp3gain.exe arquivo.mp3
```

## Decisões Arquiteturais

### Visibilidade de funções (static removido)
Funções originalmente `static` em mp3gain.c foram tornadas externas para permitir
chamadas a partir dos novos módulos (process.c, exec.c, cli.c):
- `fillBuffer`, `skipID3v2`, `frameSearch`, `getSizeOfFile`
- `changeGain`, `showVersion`, `errUsage`, `fullUsage`, `wrapExplanation`

### frequency[] não é mais static
`frequency[4][4]` em mp3gain.c tinha `static` (linkage interno). Removido para
que process.c possa referenciar via `extern`.

### rg_error.h usa guards de macro
`MMSYSERR_ERROR` e `WAVERR_BADFORMAT` agora têm guards `#ifndef` para não conflitar
com `<windows.h>` quando incluído no bloco WIN32.

### mpg123.dll precisa estar junto do .exe
A DLL de runtime do mpg123 deve estar no mesmo diretório que mp3gain.exe.
Localização original: `vcpkg_installed/vcpkg/blds/mpg123/x64-windows-rel/src/libmpg123/mpg123.dll`

## Estado Atual
- **Fase:** 4 do Roadmap (Release Readiness) - **CONCLUÍDA (v1.6.2-revival1)**
- **Subfases da Phase 3:**
  - **3.1 Baseline smoke:** concluída
  - **3.2 Comportamento stateful do CLI:** concluída
  - **3.3 Falha parcial e segurança de batch:** concluída
    - **3.3.1** split de caminhos monolíticos de write/error: concluído
    - **3.3.2** falha por arquivo explícita: concluída
    - **3.3.3** redução de estado low-level exposto: concluída para o escopo de segurança de batch
    - **3.3.4** documentação dos exits aceitáveis: concluída
  - **3.4 CI e validação cross-platform:** concluída
    - **3.4.1** matriz explícita de plataformas validadas: concluída
    - **3.4.2** cobertura de CI alinhada com plataformas validadas: concluída
    - **3.4.3** comandos de reprodução local: concluída
    - **3.4.4** política de suporte e pendências explícitas: concluída
  - **3.5 Regressões de corrupção/decode:** concluída
    - **3.5.1** classes de corrupção: concluído
    - **3.5.2** harness determinístico: concluído
    - **3.5.3** matriz reproduzível de resultados: concluído
    - **3.5.4** promoção para baseline estável: concluído
    - **3.5.5** documentação explícita da lacuna/comportamento atual: concluído
  - **3.6 Handoff para release:** concluída
    - **3.6.1** checklist de release alinhado com a baseline atual: concluído
    - **3.6.2** shape de pacote para plataformas validadas: concluído
    - **3.6.3** blockers explícitos até a Phase 4: concluído
    - **3.6.4** gate concreto para entrada na Phase 4: concluído
- **Build Windows:** Funcional localmente com MSVC e libmpg123
- **Build Linux:** Funcional (validado em WSL2/Ubuntu 24.04, gcc 13.3, cmake 3.28, libmpg123-dev 1.32.5)
- **Binário Windows:** `build/Release/mp3gain.exe` versão 1.6.2
- **Binário Linux:** `build/mp3gain` versão 1.6.2
- **Análise por faixa:** FUNCIONANDO
- **Análise de álbum:** FUNCIONANDO (dois arquivos ou mais)
- **Aplicação de gain por faixa (`-r`):** FUNCIONANDO
- **Undo de gain (`-u`):** FUNCIONANDO
- **Aplicação de gain de álbum (`-a`):** FUNCIONANDO
- **REPLAYGAIN_REFERENCE_LOUDNESS:** Corrigido de 89.0 para 95.5 dB
- **Smoke tests:** `27/27` passando com fixtures reproduzíveis em `test/fixtures/`
- **Cobertura smoke atual:** versão, análise por faixa, análise de álbum, apply track, undo, apply album, round-trip APE, separação ID3/APE, auto-clip e lote misto com arquivo inválido
- **CI:** workflows Windows e Linux presentes em `.github/workflows/ci.yml`
- **Baseline de suporte atual para release:** Windows + Linux
- **macOS:** fora da promessa de suporte imediato até validação real
- **Tratamento de falha parcial:** o caminho principal de processamento não aborta mais todo o batch no primeiro MP3 ruim
- **Probe de corrupção:** `test/probe_corruptions.ps1` documenta e reproduz o estado atual da investigação de corrupção/decode
  - matriz persistida em `test/probe_corruptions.results.json`
  - truncado: falha com `exit 1`
  - vários candidatos estáveis produzem erro real de decode no `ffmpeg`
  - o caminho atual de MP3Gain/libmpg123 ainda tolera payload/header-window/scattered-zero/sync-byte/crc-sideinfo/transition-gap/main-data-begin/reservoir-burst/cross-frame-stitch/sideinfo-shear/late-payload-scramble/late-header-poison/frame-cluster-poison com `exit 0`
  - essa discrepância está tratada como limitação conhecida da primeira release, não como bloqueio de verificação

## Bugs Corrigidos na Fase 3 (2026-03-16)
Três bugs de aliasing cross-TU foram encontrados e corrigidos em `process.c`:

1. **`extern unsigned char *buffer` (ponteiro) vs `unsigned char buffer[]` (array):**
   `wrdpntr = buffer` em `mp3gain_prime_mp3_frames` carregava os primeiros 8 bytes
   do conteúdo do arquivo como endereço, travando dentro de `skipID3v2`.
   Fix: `extern unsigned char buffer[];`

2. **Parâmetro `curframe` sombreando o global na loop de frames:**
   `mp3gain_process_mp3_frames` e funções filhas recebiam `curframe` como parâmetro,
   que ficava sempre apontando para o primeiro frame. `frameSearch()` atualizava
   apenas o global em mp3gain.c, causando loop infinito no frame 2.
   Fix: removido o parâmetro `curframe` de todas as funções da cadeia; usam o
   global `curframe` (extern) diretamente.

3. **`if (*ok)` bloqueando `mp3gain_finish_track_recalc`:**
   Ao terminar o arquivo (EOF), `frameSearch` retorna 0, `ok=0`, e a função de
   finalização da análise nunca era chamada. EOF normal não é erro.
   Fix: não sobrescrever `*ok` com o retorno da loop; deixar `*ok = 1`.

## Próximos Passos
1. Continuar o cleanup estrutural do estado legado (desacoplamento do frame-scan) como refatoração contínua, sem tratá-lo como bloqueio da release.
2. Manter macOS fora da promessa de suporte imediato até validação real.
3. Reavaliar depois se a discrepância entre MP3Gain/libmpg123 e o oráculo `ffmpeg` precisa de trabalho corretivo pós-release.

Observação:
- os `exit(...)` restantes no código ativo estão agora concentrados principalmente em caminhos de CLI/usage, não no fluxo principal por arquivo
- o estado low-level remanescente em `mp3gain.c` continua sendo débito estrutural, mas a semântica de falha parcial e segurança de batch já está estabilizada

## Problemas Conhecidos
- A DLL do mpg123 precisa ser distribuída junto com o executável no Windows
- O build do vcpkg local usa a build de debug do mpg123 para dbg e release compilada manualmente
- A cobertura automatizada ainda é parcial; os smoke tests estão fortes, mas ainda faltam regressões mais específicas para corrupção de frame/decode
- O código de suporte a DLL (replaygaindll.c) não está no path de build principal
- O núcleo legado ainda mantém bastante estado global em `mp3gain.c`

## Ponto de Retomada Técnico
- O próximo corte técnico recomendado agora é **continuar o desacoplamento do frame-scan**.
- O objetivo é reduzir dependência de globais compartilhadas sem alterar comportamento.
- Focos imediatos:
  - ~~helpers ainda presos em `mp3gain.c`, especialmente `skipID3v2`, `frameSearch` e o restante do cursor de bits do write path~~ -> **CONCLUÍDO:** Cursor de bits e arrays locais consolidados na estrutura `MP3ScanState`.
  - ~~estado global cruzando `cli.c`, `prep.c`, `exec.c`, `process.c` e `mp3gain.c`~~ -> **CONCLUÍDO:** Migrados para a struct protegida `MP3GainGlobalConfig` em `mp3gain_config.h`.
  - conversão de falhas recuperáveis restantes para tratamento por arquivo
- Ordem sugerida:
  1. ~~continuar escondendo estado de scan atrás de helpers/contextos~~ -> **CONCLUÍDO:** Isolado usando Macro Bridge para preservar a compatibilidade e modularizar o bitstream.
  2. adicionar regressão para corrupção real de frame/decode
  3. manter a história de plataformas explícita: Windows/Linux validados, macOS pendente
  4. só depois partir para empacotamento/release
