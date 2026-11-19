# DayZ 0.45.124252 — histórico de correções e pendências

Atualizado em 17/11/2026. Este arquivo reúne as alterações e os testes que aparecem nos fontes, nos pacotes de build e nos relatos desta sessão. “Preparado” ou “compilado” não significa que o comportamento foi confirmado dentro do jogo; isso fica indicado separadamente.

## Correção atual: porta, WRP e crash ao entrar

A porta voltou a falhar depois que novas builds foram criadas com dados de mapa do cliente diferentes dos dados usados pelo servidor. A V29, já alinhada ao WRP do servidor, ainda caiu ao entrar. Seu RPT registra o cliente chegando a `GAME LOADED`, muitas asserts `txtd3d9.cpp(4186): Assertion failed 'level >= 0'` e a exceção `C0000094 INT_DIVIDE_BY_ZERO` em `0177B412`. O servidor registra a conexão de Arthurnashh e um disconnect após 43,9761 segundos, sem rejeição explícita. Portanto, há evidência de uma falha adicional no cliente, na leitura de textura/mipmap; alinhar o WRP sozinho não resolveu o crash.

### Qual PBO contém o WRP?

- **PBO que contém o WRP:** `Addons\worlds_chernarusplus.pbo`
- Dentro dele, o arquivo chama-se `chernarusplus.wrp`.
- **Não confundir com:** `Addons\worlds_chernarusplus_data.pbo`, que guarda recursos/dados do mapa e não contém o WRP.
- A revisão pode ser consultada com `BankRev -properties worlds_chernarusplus.pbo`.

Antes do alinhamento, os dados eram:

| Arquivo | Revisão do PBO | SHA-256 |
|---|---:|---|
| Cliente V27: `worlds_chernarusplus.pbo` | 17983 | `0EF016A7FF6E3339C55B33B1BE04C4B77ED770385A63899CD3C1C1E24760F640` |
| Servidor: `worlds_chernarusplus.pbo` | 17865 | `0B8DAD26F376B892376C45240D630C14C126A84D0C31D0492384EF9371C78E28` |

Os WRP extraídos também eram diferentes: cliente V27 `AB0BABDB1802A0F1EBEEC28526A8F4AAFB84D00C494C03AA5FAF09B1E69FDB49`; servidor `2F3FBA7A47CEBF7AE369A805D1E7CB88668BF9B085DF36CAAF1789B50CDFB737`. O `config.bin` dentro desse PBO tinha o mesmo SHA-256 nos dois lados, então o desvio encontrado é especificamente o WRP/revisão do PBO.

### Build de teste alinhada

Foi preparada a pasta isolada:

`build\inventory-menu-v10\package-v28-wrp-aligned\`

Ela herda a V27 e troca somente `Addons\worlds_chernarusplus.pbo` e sua assinatura pela cópia correspondente do servidor. O PBO da V28 e o do servidor agora têm revisão 17865, tamanho 120164803 bytes e SHA-256 idêntico:

`0B8DAD26F376B892376C45240D630C14C126A84D0C31D0492384EF9371C78E28`

A V28 foi mantida como build isolada apenas para alinhar WRP. A V29 alinhou o WRP e integrou o navegador, mas o cliente ainda caiu ao conectar; a V30 abaixo contém a proteção adicional de mipmap.

Ponto de restauração criado em:

`build\inventory-menu-v10\restore-point-v27-before-wrp-align-20261116\`

Ele preserva o PBO original V27, sua assinatura e uma cópia de referência do PBO do servidor. Nenhum PBO de `dist\server` ou do diretório fonte `Addons` foi alterado.

## Histórico de trabalho

### Colisão e navegação de zumbis

- Foram preparados executáveis de servidor de diagnóstico e correção em `dist\server`, incluindo `DayZServer-ZombieCollisionFix.exe`, versões V104–V121, testes de varredura/subdivisão e o teste `DayZServer-ZombieStuckCollision.exe`.
- Foram criadas configurações e missões de teste em `dist\server\!ConfigServer` e diretórios `TmpZombieCollision*` para investigar colisão, travamento e passagem por objetos.
- Também existem mudanças no código nativo de colisão/zumbi, incluindo `lib\collisions.cpp`, `lib\zombie.cpp` e arquivos relacionados.
- **Estado:** houve várias iterações de diagnóstico e builds. Não há evidência neste registro de uma validação final que prove que zumbis não atravessam todos os tipos de portas, cercas e construções em todas as situações. Não tratar a correção como universalmente concluída sem teste controlado.

### Portas e ações de interação

- Foram criados testes/servidores com foco em resolução de ações de porta e procura de objeto, incluindo os pacotes V121, V123 e servidores `DayZServer-DoorUserTypeExecV124` a `V127`.
- A investigação anterior encontrou a diferença da revisão do WRP entre cliente e servidor e alinhou os dados do mapa para um teste. A falha reapareceu quando builds posteriores voltaram a empacotar o WRP de cliente mais novo.
- **Estado atual:** os pacotes V29–V31 usam o mesmo `worlds_chernarusplus.pbo` e assinatura do servidor. O WRP está alinhado, mas a porta ainda precisa ser validada novamente dentro do jogo.

### Crash na entrada: V30

- O RPT V29 evidencia chamadas de textura com nível de mip inválido e termina em divisão por zero (`C0000094`). O código tinha cálculos de pitch `dataSize / mip._h` sem validar previamente se altura/dados do mip eram válidos.
- Em `lib\d3d9\txtd3d9.cpp`, foram adicionadas validações em `TextureD3D9::GetPixel`, `TextureD3D9::GetRect` e `TextureD3D9::RequestGetRect`: fonte, nível, largura/altura e tamanho de dados precisam ser válidos antes de ler ou calcular o pitch. Para metadados incompletos, as rotinas retornam sem dividir por zero.
- Foi criado o ponto de restauração `build\inventory-menu-v10\restore-point-v30-before-texture-guard-20261116\`, contendo fonte, executável, MAP e PDB anteriores à alteração.
- A build Release Win32 do cliente compilou em `build\inventory-menu-v10\cpp-out-v30\DayZInt.exe`; os avisos existentes de compilador permaneceram, sem erros de compilação/link.
- Pacote isolado: `build\inventory-menu-v10\package-v30-texture-guard-wrp\`. O executável/MAP foram atualizados; `ui.pbo` permaneceu idêntico ao V29; `worlds_chernarusplus.pbo` e a assinatura vieram do pacote já alinhado.
- SHA-256 conferidos: EXE V30 `8835CBE75665BDD65F4F4D347FF94876C8E92882440D0F66A6F99BEBDEBC8A0A`; `ui.pbo` `78669781987AF3875796B47C4B2BB30BDA3ACD66496F1E6C47C9B7175B24B1F3`; WRP `0B8DAD26F376B892376C45240D630C14C126A84D0C31D0492384EF9371C78E28`, igual ao servidor.
- Iniciador: `Start-DayZ-Direct-UI-PBO-V30-Texture-Guard-WRP.bat`. Perfil próprio: `profile-menu-browser-wrp-v30`.
- **Estado:** build compilada e pacote verificado por hashes. A correção ainda precisa de confirmação dentro do jogo; se ocorrer outra queda, guardar o RPT/MDMP criado no perfil V30. O servidor ativo não foi reiniciado nem alterado por esta etapa.

### Proteção para índice de mip inválido: V31

- A inspeção do mesmo trecho do RPT mostrou que o assert repetido `level >= 0` fica em `TextBankD3D9::UseMipmap`, antes das rotinas de leitura cobertas pela V30. Por isso, a V30 não cobria esse caminho de índice negativo.
- Adicionei a validação de `_nMipmaps` e limitei `level`/`levelA` ao intervalo válido (incluindo o sentinela `_nMipmaps`) antes do assert e do acesso aos níveis.
- Restore point: `build\inventory-menu-v10\restore-point-v31-before-invalid-mip-level-20261117\` contém a fonte anterior a esse ajuste, EXE/MAP V30 e PDB de referência.
- V31 foi compilada incrementalmente sem erros e empacotada separada em `build\inventory-menu-v10\package-v31-mipmap-level-guard-wrp\`, com perfil `profile-menu-browser-wrp-v31`.
- O `ui.pbo` e o `worlds_chernarusplus.pbo` permanecem os mesmos do V30; o WRP continua idêntico ao PBO do servidor. O executável e o MAP são novos.
- **Estado:** a V31 foi entregue para teste, mas a captura posterior mostrou regressões visuais: terreno vermelho/rosa, fundo 3D ausente e as seis abas legacy. Não considerar a V31 corrigida.

### Inclinação/strafe com Q e E

- O pedido foi permitir correr inclinado para a esquerda/direita usando Q/E, aproveitando a inclinação que já existia.
- **Estado:** não há build final validada nem evidência de que a corrida inclinada tenha sido concluída. Continua pendente.

### Mãos e inventário

- Foram feitas iterações de correção para sincronizar o item no slot `Hands` quando ele é movido para calça, camisa, mochila ou `Vicinity`, além do cancelamento repetido de ação ao fechar o inventário.
- Também se investigou a pose de arma/lanterna que permanecia depois de guardar o item. Os pacotes de teste chegaram às versões V10–V13.
- Em seguida começou o redesenho visual do inventário inspirado na referência mais atual: ícones de equipamento, contêineres, grade de slots, `Hands`, personagem 3D e itens equipados.
- **Estado:** as correções de mãos foram iteradas, mas as capturas posteriores ainda mostravam o inventário antigo e o novo coexistindo, e o novo painel sem personagem/itens funcionais. O redesign completo como substituto funcional do inventário nativo continua pendente.

### Menu inicial, pausa e navegador de servidores

- Foram feitas builds V10–V29 com alterações em PBOs de interface, estatísticas, seleção de sobrevivente, botões e navegador; o requisito acordado é editar diretamente os arquivos/PBOs do jogo, sem depender do mod `@MenuCleanup`.
- A V27 foi montada com `ui.pbo` direto e `bin.pbo` V25 preservado. O log da V27 contém “Error loading control” para vários controles de `RscDisplayMultiplayer`. A inspeção do config também encontrou as abas sem `type=106` e substituições de `B_Cancel`/`B_Join` com tipo incompatível.
- Na V29, corrigi o tipo nativo das abas e preservei os controles ativos de voltar/jogar. Corrigi também a estrutura do config que mantinha blocos `controlsBackgroundx`/`controlsx` inativos, removi o callback do Early Access e restaurei um único controle da logo DayZ. O config passou no `CfgConvert -test` e o PBO foi empacotado/aberto pelo BankRev.
- **Ainda falta:** confirmar visualmente no jogo as três abas `OFFICIAL`, `COMMUNITIES`, `LAN`, a ausência da sobreposição legacy, o fundo 3D e o comportamento de conectar. A V29 mantém os mesmos dados WRP alinhados da V28.
- As capturas mais recentes mostram que algumas builds exibiram o menu novo, mas mantiveram textos/elementos antigos (Early Access, logo/copyright 2013), perderam os rótulos das abas ou ficaram com fundo azul sem cenário 3D.
- O menu de pausa e a tela de seleção de servidores também tiveram protótipos/alterações, porém houve relatos de crash ao abrir Change Server e de controles/abas legacy sobrepostos.
- **Estado:** a V29 traz correções estruturais do menu e navegador; ainda não foi aceita como final até a validação visual e de conexão dentro do jogo.

## Evidências e artefatos úteis

- Log do crash relatado na V27: `build\inventory-menu-v10\package-v27\profile-server-browser-direct-v27\DayZInt.RPT`.
- O log mostra a transição do cliente até `GAME LOADED`, asserts `lib\d3d9\txtd3d9.cpp(4186)` e erros de objetos abstratos de píer antes do encerramento.
- Pacote de teste mais recente: `build\inventory-menu-v10\package-v32-terrain-menu-repair\`.
- O pacote V27 permanece intacto. Os pontos de restauração estão em `build\inventory-menu-v10\restore-point-v27-before-wrp-align-20261116\` e `build\inventory-menu-v10\restore-point-v28-before-v29-menu-20261116\`.

## Procedimento manual para alinhar o WRP em outra build

1. Feche o cliente e faça uma cópia de segurança de `Addons\worlds_chernarusplus.pbo` e do `.bisign` correspondente.
2. Confira a revisão do PBO do servidor com `BankRev -properties`.
3. Copie o `worlds_chernarusplus.pbo` do servidor para o `Addons` do cliente de teste. Se a verificação de assinatura estiver ativa, a assinatura precisa corresponder ao PBO; não reutilize um `.bisign` antigo para um PBO diferente.
4. Confira que revisão e SHA-256 do PBO do cliente de teste coincidem com o servidor.
5. Não substitua automaticamente `worlds_chernarusplus_data.pbo`: ele é outro pacote e não contém o arquivo WRP. Altere-o somente se uma comparação específica confirmar que também precisa ser alinhado.


### Reparo do terreno e menus: V32 (17/11/2026)

- A captura da V31 mostrou o terreno vermelho/rosa e o menu principal sem cenário 3D; o navegador ainda mostrava INTERNET/FAVORITES/HISTORY/FRIENDS/LAN/REMOTE.
- A comparação `BankRev -d` entre o `worlds_chernarusplus_data.pbo` da V31 e o arquivo atual em `Addons` achou **509 arquivos com conteúdo diferente**, incluindo texturas `layers\*_lco.paa`, apesar de ambos indicarem revision 17380. A V31 estava combinando o WRP do servidor (17865) com um pacote de texturas diferente do Addons atual. A V32 usa o arquivo atual do Addons e sua assinatura correspondente; o WRP do servidor permanece intacto no pacote de teste.
- A V32 substitui `dta\bin.pbo` pelo pacote compilado a partir da configuração que define as abas nativas `OFFICIAL`, `COMMUNITIES` e `LAN`; `HISTORY`, `FRIENDS` e `REMOTE` ficam fora da tela. Os controles das três abas preservam seus IDs de ação nativos. `CfgConvert -test` aprovou o `config.bin`, e o PBO foi reextraído para confirmar os rótulos e classes.
- O `RscDisplayMain` volta a executar o `onLoad` nativo que havia sido zerado. O aviso Early Access recebe fechamento imediato para não reaparecer. Essa mudança foi compilada no `bin.pbo`; o cenário 3D ainda requer confirmação no cliente.
- Restore point: `build\inventory-menu-v10\restore-point-v32-before-terrain-menu-repair-20261117\` (fontes, PBOs e dados anteriores).
- Pacote isolado: `build\inventory-menu-v10\package-v32-terrain-menu-repair\`; launcher `Start-DayZ-Terrain-Menu-V32.bat`; perfil `profile-terrain-menu-v32`.
- Hashes: EXE `10F94200780BA73BE6EE9CDDC33B5D0D488D15625DD79E6BCE62A9861261CD35`; `dta\bin.pbo` V32.1 `03A7CAD7B75296BFE5DAC1E0ECCC91333A82F43493FCB4DEFEB19A8CF076CC9D`; dados do mundo `7DB7A946164ABDABF2FF658673BC8D456DD8BAB979F5420CEFC293A34266087C`; WRP `0B8DAD26F376B892376C45240D630C14C126A84D0C31D0492384EF9371C78E28`.
- **Validação pendente:** não iniciei o cliente nesta rodada nem toquei no servidor ativo. A primeira execução feita por você foi registrada no RPT: `Unable to open dta\bin.pbo`, seguido pelos avisos `.CfgAddons`/`.PreloadAddons`. O prefixo duplo foi identificado no metadado e corrigido em V32.1; a execução após a correção ainda precisa confirmar que o PBO abre e, depois, validar chão, cenário 3D, três abas e conexão.
### Correção V32.1 — prefixo do bin.pbo (17/11/2026)

- O usuário encontrou `No entry '.CfgAddons'` ao iniciar o primeiro pacote V32.
- A primeira execução registrou `Unable to open dta\bin.pbo` e depois `No entry '.CfgAddons'`/`'.PreloadAddons'`. O metadado continha `prefix=bin\\` (duas barras literais), enquanto o PBO V31 funcional usa `prefix=bin\` (uma barra). Corrigi o prefixo como causa identificada para a falha de abertura; a confirmação depende de novo RPT do V32.1.
- Corrigi o metadado, reempacotei `dta\bin.pbo` e confirmei com BankRev que agora lê `prefix = bin\`. O `config.bin` extraído do PBO tem SHA-256 idêntico ao compilado: `C2D44BC371C4AB5CED9EFF76380D453C2EED8C619E1129008BF6B08986CD1CD1`.
- O PBO corrigido tem SHA-256 `03A7CAD7B75296BFE5DAC1E0ECCC91333A82F43493FCB4DEFEB19A8CF076CC9D`.
- Ponto de restauração do pacote antes da correção: `build\inventory-menu-v10\restore-point-v32-cfgaddons-prefix-20261117\`.
- O launcher V32 permanece igual; a próxima validação necessária é iniciar o cliente e confirmar que o erro não aparece. Terreno, cenário 3D, abas e conexão ainda dependem do teste dentro do jogo.



### Execução V32.1 e personagem sem resposta (17/11/2026)

- O usuário executou V32.1 e reportou que o personagem ficou imóvel, numa pose inclinada, sem responder.
- A captura mostra o Gerenciador de Tarefas à frente do cliente; o processo `DayZInt.exe` estava `Responding=True` e em uso de CPU. Isso não prova que a janela do jogo tinha foco.
- O RPT da sessão V32.1 (iniciada às 01:14:46) confirma que o cliente alcançou `GAME LOADED` e `PLAYING`, sem repetir `Unable to open dta\bin.pbo`. Depois registrou `PLAYING -> MISSION ASKED`.
- O servidor `DayZServer-DoorUserTypeExecV127` salvou o personagem como vivo e registrou a desconexão de `Arthurnashh` após 119,672 s. O log não registra uma falha de animação do cliente; o sintoma pode ser perda de foco da janela ou interrupção da sessão. Não alterar movimento/animação sem isolar a causa.
- A captura mostra o terreno verde, sem o aspecto rosa/vermelho anterior. A cena 3D inicial e as abas continuam sem confirmação visual. O RPT ainda registra `Error loading control` para controles de `RscDisplayMain` e `RscDisplayMultiplayer`; esses erros precisam ser tratados/validados para fechar o objetivo do menu.
