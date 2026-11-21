# Plano de implementação de desempenho e migração para Rust

> **Para execução:** usar a skill `superpowers:executing-plans` e acompanhar as tarefas pelas caixas de seleção. Cada trilha pode ser executada e revisada separadamente depois de satisfazer suas dependências.

**Objetivo.** Reduzir quadros longos no cliente e CPU por tick no servidor dedicado, mantendo a funcionalidade, a compatibilidade e os caminhos atuais do motor.

**Arquitetura.** Medir a referência, validar a otimização de filas já presente no workspace e tratar as esperas do cliente e os custos do servidor em mudanças independentes. Integrar Rust por ABI C em módulos pequenos onde o perfil e a comparação com C++ justificarem a troca. O C++ continua controlando estado do motor, I/O, rede, dispositivo gráfico e ciclo de vida dos buffers.

**Tecnologias.** C++ legado, Direct3D 9, Visual Studio 2026 com MSVC `v145`, Windows SDK `10.0.26100.0`, alvo x64, PerfLog e WPR/WPA. O workflow oficial compila `Retail` e `Retail Server` em `windows-2025-vs2026`. Makefiles Linux e Rust i686 são verificações suplementares; Rust será qualificado primeiro para `x86_64-pc-windows-msvc` antes de qualquer integração ao jogo.

**Especificação.** [Avaliação de desempenho](../../AVALIACAO_DESEMPENHO.md). Esta execução parte de `50fa2a66609e3508d66a0f679f8dd7eb6e03ef86`, commit que já contém a migração de build x64/VS2026. A referência anterior `5216ad1f` permanece identificada como snapshot histórico na avaliação. As mudanças de filas do cliente e do servidor aguardam replay e medição em runtime.

## Estado da execução — 21/11/2026

| Trilha | Estado | Evidência ou bloqueio |
| --- | --- | --- |
| F1 builds | Build de referência confirmado; candidata em validação | A Action `44626990961` compilou `Retail` e `Retail Server` x64 em `windows-2025-vs2026`. A Action do PR também verificará Rust no Windows e publicará um artefato candidato. O build local Linux não substitui esse alvo. Ver [estado dos builds e testes](../../desempenho/estado-build-e-testes-2026-11-21.md). |
| F2 referência | Bloqueado | Sem executáveis e distribuição de teste para capturar cliente e dedicado. |
| S1 filas | Implementação e teste unitário concluídos; runtime pendente | Cursor e compactação estável integrados; helper testado com semântica de referência `Ref<T>`. Replay de rede e ganho ainda dependem do jogo executável. |
| S2/S3/C1/C2/C3 | Bloqueados para mudanças orientadas por perfil | Dependem das capturas F2 e de replay funcional. |
| R1 codec PBO | Piloto isolado implementado; validação Windows e corpus pendentes | 15 testes debug/release em x86_64 e i686 sob QEMU, Clippy e smoke C++ ABI x86_64 e i686 passaram localmente. A Action candidata compila e testa o crate em Windows x64. Falta comparar com corpus PBO real e medir desempenho ponta a ponta. |
| R2/R3/R4/E1 | Pendentes | Dependem de corpus, perfil, build e runtime qualificados por plataforma. |

Nenhuma funcionalidade do jogo foi removida. O crate Rust continua isolado e não foi escolhido como backend padrão.

## Restrições globais

- Preservar recursos, conteúdo, opções, formatos PBO/PAA/PAC, protocolo, saves, addons, missões e plataformas de referência. Preservar latência de entrada, ordenação e semântica de replicação e aparência nas condições de referência.
- Compilar e medir cada candidata separadamente. Manter um caminho C++ funcional para cada módulo Rust, com seleção no início do processo. O padrão permanece C++ até a qualificação e a decisão de adoção por alvo.
- Usar corpus e capturas da distribuição correspondente ao executável. Registrar a procedência em um manifesto. Manter conteúdo de distribuição fora do repositório de código quando ele vier de insumos externos.
- A menor unidade de entrega é uma revisão com um objetivo, uma comparação e um retorno ao comportamento de referência. Usar revisões distintas para instrumentação, melhoria de algoritmo, integração Rust e alteração de padrão.

## Pontos de revisão

| Caso | Resultado esperado | Tarefa |
| --- | --- | --- |
| PBO truncado, checksum inválido ou saída fora do tamanho contratado | Erro controlado, saída parcial descartada, leitura válida seguinte funciona | R1 |
| Biblioteca Rust ausente, incompatível ou com arquitetura diferente | Processo inicia pelo caminho C++ e registra o motivo uma vez | R2 |
| Rajada de rede com limites atingidos, dependências e atualizações locais | Mesma sequência efetiva de envios, mesmas flags e itens pendentes | S1 |
| Alt-Tab ou reset do dispositivo com mipmaps pendentes | Worker e recursos concluem ou são recuperados conforme a referência, sem bloqueio permanente | C1 |
| Empates da replicação ou custo dinâmico de rota | Mesmas decisões de envio e retomada da busca sob as condições reproduzidas | S2, R4 |

## Ordem e dependências

| Trilha | Entregas | Depende de |
| --- | --- | --- |
| Fundação | F1 build, F2 captura de referência | Insumos da distribuição e ambientes de execução |
| Servidor | S1 filas, S2 replicação, S3 simulação | F2 para validar ganhos. S2 usa a referência validada de S1 |
| Cliente | C1 esperas DX9, C2 streaming e VRAM, C3 renderização e consumo | F2. C2 e C3 usam a atribuição de tempo de C1 |
| Rust | R1 piloto PBO isolado, R2 integração, R3 texturas, R4 replicação/A*/visibilidade | R1 pode começar com corpus e compiladores em paralelo a F2. R2 requer R1 e F2. R3 e R4 requerem R2 qualificado no alvo ou uma qualificação ABI equivalente, além do perfil do módulo |
| Entrega | E1 paridade, adoção e reversão | Cada mudança aprovada individualmente |

As trilhas podem avançar em paralelo em cenários isolados. Se o build de uma plataforma bloquear, registrar o insumo que falta e continuar com análise de capturas existentes, corpus offline e a outra plataforma disponível. O trabalho offline não aprova ganhos no jogo.

## F1. Restabelecer builds reproduzíveis

**Arquivos.** `Game.sln`, `lib/lib.vcxproj`, `.github/workflows/build-release.yml`, `Makefiles/Makefile`. Registrar em `Docs/desempenho/` o alvo x64 da Action oficial e, em seção separada, o estado do Makefile Linux legado.

- [ ] Registrar compilador, SDK, configuração, arquitetura, dependências e hashes dos binários e conteúdo. O alvo oficial é Windows x64 / VS2026 / MSVC `v145` / SDK `10.0.26100.0`; separar Release com símbolos e Profile se o build oferecer ambos.
- [ ] Validar na Action os nomes `Retail` e `Retail Server`, a coleta de `DayZ.exe`/`DayZServer.exe` e DLLs e os hashes do artefato. A execução `44626990961` atesta o build da revisão base; registrar o run da candidata após completar.
- [ ] Manter a correção de sintaxe do Makefile como trilha suplementar. O target Linux atual ainda espera dependências ausentes (`RSA/lib` e `logger.c`); isso não é o caminho de release x64 documentado pela solução e pela Action.
- [ ] Gerar cliente e servidor executáveis. Confirmar inicialização, entrada na missão e conexão cliente-dedicado com a mesma distribuição antes da liberação pública. Arquivar comandos finais e registrar plataformas não cobertas. A aceitação em runtime exige teste do artefato exato.

## F2. Capturar a linha de base

**Arquivos.** `El/Common/perfProf.hpp`, `lib/main.cpp`, `lib/Network/networkServer.cpp`. Durante a execução, criar `Docs/desempenho/cenarios.md` e um esquema comum de resultados com revisão, plataforma, cenário, rodada, unidade e caminho da captura.

- [ ] Definir percurso urbano/rural, giro rápido, inventário, mira, chuva/noite e Alt-Tab. Rodar com cache frio e quente em séries separadas. Fixar câmera ou percurso quando possível, resolução, qualidade, driver, VSync e teto de FPS.
- [ ] Definir servidor vazio, população intermediária e próxima da capacidade, com entidades e carga equivalentes entre revisões. Incluir entrada em rajada, reconexão, gravação e sessão prolongada para memória.
- [ ] Capturar cliente com PerfLog/WPR, usando GPUView se precisar separar espera de driver e atividade de GPU. No servidor Linux usar temporização de tick do motor e CPU do processo com `perf stat`, com perfil amostrado quando disponível. Separar CPU ativa de tempo de parede.
- [ ] Coletar pelo menos cinco pares intercalados por cenário para decisões iniciais, aumentando a amostra se o efeito ficar perto da variação. Registrar p50/p95/p99 e contagem de quadros acima de 33,3 e 50 ms. No dedicado, registrar CPU por tick e por segundo, p50/p95/p99 de tick, profundidade de fila, atraso de envio e pico de memória. Medir RAM privada no Windows ou RSS no Linux, e registrar VRAM e I/O quando relevantes.
- [ ] Fixar tolerâncias de regressão a partir da dispersão da referência, antes de avaliar cada candidata. Aceitar ganho quando a métrica alvo melhorar além da variação normal e a matriz de paridade e limites passar. Para nichos, registrar a carga em que há ganho e a frequência desse cenário. Usar captura pouco instrumentada para confirmar o resultado.

## S1. Validar e concluir a otimização das filas

**Arquivos.** `lib/Network/networkClient.cpp`, `lib/Network/networkServer.cpp`, `lib/Network/networkMsg.cpp`, `Es/Containers/array.hpp`, `lib/Network/networkQueueUtils.hpp`, `lib/Network/tests/networkQueueUtilsTest.cpp`.

- [x] Isolar o diff existente de cursor e compactação. Revisar `SendMsgQueue`, tempos de vida das referências e a interação de `PrepareNextUpdate` com a fila.
- [ ] Comparar base e candidata com fila vazia, um item, muitos itens, todos dependentes, lote misto, orçamento interrompido, geração em meio ao lote e host local. Comparar ordem efetiva por identificador, flags, fences, bytes contabilizados e fila residual. Fixar a sequência da referência em cada replay.
- [ ] Capturar CPU, deslocamentos e retenção de memória em rajadas. Comparar também filas curtas, onde o custo de compactação pode superar a economia. Aceitar com paridade e ganho na carga que motivou o patch. Se falhar, manter a base como referência e corrigir essa revisão antes de empilhar S2.

## S2. Reduzir custo da replicação em C++

**Arquivos.** `lib/Network/networkServer.cpp` em `CreateObjectsList`, `CmpPlayerObjects`, `PrepareNextUpdate` e `SendMessages`.

- [ ] Separar medição de cálculo de erro, preparação, ordenação e envio sob `crOLS`. Contar candidatos, enviados, atualizações críticas e bytes por jogador.
- [ ] Se preparação ou alocação dominar, comparar reaproveitamento de memória e descarte antecipado apenas quando a equivalência de interesse e ownership for demonstrável. Se a ordenação dominar, comparar redução de movimentação ou seleção incremental na mesma sequência de envio. Medir o custo de materializar uma lista compacta.
- [ ] Reproduzir rastros com prioridades empatadas, orçamento cortando um empate, mensagens atrasadas, mudança de ownership, entrada/saída da área de interesse e valores não finitos que o motor aceite. Verificar decisões críticas e atraso por jogador. Se os empates do `QSort` legado produzirem diferença observável, manter aquele caminho até que a equivalência esteja definida e comprovada.
- [ ] Adotar a versão C++ aprovada como referência para avaliar R4. Se a ordenação ficar abaixo do ruído no total por tick, encerrar a hipótese de portá-la nessa carga.

## S3. Tratar os custos restantes do dedicado

**Arquivos.** `El/Multicore/jobs.cpp`, `lib/world.cpp`, `lib/AI/pathPlanner.cpp` e `Enfusion/src/Physics/enf_physicsbulletworld.cpp`.

- [ ] Rankear CPU ativa e p99 por subsistema sob carga fixa. Separar `mtWai`, A*, simulação, física, scripts e persistência.
- [ ] Para espera de jobs relevante, comparar lotes mínimos e espera curta seguida de evento usando a mesma capacidade de workers. Verificar perda de wakeup e caudas do tick. Para A*, medir alocações e expansões e comparar reaproveitamento de estado temporário em C++.
- [ ] Se seguirem dominantes, abrir revisões independentes para repetição de consultas físicas, scripts e persistência. Exigir paridade de contatos, veículos e cadência, e medir serialização e I/O separadamente. Para mover gravação a worker, exigir snapshot consistente, conclusão observável e recuperação de saves.
- [ ] Entregar separadamente a maior melhoria confirmada. Submeter o A* a R4 somente se o núcleo isolável seguir relevante.

## C1. Localizar e reduzir esperas do cliente

**Arquivos.** `lib/d3d9/engdd9.cpp`, `lib/d3d9/postProcess.cpp`, `lib/d3d9/txtD3D9.cpp`.

- [ ] Correlacionar quadros longos com `ppQWt`, `drwVi`, o escopo interno `drwVF`, `txMLo`, `texLd` e a linha do tempo de CPU, GPU e disco. Tratar `drwVF` como parte de `drwVi` na soma. Registrar o valor real de `GPU_MaxFramesAhead`; o default 1000 não aciona a repetição forçada com oito posições.
- [ ] Se consultas dominarem, comparar coleta de resultados prontos preservando último resultado válido, reset e limite de latência configurado. Se a segunda espera do worker dominar, medir o motivo das solicitações tardias e separar tempo de despacho, execução e espera antes de testar reagendamento.
- [ ] Rodar cenas com exposição variável, óticas, mudança rápida de câmera, Alt-Tab, reset do dispositivo e apresentação fora do fluxo normal. Validar imagem, tempo de quadro e atraso de entrada. Se I/O, conversão ou upload dominarem, encaminhar os dados a C2, C3 ou R3 conforme o componente identificado.

## C2. Reduzir cópias, recargas e pressão de memória

**Arquivos.** `lib/d3d9/engdd9.cpp` (`InitDXGIInfo`, `Clip32B`), `lib/d3d9/txtD3D9.cpp`, `lib/pactext.cpp`, `El/QStream/qbStream.cpp`, `El/FileServer/fileServer.cpp`.

- [ ] Contar descartes seguidos de recarga, bytes por upload, RAM temporária e I/O urgente. Para PBO, contar pedidos de fatia, bytes lidos e decodificações repetidas. Separar custo do cache e do codec.
- [ ] Quando houver pressão comprovada, ampliar primeiro a contabilidade interna de capacidade e somas para 64 bits. Qualificar a conversão para APIs legadas e comparar orçamento observado do driver e capacidade física. Avaliar `IDXGIAdapter3` apenas nos alvos compatíveis, com a consulta DX9 disponível.
- [ ] Comparar buffers temporários limitados por worker e consolidação de leituras idênticas em revisões distintas. Verificar offsets, precedência de addons, variantes do arquivo, urgências e cancelamento. Medir RAM retida, latência e picos em percurso frio, quente e sessão longa.
- [ ] Aceitar ajustes de residência com redução de recargas ou quadros longos, mantendo qualidade e orçamento observado. Encaminhar conversão de mip a R3 quando o perfil indicar custo de CPU na decodificação.

## C3. Reduzir custos de renderização e ociosidade

**Arquivos.** `lib/main.cpp`, `lib/scene.cpp`, `lib/d3d9/engdd9.cpp`, `lib/d3d9/ShaderCompileCache.cpp`, `lib/d3d9/postProcess.cpp`.

- [ ] Ordenar por custo medido a compilação de shaders, submissão de comandos, cópias/resolve e render targets. Comparar cache ou reaproveitamento de recursos compatíveis pelo maior custo. Verificar ordem de transparência, estado do dispositivo, óticas, pós-processamento e reset.
- [ ] Medir menu e segundo plano antes de experimentar teto configurável por contexto. Preservar o padrão da distribuição e a cadência de entrada, áudio e rede em `SleepWithUpdate`. Comparar consumo com FPS equivalente e, separadamente, o efeito do teto opcional.
- [ ] Se outro backend ou x64 aparecer como oportunidade, tratar como iniciativa independente depois da linha de base e da matriz funcional. Medir o ganho dessa mudança separado dos pilotos Rust.

## R1. Construir o piloto PBO isolado

**Arquivos.** Criar `rust/rust-toolchain.toml`, `rust/Cargo.toml`, `rust/Cargo.lock`, `rust/pbo_codec/src/lib.rs`, `rust/pbo_codec/tests/parity.rs`, `rust/pbo_codec/tests/abi_smoke.cpp` e `rust/pbo_codec/include/dayz_pbo_codec.h`. Criar o comparador fora do executável em `tools/pbo-codec-bench/`. Referências legadas em `El/QStream/ssCompress.cpp`, `El/QStream/qStream.hpp`, `El/QStream/qbStream.cpp` e `CheckBank/CheckBank.cpp`.

**Interface proposta.** Exportar `uint32_t dayz_pbo_abi_version(void)`, inicialmente com retorno 1, e `int32_t dayz_pbo_decode_v1(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len)`. A função escreve exatamente `dst_len` bytes quando retorna 0. Os buffers pertencem ao chamador, são válidos durante a chamada e não se sobrepõem. Comprimentos zero podem ter ponteiro nulo. A função só lê a entrada e só escreve na região de saída. Códigos propostos: 0 sucesso, 1 argumento inválido, 2 entrada truncada, 3 expansão além do destino, 4 checksum divergente e 5 falha interna. O chamador descarta a saída em qualquer erro. A integração qualifica os casos de entrada e saída de comprimento zero contra o comportamento legado.

- [ ] Confirmar no código e no corpus as regras de flags, janela de 4096 bytes, referências sobrepostas, checksum, bytes restantes e tamanhos zero. Documentar o comportamento aceito do formato. A variante legada em memória verifica alguns limites depois da leitura. Usar entradas válidas na comparação dessa variante até que ela esteja protegida, e exigir rejeição controlada nos casos malformados.
- [ ] Preparar manifesto com caminho, tamanho comprimido, tamanho esperado e hash da saída. Usar fixtures pequenas de literais, referência sobreposta, volta da janela, checksum, truncamento por posição e saída além do limite. Incluir arquivos reais e entradas antigas obtidas da distribuição qualificada.
- [x] Implementar o decodificador isolado, com Rust seguro no núcleo e conversão de ponteiros restrita à FFI. Validar nulos e tamanhos representáveis antes de criar slices, tratar retornos sem unwinding pela ABI e provar que o codec pode executar em chamadas independentes por worker.
- [x] Executar `cargo test --manifest-path rust/Cargo.toml --locked -p pbo_codec` quando a toolchain estiver qualificada. A suíte deve confirmar igualdade byte a byte, status esperado por fixture, preservação dos bytes sentinela fora do destino e leitura válida após uma falha.
- [ ] Rodar o comparador em modo otimizado com três variantes: C++ via `QIStream`, C++ memória para memória e Rust usando a interface acima. Registrar custo da chamada, tamanho/pico de buffer e throughput por faixa de entrada. Medir o núcleo e a operação inteira separadamente. Só encaminhar R2 se houver ganho candidato na carga relevante com paridade.

## R2. Integrar e qualificar por plataforma

**Arquivos.** `El/QStream/qbStream.cpp`, projetos existentes que compilam `El/QStream` e Makefiles da fundação. Criar `El/QStream/pboCodecBackend.hpp` e `El/QStream/pboCodecBackend.cpp` para a seleção e o carregamento. A interface consumida é a ABI v1 de R1.

- [ ] Qualificar primeiro `x86_64-pc-windows-msvc` para os processos x64 produzidos por `Retail` e `Retail Server`, validar carregamento pelo toolset VS2026 e piso de SO da distribuição. Linux i686 permanece experimento suplementar; qualificar outras arquiteturas somente se houver uma distribuição mantida para elas. Manter C++ nos alvos sem biblioteca compatível.
- [ ] Oferecer seleção de inicialização `cpp` ou `rust` pelo mecanismo de configuração existente. O carregador resolve apenas a biblioteca esperada em local controlado pela distribuição, checa a ABI e mantém o módulo carregado enquanto houver chamadas. Falha de disponibilidade ou de ABI seleciona C++ com diagnóstico único. Erro de decodificação após a seleção segue o contrato de erro da leitura.
- [ ] Conectar a chamada a uma entrada `CompMagic` de `QFBank::Read`, após leitura e antes de criar a vista da fatia. Preservar precedência de patches, os fluxos de outros formatos e o buffer do chamador. Medir qualquer cópia exigida para obter o bloco contíguo.
- [ ] Validar DLL ou biblioteca ausente, ABI de versão errada, arquitetura incompatível, entrada corrompida e vários workers. Verificar que a troca de backend não altera formato, assets nem a sequência de carga. Comparar partida e carregamento de missão nos dois backends.
- [ ] Aproveitar as mesmas capturas previstas em F2. Aprovar o módulo por sistema e arquitetura com ganho além da variação esperada na métrica alvo e paridade. Se o ganho se limitar à leitura de assets, registrar esse alcance. Distribuir primeiro com adoção explícita e reverter pela seleção `cpp` no próximo início, se necessário.

## R3. Piloto Rust para mipmaps PAA/PAC

**Arquivos.** `lib/pactext.cpp`, `lib/d3d9/txtD3D9.cpp`, `El/LZO/`. Após confirmar a variante selecionada, criar `rust/texture_codec/` e um comparador de mipmaps fora do motor.

- [ ] Selecionar a variante PAA ou PAC que explique a maior parcela combinada de `pacLZ` ou `pacD*` nos quadros longos. Registrar suporte de formato e pitch da variante, chave de seleção e referência C/C++ correspondente, inclusive LZO direto do stream quando aplicável.
- [ ] Definir interface C por mip completo com entrada e saída, comprimentos, formato, largura, altura e bytes por linha. Especificar unidades, valores de formato e limites a partir de `PacLevelMem` antes de congelar a ABI. O motor fornece o destino e coordena I/O, mip selecionado, upload e dispositivo.
- [ ] Comparar biblioteca atual, C++ com buffer temporário reaproveitado e Rust, com o mesmo resultado e contabilizando o custo de montar a entrada. Validar DXT suportados, alfa, dimensões mínimas, pitch com preenchimento, paleta quando aplicável e amostras truncadas.
- [ ] Integrar apenas a variante que passar. Usar caminho legado para as demais e opção de retorno por módulo. Medir p99 de quadro, bytes temporários, recarga e imagens nos cenários C1 e C2. Se o ganho offline sumir em runtime, manter a comparação como evidência e encerrar a integração.

## R4. Pilotos Rust para replicação, A* e visibilidade

**Arquivos de origem.** `lib/Network/networkServer.cpp`, `lib/AI/pathPlanner.cpp` e `lib/occlusion.cpp`. Criar um crate independente por candidato aprovado para piloto, em `rust/replication_rank/`, `rust/strategic_path/` ou `rust/visibility_batch/`. Executar cada candidato como revisão própria.

| Candidato | Fronteira e procedimento | Resultado esperado |
| --- | --- | --- |
| Replicação | Após S2, passar arrays compactos de índice e prioridade e receber índices em lote. Medir o preparo dos arrays. Repetir replays S2 com a mesma entrada e o mesmo algoritmo C++ e Rust. | Mesma sequência de envios sob o mesmo orçamento, inclusive empates, casos não finitos e caminhos críticos. Redução de CPU por tick em carga real. |
| A* estratégico | Após S3, passar snapshots de custos apenas se puderem ser preparados e invalidados sem regressão. Manter estado privado da busca e chamar uma vez por fatia. Definir criação, avanço, cancelamento e liberação no lado que alocou. | Mesma rota, desempate, parada e retomada sob os orçamentos comparados. Testar veículo destruído, alteração do mapa e buscas intercaladas. O perfil deve incluir montagem do snapshot. |
| Visibilidade | Após C3 ou um perfil equivalente, processar em lote volumes, parâmetros de transformação e dados necessários de oclusão. Devolver uma máscara. Comparar com C++ vetorizado com as mesmas instruções de CPU disponíveis. | Decisões iguais em caixas tangentes, plano próximo e casos limítrofes. Medir custo de extração e consumo da máscara e efeito no quadro completo. |

- [ ] Escolher o candidato de maior custo observado. Antes de implementar, registrar a ABI específica, layout e fixture de referência para esse candidato com os contratos da tabela.
- [ ] Comparar referência em C++, algoritmo ou layout melhorado em C++ e Rust com as mesmas entradas. Usar a política de integração e retorno de R2. Aprovar apenas após paridade e ganho de ponta a ponta no alvo.
- [ ] Registrar os demais candidatos como pendentes de medição ou descartados para a carga observada. Revisitar apenas com perfil que mostre mudança relevante de custo.

## E1. Validar a combinação e liberar por etapas

**Arquivos de entrega.** `Docs/desempenho/resultados.md`, manifesto de builds e capturas, instruções de configuração e reversão no roteiro de execução.

- [ ] Reexecutar a matriz F2 com cada revisão aprovada e com a combinação proposta. Cobrir protocolo entre builds compatíveis, entrada e reconexão, áudio e voz, opções de imagem e interface, física e IA, scripts, addons, missões e saves existentes. Registrar divergências e tolerâncias justificadas.
- [ ] Para cada alteração, registrar commit, alvo, perfil antes/depois, cenário, ganho, variação e custo de RAM, CPU ou latência em outros cenários. Registrar a sequência de reprodução e salvar os dados brutos internamente.
- [ ] Liberar primeiro em ambiente de homologação e habilitar por alvo e módulo. Repetir o cenário de reversão antes da adoção padrão. Retornar ao backend C++ ou à revisão anterior se houver divergência funcional, regressão além do orçamento ou falha de inicialização.
- [ ] Encerrar a trilha quando cada item tiver decisão registrada com evidência: adotado, mantido como opção, descartado na carga avaliada ou bloqueado por insumo de plataforma. O aceite técnico de cada alvo exige seu próprio binário e captura.

**Escopo posterior.** Renderer completo, Bullet, VM, transporte e agendador completo ficam fora da migração inicial. As etapas S3 e C3 cobrem melhorias nos fluxos existentes, e R4 cobre os núcleos isoláveis. Outros custos da avaliação, como UI, áudio e allocators, entram após essas trilhas se aparecerem nos perfis. Para cada um, abrir uma revisão focada na origem do custo e usar a mesma matriz de paridade. Uma migração integral exige paridade de todo o subsistema, um plano próprio e ganho medido.
