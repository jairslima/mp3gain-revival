# Plano de Reconstrucao da GUI

Este documento guia o desenvolvimento da interface em Avalonia UI.

## Status
- [x] Etapa 1: Desenho da Tela e instalacao do DataGrid
- [x] Etapa 2: Estruturacao dos modelos e bindings
- [x] Etapa 3: Wrapper do motor CLI com execucao em background
- [x] Etapa 4: Eventos de botoes e interacoes nativas do sistema

## Situacao Atual
- A GUI ja abre a janela principal e exibe o grid de arquivos.
- Ja existem modelos/bindings basicos para popular as colunas principais.
- O wrapper do CLI ja executa analise em background.
- A tela ja permite:
  - adicionar arquivos
  - adicionar pasta
  - tocar o arquivo selecionado no player padrao
  - limpar selecao parcial
  - limpar tudo
  - rodar analise
  - aplicar ganho de faixa
  - cancelar operacao em andamento
- O baseline de loudness da GUI ja foi alinhado para `95.5 dB`.
- O menu foi reduzido ao que e funcional ou util no estado atual.
- A janela `Sobre` foi implementada com base no conteudo relevante do original.
- O pacote Windows final agora distribui GUI + CLI juntos.

## O Que Ainda Falta
- Album Gain e fluxo de album ainda podem ser adicionados em etapa futura.
- Iconografia e refinamento visual ainda podem aproximar mais a GUI do original.
- Preferencias avancadas do menu original continuam fora de escopo ate haver necessidade real.

## Escopo
- A GUI nao bloqueia mais a conclusao da recuperacao do CLI.
- No Windows, GUI + CLI ja sao distribuidos como um unico produto na mesma pasta final.
- A trilha futura passa a ser refinamento funcional/visual, nao integracao basica.

## Proximo Corte Recomendado
1. Se necessario, implementar Album Gain e operacoes de album.
2. Refinar iconografia e detalhes visuais da janela principal.
3. Só expandir preferências/menu antigo quando houver valor funcional real.
