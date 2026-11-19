# DayZ 0.45 — resumo do trabalho até agora

**Data do registro:** 13/11/2026  
**Raiz da source:** `D:\SOURCEDAYZS!!\DayZ-0.45.124252`


## Estado mais recente: queda de FPS perto de cidades

- O problema relatado continua: ao chegar perto de construções, árvores e áreas urbanas — incluindo Berezino — o usuário observou uma queda muito forte. No teste mais recente, após andar aproximadamente 50–100 metros, relatou oscilação entre 0 e 1 FPS, tornando o jogo injogável.
- O usuário pediu para interromper por ora a investigação/otimização e registrar o que já foi feito e observado. Portanto, este relatório é um ponto de parada, não uma confirmação de correção.
- A hipótese compartilhada pelo desenvolvedor da versão é que o custo pode estar no processamento de objetos antes do desenho e no trabalho feito dentro das funções `Scene::DrawObject***`, incluindo preparação de materiais e polígonos. Isso é uma pista atribuída ao desenvolvedor; ainda não foi confirmado aqui com perfil de CPU ou medição por função.
- Também foi levantada a possibilidade de gargalo no ciclo de vida de solicitações do carregador assíncrono. A própria análise observou que cancelamento/conclusão mexem em filas e objetos compartilhados; aumentar workers sem proteger esses estados pode introduzir corrupção ou crashes. Não há evidência que isso, isoladamente, seja a causa da queda de FPS.
- A limitação a 32 bits pode restringir o espaço de endereçamento e aumentar o risco de pressão de memória. DirectX 9, por si só, não explica nem garante a queda: CPU, GPU, chamadas de desenho, materiais, visibilidade, carregamento e sincronização também podem limitar o frame rate. A máquina e a cena precisam ser consideradas; não há benchmark comparativo controlado registrado neste relatório.

### O que os testes permitem concluir

- As capturas e os relatos mostram que o cliente consegue chegar ao mundo e que a queda aparece perto de construções/vegetação; portanto, o problema não ficou limitado à tela de menu ou à simples inicialização do cliente.
- Houve uma tentativa anterior descrita como build/Release otimizado e uma sessão de teste em Berezino, mas o resultado relatado foi pior, não uma melhora. Não se deve promover esse candidato como correção.
- Não temos neste registro uma captura sincronizada de frame time, uso por thread, GPU, número de objetos desenhados ou duração de cada `Scene::DrawObject***`. Sem isso, a causa exata e o efeito de cada alteração continuam indeterminados.
- A frase do desenvolvedor sobre processamento pré-render e preparação de materiais/polígonos é uma pista plausível e prioritária para orientar a próxima investigação, não prova de que todas as quedas vêm exclusivamente daí.

### Alterações e risco de regressão a revisar

- O checkout tem mudanças recentes nos caminhos de renderização e carregamento, incluindo `lib/shapeDraw.cpp`, `lib/landClutter.cpp`, `lib/transLight.cpp`, `lib/pactext.cpp`, `lib/memNedAlloc.cpp` e o backend `lib/d3d11/`. As datas de modificação mostram trabalho recente, mas não identificam por si só qual mudança alterou o FPS.
- O relatório técnico registra uma sincronização adicionada em `shapeDraw.cpp` para proteger a rotação das listas de geometria contra operações concorrentes. Ela evitou a repetição de um crash de lista em um teste de mais de 60 segundos, mas não demonstra melhoria de desempenho em cidade.
- Também existem diagnósticos/guardas adicionados durante a investigação de texturas PAC/PAA, clutter e memória. Instrumentação e validações podem alterar o custo ou o comportamento; precisam ser distinguidas de uma otimização deliberada.
- Antes de retomar otimizações, registrar o executável exato, configuração, renderer, argumentos, pacote de dados e alterações em relação ao build testado. Preservar um build de referência para comparação.

## Pista técnica principal a investigar quando retomarmos

O relato do desenvolvedor aponta para preparação de objetos e materiais no caminho anterior ao envio à GPU. Uma continuação segura pode examinar:

1. O caminho de chamadas que chega a `Scene::DrawObject***`, incluindo trabalho repetido por objeto/frame.
2. Preparação e seleção de materiais, polígonos, estados e buffers; procurar recomputação que possa ser reutilizada ou cacheada sem mudar a semântica visual.
3. O custo da visibilidade/culling e a quantidade de objetos/LOD submetidos ao redor de cidades.
4. Esperas, locks e concorrência entre o carregamento assíncrono e o thread principal de renderização.
5. A separação entre tempo de CPU do thread principal, outros threads e GPU. Só depois comparar uma mudança por vez em uma rota reproduzível.

Esses itens são um plano de investigação; não são resultados já medidos.

## Renderer DirectX 9

O cliente usa Direct3D 9 e carrega os shaders do diretório de instalação.
A solução de compilação é [Game.sln](Game.sln), com VS2026 e configurações x64 para cliente e servidor.

## Conteúdo do jogo, mapa e navegação

- O trabalho anterior localizou e extraiu o mundo `ChernarusPlus.wrp` para `conteudo-044/extraidos/Addons/worlds_chernarusplus/ChernarusPlus.wrp`, conforme caminho fornecido na conversa.
- A necessidade de recuperar os demais `.wrp`, fontes/arquivos dos addons e dados de navegação do mapa foi discutida. O objetivo inclui malhas/componentes locais, vértices, conexões e a relação entre terreno e objetos.
- A conversa não fornece evidência suficiente para afirmar que todos os WRP, navmeshes, geometrias de todos os objetos ou conexões entre superfícies foram extraídos, reconstruídos e validados. O pedido de validação 100% do grafo não deve ser tratado como concluído sem relatório de validação e artefatos verificáveis.
- Também foi estabelecido que geometria editável de trabalho não deve ser empacotada indiscriminadamente dentro dos PBOS de runtime; o pacote deve conter os recursos que o jogo realmente carrega.

## Cliente, servidor e pacote local

- Foram montados/testados artefatos de cliente e servidor em diretórios de `dist`/`build`; a conversa registra que o cliente chegou ao menu e conectou ao servidor local em uma sessão posterior.
- O log mostrado na captura chegou a registrar `Mission dayz_Auto.ChernarusPlus read from directory`, `Reading mission ...`, `Mission read.`, conexão com Steam e jogador conectado. Logo, naquela execução a missão foi lida e o jogador chegou a conectar; isso não garante que todos os arquivos do servidor ou persistência estejam corretos em outras execuções.
- Foi criada/ajustada uma inicialização local que verifica a porta UDP 27017 e interrompe se ela já estiver em uso, para evitar duas stacks locais concorrentes. A mensagem da captura indicava exatamente porta em uso.
- Foi discutida uma base de dados totalmente local dentro da pasta do servidor. A imagem fornecida mostra a estrutura esperada de um servidor antigo, incluindo `Addons`, `BattlEye`, `config`, `dayz_Auto.ChernarusPlus`, `dll`, `dta`, `Keys`, `MPMissions`, `DayZServer`, `proxyexport.bin` e `spawnpoints_players.bin`.
- O usuário apontou corretamente que uma pasta de servidor utilizável também precisa da missão, parâmetros corretos de inicialização, arquivos de spawn e persistência de personagem/mundo/loot. Este documento não atesta que o banco local implemente integralmente persistência compatível com o protocolo/formato do jogo.
- A busca por `BEServer.dll` foi solicitada, mas este resumo não confirma sua localização ou compatibilidade. A instalação do BattlEye deve ser verificada no pacote concreto antes de iniciar o servidor.

## Arquivos de referência existentes

- [`PRIVATE_SERVER_AUTH_DESIGN.md`](PRIVATE_SERVER_AUTH_DESIGN.md) e [`PRIVATE_SERVER_AUTH_STATUS.md`](PRIVATE_SERVER_AUTH_STATUS.md) — desenho e estado de autenticação do servidor privado.
- `build/port-x64/` — builds, logs e perfis de teste. Os diretórios de perfil/RPT e dumps devem ser consultados pela data e pelo executável de cada sessão; não misturar evidências de sessões diferentes.

## Próxima retomada

1. Primeiro identificar sem ambiguidade qual executável/configuração/renderer foi testado quando o FPS caiu para 0–1 em Berezino.
2. Comparar as alterações do checkout com o último build jogável, separando instrumentação, correções de estabilidade e mudanças que afetam o caminho de renderização.
3. Retomar a pista de processamento de objetos e preparação de materiais com medições por thread/função ou instrumentação leve e controlada; evitar mudanças simultâneas em vários subsistemas.
4. Fazer um teste comparável na mesma rota, sem alterar missão/servidor/assets entre as medições.

