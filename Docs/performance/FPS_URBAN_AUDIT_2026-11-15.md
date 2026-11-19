# Auditoria de queda de FPS em áreas urbanas

**Árvore analisada:** `DayZ-0.45.124252` (`SOURCE DEFINITIVA`)  
**Data:** 2026-11-15  
**Escopo desta rodada:** custo de preparação de objetos, instancing D3D9, LOD/oclusão, luzes, sombras e evidência dos assets.

## Resumo executivo

O código confirma dois caminhos de CPU que podem crescer com a quantidade de objetos visíveis numa cidade:

1. **Gestão de Far LOD:** em builds sem cheats, `FarLodsDefault` fica em `2`. `Scene::DrawPreparation` passa por candidatos estáticos, recalcula `ClippingInfo` e chama `SceneCoverage::RenderBBox` para cada um, usando uma grade de cobertura de 64×32. A operação ocorre antes do desenho e seu custo cresce com a quantidade de candidatos. É uma suspeita forte para medir, ainda não prova que seja a causa dominante.
2. **Fallback de instancing D3D9:** o backend só usa a rota instanciada quando o vertex buffer acomoda mais de uma cópia do modelo. Caso contrário, cai em `Engine::DrawMeshTLInstanced`, que percorre instância por instância e repete preparação de mesh/material/seções. Modelos urbanos grandes podem atingir esse limite, mas falta saber quantos modelos usados em Berezino o atingem em runtime.

Outros custos coerentes com a queda perto de construções incluem repetição de passes de sombra/profundidade, objetos que não podem ser agrupados, custo por seção/material, streaming de texturas e sort/merge da cena. A fonte contém marcadores de profiler para separar esses custos. Não há evidência suficiente nesta análise estática para escolher um único culpado ou afirmar ganho de FPS.

## Evidência direta no código

### A. Far LOD e cobertura CPU antes do desenho

- `lib/scene.cpp:9322-9326`: `FarLodsDefault` é `2`; a variável é ajustável por tecla somente em builds com `_ENABLE_CHEATS`, enquanto os demais builds usam o valor constante.
- `lib/scene.cpp:9340-9348`: `Scene::DrawPreparation` começa o escopo `oPrep` e, entre outras tarefas, prepara memória de textura e executa coleta de memória.
- `lib/scene.cpp:9374-9378`: cria `farLODObjects` e dimensiona a lista com base no total de objetos quando Far LOD está ativo.
- `lib/scene.cpp:9447-9474`: para `FarLods >= 2`, cria `SceneCoverage(64,32)`, itera os candidatos, filtra objetos móveis, chama `ClippingInfo` e rasteriza a bounding box com `RenderBBox`.
- `lib/scene.cpp:9475-9513`: analisa camadas de cobertura e calcula complexidade/LOD por forma.
- Marcadores existentes: `oPrep`, `oFLod`, `occBB`, `occBT`.

**Interpretação:** o próprio código deixa uma carga de CPU de preparação proporcional a candidatos estáticos visíveis, potencialmente elevada em cidade. `64×32` é pequeno, mas o custo total multiplica por objetos, vértices da bbox e cálculos de LOD. Só o profiler pode dizer se domina o frame.

### B. Instancing real versus fallback por objeto

- `lib/d3d9/engdd9.cpp:7257-7277`: o buffer limita cópias a 32.000 vértices para geometria frequente e 8.000 para não frequente. Se o modelo passar de metade desse limite, `nInstances` fica em `1`.
- `lib/d3d9/engdd9.cpp:8731-8807`: `DrawMeshTLInstanced` calcula quantas cópias cabem. Quando cabem mais de uma, usa `DrawMeshTLSectionsInstanced`; caso contrário, despacha para a implementação genérica.
- `lib/engineDraw.cpp:1370-1458`: a implementação genérica percorre cada instância, chama `BeginInstanceTL`, inicia o mesh e prepara/desenha as seções para cada objeto.
- `lib/drawpoly.cpp:335-359`: `PrepareTL` carrega mipmaps, combina material e prepara o triângulo.
- `lib/d3d9/engdd9.cpp:7942-8000` e `9154-9235`: setup de luzes ocorre na preparação da instância.
- `lib/object.cpp:527-559`: `CanBeInstanced` verifica geometria/VBuffer, animação e algumas condições, mas não a capacidade efetiva de cópias que o D3D9 buffer aceitará.
- Marcadores existentes: `mrgIn`, `mjPrp`, `o1Drw`, `obDrw`, `inst`, `prpTx`.

**Interpretação:** existe uma diferença estrutural de custo entre instancing acelerado e fallback. É o caminho mais alinhado com o relato do desenvolvedor sobre `Scene::DrawObject***` e preparação de materiais. Falta medir frequência por modelo e custo agregado.

### C. Agrupamento pode se desfazer antes do backend

- `lib/scene.cpp:4341-4370`: grupos podem ser enviados como grupo quando instancing está habilitado, mas o modo `GroupUnlessClipped` cai para objetos individuais quando há flags de clipping; proxies também podem forçar esse caminho.
- `lib/scene.cpp:7287-7288`: a etapa de merge exige `enableInstancing` e `CanBeInstanced` nos LODs de origem/destino.
- `lib/global.hpp:275-277`: sem `_ENABLE_CHEATS`, instancing fica compilado como `true`; isso não significa que todos os objetos consigam usar o caminho acelerado.

**Interpretação:** o número de objetos renderizados não é igual ao número de chamadas draw. Clipping, animação, proxies, material/LOD incompatível e tipo de buffer podem quebrar lotes. A cidade pode piorar por composição de muitos casos pequenos.

### D. Percurso da grade do mundo e custo por objeto perto da câmera

- `lib/world.cpp:8165-8208`: `World::PrepareDraw` percorre as células da grade dentro do alcance, faz rejeição por distância e carrega a lista de objetos da célula.
- `lib/world.cpp:8210-8237`: calcula/testa esfera da lista contra a câmera e decide clipping.
- `lib/world.cpp:8293-8337`: percorre grupos. A saída agregada para objetos não oclusores (caso comum em árvores/arbustos) só é considerada quando `distXZ2 > 200²` e não há sombras possíveis. Fora desse caso, continua objeto por objeto.
- `lib/world.cpp:7542-7546`: `HORIZON_PRUNE` vira `1` quando `GEOM_OCCLUDERS` não está definido. `lib/wpch.hpp:41` deixa `GEOM_OCCLUDERS` comentado e as definições do `lib2010.vcxproj` não o habilitam; portanto, a rota normal desta árvore compila o pruning por horizonte.
- `lib/world.cpp:8062-8091`: `disablePruning` começa em `0` (ou constante `0` fora de cheats), então as condições de pruning ficam ativas no modo normal.
- `lib/world.cpp:8343-8419`: para cada membro de grupo o código pode executar pruning, `ClippingInfo`, checks de oclusão/horizon e `Scene::ObjectForDrawing`.
- `lib/world.cpp:8428-8500`: objetos fora dos grupos também são processados individualmente, com clipping/pruning e registro de desenho.
- `lib/world.cpp:7939-7947`: o caminho inteiro está sob o escopo `wPrep`, o qual já pode ser observado no profiler.

### E. Cálculo de horizonte repete transformações de vértices

- `lib/world.cpp:8397-8398`: para formas que podem ocluir, o percurso adiciona o horizonte da forma com `prune.AddObjectHorizon`.
- `lib/world.cpp:7299-7337`: a função percorre cada aresta de oclusão e transforma os dois endpoints para view space; se a aresta compartilha vértices com outras, o mesmo vértice é transformado repetidas vezes.
- `lib/world.cpp:7302-7303`: o próprio código registra: “TODO: if the object is too small…” e “TODO: optimization: transform each vertex to view/screen space only once”.

**Interpretação:** esta é a pista estática mais concreta encontrada para custo urbano antes do desenho: o pruning por horizonte examina as arestas das formas oclusoras. Cidades têm muitas casas/estruturas potencialmente oclusoras; edifícios com muitos `OcclusionEdge` podem fazer o loop crescer. Ainda falta medir quantidade de arestas e custo total de `AddObjectHorizon` na rota de Berezino. Um próximo passo seguro é instrumentar tempo/chamadas/arestas e, se dominante, pré-transformar vértices únicos por objeto preservando exatamente o clipping e a ordem das arestas.

**Interpretação do percurso:** esta é uma hipótese estrutural especialmente alinhada à piora “perto de construções e árvores”: o motor pode reduzir trabalho por grupo à distância, mas a região próxima entra num caminho mais granular. Ainda é necessário verificar quantos objetos por célula e quanto tempo cada check custa na rota real. O limite de 200 m é uma condição do atalho de grupos não oclusores, não prova de uma mudança abrupta no FPS exatamente aos 200 m.

### F. Carregamento síncrono de listas de objetos em células próximas

- `lib/world.cpp:8204-8207`: a distância do centro da célula escolhe `UONoWait` apenas depois de 200 m; perto, usa `UOWait`.
- `lib/landscape.hpp:2236`: `IsStreaming()` é verdadeiro quando existe `_objIndex` de streaming.
- `lib/landSave.cpp:2703-2735`: `UseObjects` reutiliza uma lista residente ou chama `LoadObjects` quando ela ainda não está carregada.
- `lib/landSave.cpp:2356-2389`: quando `noWait` é falso, não retorna cedo por falta de preload; entra na leitura da lista e inicia desserialização (`SerializeBinStream`).
- `lib/landSave.cpp:2415-2517`: lê as entradas da célula e executa `ObjectCreate` por objeto, além de ajustes de posição/terreno.
- A carga tem o escopo existente `lndLO` e contabiliza `TCLoadObjects`; o chamador está dentro de `World::PrepareDraw` (`wPrep`).

**Interpretação:** se o mapa está em modo streaming e a célula ainda não foi carregada, a aproximação pode bloquear o frame para ler/criar conteúdo. Isso é plausível para picos/congeladas na primeira entrada numa cidade, mas não explica por si só FPS continuamente baixo depois que as células estão residentes. A confirmação é comparar `lndLO`/`TCLoadObjects` no primeiro percurso e numa segunda passagem pela mesma rota.

### G. Conteúdo real presente

Inventário somente de leitura com o `BankRev` da source e os PBOs em `D:\SOURCEDAYZS!!\DayZ\Addons`:

| PBO | Entradas | Modelos `.p3d` |
|---|---:|---:|
| `assets_buildings.pbo` | 543 | 98 |
| `assets_buildings2.pbo` | 1.434 | 139 |
| `structures_houses.pbo` | 977 | 83 |
| `worlds_chernarusplus.pbo` | 2 | 0 (contém `chernarusplus.wrp`) |
| `worlds_chernarusplus_data.pbo` | 8.002 | 2.073 imagens reconhecidas pelo filtro |

As contagens de modelos são por arquivo listado, não frequência de instâncias no mapa. A WRP identifica o mundo, mas esta rodada não decodificou seu conteúdo nem contou instâncias por cidade.

## Hipóteses priorizadas e como distingui-las

| Prioridade | Hipótese verificável | Evidência atual | Sinal que a confirma | Sinal que a enfraquece |
|---|---|---|---|---|
| Alta | Rasterização de cobertura Far LOD consome CPU por objeto | Caminho `oFLod` percorre estáticos e chama `ClippingInfo`/`RenderBBox` | `oFLod`/`occBB` domina ou cresce muito em Berezino | Tempo baixo mesmo com muitos candidatos |
| Alta | Pruning por horizonte transforma endpoints de aresta repetidamente | `HORIZON_PRUNE` ativo por padrão; `AddObjectHorizon` tem TODO explícito de transformar vértice uma vez | custo/chamadas/arestas em `wPrep` correlacionam com casas oclusoras | tempo baixo nessa função ou poucos edges nos modelos usados |
| Alta | Preparação por membro de grupo domina perto do jogador | `World::PrepareDraw` tem atalho de grupo só para não oclusores distantes sem sombra; perto percorre cada membro | crescimento de tempo/contagem em `PrepareDraw` e `ObjectForDrawing` por célula | custo por objeto baixo e backend/ GPU domina |
| Alta para engasgos na primeira passagem | Lista de objetos da célula é carregada sincronamente perto | `UOWait` até 200 m; `LoadObjects` desserializa e chama `ObjectCreate` | `lndLO`/`TCLoadObjects` alto só na primeira aproximação | mesma queda depois da rota aquecida e `lndLO` baixo |
| Alta | Modelos grandes caem no fallback por cópia | Buffer pode ficar com capacidade 1; fallback percorre objetos | muitos lotes com capacidade 1 e tempo alto em `inst`/`obDrw` | quase todos os modelos cabem em lotes e `inst` é barato |
| Alta | O agrupamento de render se desfaz por clipping/proxy/LOD | `GroupUnlessClipped`, `CanBeInstanced`, proxies | sobe o número de `ObjDraw`/draws individuais perto das casas | proporção de lotes permanece estável entre campo e cidade |
| Média-alta | Sombras/profundidade repetem preparação de geometria | há vários passes e caminhos dedicados no `scene.cpp` | custo aparece em passes de sombra e cai ao desativar sombras num A/B controlado | passe principal domina sem custo secundário |
| Média | Número de seções/material por casa multiplica preparação | cada seção chama `PrepareTL`; textura/material são considerados | tempo se correlaciona com seções/materials, não apenas instâncias | formas urbanas têm poucas seções e preparação não pesa |
| Média | Streaming de texturas/pressão de memória produz engasgos | `AllocateTextureMemory`, `PrepareTextures`, mipmap hint e GC ocorrem durante o frame | picos coincidem com leitura/decodificação/evicção ou faltas de mip | queda sustentada sem I/O/streaming e após assets residentes |
| Média | Sorting/merge vira gargalo pela quantidade de objetos | escopos `mrgIn`, `mjPrp`, `oSo*`, `o1Drw`; listas e sort por frame | esses escopos crescem com objetos mais que o draw backend | custo baixo nesses escopos |
| Média | Luzes locais quebram lotes ou aumentam setup | grid seleciona listas de luz; `SetupLights` prepara estado por instância/célula | queda maior à noite/perto de luzes, grupo subdividido por lista | comportamento idêntico sem luzes locais |
| Média | Orçamento adaptativo de complexidade/LOD custa ou escolhe mal | `AdjustComplexity`, `ComputeTerrainComplexity`, cálculos Far LOD | custo alto no `oPrep`/complexity e muitos objetos no LOD caro | orçamento fora do frame crítico |
| Média-baixa | Alpha/transparência/folhagem aumenta sort e overdraw | `DrawPass2` ordena objetos alpha e limita sprites; parte do custo é GPU | profiler mostra `oPas2`, GPU-bound ou overdraw urbano | CPU-bound na preparação antes do draw |
| Média-baixa | Gargalo é GPU, não CPU | cenas urbanas têm mais polígonos/texturas e vários passes | GPU saturada, GPU frame time alto | GPU ociosa com uma thread CPU saturada |
| Confundidor | Build concorrente altera FPS durante ensaio | havia dois MSBuild e vários `cl.exe` ativos nesta inspeção | FPS melhora ao repetir sem compilação concorrente | queda igual em máquina ociosa |

## Lattice de cenários (sem transformar palpites em fatos)

Para evitar uma lista artificial de milhares de causas, a análise será organizada como combinações de fatores observáveis:

- submissão: grupo acelerado, grupo com fallback, objeto individual, animado/proxy;
- preparação da grade do mundo: lista vazia, grupo agregado, membros individuais, teste de pruning/oclusão/horizonte;
- capacidade D3D9: zero/erro, uma cópia, lote pequeno, lote grande;
- passe: cor, alpha, depth/priming, cascata de sombra, RTT/primeira pessoa;
- geometria: baixa/média/alta contagem de vértices e seções;
- material: textura residente ou faltante, alfa, shader/material simples ou complexo;
- iluminação: sem luz local, grid com luzes, muitas células/listas distintas;
- visibilidade: totalmente visível, clipping parcial, oclusão, transição de LOD;
- estado de execução: frame normal/repetido, células/streaming frio ou aquecido, build concorrente/ocioso.

Uma grade de 4×4×5×3×3×3×3×2 já gera **12.960 combinações de cenário**. Elas não são 12.960 causas distintas nem foram simuladas. Servem para desenhar testes que isolam fatores em vez de fazer mudanças aleatórias. A fonte estática consegue provar os caminhos acima; confirmar as combinações exige contadores/profiler em execução.

## Plano de validação e correção

1. Aguardar builds ativos terminarem e garantir que o cliente de teste está ocioso de MSBuild/`cl.exe`; build concorrente invalida comparação.
2. Capturar, na mesma rota em campo e em Berezino, tempo de CPU/GPU por frame e scopes já existentes: `wPrep`, `lndLO`, `oPrep`, `oFLod`, `occBB`, `mrgIn`, `mjPrp`, `o1Drw`, `obDrw`, `inst`, `prpTx`, `oPas2`, `hObj` e passes de sombra. Repetir a rota para separar carregamento frio de custo sustentado.
3. Se esses scopes não isolarem o gargalo, adicionar contadores temporários com novo restore point: candidatos Far LOD; instâncias totais; batches; modelos com capacidade de buffer 1; fallback count; seções preparadas; draws por passe; objetos individuais após clipping/proxy.
4. Fazer A/B com uma variável por vez. Para Far LOD, comparar valor atual `2` com `0` em build de teste e observar custo de CPU **e** carga GPU/qualidade de LOD. Para instancing, agrupar por faixa de vértices e caminho escolhido; não aumentar buffers globalmente sem verificar VRAM e limites D3D9.
5. Repetir exatamente a rota com o executável `Retail` candidato e validar o FPS percebido pelo usuário. Sem essa passagem, a conclusão fica limitada à análise do código e à compilação; não se pode declarar ganho de FPS.

## Alavancas A/B já presentes na source

Há controles de diagnóstico existentes, condicionados a `_ENABLE_CHEATS` em `lib/world.cpp`:

- `DIK_5` ou `CXTPruneMode` percorre `Full` → `Grid` → `None` (`world.cpp:8087-8095`). `Full` usa teste por objeto e a inserção das linhas de horizonte; `Grid` deixa de executar `CheckMinmaxBox` por objeto, mas mantém a inserção; `None` desliga também a inserção. Isso separa o custo do teste por objeto do custo de construir o horizonte, mas muda culling e pode aumentar o trabalho de desenho. Deve ser usado como ensaio curto e comparável, não como configuração permanente.
- `DIK_0` percorre os níveis de `FarLods` (`scene.cpp:9322-9326, 9360-9368`). O nível `0` ignora o bloco de cobertura Far LOD; `1` continua ativo com um caminho diferente; `2` é o padrão não-cheat e percorre os candidatos estáticos com `SceneCoverage(64,32)` (`scene.cpp:9447-9474`). Esse A/B isola a preparação Far LOD, com a ressalva de que pode mudar qualidade/complexidade e carga GPU.

Esses atalhos tornam possível testar primeiro sem mudar algoritmo. Confirmar se a build de teste aceita os atalhos exige verificar se `_ENABLE_CHEATS` está habilitado; a configuração `Retail` observada define `_SUPER_RELEASE=1`, então não se deve presumir que ela exponha esses controles.

## Candidato implementado; efeito ainda sem medição em jogo

`HorizonPrune::AddObjectHorizon` transforma e calcula o código de clipping dos dois endpoints para cada aresta. Como as arestas são ordenadas por área (`lib/Shape/shape.cpp:8968-8994`), implementei uma cache direta de 128 slots por índice do vértice e geração por chamada/forma. Um conflito de hash só causa transformação repetida. Quando os dois endpoints da mesma aresta colidem no mesmo slot, o primeiro resultado é copiado antes de consultar o segundo, mantendo a geometria correta. O método agora tem o scope agregado `hObj`. Também inicializei `_lastObject` a `NULL`, pois `SetModel` o consultava antes de qualquer inicialização no construtor.

Isso reduz transforms/clipping repetidos quando arestas reutilizam vértices, sem mudar a ordem nem os critérios de clipping. A cache também acrescenta lookup e ocupa cerca de alguns KB no objeto `HorizonPrune`; o ganho líquido depende das formas e da frequência de arestas repetidas. O FPS real ainda precisa ser comparado no mesmo trecho do mapa, com e sem a alteração.

## Evidências adicionais desta continuação

### G. Preparação de instâncias e transição de LOD por grupo

- `Scene::DrawObjectsInstanced` (`lib/scene.cpp:6099-6300`) percorre cada membro de cada lista para montar `ObjectInstanceInfo`; por membro calcula visibilidade, transform, índice de luz quando ativo e coeficiente de sombra (`GetLandShadowSmooth`). Com mapa de sombra preenchido, esse coeficiente consulta quatro amostras bilineares do shadow map e pode consultar também `Landscape::SurfaceY` (`scene.cpp:3172-3220`, `landscape.cpp:7780-7800`). É custo candidato dentro de `o1Drw`, ainda não sabemos quantos membros/moldes atingem esse ramo em Berezino.
- `SortObject::UpdateBlendFactor` soma ou subtrai `GetLodTransitionDelta`, que é o delta de tempo do frame (`scene.cpp:3332-3365`, `scene.hpp:817-823`). Antes da correção, o ramo de listas chamava isso dentro dos loops de membros; como a lista compartilha um único `SortObject`, uma lista com N membros podia atualizar o mesmo blend até N vezes no mesmo frame.
- Corrigi `DrawObjectsInstanced` para calcular o coeficiente uma vez e atualizar o blend uma vez por grupo, depois de percorrer seus membros (`scene.cpp:6144-6190`). Isso evita transição dependente do tamanho da lista e remove chamadas repetidas de `GetBlendingCoef`. O impacto em FPS pode ser pequeno; o efeito esperado mais direto é transição de LOD consistente. A compilação ainda precisa confirmar esta segunda alteração nesta rodada.

### H. Atalhos A/B de agrupamento e thread de render

Em builds com `_ENABLE_CHEATS`, existem controles para comparar sem editar dados do mapa:

- `DIK_1` percorre `Individual`, `Group`, `GroupUnlessClipped` e `GroupFirst` (`scene.cpp:4307-4350`). O modo normal é `GroupUnlessClipped`; clipping e necessidade de preparar proxies podem expandir uma lista em objetos individuais.
- `DIK_L`/`CXTInstancing` alterna `Glob.config.enableInstancing`; `DIK_P`/`CXTUseCB` alterna command buffers (`d3d9/engdd9.cpp:24807-24817`). A comparação deve manter a rota fixa e só alternar um controle, porque desligar instancing também aumenta draws e pode ficar mais lento.
- `diag_captureSlowFrame ["o1Drw",0.016]` ou `diag_captureSlowFrame ["wPrep",0.016]` arma a captura do primeiro frame que ultrapassar 16 ms no scope escolhido (`gameStateExt.cpp:16984-17008,29127-29129`). Ao ficar pronta, `World::Draw` abre o diálogo do profiler (`world.cpp:9687-9692`). Esse mecanismo permite capturar Berezino sem inventar um contador novo; confirmar comandos disponíveis depende de a build habilitar `_ENABLE_PERFLOG`.

### I. Hipótese eliminada: chamadas frequentes ao garbage collector

`MergeInstances` chama `FreeOnDemandGarbageCollect` ao descarregar cadeias, o que parecia um custo repetitivo (`scene.cpp:7265,7397,7475`). Porém `El/FreeOnDemand/memFreeReq.hpp:684-689` define essa função inline com corpo vazio nesta configuração da source. Portanto, essas chamadas não fazem coleta nem varredura e não explicam a queda de FPS no PC; não devem ser “otimizadas” como se fossem trabalho ativo.

## Proteção e estado desta rodada

- Restore point imediatamente anterior à edição: `restore-points/fps-horizon-vertex-cache-before-edit-20261115-135230`; SHA-256 original de `lib/world.cpp`: `D9D107F07400B26C93E102574F3510D433F5DF2E653630A24160014F94C961F6`.
- Restore point imediatamente anterior à alteração de LOD: `restore-points/fps-lod-group-before-edit-20261115-191628`; SHA-256 original de `lib/scene.cpp`: `C46E6C36049BC9373CD13C69C17245817B2F0981B0580EF967567EAE44F62749`.
- Código alterado em `lib/world.cpp` e `lib/scene.cpp`; a análise e os resultados ficaram neste documento.
- O cache de índice da light grid já existente na árvore tem restore point anterior em `restore-points/fps-lightgrid-20261115-120536`; seu manifesto registra hashes dos arquivos originais `lib/scene.cpp` e `lib/Shape/shape.hpp`.
- Build `Release Win32` da árvore `SOURCE DEFINITIVA` concluiu com exit code `0`, gerando `DayZInt.exe` e `binarize.exe` em `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-horizon-vertex-cache-20261115`. O `DayZInt.exe` tem 13.843.456 bytes e SHA-256 `C92D1C0479DD30AC0C8C32870FE3CAA21BB29E5F82DD264C844B8D7599EC034C`.
- Build `Retail Win32` também concluiu com exit code `0`, gerando o candidato `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-horizon-vertex-cache-retail-20261115\DayZ.exe` (11.500.544 bytes; SHA-256 `89CA8021E5C7084F290DE81700B8B619BD40AA14CBB23604C64D4CD95BB3EAEE`).
- `dist/DayZ.exe` permaneceu com timestamp `12:58:27`; o candidato não foi copiado para `dist` nem para a pasta montada do jogo.
- A primeira tentativa de build dentro do caminho `SOURCE DEFINITIVA` falhou somente no passo antigo de `copy/editbin`, que não cita paths com espaços; a repetição com `OutDir` sem espaços concluiu. Não copiei binários para `dist` nem para a pasta montada do jogo.
- O build prova que a alteração compila; não houve sessão de jogo A/B nesta rodada. Nenhuma melhoria de FPS foi afirmada ou medida.

## J. Desdobramento da preparação/desenho de objetos

A inspeção seguiu o caminho até `Shape::DrawSections` (`lib/object.cpp:1968-2160`, `lib/shapeDraw.cpp:74-154`): em cada objeto o caminho normal chama `Animate`, `SelectAggLights`, `PrepareTextures`, otimiza o LOD, prepara matrizes e chama `Shape::Draw`. O caminho instanciado (`Object::DrawSimpleInstanced`, `object.cpp:1733+`) prepara texturas uma vez por grupo/modelo e envia `DrawMeshTLInstanced` para o grupo. Assim, construções que não podem ser instanciadas podem repetir uma parte cara desse trabalho por objeto; quantas entram nesse caminho em Berezino depende dos assets e precisa ser medido.

`Shape::Draw` já tinha o scope `cuDrw` ao redor de `DrawSections`. Dentro do loop por seção, o motor consulta `PolyProperties`, calcula bias, cria `TLMaterial`, chama `CreateMaterial`, executa `PrepareTL` (incluindo offsets UV quando animados) e envia `DrawSectionTL`. Isso confirma que a dica do desenvolvedor aponta para uma cadeia existente e potencialmente repetida; não prova que ela explique sozinha a queda observada.

Para separar as fases sem alterar resultados de renderização, acrescentei scopes de profiling já suportados pelo motor:

- `obAni`: animação de objeto;
- `obLit`: seleção/agregação de luzes;
- `obTex`: preparação de texturas no desenho individual;
- `obShp`: desenho da forma, comparável ao `cuDrw` interno;
- `inTex` e `inDrw`: preparação de texturas e envio instanciado.

O objetivo é poder distinguir custo repetido de `Object::Draw` do custo por grupo instanciado, e localizar se o tempo está na preparação antes de `cuDrw` ou dentro das seções/material. Não apliquei cache de material ou luz: os resultados dependem de animação, UVs, luzes, especiais, clipping e parâmetros de desenho, então um cache especulativo pode renderizar incorretamente ou aumentar o trabalho. O candidato precisa de profiling em uma rota repetida antes de escolher uma otimização comportamental.

- Restore point anterior a estes scopes: `restore-points/fps-object-draw-scopes-before-edit-20261115-200500`; SHA-256 do `lib/object.cpp` original: `BB571E00213A5FF8240257DC5F65BEFB8CE803A8ABCB67C1B573106ECE1E5506`.
- Build Profile Win32 isolado para expor scopes detalhados concluiu e gerou `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-object-render-scopes-profile-sln-20261115\DayZProfile.exe` (16.051.200 bytes, SHA-256 `BF704538DB4EB408DAF5D8BA90028FA62F077E7DE919C3B6DB908A8A99C9AFA7`). O log não contém erros C/LNK/MSB e termina com `lib2010.vcxproj -> ...\DayZProfile.exe`; restam avisos LNK4099 de bibliotecas externas sem PDB. `dist\DayZ.exe` não foi alterado (11.499.520 bytes, timestamp 15/11/2026 12:58:27). Ainda não há teste in-game/A-B.

### Instancing limits verified in code

`Object::CanBeInstanced` (`lib/object.cpp:527-559`) rejects objects when craters need drawing, thermal vision needs a per-object heat source, animation is unsupported by the build, the shape is missing, or the LOD lacks a vertex buffer. `ObjectColored::CanBeInstanced` always returns false (`object.cpp:7002+`). These rules explain why some visible objects go through the expensive individual `Object::Draw` path even when neighboring static objects batch. They do not tell us how many Chernarus buildings fall into each category, so removing the restrictions globally would be unsafe; the new phase scopes are intended to reveal whether the individual path dominates first.

The scene submission path adds another constraint: `World::PrepareDraw` only invokes `Scene::ObjectsForDrawing` for a non-occluding group when `distXZ2 > 200^2` and the shape is not animated (`world.cpp:8380-8385`). Nearer objects, including everything inside this 200 m radius, continue through the per-object loop. Group submission also falls back to individuals when clipping is active (`scene.cpp:4320-4360`). This makes high per-object preparation volume near buildings a source-supported hypothesis, while moving that 200 m gate or bypassing clipping/occlusion would risk visibility errors. The `wPrep`, `oFLod`, `oSo*`, and `o1Drw`/`oPas*` scopes already present in the engine help separate visibility/LOD preparation from submission and draw passes; the added `ob*`/`prp*` scopes isolate the work per object and texture branch.

The built-in profiler is CPU-scope based; if the measured CPU stages are small while frame time remains high, this alone will not rule out a GPU/driver limit. That case needs a separate D3D9 GPU timing or frame capture before changing draw distance, resolution, shaders, or asset content.

### Possível custo repetido dentro de `prpTx`

A source contém um caminho rápido por `TexAreaIndex`: com `_texArea._valid`, `Shape::PrepareTextures` percorre texturas únicas (`shapeDraw.cpp:1576-1590`) em vez de revarrer seções e todos os estágios de materiais (`1592-1690`). No inventário da source, `BuildTexAreaIndex()` é chamado pelos loaders de landscape/estradas; o outro ponto de construção encontrado está comentado em `Shape::CheckPushbufferCanBeUsed` (`shape.cpp:4239`). Portanto, os modelos de objetos comuns podem chegar ao caminho de varredura por seção em cada chamada de `PrepareTextures`. Isso é uma hipótese forte para conferir com o scope já existente `prpTx` e com o novo `obTex`, mas não apliquei construção global do índice: materiais e seções podem ser animados/substituídos, e o índice retém ponteiros para referências de textura; seria preciso provar validade, concorrência e ciclo de vida antes de tornar o cache geral.


### Escopos de ramo de preparação de textura

Na sequência da análise do índice, acrescentei `prpIX` ao ramo `_texArea._valid` e `prpSc` ao caminho que percorre as seções e estágios (`lib/shapeDraw.cpp:1576-1694`). São scopes detalhados filhos de `prpTx`; não mudam seleção de mipmap nem conteúdo visual. Isso permite medir diretamente qual caminho responde pelo tempo agregado quando a ferramenta de captura do Profile grava um frame. O uso de `TexAreaIndex` ainda não foi ativado ou modificado.

- Restore point anterior à edição: `restore-points/fps-texture-prep-profile-scopes-before-edit-20261116-001500`; SHA-256 original de `lib/shapeDraw.cpp`: `B70D06B02496A22A51F33B3BA03156F878EA93E88720107E3D806A40D78CC801`.
- O candidato Profile anterior não contém estes dois scopes; será recompilado em uma nova pasta de saída, reaproveitando apenas os objetos intermediários do build Profile anterior.

### Build Profile atualizado com separação do caminho de textura

O rebuild isolado concluiu o projeto `lib2010.vcxproj` e compilou `shapeDraw.cpp`; o log termina com `lib2010.vcxproj -> ...\DayZProfile.exe` e não contém erros C++, linker ou `Build FAILED`. Os avisos restantes são de PDB ausente em bibliotecas externas (`LNK4099`) e avisos de compatibilidade do linker.

- Executável: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-texture-branch-profile-20261116\DayZProfile.exe` (16.053.248 bytes, SHA-256 `4832A3C467D28924D8727A6CB1E7979B237C61442A79A49AB7CE6B80213406E4`).
- Log: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-texture-branch-profile-20261116\build.stdout.log`.
- O alvo `Profile` define `_PROFILE`; `normalConfig.h` então configura `_ENABLE_PERFLOG=2`, habilitando os counters detalhados usados pelos scopes `prpIX` e `prpSc`.
- `dist\DayZ.exe` permaneceu intacto (11.499.520 bytes, SHA-256 `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`). O executável novo não foi instalado na pasta do jogo.
- A compilação confirma que os scopes foram incorporados, mas não fornece ainda amostras de Berezino. Sem uma captura Profile durante a passagem urbana, não há evidência para escolher cache de texturas ou afirmar ganho de FPS.

### Como obter evidência da captura

Em `PerfLogFrame`, a configuração `_PROFILE && !_DEBUG` inicializa `PerfCap` ligado. Quando uma frame ultrapassa o limite de lentidão, a source chama `GPerfProfilers.Diagnose(true)` e `GPerfCounters.Diagnose()` via `LogF`; portanto, o primeiro insumo prático é o `.RPT` criado pelo executável Profile enquanto se repete a mesma rota. `World::UpdatePerfLog()` também habilita os contadores e o profiler no cliente. A UI de captura do motor pode analisar uma captura e copiar todas as threads pelo botão `IDC_CAPTURE_EXPORT_ALL`; o código faz export para clipboard.

Procedimento para a próxima passagem controlada: fechar o client antigo, iniciar este `DayZProfile.exe` em uma pasta de jogo montada com os mesmos dados e perfil usados no Release, percorrer Berezino e arredores até ocorrerem as frames ruins, fechar normalmente e recolher o `.RPT` recém-criado. Procurar entradas `WARN: Slow frame detected` seguidas pelo diagnóstico dos counters, em especial `obAni`, `obLit`, `obTex`, `obShp`, `inTex`, `inDrw`, `prpIX` e `prpSc`. A rota deve ser repetida sem mudar resolução, configurações gráficas, cenário, servidor ou addons. Essa captura pode dizer qual caminho consome tempo; a comparação entre o candidato Retail e o baseline ainda exige teste A/B separado.

- Restore point do relatório antes deste complemento: `restore-points/fps-perf-capture-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.
- Restore point deste complemento sobre instancing: `restore-points/fps-instancing-analysis-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.

### Medição da travessia de grade em `World::PrepareDraw`

O scope `wPrep` abrangia a função inteira; só `hPrun` separava a parte de horizonte. Para distinguir traversal/culling da contribuição do horizonte, acrescentei scopes detalhados neutros:

- `wGrd` mede o conjunto das linhas de grade percorridas dentro de `PrepareDraw`;
- `wCel` mede uma célula individual da grade, incluindo quick-cull, aquisição da lista de objetos, clipping/pruning e submissão dos objetos dessa célula.

As chaves de visibilidade, os limites da grade, o caminho de instancing, e as chamadas de desenho permanecem iguais. Restore point antes da edição: `restore-points/fps-world-prep-grid-scopes-before-edit-20261116/world.cpp` (SHA-256 original `B259DE534E708A2106712F44F8488368CDC148C26C34E2C258A5B1B07820ED85`).

Build Profile isolado concluído em `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-world-prep-grid-profile-20261116\DayZProfile.exe` (16.053.248 bytes, SHA-256 `A6298CF0C217C8ACDFEEFFFFCDF148EFE465403496A2F8E6E09EB3ACF9D73FE7`). O log finaliza com o target `lib2010.vcxproj -> ...\DayZProfile.exe`; não há erros de compilação/link. Persistem avisos LNK4099 de PDBs externos ausentes. `dist\DayZ.exe` continua com SHA-256 `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`. Ainda falta coletar o `.RPT` do Profile em Berezino para determinar se a queda fica na travessia de células, no horizonte, na preparação individual ou no draw.

- Restore point do relatório antes de registrar essa build: `restore-points/fps-world-prep-grid-build-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.

### Revisão estática da alteração de blend LOD por registro instanciado

Comparei `lib/scene.cpp` com o restore point `restore-points/fps-lod-group-before-edit-20261115-191628/lib/scene.cpp`. `DrawObjectsInstanced` só recebe `updateBF=true` no segundo passe entre LOD fonte/destino (chamadas em `MergeInstances::Flush`, linhas próximas a 6974 e 6987); portanto, esse é o passe que avança o estado de transição. Para um `SortObject` com lista instanciada, o código anterior atualizava `blendFactor` duas vezes após as duas partes da lista e recalculava a cor por instância; a versão atual calcula uma cor por `SortObject` e avança o blend uma vez. Como a lista compartilha o mesmo registro/estado de LOD, isso evita avanço duplicado e mantém cor uniforme dentro da lista. Restore point traz SHA-256 original `C46E6C36049BC9373CD13C69C17245817B2F0981B0580EF967567EAE44F62749`.

Essa inspeção dá suporte estático à coerência da atualização única, mas ainda não substitui comparação visual de transições LOD nem teste de desempenho em rota urbana. O candidato Profile atual inclui essa alteração; a build Retail anterior continua isolada em `BuildOutput` e não foi instalada sobre `dist`.

- Restore point do relatório antes desta revisão: `restore-points/fps-blend-review-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.

### Runtime de diagnóstico isolado

Preparei `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-world-prep-grid-runtime-20261116` para executar a build Profile ao lado de uma estrutura de jogo. O executável foi copiado e seu SHA-256 confere com o artefato de build (`A6298CF0C217C8ACDFEEFFFFCDF148EFE465403496A2F8E6E09EB3ACF9D73FE7`). `Addons`, `dta`, `dll`, `BattlEye` e `Keys` são junctions para `D:\SOURCEDAYZS!!\DayZ`; cada destino foi verificado. O launcher cria perfil próprio `profile-fps-grid` e aceita parâmetros extras (`-connect`/`-port`) sem conectar automaticamente. Nenhum client foi iniciado durante o preparo; os processos existentes de teste foram deixados intactos.

Launcher: `Start-DayZProfile-FPS.bat`. Instruções: `README-FPS-DIAGNOSTIC.txt`. Os assets referenciados permanecem no jogo-base; nada foi copiado para `dist` e `dist\DayZ.exe` conserva o hash registrado acima.

- Restore point do relatório antes de registrar o runtime: `restore-points/fps-profile-runtime-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.

### D3D9: custo do caminho instanciado e do fallback

Instrumentei `EngineDD9::DrawMeshTLInstanced` (`lib/d3d9/engdd9.cpp`) com dois scopes detalhados e contadores ativos apenas quando `_ENABLE_PERFLOG` está habilitado:

- `d9HW`: tempo do caminho que constrói/usa o buffer de instâncias;
- `d9FB`: tempo do fallback genérico `Engine::DrawMeshTLInstanced`, que percorre as instâncias individualmente;
- `d9HWc`/`d9HWi`: chamadas do caminho acelerado e quantidade de instâncias nele;
- `d9FBc`/`d9FBi`: chamadas de fallback e quantidade de instâncias submetidas ao fallback.

Os acumuladores usam `DEF_COUNTER_P_EX` diretamente sob `#if _ENABLE_PERFLOG`, pois `ADD_COUNTER_P_EX` não está definido na versão local de `perfLog.hpp`. O build Profile validou as macros e a inclusão condicional; o caminho de Release (`_ENABLE_PERFLOG=0`) não instancia esses counters. A decisão de caminho, os limites do vertex buffer e o processamento dos objetos não foram alterados.

- Restore point antes da instrumentação: `restore-points/fps-d3d9-instancing-scopes-before-edit-20261116/engdd9.cpp` (SHA-256 `7F5B1FBFEA7AB4E7939F791B07D2CDA3912FEAC58EC607985BE0146E1DDA5F1B`).
- Restore point anterior à correção das macros de profiling: `restore-points/fps-d3d9-prof-counter-fix-before-edit-20261116/engdd9.cpp`.
- Build isolado: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-d3d9-instancing-profile-20261116\DayZProfile.exe` (16.054.272 bytes; SHA-256 `53B27D2ADF20EECCE0F9190B9825CD9959A614337EC86FBAA9EFB0281DF0A407`). O log termina com `lib2010.vcxproj -> ...\DayZProfile.exe` e não contém erros C/LNK/MSB; permanecem avisos LNK4099 de PDBs externos ausentes.
- Runtime isolado: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-d3d9-instancing-runtime-20261116`, com PDB/MAP que tiveram hashes conferidos contra o build e assets via junctions para `D:\SOURCEDAYZS!!\DayZ`.
- `dist\DayZ.exe` segue intacto, SHA-256 `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

A captura Profile ainda não foi executada. A interpretação de fallback ou ganho de FPS depende de `d9FB*` versus `d9HW*` numa passagem real pela cidade e não pode ser inferida apenas da compilação.

- Restore point do relatório antes de registrar esta instrumentação/build: `restore-points/fps-d3d9-profile-build-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.

### Candidato: reutilizar faixas de seção no fallback instanciado

Na rota genérica de `Engine::DrawMeshTLInstanced` (`lib/engineDraw.cpp`, a partir da linha 1370), o código anterior redescobria as mesmas faixas contíguas de seções visíveis para cada instância de uma mesma shape. A alteração calcula essas faixas uma vez por chamada quando `nInstances>1` e reutiliza início/fim/bias ao desenhar cada instância. Para uma instância, mantém o caminho original. Não modifica o material, a ordem das seções, o bias usado nos flushes nem o número de chamadas de desenho.

- Restore point antes da edição: `restore-points/fps-fallback-section-runs-before-edit-20261116/engineDraw.cpp` (SHA-256 `B71244AC752D4768D9120B09100343BE31A5A3D2D134815F8AAE363DAA04F538`); cópia pré-ajuste da declaração intermediária: `restore-points/fps-fallback-run-descriptor-fix-before-edit-20261116/engineDraw.cpp` (SHA-256 `16179A0C47BA308A7615F0C204EF0BB0771928C514B3CA49DC695FDD9B9FE4EA`). O restore point para a edição final que substituiu o struct por arrays é `restore-points/fps-fallback-int-arrays-before-edit-20261116/engineDraw.cpp`.
- A primeira tentativa de build deste candidato falhou no tipo local `InstancedSectionRun`, porque o `AUTO_STATIC_ARRAY` desta source exige `MoveData` no tipo armazenado. Substituí o struct por três `AUTO_STATIC_ARRAY(int,...)`, seguindo os tipos suportados pelo projeto. O build seguinte compilou `engineDraw.cpp` e terminou com código 0.
- Artefato Profile: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-fallback-section-runs-profile-20261116\DayZProfile.exe` (16.056.320 bytes, SHA-256 `0BBA3DC8522D07101392C444CEE5E04F3AFBECEA9B7EBE2965C79B563383FF55`). PDB: SHA-256 `1CD3C97536C59EDC71051EFBFB4A744404276EC2454035FE4524DCBC0AB310B9`; MAP: 13.777.601 bytes. Log: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-fallback-section-runs-profile-20261116\build.stdout.log`; linker terminou com `lib2010.vcxproj -> ...\DayZProfile.exe`. Avisos restantes são sobretudo PDBs externos não encontrados (`LNK4099`).
- Runtime isolado: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-fallback-section-runs-runtime-20261116`, com EXE/PDB/MAP idênticos ao build, perfil próprio `profile-fps-fallback`, launcher `Start-DayZProfile-FPS.bat` e README. `Addons`, `dta`, `dll`, `BattlEye` e `Keys` são junctions para o jogo-base `D:\SOURCEDAYZS!!\DayZ`. Não iniciou o client nem alterou os runtimes anteriores.
- `dist\DayZ.exe` não foi usado como saída. Não há teste A/B em Berezino nem captura de tempo deste candidato; portanto, ainda não existe evidência de melhora de FPS. O scope `d9SR` registra o custo de preparar as faixas; a próxima validação precisa comparar o mesmo percurso e conferir também o tempo total em `d9FB`.
- Restore point deste complemento do relatório: `restore-points/fps-fallback-section-runs-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md` (SHA-256 anterior `E08D3EC615EEC0CB56971C1A6E944296D37AAFF1030C14BE4078D4A494CCD616`).

### Revisão de segurança do índice de preparação de texturas

A investigação de `Scene::DrawObject*` esclareceu que `Scene::DrawObject(Object*)` (`lib/scene.cpp:11174`) só encaminha para `obj->Draw(...)`. Para objetos individuais, o custo passa por `Object::Draw` (`lib/object.cpp:1974+`): animação (`obAni`), seleção de luzes (`obLit`), `PrepareTextures` (`obTex`) e `Shape::Draw` (`obShp`). Para grupos, `Object::DrawSimpleInstanced` prepara texturas uma vez por grupo, mas também pode cair no ramo especial de estradas (`ClipLandOn`), que volta a desenhar cada instância.

O `TexAreaIndex` é um acelerador real já implementado: `Shape::PrepareTextures` usa o caminho de índice quando `_texArea._valid`, percorrendo texturas únicas em `prpIX`; sem índice percorre seções e estágios de materiais em `prpSc`. Porém, a ativação indiscriminada não é segura com o estado atual da source:

- `TexAreaIndex::Item::_tex` guarda ponteiro para `RefR<Texture>`, não uma referência forte. O comentário em `shape.hpp:521` diz que isso foi escolhido porque modelos alteram texturas durante o runtime.
- Materiais/seções são modificados por `SetMaterialExt`/`SetTexture` em diferentes subsistemas; a busca não encontrou invalidação geral de `_texArea` nesses mutadores. `Shape::Clear()` invalida o índice, mas não cobre alterações de materiais durante a vida do shape.
- O único auto-build encontrado em `Shape::CheckPushbufferCanBeUsed` (`shape.cpp:4239`) está comentado com uma hotfix: construção do índice podia produzir `areaOTex == INF` e causar floating-point traps em `Texture::PrepareAndCheckMipmapLevel()`.
- A divisão por `GetUVScale2D/3D()` em `BuildTexAreaIndex` pode dividir por zero quando a escala UV degenerada é zero. Portanto, remover só o comentário da hotfix pode reintroduzir crash/corrupção de seleção de mipmap.
- Os builds explícitos encontrados são os de landscape/roads, depois da preparação dos respectivos materiais. Isso indica que a intenção é usar o índice apenas onde os dados estão estabilizados.

Conclusão de engenharia: a oportunidade existe e pode ser grande em modelos de muitas seções, mas a alteração de baixo risco seguinte não é ativar o cache global. Primeiro precisamos obter a proporção e o custo real de `prpSc` versus `prpIX` numa captura urbana. Se `prpSc` dominar, o caminho seguro é projetar um índice de curta duração de referências/valores ou uma política de cache com invalidação centralizada e proteção para escalas UV zero/NaN/Inf, depois validar qualidade de mipmap e materiais animados. Nenhuma dessas mudanças funcionais foi aplicada nesta revisão.

Ajuste da leitura da pista do desenvolvedor: “Scene::DrawObject***” descreve o hotspot conceitual, mas a função literal `Scene::DrawObject` é trivial; a preparação individual relevante está em `Object::Draw` e o draw de seções em `Shape::Draw`/engine backend. Os scopes existentes `obAni`, `obLit`, `obTex`, `obShp`, `prpIX` e `prpSc` separam esses custos sem presumir qual domina.

- Fontes verificadas: `lib/scene.cpp:11174`, `lib/object.cpp:1733-2214`, `lib/shapeDraw.cpp:1565-1694`, `lib/Shape/shape.hpp:521-546,804,942-946`, `lib/Shape/shape.cpp:2495,4239-4243,5868-5974`, e chamadas explícitas em `lib/landscape.cpp:3881,4130`.
- Restore point do relatório antes desta revisão: `restore-points/fps-texture-index-safety-review-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md` (SHA-256 `9734E4FEF2EE6339356CD902562289423FEF2E77BC1470AAB97A540036727720`).

### Alcance real do candidato de faixas e medição de streaming

Revisei as condições que chamam `Scene::ObjectsForDrawing` no percurso de `World::PrepareDraw` (`lib/world.cpp:8254-8388`). O agrupamento de `ObjectGroup` para draw instanciado só é escolhido no caso especial de objetos que não podem ocluir (árvores/bushes), sem sombra possível, a mais de 200 m e com animação `AnimTypeNone`. Construções próximas do jogador seguem o processamento individual. Assim, o candidato `fps-fallback-section-runs` serve para medir/remover trabalho repetido em batches de modelos compatíveis, mas é pouco provável que explique sozinho a queda ao entrar em Berezino. Não o trato como correção da queda urbana.

O carregamento de células já tem scopes úteis, então não acrescentei um segundo scope redundante neste turno:

- `wCel` mede o trabalho total dentro de cada célula em `World::PrepareDraw`;
- `Landscape::DoPreloadObjects` (`lib/landSave.cpp:1247+`) já usa `lndPO` e `TCLoadObjects` ao solicitar dados/precargar shapes para chamadas sem espera;
- `Landscape::LoadObjects` (`lib/landSave.cpp:2356+`) usa `lndLO` e `TCLoadObjects` para desserializar a lista;
- `Landscape::UseObjects` (`lib/landSave.cpp:2703+`) chama `LoadObjects(..., noWait!=UOWait)`. A até 200 m, `World::PrepareDraw` escolhe `UOWait`; além disso, seu `wCel` envolve toda a chamada e o restante do culling/draw da célula.

Isso separa, numa captura já habilitada, o custo da célula, do preload e do load efetivo. A policy `UOWait/UONoWait` não foi alterada, pois trocá-la pode causar ausência/pop-in de objetos e o profiler ainda não mostrou que o wait é o culpado. O artefato de medição precisa ser executado na mesma rota; build bem-sucedido e leitura de source não provam FPS maior.

Integridade conferida após esses trabalhos: `D:\SOURCEDAYZS!!\SOURCE DEFINITIVA\DayZ-0.45.124252\dist\DayZ.exe` continua SHA-256 `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`. O jogo-base `D:\SOURCEDAYZS!!\DayZ\DayZ.exe` está presente e tem SHA-256 `B1DAA0DC70E8611180D106A61057AD94AFC891E6AFAB3CAF9195FD0C4E57084C`.

- Fontes: `lib/world.cpp:8254-8388`; `lib/landSave.cpp:1247-1308,2356-2410,2703-2735`; `lib/landscape.hpp:2267`.
- Nenhum arquivo de código foi alterado nesta revisão; este complemento só registra a limitação do candidato e o mapa de scopes de streaming já existente.
- Restore point deste complemento: `restore-points/fps-instancing-and-streaming-followup-doc-before-edit-20261115/FPS_URBAN_AUDIT_2026-11-15.md` (SHA-256 anterior `F1AEB0CEF48AA71FBEE9B05A83D37E5D35FF3D1AF249D322DA2A6F848D8870D3`).

### Próximo candidato estático: agrupar seções adjacentes no draw individual

A preparação individual passa por `Shape::DrawSections` (`lib/shapeDraw.cpp:74-154`), que hoje chama `PrepareTL` e `DrawSectionTL(i,i+1)` uma vez para cada seção visível. O backend D3D9 já aceita um intervalo `[beg,end)` em `EngineDD9::DrawSectionTL` (`lib/d3d9/engdd9.cpp:16931+`), e `DrawIndexedPrimitive` percorre as seções no intervalo (`12209+`). `Engine::DrawMeshTLInstanced` já usa a regra de agrupar seções contíguas com `ShapeSectionInfo` igual e `_minBoneIndex`/`_bonesCount` iguais, o que confirma um precedente interno para esse formato de draw.

Um candidato para construções individuais seria agrupar somente seções contíguas quando a `PolyProperties` efetiva for igual, os parâmetros de ossos forem iguais e não houver UV animado no intervalo. `PolyProperties::operator==` compara textura, flags e material; o mipmap/AOT continua sendo preparado antes em `Shape::PrepareTextures`, seção por seção. Para manter o comportamento exato, o agrupamento não deve atravessar seções ocultas, crateras ou seções com `GetUVchanged()==true`; o bias também precisa permanecer igual (fica igual quando `Special()` é igual). O pipeline D3D9 conta chamadas reais em `dPrim` e já expõe `ddDST` para o tempo de `DrawSectionTL`.

Este candidato não foi implementado: ainda requer revisão completa de `PrepareTriangleTL` e do comportamento de cada backend, além de build isolado e verificação visual. O comentário histórico em `Shape::DrawSections` diz que uma antiga fusão de seções dependia do `FaceArray`; não é possível assumir que esse comentário se aplica à regra mais restrita de faixas contíguas sem esclarecer o histórico. A prova parcial no backend instanciado e o argumento `[beg,end)` são evidência de viabilidade, mas não bastam para declarar equivalência visual.

Neste momento dois builds `Retail Server` estavam ativos sobre a mesma source (`DayZ.sln /t:Rebuild` em `build/door-actions-fix-v121-20261115` e `lib` em `lib/build/zombie-collision-diag-v121`). Por segurança, não editei `shapeDraw.cpp` enquanto esses compiladores estavam em execução. Antes de qualquer implementação, confirmar que os processos MSBuild/cl terminaram e salvar restore point dedicado.

- Fontes consultadas: `lib/shapeDraw.cpp:74-154`; `lib/Shape/poly.cpp:18-35`; `lib/Shape/poly.hpp:35-99`; `lib/engineDraw.cpp:1370-1465`; `lib/d3d9/engdd9.cpp:12209-12340,16931-17046`; equivalentes `EngineDDT::DrawSectionTL` e `EngineDDXB::DrawSectionTL`.
- Restore point do relatório antes desta nota: `restore-points/fps-drawsections-group-review-doc-before-edit-20261115/FPS_URBAN_AUDIT_2026-11-15.md` (SHA-256 anterior `7BB8EE64F6E59E63A2B86EC7F7DA27CCD89FFEE1959CBB4AA327AFB5772281F9`).

## 2026-11-16 — candidato individual `DrawSections` implementado e compilado

Implementei em `lib/shapeDraw.cpp` o agrupamento de seções adjacentes no draw individual, com restore point criado antes da edição:

- Original: `restore-points/fps-drawsections-group-before-edit-20261115/shapeDraw.cpp`, SHA-256 `4BDD66170DFE51EFF8B1EDC36EF350E4AD09C7F18E8938F65946A727007F12CE`.
- Atual: SHA-256 `43820FEDE60246817043A66345BA6B0F36173BBE33D3532A132AF454107C03FD`.

O agrupamento exige seções contíguas, `PolyProperties` iguais e os mesmos `_minBoneIndex`/`_bonesCount`. Ele não atravessa seção oculta/proxy, seção com UV animado nem passes de crateras; nesses caminhos permanece o draw unitário original. Foi adicionado o scope `dSec` sob `cuDrw`, com contagem de draws e de seções agrupadas. Isso mede o número de preparações/envios que a otimização efetivamente elimina em Profile; não mede FPS por si só.

Revisão estática do backend D3D9: `EngineDD9::DrawSectionTL` usa os parâmetros do primeiro section para shader/skinning e envia `[beg,end)` a `DrawIndexedPrimitive`. O precedente existente em `Engine::DrawMeshTLInstanced` também agrupa seções contíguas por propriedades e faixa de ossos. A igualdade de `PolyProperties` cobre textura, flags especiais e material. Os fatores de AOT/mipmap são preparados antes em `Shape::PrepareTextures`, seção por seção; UV animado é excluído do novo agrupamento. Essas evidências sustentam o candidato, mas ainda falta validar equivalência visual e o backend D3D9 em execução.

A build Profile isolada terminou com exit code 0 e gerou `DayZProfile.exe` via `DayZ.sln /t:lib`, Win32/Profile, no diretório `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-drawsections-group-profile-20261115-2120`. O log registra compilação de `shapeDraw.cpp` e link de `lib2010.vcxproj`; não apareceram erros de compilação/link. Permanecem avisos de símbolos/PDB de bibliotecas antigas. SHA-256 do executável: `EBF453D99B646E7C6397255097224F084B0AF24D57592003A366ACB6B7C7B07C`.

Runtime de revisão preparado em `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-drawsections-group-profile-runtime-20261116`: executável e arquivos de símbolos da build, launcher independente e perfil próprio; `Addons`, `dta`, `dll`, `BattlEye` e `Keys` são junctions verificadas para `D:\SOURCEDAYZS!!\DayZ`. O hash do executável no runtime confere com o artefato de build. Não foi instalado em `dist` nem iniciado automaticamente.

Ainda falta o teste do usuário no mesmo trajeto urbano, comparando runtime anterior e candidato e revisando objetos/texturas/LOD para artefatos. Não afirmo ganho de FPS nem equivalência visual antes disso. O processo de teste anterior e os builds Retail já em andamento foram preservados; `dist\DayZ.exe` permanece intocado.

- Restore point deste relatório antes deste complemento: `restore-points/fps-drawsections-group-build-doc-before-edit-20261116/FPS_URBAN_AUDIT_2026-11-15.md`.

## 2026-11-17 — índice de texturas por shape no draw individual (`_texAreaDraw`)

Esta seção registra a implementação do candidato de menor risco identificado na revisão de segurança de 2026-11-16. Ela substitui a conclusão anterior de não ativar o índice: em vez de apenas remover o comentário histórico, o índice novo é separado do `_texArea` do landscape, não faz divisão por escala UV, satura o AOT problemático e tem invalidação explícita nos mutadores de seção/material.

### O que foi implementado

- `TexAreaIndex::Item` agora guarda `_z2Scale` (escala UV do estágio) e `_forceMipmap0` (estágio sem relação com UV).
- `TexAreaIndex::AddItem` recebe os dois valores, deduplica por textura + escala + modo e satura AOT não finito ou acima de `1e20f`. Isso elimina a multiplicação de `FLT_MAX` por `aotFactor` que produzia `1.#INF000` no caminho antigo (`lib/Shape/shape.cpp:5893`).
- `Shape` ganhou `_texAreaDraw` (`lib/Shape/shape.hpp:815`), separado de `_texArea` (landscape/estradas).
- `Shape::BuildTexAreaDrawIndex()` (`lib/shapeDraw.cpp:1605`) monta o índice a partir das seções e estágios do material. Para estágios com `uvScale >= 0` guarda `areaOTex` já corrigido por `RepairAreaOfTexture` e o multiplicador `uvScale`; para `uvScale < 0` guarda `_forceMipmap0`. Não há divisão por `GetUVScale2D/3D()` na construção.
- A construção ocorre uma vez por shape/LOD após a criação do vertex buffer, em `Shape::CheckPushbufferCanBeUsed` (`lib/Shape/shape.cpp:4245-4248`), somente quando `_canUsePushBuffer && !_texArea._valid && !_texAreaDraw._valid`. O custo é atribuído a `TCCreateVertexBuffer` por um `TimeManagerScope` (`lib/shapeDraw.cpp:1609`).
- `Shape::PrepareTextures` (`lib/shapeDraw.cpp:1670`) prefere `_texArea` e só depois `_texAreaDraw`. No caminho do índice aplica `PrepareTextureScr(z2*_z2Scale, special, _aot*aotFactor)` ou `LoadHeaders()+PrepareMipmap0(true)` para `_forceMipmap0`. O caminho antigo continua em `prpSc` (`shapeDraw.cpp:1721`); o índice é medido em `prpIX` (`1699`).
- Invalidação de `_texAreaDraw`: `Shape::Clear` (`shape.cpp:2496`), `Shape::FindSections` (`4210`), `Shape::SetAsOneSection` (`4225`), as duas sobrecargas de `Shape::SetUniformMaterial` (`4602`, `4615`) e `Shape::RecalculateAreas` (`5877-5878`).

### Onde o custo estava, em ordem

O mapa abaixo é o resultado da inspeção estática; ele não substitui a captura Profile.

1. Varredura de seções e estágios de material em `Shape::PrepareTextures`, uma vez por objeto individual e uma vez por grupo instanciado. É o alvo desta alteração.
2. Preparação por seção em `Shape::DrawSections` (`PrepareTL`, bias, UV animado e `DrawSectionTL`). O agrupamento de seções contíguas já está na árvore (`dSec`), mas ainda sem A/B.
3. Trabalho por membro no caminho instanciado: `GetLightGridIndex`, `GetLandShadowSmooth` (quatro amostras bilineares) e ordenação por lista de luzes em `Scene::DrawObjectsInstanced`.
4. Cobertura/preparação Far LOD e ordenação (`oFLod`, `occBB`) antes do desenho.
5. Travessia da grade do mundo e streaming de células (`wPrep`, `wCel`, `lndLO`, `lndPO`), incluindo `UOWait` até 200 m.
6. `ClippingInfo` duplicado e caminho individual para objetos próximos em `World::PrepareDraw`; o atalho de grupo só vale para não oclusores sem sombra acima de 200 m.
7. Seleção/reconstrução da light grid e ordenação de `SortObject` (`FindSortObject`).
8. GPU/overdraw urbano ainda não foi descartado; os scopes atuais são de CPU.

### Equivalência com a varredura original

| Caso | Varredura antiga | `_texAreaDraw` |
|---|---|---|
| Textura base da seção | `ss.GetAreaOverTex(0)` com `z2` e `special` | mesmo valor guardado por seção; `z2*1.0f` |
| Estágio com `uvScale >= 0` | `RepairAreaOfTexture(ss.GetAreaOverTex(uvStage))`, `z2*uvScale` | mesmo AOT corrigido e mesmo `uvScale` guardados |
| Estágio com `uvScale < 0` | `LoadHeaders()` + `PrepareMipmap0(true)` | mesmo caminho, marcado por `_forceMipmap0` |
| Mesma textura com escalas diferentes | duas chamadas com `z2` diferentes | duas entradas, pois a deduplicação inclui `_z2Scale` |
| `AnimationContext` com seções próprias | usa `GetSS(animContext,...)` | índice desativado (`SectionsChanged()`) |
| Thermal vision / textura TI | prepara `_stageTI` no fim da varredura | índice desativado; varredura antiga |
| Passe de sombra (`prepareBaseTextureOnly`) | prepara só a textura base | índice desativado; varredura antiga |
| `_useWorldEnvMap` | `GetTextureForDraw()` troca pela textura do landscape | índice não é construído; varredura antiga |
| Landscape/estradas | `_texArea._valid` | `_texArea` tem precedência; `_texAreaDraw` nem é construído |

O conjunto de texturas preparadas é o mesmo: a deduplicação só junta chamadas idênticas de textura, escala e modo. Quando a mesma textura aparece em seções diferentes com o mesmo AOT, a varredura antiga chamaria `PrepareTextureScr` repetidamente; o índice chama uma vez.

### Limites, riscos e o que ainda não foi provado

- O índice guarda `const RefR<Texture>*`, igual ao `_texArea` existente. Trocas in-place (`SetTexture`, `SetStageTexture`, recarga de textura) mantêm o endereço e são seguidas; substituição de seção ou de material é coberta pelos pontos de invalidação acima. Hot-reload de material em build de desenvolvimento pode deixar o índice antigo até o vertex buffer ser recriado; no fluxo Retail não há hot-reload.
- Não há divisão nem multiplicação de `FLT_MAX` na construção; o AOT é saturado em `1e20f` e o `Assert(aot>=0)` permanece.
- A construção acontece na thread principal, depois de `ConvertToVBuffer` e fora do escopo de desenho; `PrepareTextures` lê o índice na mesma thread. Não foi adicionado lock novo; a premissa é a mesma do `_texArea`.
- Custo de memória: cerca de 16 bytes por item em x86 (ponteiro, dois floats e dois bools) mais o overhead do `AutoArray`; dezenas de itens por LOD de construção. O total depende de quantos shapes recebem vertex buffer e precisa ser medido se houver preocupação de VRAM/RAM.
- A construção desloca uma varredura completa de seções/estágios para a criação do vertex buffer. Na primeira aproximação de uma cidade, parte do custo pode aparecer em `TCCreateVertexBuffer`/streaming, embora seja paga uma vez por shape/LOD em vez de a cada draw.
- **Não há medição de FPS, frame time ou equivalência visual nesta rodada.** O build apenas prova que o código compila e liga. A hipótese é que `prpSc` caia e `prpIX` suba; se `prpSc` já era pequeno, o efeito no frame será pequeno.

### Build

Build Profile isolada, Win32, toolset v100 (`_MSC_VER=1600`), MSBuild 15.9, `DayZ.sln /t:lib`, com `OutDir` sem espaços em `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-texarea-draw-index-profile-20261117\`. O log termina com `lib2010.vcxproj -> ...\DayZProfile.exe` e não contém erros C, LNK ou MSB; restam avisos de PDBs externos ausentes (`LNK4099`) e de importação local em bibliotecas de áudio (`LNK4217`/`LNK4049`), já presentes em builds anteriores.

- `DayZProfile.exe`: 16.122.368 bytes, SHA-256 `826C348CC505A8652AEDE671D25D3D71270598C516C03C99DB1F83A8D0394DEB`.
- `DayZProfile.pdb`: 75.787.264 bytes, SHA-256 `D916E6A7237458D298644489D1013DEC61D75CE32936B83A904198435636017B`.
- `DayZProfile.map`: 13.783.182 bytes, SHA-256 `483B2513397E0A464EA3C2694CE8E9BB2B03DCE9DF7C6A6DC18E50EDAAFF71B7`; o mapa contém `?BuildTexAreaDrawIndex@Shape@@QAEXXZ ... shapeDraw.obj`, confirmando que o método novo entrou no binário.
- Log: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-texarea-draw-index-profile-20261117\build.stdout.log`.
- Runtime isolado: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-texarea-draw-index-profile-runtime-20261117`, com EXE/PDB/MAP cujos hashes conferem com o build, perfil próprio `profile-fps-texareadraw`, launcher `Start-DayZProfile-FPS.bat` e README. `Addons`, `BattlEye`, `dll`, `dta` e `Keys` são junctions para `D:\SOURCEDAYZS!!\DayZ`; `steam_api.dll` e `steam_appid.txt` foram copiados. O runtime não foi iniciado.
- `dist\DayZ.exe` não foi usado como saída e permanece com SHA-256 `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

Correções de ambiente necessárias para esta build, todas com restore point:

1. `El/Speech/LPC/openlpc.cpp` usava `round()`, inexistente no CRT deste toolset. Restaurada a variante com `openlpc_round` + fallback `lrintf` da árvore irmã. Restore point `restore-points/fps-openlpc-round-msvc-before-edit-20261117`; original `38B8CA2C4537A3E7E7EB224858612198F802B7747FF9E58CDC39E0146F8B0924`, restaurado `DC0CB75436CC7F96953688545A9A13BFD8344B13BABF9BEDBF01372BE473EB9E`.
2. `build\main-menu-final-test\bin` continha Bullet compilada com `_MSC_VER=1900`, incompatível com os objetos `1600`. As três libs foram trocadas pelas versões `1600` de `extern`. Restore point `restore-points/fps-profile-buildenv-bullet-vs2015-20261117/bin-vs2015-backup` com os hashes das libs substituídas.
3. `build\main-menu-final-test\simul` continha Simul compilada com `1900`. Foi substituída pelas libs VC10 Static Release de `extern\SimulWeather\Simul\lib\Win32\VC10\Static Release`. Restore point `restore-points/fps-profile-buildenv-simul-vc10-20261117/simul-vs2015-backup` com os hashes originais.
4. `lib\lib2010.vcxproj` dependia de `legacy_stdio_definitions.lib` (shim VS2017) e compilava `crtCompat.cpp`, que duplicava `__iob_func` contra a LIBCMT do VC10 (`LNK2005`), além de puxar `legacy_stdio_wide_specifiers.lib` (`LNK1104`). A dependência foi removida de todas as configurações e a entrada de `crtCompat.cpp` foi retirada da compilação. Restore point `restore-points/fps-profile-buildenv-vcxproj-v100-20261117/lib2010.vcxproj`; SHA-256 original `4FAC5A53E7F7075930AC515E3D6C2E57B915C358DB3D743F3518F570A3CFF980`, atual `DD873214099ECAF14E492CAA01F14B9BADE587FAC7F91716F16B8668072B8300`. O arquivo `extern\legacy_stdio_definitions.lib` copiado durante a tentativa ficou sem uso.

Hashes finais das fontes do candidato:

- `lib/Shape/shape.hpp` `18CF428F39675B1C889AF91166C2E44ED277A5BCDDDE02B4CBFB0C877789D5C8`
- `lib/Shape/shape.cpp` `20CF4261444001EAA6507A6DB6C0C74C67439B99F52EEBFBFB68C64C15E5CACA`
- `lib/shapeDraw.cpp` `F274A05635D993C1A69CD50B4149FC80F71C72F0771D06B258E165F9B095774D`

### Validação necessária

1. Executar o runtime Profile candidato na mesma rota de Berezino, com as mesmas configurações, e comparar `obTex`/`prpTx` e principalmente `prpIX` (índice, incluindo o antigo caminho do landscape) contra `prpSc` (fallback). No candidato, `prpSc` deve ficar restrito a objetos animados, thermal, passe de sombra, env map e casos sem índice; a diferença de contagem/tempo entre os dois scopes mede o que a alteração realmente remove.
2. Comparar com o runtime Profile anterior (baseline) na mesma rota e com uma variável por vez. Se o baseline não for o mesmo commit, reconstruir o baseline a partir dos arquivos do restore point para um par limpo.
3. Revisar visualmente texturas de construções a média/longa distância (nitidez de mipmap), superfícies alfa/transparentes, objetos animados, thermal vision e o passe de sombra. O índice não pode piorar a escolha de mipmap nem deixar textura sem preparo.
4. Se o frame time não mudar mesmo com `prpSc` reduzido, o gargalo não é a varredura de texturas; usar `dSec`, `o1Drw`, `inst`, `oFLod` e os passes de GPU antes de alterar mais código.
5. Só declarar ganho de FPS depois da comparação A/B no mesmo percurso, com o cliente e a máquina ociosos de builds concorrentes.

- Restore point deste relatório antes desta seção: `restore-points/fps-texarea-draw-index-doc-before-edit-20261117/FPS_URBAN_AUDIT_2026-11-15.md`, SHA-256 anterior `0FD94BB07066EA453F14CCE65026430D0CDAA8547430AB1188612D2B6FD39FED`.

---

## 2026-11-17 - Candidatos de correcao: preparacao por objeto nos passes de profundidade

Diagnostico de trabalho confirmado pelo desenvolvedor: a queda perto de construcoes/casas/mata vem do processamento de objetos antes da renderizacao (`Scene::DrawObject***`, preparacao de materiais/poligonos), nao de sombra e nao do limite de vsync. Com `FSAA=2` o mesmo objeto proximo e desenhado **3 vezes por frame**:

1. `RMDPrime` - mapa de profundidade para a mascara SSSM (render target separado, sem MSAA);
2. `RMZPrime` - z-only que preenche o depth buffer principal;
3. passe de cor (`RMCommon`).

Os dois primeiros existem apenas para profundidade: usam shaders `PSAlphaOnly`, nao escrevem cor e nao consomem material nem iluminacao. Mesmo assim, executavam a mesma preparacao por objeto do passe de cor.

### Candidato A - "depth-prime lean" (Retail, 12:32)

Alteracoes, todas restritas a `RMDPrime`/`RMZPrime`:

1. `lib/object.cpp` (~2148 e ~2199): `PrepareTextures(..., prepareBaseTextureOnly=true)` quando `dp._zprimeIsBeingRendered` (somado ao caso ja existente `_sbIsBeingRendered`). So a textura base - fonte do alpha test - e preparada; normal map, detalhe e specular ficam de fora.
2. `lib/object.cpp` (~2103): `SelectAggLights` e pulado no `RMZPrime`; a lista de luzes entregue ao shape fica vazia. O passe nao consome iluminacao.
3. `lib/d3d9/engdd9.cpp` (~15674, `PrepareTriangleTL`): retorno antecipado antes da combinacao de cor de material e de `DoSetMaterial` quando `_renderingMode==RMDPrime || RMZPrime`.

Build: `DayZ045-fps-depthprime-lean-retail-20261117`, MSBuild 15.0, Retail/Win32, `DayZ.sln /t:lib`, `exit=0`, sem erros C/LNK/MSB (restam apenas `LNK4099` de PDBs externos ja conhecidos).

- `DayZ.exe`: 11.567.104 bytes, SHA-256 `F279A9FEE966F0766B0C1C9E4DE3E88D9C48AAFCF7623239F931AF3CE1613B9C`.
- `DayZ.pdb`: SHA-256 `1E9D82614F576F2C74A09AADFC78595D03BCAD5033991D7AB2E4ACDAD4E6D129`.
- `DayZ.map`: SHA-256 `6C8189E2F880FF0432C4148C30E4739AD2CAC91BF310B55438C93B0A2FB6ECE6`.
- Runtime: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-depthprime-lean-retail-runtime-20261117`, com `Start-DayZ-Retail-FPS.bat`, `README-FPS-RETAIL.txt`, EXE/PDB/MAP copiados do build, perfil `perfil-janela` com `vsync=0`, `FSAA=2`, `AToC=0`, `shadowQuality=4`, `shadowZDistance=250`, `viewDistance=3000`, `preferredObjectViewDistance=1736.2638`, `sceneComplexity=500000`, `ssaoEnabled=0`.
- Restore point antes das edicoes: `restore-points/fps-depthprime-lean-before-edit-20261117-120927`.
- `dist\DayZ.exe` permanece `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

Risco conhecido: o `RMDPrime` alimenta a mascara SSSM. A folhagem usa a textura base (preservada) para o alpha test; arvores tambem amostram o sampler1 (normal map), ligado por `SetupMaterialTextures` e nao pela preparacao base-only. Se aparecer recorte/contorno errado em arvore ou sombra estranha em vegetacao, o candidato e revertido.

**Status: aguardando A/B in-game no mesmo ponto de Berezino. Nenhum ganho de FPS afirmado.**

### Candidato B - remocao do passe `RMZPrime` (Retail, em compilacao)

Desenho:

- `lib/d3d9/engdd9.hpp`: novo membro `_mainDepthPrimed` e `SetMainDepthPrimed`. `ZPrimed()` deixa de retornar `true` fixo e passa a devolver `_mainDepthPrimed`.
- `lib/landscape.cpp` (`PrimeDepthBuffer`): apos `EndDepthMapRendering`, `SetMainDepthPrimed(zOnly || GetFSAA()==0)`. Sem o passe z-only e com `FSAA>0`, o flag fica falso e o passe de cor volta ao comportamento normal.
- `lib/world.cpp`: chamada `PrimeDepthBuffer(..., zOnly=true, ...)` comentada em `World::Simulate` e no caminho R2T.

Racional: o `RMZPrime` reenvia os mesmos objetos proximos (raio `max(GetShadowZ(),100)` = 250 m) apenas para preencher o depth principal; o passe de cor entao usa `ZFUNC EQUAL` e desliga alpha test. Sem ele, `ZFUNC` volta a `LESSEQUAL` com `ZWRITE` e alpha test normal. Para geometria opaca o resultado e o mesmo, sem uma submissao inteira de objetos. A mascara SSSM continua sendo produzida pelo `RMDPrime`, e `_zPrimingDone` continua controlando `_doSSSM`/`PSTBasicSSSM`, entao a tecnica de sombra proxima nao muda.

O que desaparece: uma das tres passadas de objetos por frame (`RMDPrime`, `RMZPrime`, cor -> `RMDPrime`, cor), incluindo preparacao por objeto, transforms, selecao de LOD e submits. Trade-off: o passe de cor passa a escrever profundidade (overdraw de depth), normalmente barato com early-Z; o ganho esperado e de CPU/driver, que e onde o sintoma aparece.

Risco: qualquer dependencia escondida de `ZFUNC EQUAL` no passe de cor (stencil/SSSM) aparece como artefato. `groundPrimed` continua preenchido com o mesmo retangulo, entao o landscape proximo mantem `_zPrimingDone` para SSSM, mas agora escreve profundidade normalmente.

Detalhe encontrado na revisao: `EngineDD9::InitDraw` nao limpava o depth principal - o bloco de clear para FSAA estava comentado. Quem limpava era o passe z-only (`Clear(true,false)` em `BeginDepthMapRendering(true)`). Com o passe removido, `InitDraw` passa a incluir `D3DCLEAR_ZBUFFER` (e stencil, se houver) quando `_mainDepthPrimed` e falso, evitando testar contra o depth do frame anterior.

Restore point antes das edicoes: `restore-points/fps-rmzprime-removal-before-edit-20261117-1235` (`engdd9.hpp`, `engdd9.cpp`, `landscape.cpp`, `world.cpp`).

Build: `DayZ045-fps-noprime-retail-20261117`, MSBuild 15.0, Retail/Win32, `DayZ.sln /t:lib /m:4`, `exit=0`, sem erros C/LNK/MSB. A primeira tentativa falhou com `C1090` (PDB do compilador corrompido) e a segunda com `C2039` (`GEngine` e `Engine*`); corrigido adicionando `virtual ZPrimed()`/`virtual SetMainDepthPrimed` na classe base `Engine` (`lib/engine.hpp`).

- `DayZ.exe`: 11.567.104 bytes, SHA-256 `34CA9C61F8A974E41E27735F3EE8D2EE180C09A17AE1AB3C8F4CDDE82CEA5E26`.
- `DayZ.pdb`: SHA-256 `31DD70E503799AC07D19C3AF4CB3B55C12B94F472AA20923AC799E15364FDC1B`.
- `DayZ.map`: SHA-256 `04C5A017E841CA060CFC61AD55894F593A4F2005A7EE6FC9EE3B851BAB941556`; o mapa contem `?ZPrimed@EngineDD9@@UBE_NXZ` e `?SetMainDepthPrimed@EngineDD9@@UAEX_N@Z`, confirmando que o candidato entrou no binario.
- Runtime: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-noprime-retail-runtime-20261117`, com `Start-DayZ-Retail-FPS.bat`, `README-FPS-RETAIL.txt`, EXE/PDB/MAP copiados do build (hash do EXE confere), `Addons`/`BattlEye`/`dll`/`dta`/`Keys` como junctions e perfil `perfil-janela` com `vsync=0`, `FSAA=2`, `AToC=0`, `shadowQuality=4`, `shadowZDistance=250`, `viewDistance=3000`, `preferredObjectViewDistance=1736.2638`, `sceneComplexity=500000`, `ssaoEnabled=0`.
- `dist\DayZ.exe` permanece `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.
- Hashes das fontes ao final: `lib/d3d9/engdd9.hpp` `A09ACC1A24C6C967627CB8530BD16BFA3EBAA16A3B8A56A83E1C4104B0CF7E20`; `lib/d3d9/engdd9.cpp` `FF6A9F4708416B60E9E781EEFE3BCF1C8022549A30C3A8FE89467891C0FBE391`; `lib/landscape.cpp` `D5F970B5CFA215C576AF7E74557974655D31329BAA377E68564332B47656D7F1`; `lib/world.cpp` `FBDA870A757978494920E10EAF6BFF5AB97082689CA7E1080D968BA814DA46F0`; `lib/engine.hpp` `D2ACBAF4CB0D76CBDC79C5480A70873C03D8B9DA8C127E44D2736BE356AE0C5C`.

Checklist visual do candidato B: furos no cenario (depth nao limpo - `InitDraw` agora limpa), flicker em estradas/decalques sobre o terreno (passam a escrever profundidade), recorte de folhagem/arvores, sombra da vegetacao e objetos que sumam perto de casas.

**Status: aguardando A/B in-game no mesmo ponto de Berezino. Nenhum ganho de FPS afirmado.**
Rastreabilidade dos binarios: o candidato B foi compilado sobre a arvore que ja continha as edicoes do candidato A (`object.cpp:2103/2148/2199` e `d3d9/engdd9.cpp:15683` seguem ativos). Portanto o binario B e **A+B**, e a comparacao in-game e progressiva: original -> A -> A+B. O `.map` de A nao contem `SetMainDepthPrimed`; o de B contem `?ZPrimed@EngineDD9@@UBE_NXZ` e `?SetMainDepthPrimed@EngineDD9@@UAEX_N@Z`. Folha de teste com os tres executaveis, hashes e protocolo: `D:\SOURCEDAYZS!!\BuildOutput\FPS-AB-TEST-20261117.txt`.

**Proximo passo bloqueado no teste manual:** sem o A/B no mesmo ponto de Berezino nao ha como atribuir a queda ao codigo removido nem afirmar ganho. Se A+B nao mudar a oscilacao, o proximo corte sera no passe de cor (`Scene::DrawObjectsInstanced`/`Shape::DrawSections`).
### Candidato C - sem GetLandShadowSmooth por instancia nos passes de profundidade (A+B+C, 13:17)

Alteracao em `lib/scene.cpp`, `Scene::DrawObjectsInstanced` (3 pontos: ~6164, ~6182, ~6205):

```cpp
info._shadowCoef = dp._zprimeIsBeingRendered ? 1.0f : GetLandShadowSmooth(info._origin.Position());
```

Antes o coeficiente era calculado para cada instancia em todos os passes. `GetLandShadowSmooth` amostra o mapa de sombra do terreno (4 amostras, `GetLandShadowDepthSmooth`) e pode chamar `Landscape::SurfaceY`. Nos passes `RMDPrime`/`RMZPrime` os shaders sao `PSAlphaOnly` e nao consomem `to.instanceShadow` (o valor vem do VS, mas o PS nao o usa); o trabalho era descartado. Com o candidato B o `RMZPrime` ja nao existe, entao o corte elimina o calculo no `RMDPrime` - o unico passe de profundidade restante - e mantem o valor real no passe de cor (`dp._zprimeIsBeingRendered == false`).

Nada no passe de cor foi alterado; a expressao preserva o caminho original quando `dp._zprimeIsBeingRendered` e falso.

Build: `DayZ045-fps-noprime-noshadowcoef-retail-20261117`, MSBuild 15.0, Retail/Win32, `DayZ.sln /t:lib /m:4`, `exit=0`, sem erros C/LNK/MSB.

- `DayZ.exe`: 11.567.104 bytes, SHA-256 `89264F51294BD9A0CA9B6EE8EFDA81244D264DF63EF8535AEDA030102CAEDA39`.
- `DayZ.pdb`: SHA-256 `F125793F9234A2F9CA8146B43B902C10987B674A1F9C872198BC425BF3FBF15A`.
- `DayZ.map`: SHA-256 `5CDEAC0B1972F210EFB6BBFD2C84865865D8EE2FF7AEADE2D4675748018E7513`; contem `?ZPrimed@EngineDD9@@UBE_NXZ` e `?SetMainDepthPrimed@EngineDD9@@UAEX_N@Z` (herdados de B).
- Fontes: `lib/scene.cpp` `4375B8E9A80772481ED8A72B2E1A972058865E071F4EBD50939C88F83EDE76AC`; `lib/scene.hpp` `E12D6E223980542139619344E8DC0F370A4ABA8EFAFFE1EF7A5AFBEF959323B6` (inalterado).
- Restore point antes da edicao: `restore-points/fps-prime-shadowcoef-before-edit-20261117-1340` (`scene.cpp` anterior `CC1A7C4777AEAE8DB24F833D1449CD6B8D58BE5BBFD213D6048F77D9C0EDAEBB`).
- Runtime: `D:\\SOURCEDAYZS!!\\BuildOutput\\DayZ045-fps-noprime-noshadowcoef-retail-runtime-20261117` (EXE/PDB/MAP copiados, hash do EXE confere, `Start-DayZ-Retail-FPS.bat`, `README-FPS-RETAIL.txt`).
- `dist\\DayZ.exe` permanece `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

Risco: nenhum no passe de cor (nao foi tocado). No passe de profundidade o valor 1.0 so alimenta `instanceShadow`, que o `PSAlphaOnly` ignora; se algum shader de profundidade passar a consumir esse termo, o valor precisaria voltar.

**Status: aguardando A/B in-game no mesmo ponto de Berezino (original vs A vs A+B vs A+B+C). Nenhum ganho de FPS afirmado.**

### Candidato D - passe de profundidade sem light grid por instancia (A+B+C+D, compilado)

Alteracoes em `lib/scene.cpp`, `Scene::DrawObjectsInstanced`, todas condicionadas a `dp._zprimeIsBeingRendered` (passe de profundidade). O passe de cor nao mudou.

1. Os tres pontos que preenchem `info._lightGridIndex` (~6161, ~6179, ~6201) param de chamar `GetLightGridIndex(x,z)` no passe de profundidade e gravam `-1`. Esse indice so existe para ordenar/agrupar instancias por luz; o passe nao consome luz.
2. No despacho da cadeia (~6263), o passe de profundidade entra no ramo de desenho unico, com lista de luzes vazia, em vez de executar `QSort(instances, ctx, CmpInstances)` e o laco de agrupamento por light grid. Antes, cada cadeia era ordenada (O(n log n) por cadeia, por passe) e dividida em varios `DrawSimpleInstanced`, um por grupo de luzes, sem nenhum efeito no resultado (apenas profundidade).

Racional: com o candidato B o unico passe de profundidade restante e o `RMDPrime` (mascara SSSM). Ele reenvia todos os objetos proximos com shaders `PSAlphaOnly`. Depois de A (sem preparo de material, sem `SelectAggLights`, sem `DoSetMaterial`) e C (sem `GetLandShadowSmooth` por instancia), o que restava de trabalho por instancia era exatamente o indice de light grid, a ordenacao por luz e a fragmentacao da cadeia em grupos. Nada disso afeta os pixels: os shaders de profundidade nao leem luz.

Risco: `info._lightGridIndex == -1` so e lido pelo comparador e pelo laco de agrupamento, ambos pulados no passe de profundidade (verificado por varredura: `_lightGridIndex` aparece apenas em `scene.cpp` 6061, 6062, 6294, 6300 e nos tres pontos de preenchimento). `DrawSimpleInstanced` e `DrawMeshTLInstanced` nao usam o campo.

Restore point antes da edicao: `restore-points/fps-prime-lightgrid-before-edit-20261117-1330` (`scene.cpp` `4375B8E9A80772481ED8A72B2E1A972058865E071F4EBD50939C88F83EDE76AC`).

Build: `DayZ045-fps-primelightlean-retail-20261117`, MSBuild 15.0, Retail/Win32, `DayZ.sln /t:lib /m:4`. A primeira tentativa (obj limpo) e a segunda (incremental) falharam com `fatal error C1090: PDB API call failed, error code '23'` em arquivos diferentes (animation.hpp/soundScene.cpp/simul.cpp), sem erro de codigo. A terceira tentativa, retomada sem apagar `obj` (reaproveitando 170 objetos) e com `mspdbsrv` encerrado, terminou com `exit=0` e sem error C/LNK/MSB.

- `DayZ.exe`: 11.567.104 bytes, SHA-256 `14E21045C65B40717895780318DD2F2C1AA29C53ED4BA79AB61C759E9F1474D6`.
- `DayZ.pdb`: SHA-256 `31BFF20F179E9257149B66F4FC6E34AF403828F521CF44560BBB11497F83AE6C`.
- `DayZ.map`: SHA-256 `706DBDFA3A5F05C434A8CB1ECA289F49C85271DA5C3E235A390596E52D76A9B9`; contem `?ZPrimed@EngineDD9@@UBE_NXZ` e `?SetMainDepthPrimed@EngineDD9@@UAEX_N@Z`.
- Fonte: `lib/scene.cpp` `350C9B4F6298D4BF21B539135E2A84E54FAA9DA77A8303E3D4F9D6267F1128D9`.
- Runtime: `D:\\SOURCEDAYZS!!\\BuildOutput\\DayZ045-fps-primelightlean-retail-runtime-20261117` (EXE/PDB/MAP copiados, hash do EXE confere, `Start-DayZ-Retail-FPS.bat`, `README-FPS-RETAIL.txt`).
- `dist\\DayZ.exe` permanece `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

**Status: build concluido. Aguardando A/B in-game no mesmo ponto de Berezino (original vs A vs A+B vs A+B+C vs A+B+C+D). Nenhum ganho de FPS afirmado.**

### Candidato E - preparo de secao 1x por run no instancing por software (A+B+C+D+E, compilado)

Primeiro corte no **passe de cor**. Alvo: `Engine::DrawMeshTLInstanced` (`lib/engineDraw.cpp`), caminho `nInstances>1` usado quando o vertex buffer da forma nao foi criado para instancing de hardware e o desenho cai para uma submissao por instancia.

Antes, para cada instancia e para cada run de secoes, o codigo refazia `TLMaterial` + `PrepareTL` completo (`LoadTextureMipmaps`, `PolyProperties::Combine`, `DoPrepareTriangle` com textura/sampler/render states/pixel shader e `DoSetMaterial`). Esse estado depende apenas da secao/run, nao da instancia. Numa cadeia de centenas de instancias com varios runs, eram milhares de preparos completos por frame.

Depois:

```cpp
// instancia 0: fixa tipo de pixel shader e escala/offset de UV (texgen)
SetInstanceInfo/BeginInstanceTL/BeginMeshTL(instances[0]);
for (r : runs) {
  CreateMaterial(mat, HWhite);
  PrepareTL(run r);              // UMA vez por run
  for (j : instances) {
    SetInstanceInfo/BeginInstanceTL/BeginMeshTL;
    ApplyMaterialTL(cb, mat);    // so material/luz no espaco do objeto
    DrawSectionTL(run r);
    EndMeshTL;
  }
}
```

- `ApplyMaterialTL` e um novo virtual em `Engine` (vazio) implementado em `EngineDD9`. Ele reproduz apenas o trecho final de `PrepareTriangleTL`: modulacao por `_modColor` + `DoSetMaterial`. Essa e a unica parte de `PrepareTriangleTL` que depende da instancia, porque `SetupMaterialLights` transforma a direcao da luz para o espaco do objeto via `cbState._world` (`TransformAndNormalizeVectorToObjectSpace`).
- No passe de profundidade (`RMDPrime`/`RMZPrime`) `ApplyMaterialTL` retorna cedo, preservando o candidato A. Ainda assim o E remove `DoPrepareTriangle`/`LoadTextureMipmaps` por instancia nesse passe, que o A nao removia.
- A ordem `BeginInstanceTL`/`BeginMeshTL` da instancia 0 antes do `PrepareTL` e obrigatoria: `BeginInstanceTL` seleciona o tipo de pixel shader (PSTBasic/PSTBasicSSSM/PSTAlphaOnly) e `BeginMeshTL` grava a escala/offset de UV consumida por `EnableCustomTexGen` dentro de `DoPrepareTriangle`. Sem isso o texgen usaria o estado da forma anterior.
- O laco original permanece intacto para `nInstances==1` (objetos unicos), sombra, caminho de instancing de hardware (`DrawMeshTLSectionsInstanced`), LOD, sorting e passe de cor de objetos unicos.

Restore point: `restore-points/fps-colormaterial-before-edit-20261117-1325` (`engine.hpp` `D2ACBAF4CB0D76CBDC79C5480A70873C03D8B9DA8C127E44D2736BE356AE0C5C`, `engineDraw.cpp` `50949F1F62EDDE59FD9BC329E5C37A84B88307144192107CB6CAD1B2CF35CEF2`, `engdd9.hpp` `4B73AF821D8FE29A2148856A6208772796D13DE3395549D5A195D91D5C8D994C`, `engdd9.cpp` `FF6A9F4708416B60E9E781EEFE3BCF1C8022549A30C3A8FE89467891C0FBE391`).

Fontes com E:

- `lib/engine.hpp` `94A6E8224BD0EACBB2C7FC5F9A401C5203872BE444E52BC8AAF7822DBF1BECC1` (novo virtual).
- `lib/engineDraw.cpp` `DB51E4C580163CA9B780F6BF80795D073BB2482003F96A9D7F89B2165EE0FC4F` (novo caminho run-outer).
- `lib/d3d9/engdd9.hpp` `C8593E1FB4318659581AA4C4016536922EA08DE2ECE67F62178DE2F350B325EC` (declaracao).
- `lib/d3d9/engdd9.cpp` `757236D61692FF413EBC21019EF8E0F9EAFB5E81EC5427899663616320A0B6A8` (implementacao de `ApplyMaterialTL`).

Riscos a observar: UV deslocado/repetido (ordem BeginMeshTL/PrepareTL), cor/luz diferente entre instancias da mesma cadeia (ApplyMaterialTL ausente ou fora de ordem), objeto sumindo/shader errado. O caminho de instancing por software e usado principalmente em vegetacao/objetos repetidos; casas unicas (`nInstances==1`) nao passam por ele.

Build: `DayZ045-fps-colorinstance-retail-20261117`, MSBuild 15.0, Retail/Win32, `DayZ.sln /t:lib /m:4`, `exit=0` e sem error C/LNK/MSB.

- `DayZ.exe`: 11.568.128 bytes, SHA-256 `BCEE389B1A9C7496016853AD3ADC639B9846CE9687F4AB8670B723F1CA7FCB95`.
- `DayZ.pdb`: SHA-256 `B6342BD5F4A927576DFBC1DAA88045ED09EF3B16A15E5B9CD016F9D2FF157702`.
- `DayZ.map`: SHA-256 `748532D5F26215A038C7D8C3825C24AD7273071221F1944A065D9CFCC4BAFF96`; contem `?ApplyMaterialTL@EngineDD9@@MAEXHABUTLMaterial@@@Z` e `?DrawMeshTLInstanced@EngineDD9@@...`.
- Runtime: `D:\\SOURCEDAYZS!!\\BuildOutput\\DayZ045-fps-colorinstance-retail-runtime-20261117` (EXE/PDB/MAP copiados, hash confere).
- `dist\\DayZ.exe` permanece `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

**Status: build concluido. Aguardando A/B in-game no mesmo ponto de Berezino (original vs A vs A+B vs A+B+C vs A+B+C+D vs A+B+C+D+E). Nenhum ganho de FPS afirmado.**

### Candidato F - sem submissao de objetos no depth prepass quando FSAA=0 (A+B+C+D+E+F, compilado)

Corte no nivel de submissao, acima de `Shape::DrawSections`: atinge objetos unicos (casas) e instanciados igualmente, porque remove a **segunda chamada inteira** de `Object::Draw` no frame.

Com `FSAA=0` (configuracao real do usuario), `World::Simulate` chama `Landscape::PrimeDepthBuffer(..., zOnly=false, ...)`. Em `landscape.cpp:5993` o passe desenhava `Scene::DrawDepthPrime`, que submetia `_drawPass1ON._original`, `_drawPass1ON._blending` e `_drawPass1AN` em `RMDPrime`. Depois `SetMainDepthPrimed(true)` fazia o passe de cor redesenhar exatamente as mesmas listas com `ZFUNC EQUAL`. Eram duas submissoes completas por objeto (fora as cascatas de sombra).

Depois:

- `Scene::DrawDepthPrime` ganhou `bool drawObjects` (sem default, para nao conflitar com o `drawListId` de `_HELISIM_RENDER_TO_TEXTURE`; o unico chamador passa todos os argumentos).
- Em `Landscape::PrimeDepthBuffer`, `primeObjects = zOnly || GEngine->GetFSAA()!=0`. Com `FSAA=0` e `zOnly=false`, `drawObjects=false`: as listas de objetos nao sao submetidas no depth prepass.
- `SetMainDepthPrimed(primeObjects && (zOnly || GEngine->GetFSAA()==0))` fica `false` nesse caso. O passe de cor volta ao caminho normal (`ZFUNC LESSEQUAL` + `ZWRITE TRUE`) e o clear principal limpa Z/stencil (`engdd9.cpp:3439`).
- `DrawVolShadows` e `DrawPass3Part`/`DrawPass4Part` continuam no `DrawDepthPrime`; `DrawRectShadowBuffer` (shadow maps), `shadowQuality=4` e `shadowZDistance=250` nao foram alterados.
- Com `FSAA>0` o comportamento antigo e mantido (depth prepass separado continua necessario).

O que isso remove por objeto unico, por frame: uma chamada de `Object::Draw` com `Animate`, `SelectAggLights`, `PrepareTextures`, `UseDrawMatrices`, `Shape::Draw` -> `DrawSections` (`CreateMaterial`/`PrepareTL`/`DrawSectionTL`) e os `DrawIndexedPrimitive` correspondentes. O interior de `DrawSections` ja une secoes adjacentes com `PolyProperties` identicas (`shapeDraw.cpp:144-163`, comparando `_texture`/`_special`/`_surfMat` em `poly.cpp:18-35`); o desperdicio real era a submissao inteira repetida.

Restore point: `backup-fps-f-20261117-164157` (`scene.cpp` `350C9B4F6298D4BF21B539135E2A84E54FAA9DA77A8303E3D4F9D6267F1128D9`, `scene.hpp` `E12D6E223980542139619344E8DC0F370A4ABA8EFAFFE1EF7A5AFBEF959323B6`, `landscape.cpp` `D5F970B5CFA215C576AF7E74557974655D31329BAA377E68564332B47656D7F1`).

Fontes com F:

- `lib/scene.cpp` `99B6BFFC91C536904FAC978BABC8E9F2904B971FE2BEC34A77629A9A8607650C`.
- `lib/scene.hpp` `539C579BD15F36D26B1842CB0449EA7D4ABB09A3E9A8F82939D8785B83A13D20`.
- `lib/landscape.cpp` `FAA0BC763CDB57F8E298156F73CE6F5B05EE8E3934A430C5E09C60C4093D6D24`.

Riscos: textura linear de profundidade gerada no prepass deixa de conter os objetos; efeitos que a usam (DOF/SSSM) podem mudar. Com `PostFX=0` e `ssaoEnabled=0` o uso esperado e nulo, mas precisa de conferencia visual. Diferenca de oclusao/profundidade, objeto aparecendo atraves de parede, sombra alterada.

Build: `DayZ045-fps-noprimeobjects-retail-20261117`.`DayZ.exe` 11.568.128 bytes, SHA-256 `CA4F03D7A5C1E8A4C575C5D5BD1C95841B63D87B091191D8EB9E6F276A4FB329` (PDB `033D1D84B07B3D233733D5C70DF1D9CBBCA029FBC16080524E8FC078A4B1089F`, MAP `22084066556D3DBA4C6E410B5381D0FDC505784F6C87EA2E124B2E66E8DE4D7A`; build exit=0, sem error C/LNK/MSB). O `.map` contem `?DrawDepthPrime@Scene@@QAEXAAUDrawContext@1@_NW4PredicationMode@Engine@@1H@Z` e `?SetMainDepthPrimed@EngineDD9@@UAEX_N@Z`. Runtime: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-noprimeobjects-retail-runtime-20261117` (EXE/PDB/MAP copiados, hash conferido). `dist\DayZ.exe` permanece `FE8AA125C436B66F40C7ED866F713892F6E27AE79026571E4650D015C6DA0C23`.

**Status: build concluido. Aguardando A/B in-game no mesmo ponto de Berezino (original vs A vs A+B vs A+B+C vs A+B+C+D vs A+B+C+D+E vs A+B+C+D+E+F). Nenhum ganho de FPS afirmado.**

---

## 2026-11-18 - Teste Release do F e correcao F2

O F Retail acima nao pode ser testado com o pacote V38: o Retail verifica assinatura de PBO e recusa o `ui.pbo` modificado. O teste real foi feito com um F **Release**, compilado sobre a mesma arvore cumulativa A+B+C+D+E+F.

### Build e teste do F Release

- Build: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-noprimeobjects-release-20261117`, Release/Win32, `DayZ.sln /t:lib /m:4`, `exit=0`, sem error C/LNK/MSB.
- `DayZInt.exe`: 13.916.672 bytes, SHA-256 `7C71EA457671EDBCA91F25297981075840FD2506FFE1DE7ACE16FFD7475312EB`.
- `DayZInt.pdb`: SHA-256 `AD464C876D14F3519E0B8EE2317175DD43ABC3010D7658DC6BA9692FF6C853E7`.
- `DayZInt.map`: SHA-256 `D6C3D7B21103A0E8FC92307E4005B296E04C70D732128FD94BD98D07ACFD7E0A`; contem `?DrawDepthPrime@Scene@@QAEXAAUDrawContext@1@_NW4PredicationMode@Engine@@1H@Z` e `?SetMainDepthPrimed@EngineDD9@@UAEX_N@Z`.
- Runtime: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-noprimeobjects-release-runtime-20261117`, com `DayZInt-F.exe`, `DayZInt-ORIGINAL-v37-clean.exe`, perfil `perfil-janela`, `Addons` via junction para o pacote V38.
- Perfil: `FSAA=0`, `MSAA=0`, `PostFX=0`, `ssaoEnabled=0`, `VSync=0`, `PPAA=0`, `AToC=0`.
- RPT do runtime F: 39.220 bytes, sem `Corrupted data detected.` e sem `ErrorMessage`; apenas warnings de material/conteudo.

**Resultado do teste in-game:** o FPS subiu de forma perceptivel, mas a cena ficou incompleta. O personagem nao aparece em 3a pessoa; casas, arvores e construcoes aparecem de longe e **desaparecem quando a camera se aproxima**; a grama pisca. Nao ha numero de FPS registrado. O ganho e **invalido** porque parte da cena nao foi desenhada.

### Causa raiz

Com predicao/command buffers ligados, o passe de cor nao percorre de novo a lista de objetos: ele reproduz os comandos gravados no depth prepass. O F pulou `StartCBPredicatedScope`/`DrawPass1Part` quando `drawObjects=false`. O escopo `_oPasO` ficou **nunca gravado** (`beg=-1`, `end=-1`). Em `CopyCBPredicatedScope`, o teste `scope.end <= scope.beg` classificou esse escopo como "vazio" e retornou `true`; o passe de cor entao pulou `DrawPass1Part` e a lista `_drawPass1ON` nao foi desenhada. O `Assert(scope.beg>=0)` desaparece em Release.

Correcao da afirmacao anterior: o passe de cor **nao reenvia todos os objetos com `ZFUNC EQUAL`** quando a predicacao esta ligada; ele reproduz os command buffers. A formulacao anterior era imprecisa.

### Correcao F2

- `lib/d3d9/engdd9.cpp`, `EngineDD9::CopyCBPredicatedScope`: retornar `false` quando `scope.beg < 0`, antes do teste de escopo vazio. Assim, o passe de cor cai no caminho direto e desenha as listas com `ZFUNC LESSEQUAL` + `ZWRITE TRUE`; `_mainDepthPrimed` continua `false` quando o prepass nao submete objetos.
- `lib/scene.cpp`: comentario em `DrawDepthPrime` registrando que o escopo nao gravado forca o desenho direto no passe de cor.
- Build: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-noprimeobjects-release-f2-20261118`, Release/Win32, `exit=0`, sem error C/LNK/MSB.
- `DayZInt.exe`: 13.916.160 bytes, SHA-256 `04BEB7A5F29BE963DFFA6A0A612D26F8AAE54CACA02E9EB765EAE58C93EF37AE`.
- `DayZInt.pdb`: SHA-256 `820A72D9A758358C67824F3E1EACFE8081FCCF5A90A87C75B77643C5DA1DCAA2`.
- `DayZInt.map`: SHA-256 `2B87863BC6C4DC6260215BBBC0205DEEB16A88DD6B4617B65B79B3076E7931EA`; contem `DrawDepthPrime` e `CopyCBPredicatedScope@EngineDD9`.
- Fontes F2: `lib/d3d9/engdd9.cpp` `DD4866460CF783DEF142BACA0D78CBB8D14BF0191263B13CC4F77C0DE5EB2615`; `lib/scene.cpp` `09E1FC6B886A4D29C993EA29BF1455A260D1F21C8F541320FE814596FD5E8404`; `lib/scene.hpp` `539C579BD15F36D26B1842CB0449EA7D4ABB09A3E9A8F82939D8785B83A13D20`; `lib/landscape.cpp` `FAA0BC763CDB57F8E298156F73CE6F5B05EE8E3934A430C5E09C60C4093D6D24`.
- Restore point: `backup-fps-f2-20261118-003054`.
- Runtime F2: `D:\SOURCEDAYZS!!\BuildOutput\DayZ045-fps-noprimeobjects-release-runtime-f2-20261118`, com `DayZInt-F2.exe`, `DayZInt-F.exe`, o original, launcher `Start-DayZ-Release-F2.bat` e `README-FPS-F2.txt`.

**Status da F2 (18/11/2026):** testada no mesmo ponto de Berezino. **Personagem e casas/arvores voltaram**: personagem visivel em 1a/3a pessoa; casas/arvores presentes de longe, de perto e no contato. **Mas a cena nao fechou**: a **grama e intermitente** (na maior parte do tempo nao aparece; surge so de vez em quando, em manchas) e a **sombra do personagem aparece nas costas** (sobre a camisa e as pernas, em vez de ficar projetada no chao). RPT com 37.920 bytes, sem `Corrupted data detected.` e sem `ErrorMessage`. **FPS baixo**, sem ganho; o relato nao trouxe numero. Conclusao: a F2 e correcao visual **parcial** e **correcao de performance invalida**. A linha F/F2 esta encerrada.

**Prints da F2:** `site/public/fps-2026-11-18/f2-teste-01.jpg` (1108x834, 274.295 B, SHA-256 `57FAB8846CA97C498CCAB4E0EC6E2D878756EAAB79FA588F22DE62CCECE3F730`) e `f2-teste-02.jpg` (1920x1080, 407.643 B, SHA-256 `75EE18475D221203712CFD527EA193992AC315E87F0E9EC9600C9BDEC7B1457F`).

**Ponto de retomada:** primeiro comparar F2 vs Original no mesmo ponto para confirmar se grama e sombra sao regressoes da F2. Depois compilar A+B+C+D+E em Release (sem F/F2) a partir de `backup-fps-f-20261117-164157` e comparar com o Original, vsync off, cena completa. Se nao pagar, medir o passe de cor (`Scene::DrawObjectsInstanced` / `Object::Draw` / `Shape::DrawSections`) antes de cortar. Detalhes em `site/src/content/fps-drop.md`, secao "Ponto de retomada".

### Artefatos de teste

- Prints: `D:\SOURCEDAYZS!!\site\public\fps-2026-11-18\f-teste-01.jpg` ... `f-teste-06.jpg`.
- Documentacao da frente: `D:\SOURCEDAYZS!!\site\src\content\fps-drop.md` (aba "Resolver Drop de FPS" no site).
- Folha A/B: `D:\SOURCEDAYZS!!\BuildOutput\FPS-AB-TEST-20261117.txt`.
