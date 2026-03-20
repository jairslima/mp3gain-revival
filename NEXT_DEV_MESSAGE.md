# Next Dev Message

Trabalho concluído até aqui:

- Phase 3 foi fechada para a baseline Windows/Linux.
- O projeto está em **Phase 4: Release Readiness**.
- `cmake --build build`: OK
- `bash test/smoke_test.sh`: `27/27 passed`
- `pwsh -File test\probe_corruptions.ps1`: OK
- O pacote Linux já foi gerado com `pwsh -File scripts/package_release.ps1 -Platform linux -Version phase4-dev`.

Ponto pendente principal:

- O pacote Windows ainda não foi gerado neste workspace porque faltam:
  - `build/Release/mp3gain.exe`
  - `build/Release/mpg123.dll`

Próximo passo recomendado:

1. Produzir um snapshot Windows Release com esses dois artefatos.
2. Rodar:
   `pwsh -File scripts/package_release.ps1 -Platform windows -Version <tag>`
3. Revisar o conteúdo final da release com base em:
   - `docs/RELEASE.md`
   - `docs/RELEASE_CANDIDATE.md`
   - `CHANGELOG.md`
   - `STATUS.md`
   - `HANDOFF.md`

Decisões já tomadas:

- Baseline suportada da primeira release: Windows + Linux
- macOS: fora da primeira release até validação real
- discrepância MP3Gain/libmpg123 vs `ffmpeg`: tratada como known issue da primeira release

Arquivos mais importantes para continuidade:

- `STATUS.md`
- `HANDOFF.md`
- `docs/VALIDATION.md`
- `docs/RELEASE.md`
- `scripts/package_release.ps1`
- `test/probe_corruptions.ps1`
- `test/probe_corruptions.results.json`
