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
- **Fase:** 2 do Roadmap (Build Recovery) - **CONCLUÍDA**
- **Build:** Funcional no Windows com MSVC e libmpg123
- **Binário:** `build/Release/mp3gain.exe` - confirmado funcionando (versão 1.6.2)
- **Git:** Repositório inicializado, primeiro commit feito

## Próximos Passos (Fase 3 - Verificação)
1. Testar análise real de arquivos MP3 (`mp3gain.exe arquivo.mp3`)
2. Testar aplicação de gain (`mp3gain.exe -r arquivo.mp3`)
3. Adicionar testes smoke básicos
4. Configurar CI (GitHub Actions para Windows)
5. Verificar build no Linux (via WSL ou CI)
6. Publicar no GitHub como fork LGPL oficial

## Problemas Conhecidos
- A DLL do mpg123 precisa ser distribuída junto com o executável
- O build do vcpkg local usa a build de debug do mpg123 para dbg e release compilada manualmente
- Não há testes automatizados ainda
- O build Linux ainda não foi validado neste workspace
- O código de suporte a DLL (replaygaindll.c) não está no path de build principal
