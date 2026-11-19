# Pendências: menu principal e inventário

Este documento registra o que falta para substituir de verdade as interfaces do DayZ 0.45.124252. O pacote V10 é intermediário; não deve ser tratado como versão pronta.

## Estado atual confirmado

- O `bin\rscScreen.hpp` recebeu alterações de layout para o menu e o inventário.
- O `config.bin` do V10 foi compilado, empacotado em `build\inventory-menu-v10\package-v10\dta\bin.pbo` e reextraído. O hash do `config.bin` extraído bate com o compilado.
- Há um ponto de restauração anterior às últimas mudanças em `build\inventory-menu-v10\20261116-restore-before-menu-inventory-fix`.
- O V10 mantém controles nativos do inventário (`IDC_INVENTORY_CONTAINER`, `IDC_INVENTORY_SOLDIER`, `IDC_INVENTORY_HANDS` e `IDC_INVENTORY_CONTENT`). Isso preserva a base de proximidade, preview, mãos e conteúdo, mas **não prova que o redesenho esteja completo nem validado dentro do jogo**.
- Na última captura fornecida, o menu ainda exibia elementos antigos. A última mudança de ordem de desenho foi compilada, mas não foi confirmada visualmente no cliente.
- O inventário mostrado ainda não corresponde integralmente à referência moderna. Não afirmar que ele foi substituído ou que está idêntico.

## Menu principal — pendências

- [ ] Remover visualmente o copyright antigo de 2013 e qualquer texto/arte legado, sem cobrir a tela com uma faixa ou bloco preto.
- [ ] Exibir somente a logo DayZ pequena, na posição e escala da referência do menu de pausa.
- [ ] Manter o copyright novo de 2019, uma única vez, sem duplicatas.
- [ ] Confirmar no jogo que não aparece “Early Access Alpha”, “UNREGISTERED STRING”, logo duplicada nem texto antigo.
- [ ] Conferir o fundo 3D e o personagem, sem personagens ou elementos indesejados ao fundo.
- [ ] Confirmar que Play, Change Server e Rename continuam abrindo as telas e executando as ações esperadas.
- [ ] Confirmar que as setas mudam de sobrevivente e que cada personagem preserva uma história/loot independente, se esse recurso continuar dentro do escopo.
- [ ] Substituir valores estáticos como “SYNCED” por estatísticas reais sincronizadas; validar tempo sobrevivido, jogadores mortos, infectados mortos e distância percorrida.
- [ ] Confirmar que o ícone de mods abre/representa os mods carregados.

## Inventário — pendências

- [ ] Remover a duplicação: deve existir uma só interface visível e interativa. Não deixar um inventário antigo por cima/atrás de um mockup novo.
- [ ] Fazer o layout novo ser a tela de inventário usada pelo jogo, não uma imagem ou painel decorativo separado.
- [ ] Reproduzir a composição da referência: arredores à esquerda, personagem 3D central, mãos abaixo do personagem e equipamento/cargas à direita.
- [ ] Integrar a barra de ícones de equipamento da referência. Localizar os ícones nos PBOs modernos e confirmar formato/caminho compatível com o cliente 0.45 antes de empacotar; não referenciar EDDS de Enfusion como se fossem PAA compatíveis.
- [ ] Fazer os itens realmente equipados aparecerem nos slots correspondentes, com estado vazio quando não houver item.
- [ ] Mostrar as cargas reais de roupas e recipientes (por exemplo camisa, calça e mochila), incluindo itens, capacidade, nome e estado aberto/fechado.
- [ ] Manter slot Hands visível e ligado ao item real nas mãos; drag-and-drop para/desde Hands precisa atualizar a tela e o estado do personagem.
- [ ] Manter proximidade dinâmica e permitir mover itens entre Vicinity, equipamento, mãos e cargas.
- [ ] Garantir que os controles novos não interceptem cliques/arrastos dos controles nativos e que a rolagem alcance toda a lista.
- [ ] Ajustar escala e posições para diferentes resoluções/aspect ratios sem cortar mãos, personagem, cargas ou atalhos.
- [ ] Validar no jogo com itens reais: vazio, roupa com carga, calça com item, mochila, lanterna/arma nas mãos, mover para inventário e jogar em Vicinity.

## Build, restauração e validação de entrega

- [ ] Preservar o ponto de restauração V10 antes de qualquer próxima edição; criar outro ponto versionado antes de mudanças adicionais.
- [ ] Compilar todos os fontes alterados e tratar avisos/erros de build relevantes.
- [ ] Reempacotar os recursos no PBO separado do build de teste.
- [ ] Extrair novamente os recursos empacotados e comparar hashes com os artefatos compilados.
- [ ] Abrir o executável do pacote isolado e conferir visualmente o menu e o inventário dentro do cliente.
- [ ] Testar as ações funcionais do inventário e as ações do menu; uma compilação sem erros sozinha não fecha essa validação.
- [ ] Confirmar compatibilidade entre os dados do cliente e do servidor usados no teste, incluindo WRP/revisão alinhados, para evitar “Corrupted data detected”.
- [ ] Atualizar o launcher e o README do pacote com instruções exatas, build/versão e limitações conhecidas.
- [ ] Só marcar como pronto e entregar o pacote depois de todas as verificações acima passarem.

## Artefatos intermediários

- Pacote V10: `build\inventory-menu-v10\package-v10`
- Launcher V10: `build\inventory-menu-v10\package-v10\Run-Inventory-Menu-V10.bat`
- Ponto de restauração V10: `build\inventory-menu-v10\20261116-restore-before-menu-inventory-fix`


## Atualização V32 — terreno e navegação de servidores

- [x] Comparar os dados de terreno usados pela V31 com os atuais: a comparação encontrou 509 arquivos de textura de layer diferentes no `worlds_chernarusplus_data.pbo` (ambos reportavam revision 17380).
- [x] Montar V32 com o pacote de dados de terreno atual e a assinatura correspondente, preservando o WRP revision 17865 alinhado ao servidor de teste.
- [x] Compilar o `bin.pbo` com apenas `OFFICIAL`, `COMMUNITIES` e `LAN` visíveis; deixar `HISTORY`, `FRIENDS` e `REMOTE` ocultas e manter IDs de controle de abas.
- [x] Restaurar a inicialização de `RscDisplayMain` e fechar o diálogo Early Access antigo.
- [x] Criar ponto de restauração versionado antes das alterações.
- [x] Compilar e validar sintaticamente o `config.bin` com CfgConvert; reextrair o PBO para verificar labels/classes e conferir hashes do pacote.
- [ ] Confirmar no cliente que o terreno não está mais vermelho/rosa.
- [ ] Confirmar no cliente que a cena 3D do menu principal voltou e o Early Access não aparece.
- [ ] Confirmar no jogo que o navegador mostra apenas as três abas desejadas e que ações de entrar/Change Server funcionam.
- Pacote isolado para teste: `build\inventory-menu-v10\package-v32-terrain-menu-repair\`.
- Launcher: `Start-DayZ-Terrain-Menu-V32.bat`.
- Restauração: `build\inventory-menu-v10\restore-point-v32-before-terrain-menu-repair-20261117\`.
- O servidor ativo não foi reiniciado nem alterado. A correção funcional só fica confirmada após a captura/teste do usuário.
### Atualização V32.1 — erro CfgAddons

- [x] Corrigir `prefix=bin\\` incorreto em `dta\bin.txt` para `prefix=bin\` e reempacotar o PBO.
- [x] Confirmar o prefixo com BankRev e comparar o hash do `config.bin` reextraído com o compilado.
- [ ] Reabrir o V32 e confirmar que o erro `No entry '.CfgAddons'` desapareceu.
- PBO corrigido: SHA-256 `03A7CAD7B75296BFE5DAC1E0ECCC91333A82F43493FCB4DEFEB19A8CF076CC9D`.
- Restore point anterior ao reparo: `build\inventory-menu-v10\restore-point-v32-cfgaddons-prefix-20261117\`.

### V32.1 — personagem sem resposta e sessão

- [ ] Reproduzir o sintoma com a janela do DayZ em foco; a captura tinha o Gerenciador de Tarefas em primeiro plano.
- [ ] Confirmar se o cliente continua conectado ao servidor quando o personagem para de responder. O log atual mostra `PLAYING -> MISSION ASKED` no cliente e desconexão no servidor após 119,672 s.
- [ ] Tratar os `Error loading control` de `RscDisplayMain` e `RscDisplayMultiplayer` que permanecem no RPT.
- [ ] Confirmar novamente terreno, cena 3D inicial e apenas as abas `OFFICIAL`, `COMMUNITIES` e `LAN`.
