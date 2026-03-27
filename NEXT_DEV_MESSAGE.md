# Next Dev Message

Trabalho concluído até aqui:

- Phase 3 foi fechada para a baseline Windows/Linux.
- O projeto está em **Phase 4: Release Readiness**.
- `cmake --build build`: OK
- `bash test/smoke_test.sh`: `27/27 passed`
- `pwsh -File test\probe_corruptions.ps1`: OK
- O pacote Linux já foi gerado com `pwsh -File scripts/package_release.ps1 -Platform linux -Version phase4-dev`.
- O pacote Windows também já foi gerado com `pwsh -File scripts/package_release.ps1 -Platform windows -Version phase4-dev`.
- Em 2026-03-27 a suíte smoke foi corrigida para funcionar corretamente mesmo quando o `bash` seleciona o binário Windows (`build/Release/mp3gain.exe`).
- Em 2026-03-27 o probe de corrupção foi corrigido para detectar um binário Linux real (`build/mp3gain` ou `build-wsl/mp3gain`) em vez de depender de um caminho hardcoded inválido.
- Em 2026-03-27 a GUI Avalonia foi alinhada para a referência atual de loudness de `95.5 dB`.

Ponto pendente principal:

- Publicar a primeira release pública da baseline CLI Windows/Linux a partir dos artefatos já validados.
- Versão candidata preparada para essa publicação: `1.6.2-revival2`.

Próximo passo recomendado:

1. Escolher a versão/tag final da primeira release pública.
2. Publicar a release usando os pacotes já validados:
   - `dist/mp3gain-linux-phase4-dev.zip`
   - `dist/mp3gain-windows-phase4-dev.zip`
3. Revisar o conteúdo final da release com base em:
   - `docs/RELEASE.md`
   - `CHANGELOG.md`
   - `STATUS.md`
   - `HANDOFF.md`
   - `docs/AUDIT_2026-03-27.md`

Decisões já tomadas:

- Baseline suportada da primeira release: Windows + Linux
- macOS: fora da primeira release até validação real
- discrepância MP3Gain/libmpg123 vs `ffmpeg`: tratada como known issue da primeira release
- a GUI Avalonia continua como trilha separada de expansão, não como bloqueio da release CLI

Arquivos mais importantes para continuidade:

- `STATUS.md`
- `HANDOFF.md`
- `docs/VALIDATION.md`
- `docs/RELEASE.md`
- `scripts/package_release.ps1`
- `test/probe_corruptions.ps1`
- `test/probe_corruptions.results.json`
- `docs/AUDIT_2026-03-27.md`
