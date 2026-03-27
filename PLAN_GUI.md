# Plano de Reconstrucao da GUI

Este documento guia o desenvolvimento da interface em Avalonia UI.

## Status
- [x] Etapa 1: Desenho da Tela e instalacao do DataGrid
- [x] Etapa 2: Estruturacao dos modelos e bindings
- [x] Etapa 3: Wrapper do motor CLI com execucao em background
- [ ] Etapa 4: Eventos de botoes e interacoes nativas do sistema

## Situacao Atual
- A GUI ja abre a janela principal e exibe o grid de arquivos.
- Ja existem modelos/bindings basicos para popular as colunas principais.
- O wrapper do CLI ja executa analise em background.
- A tela ja permite:
  - adicionar arquivos
  - limpar tudo
  - rodar analise
- O baseline de loudness da GUI ja foi alinhado para `95.5 dB`.

## O Que Ainda Falta
- Implementar `Adicionar Pasta`
- Implementar `Ganho Radio`
- Implementar `Limpar Arquivo(s)` para selecao parcial
- Implementar `Cancelar`
- Completar o comportamento dos menus superiores
- Refinar o texto visual padrao da interface para refletir o baseline atual em toda a tela
- Validar empacotamento/distribuicao propria da GUI quando esse track avancar

## Escopo
- A GUI continua nao bloqueando a conclusao da recuperacao do CLI.
- No Windows, o objetivo agora e distribuir GUI + CLI como um unico produto na mesma pasta final.
- A trilha separada pos-release continua valendo para evolucao funcional da GUI, nao para a organizacao do pacote.

## Proximo Corte Recomendado
1. Fechar a Etapa 4 para tirar a GUI do estado de prototipo.
2. Consolidar um pacote Windows unico em que `MP3GainUI.exe`, `mp3gain.exe` e `mpg123.dll` saiam juntos.
3. Depois decidir se a GUI passa a ter distribuicao propria ou continua apenas como wrapper experimental.
