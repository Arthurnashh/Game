# Investigação do menu inicial — DayZ 0.45.124252

Data: 2026-11-16

## Evidência local

- `Addons/ui.pbo` existe (48,353,462 bytes) e foi extraído apenas para pesquisa em `build/main-menu-pbo-research-20261116/ui`. A extração não altera o PBO original.
- O PBO contém `config.bin`, recursos gráficos e scripts GUI clássicos. `scripts/gui/rscdisplaymain.sqf` é apenas o dispatcher genérico da classe (guarda `_mode`, `_params`, `_class` e `myDisplay`); não contém a implementação do menu DayZ nem lógica de stats/personagens.
- O logo já está disponível no conteúdo empacotado: `\\DZ\\ui\\data\\Logos\\dayz_ca.paa`. A classe de menu também referencia ícones GUI existentes no `ui.pbo`.
- O ponto de entrada funcional do menu na source é `lib/UI/optionsUI.cpp`: `DisplayMain::DisplayMain` carrega `RscDisplayMain`; `DisplayMain::OnButtonClicked` encaminha Play, Character, Settings, Exit, Rename e Change Server para os handlers existentes.
- O recurso de tela está em `cfg/Bin/rscScreen.hpp`. Já existe ali um protótipo visual com logo, preview 3D, painel, botões e IDs 12040–12050 para estatísticas. Os valores ainda são `--`; não há atualização desses IDs em `optionsUI.cpp`.
- `lib/Network/network.hpp` declara somente `characterTimeAlive` entre as estatísticas Steam deste ramo. Não encontrei no menu uma fonte já conectada para kills, infectados ou distância.
- O seletor atual de Character encaminha ao fluxo legado de aparência. Trocar aparência não cria uma identidade de personagem com inventário separado. Personagens independentes exigem uma chave de identidade/slot no salvamento do servidor e associação dessa chave ao login; alterar apenas `ui.pbo` não separa loot.

## Conclusão sobre os PBOS

Sim, o `ui.pbo` é parte do caminho para customizar a interface: contém o pacote de recursos/configuração que o cliente carrega. Mas a lógica que torna os botões funcionais já está na source C++, e as telas C++ dependem de classes Rsc fornecidas pelos addons. O menu desejado atravessa pelo menos três camadas: recurso Rsc/config, handlers C++ do client e persistência/identidade no servidor. Não é uma recompilação de `ui.pbo` isolada.

## Próximos passos de implementação

1. Promover o protótipo de `cfg/Bin/rscScreen.hpp` para a fonte canônica usada no build de addons; evitar editar snapshots em `cfg/build`.
2. Conectar setas a uma seleção de personagem real, sem confundir a seleção visual legada com um survivor salvo.
3. Definir uma identidade de personagem por slot no servidor e separar os arquivos/registros de persistência; só então a troca preserva loot/histórico por personagem.
4. Instrumentar e persistir tempo vivo, kills de jogadores, infectados e distância no servidor; publicar os valores ao menu sem usar placeholders estáticos.
5. Manter Play/Change Server/Rename/Mods ligados aos handlers nativos e validar a tela compilada com o mesmo conjunto de addons do client.

## Limite desta etapa

A análise local confirmou onde estão os arquivos e que o PBO contém assets/scripts genéricos; ainda não foi alterado nenhum addon/PBO nem foi implementada persistência multi-personagem ou coleta de estatísticas. A pasta extraída é material de pesquisa, não uma fonte de build.

## Confirmação do salvamento e avanço de código

A extração de server_data.pbo confirmou o fluxo da missão:

- scripts/functions/dbloadplayer.sqf usa getClientUID como chave em dbFindCharacter, dbServerLoadCharacter, dbCreateCharacter, dbKillCharacter e nos salvamentos.
- Ao conectar, _createPlayer procura a chave base antes de decidir se há survivor vivo; clientReady carrega a mesma chave. clientNew também cria o primeiro personagem sob essa chave.
- Portanto, múltiplos slots pedem um protocolo de seleção client→server disponível antes da busca inicial e aplicado de modo consistente a todas essas operações. Só acrescentar setas no menu faria uma seleção falsa e poderia carregar/sobrescrever o survivor errado.

Foi iniciado um primeiro ajuste funcional no menu: o botão nativo de launcher de mods, já existente no engine, agora é mantido visível no menu moderno, aparece abaixo do painel como MODS e continua abrindo DisplayModLauncher. Os arquivos C++ e Rsc foram copiados antes da alteração para backup-main-menu-20261116-0752. Ainda não foi criado um ícone de pata dedicado; o PBO inspecionado não tem um recurso de pata identificado pelo nome.

## Estado da implementação

- Ações nativas existentes (Play, Rename, Change Server e Mods): encaminhadores de UI já existem no C++; o botão Mods foi exposto no layout atual.
- Logo e preview 3D: o logo DayZ e o modelo de survivor já são carregados pelo recurso do menu.
- Estatísticas: controles visuais existem, mas coleta/persistência para as quatro métricas ainda precisa de implementação.
- Seleção de personagem com inventário isolado: não implementada; precisa do identificador de slot no handshake e no ciclo de persistência do servidor.

## Implementação em andamento — 2026-11-16

A fonte agora contém o primeiro fluxo integrado para três slots de survivor:

- As setas do menu ciclam os slots 1–3 e persistem a seleção em `profileNamespace` (`dayzMainMenuCharacterSlot`).
- Slot 1 preserva o UID antigo; slots 2 e 3 usam UIDs derivados (`_survivor_1` e `_survivor_2`). O servidor usa o mesmo UID no load, save, disconnect, respawn e morte, isolando o inventário no banco existente.
- O servidor acumula sobrevivência e distância a pé, contabiliza kills de jogadores e infectados, e salva os valores no `profileNamespace` do servidor. A missão envia o snapshot ao cliente, que o guarda no profile local; o menu lê os valores do slot selecionado. Antes do primeiro sync, mostra `NOT SYNCED` em vez de números inventados.
- O overload nativo de `dbServerLoadCharacter` aceita `[id, playerUID]`. O callback recebe o UID puro, enquanto a chamada HTTP constrói separadamente o parâmetro `?uid=`.
- A revisão dos stats e slots está em `build/main-menu-pbo-research-20261116`; PBOS candidatos ficam em `build/main-menu-live-data-20261116/packed`. Os PBOS originais em `Addons` não foram substituídos.
- Backups dos arquivos C++/Rsc e SQF estão em `backup-main-menu-multislot-20261116-0810` e `backup-main-menu-live-data-20261116`.

### Validação e limitações atuais

Os PBOS candidatos foram empacotados com FileBank e seus diretórios foram conferidos com BankRev. Ainda não houve teste em jogo nem compilação final. A tentativa de build com o VS 2017 instalado foi bloqueada: inicialmente faltava o Windows SDK 8.1; ao apontar para o SDK 10.0.19041, o compilador v141 falhou em código legado do motor (`Matrix3P`, C2280), antes de validar a alteração de UI. Esta source pede toolchain antigo (v100). Também não há chave privada de assinatura no workspace; os PBOS candidatos precisam ser assinados antes de substituir os originais em um servidor que valide assinaturas.

A métrica de distância conta deslocamento a pé; a cache do menu guarda o último snapshot recebido do servidor. A integração está implementada no código-fonte, mas precisa de build com o toolchain compatível e teste cliente/servidor para confirmar o comportamento real.
