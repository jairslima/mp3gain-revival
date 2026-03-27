# Next Dev Message

Trabalho concluído até aqui:

- O escopo atual do projeto foi concluído para a baseline suportada.
- A baseline suportada está fechada em Windows + Linux.
- `cmake --build build`: OK
- `bash test/smoke_test.sh`: `27/27 passed`
- `pwsh -File test\probe_corruptions.ps1`: OK
- O pacote Linux final já foi gerado com `pwsh -File scripts/package_release.ps1 -Platform linux -Version 1.6.2-revival2`.
- O pacote Windows final já foi gerado com `pwsh -File scripts/package_release.ps1 -Platform windows -Version 1.6.2-revival2`.
- Em 2026-03-27 a suíte smoke foi corrigida para funcionar corretamente mesmo quando o `bash` seleciona o binário Windows (`build/Release/mp3gain.exe`).
- Em 2026-03-27 o probe de corrupção foi corrigido para detectar um binário Linux real (`build/mp3gain` ou `build-wsl/mp3gain`) em vez de depender de um caminho hardcoded inválido.
- Em 2026-03-27 a GUI Avalonia foi alinhada para a referência atual de loudness de `95.5 dB`, ganhou interações reais e passou a integrar o pacote Windows unificado.

Ponto pendente principal:

- Nenhum bloqueio técnico relevante ficou para trás no escopo atual.
- O único pendente externo é publicar/taggear a release `1.6.2-revival2` quando o processo de credenciais permitir.

Próximo passo recomendado:

1. Publicar a release usando os pacotes validados:
   - `dist/mp3gain-linux-1.6.2-revival2.zip`
   - `dist/mp3gain-windows-1.6.2-revival2.zip`
2. Revisar o conteúdo final da release com base em:
   - `docs/RELEASE.md`
   - `CHANGELOG.md`
   - `STATUS.md`
   - `HANDOFF.md`
   - `docs/AUDIT_2026-03-27.md`

Decisões já tomadas:

- Baseline suportada da primeira release: Windows + Linux
- macOS: fora da primeira release até validação real
- discrepância MP3Gain/libmpg123 vs `ffmpeg`: tratada como known issue da primeira release
- a GUI Avalonia deixou de ser só trilha separada e passou a compor o pacote Windows entregue no escopo atual

Arquivos mais importantes para continuidade:

- `STATUS.md`
- `HANDOFF.md`
- `docs/VALIDATION.md`
- `docs/RELEASE.md`
- `scripts/package_release.ps1`
- `test/probe_corruptions.ps1`
- `test/probe_corruptions.results.json`
- `docs/AUDIT_2026-03-27.md`
