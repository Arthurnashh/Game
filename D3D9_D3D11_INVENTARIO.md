# Inventário D3D9 → D3D11

Inventário individual dos 42 arquivos existentes em `lib/d3d9/`. O estado do equivalente descreve o que existe na árvore DX11 em 13/11/2026; a presença de helper equivalente não significa integração completa no engine. A primeira implementação deste ciclo é um cache persistente de bytecode HLSL, ligado ao compilador DX11.

## Implementação nesta etapa

- `lib/d3d11/Dx11ShaderCache.hpp/.cpp`: cache binário v2 para bytecode compilado por `D3DCompile`, identificado pelo conteúdo do shader, macros ordenadas, source name, entry point, profile, flags e versão do compilador; valida header/tamanhos/hash antes de ler; grava em arquivo temporário e publica por rename atômico. Cache inválido ou indisponível resulta em recompilação normal. Um `ID3DInclude` arbitrário desativa o cache para evitar servir bytecode com includes desatualizados.
- `lib/d3d11/Dx11ShaderCompiler.hpp/.cpp`: compilação VS/PS aceita `D3D_SHADER_MACRO`, `ID3DInclude` e source name, além de compilar sem opções como antes. Falhas continuam retornando HRESULT e diagnostics do compilador.
- `lib/d3d11/Dx11TLRenderer.cpp`: os shaders de vértice e pixel do TL agora são criados via `Dx11ShaderCompiler`; o blob compilado de VS continua sendo passado a `CreateInputLayout`, preservando a geração do layout.
- `lib/d3d11/Dx11RenderTarget.hpp/.cpp`: cria um alvo de cor single-sample com RTV+SRV e depth-stencil opcional com DSV; oferece resize transacional, bind com viewport, limpeza e remoção explícita do mesmo recurso entre SRV e output-merger. Não contém passes FXAA/SMAA ou cadeia de pós-processamento; apenas fornece a infraestrutura de alvo.
- `lib/lib2010.vcxproj`: adiciona fonte/header ao alvo `Retail|x64`.
- Validação de build: o cache/compiler original compilaram isoladamente; após integração TL, `lib2010.vcxproj` compilou/linkou Retail x64 em `build/dx11-shader-cache-verify/DayZ.exe`. O `Dx11RenderTarget` passou em compilação isolada e no build Retail x64 completo e limpo em `build/dx11-render-target-verify/`; o build terminou com 0 erros e 9.689 avisos e criou `DayZ.exe`/`DayZ.map`/PDB. Depois, a extensão de macros/includes passou compilação x64 isolada e um harness WARP compilou: cache separado por macro, repetição com bytecode idêntico e compilação com `ID3DInclude`. O build completo ainda não foi repetido após essa extensão. Não foi executado teste visual do render target.
- Limite atual: `ID3DInclude` funciona, mas desativa a persistência do cache porque o conteúdo retornado pode mudar. Ainda faltam carregamento de shader via FileBank/LazyLoadFile, agrupamento por seções e compatibilidade `.shdc` de `D3DXCompileShader_Cached`; o backend D3D9 permanece compilado no alvo x64 e `d3dx9_41.dll` continua como dependência do executável.

## Fontes e cabeçalhos do backend

| Arquivo | Papel/API observado | Correspondente DX11 / estado |
|---|---|---|
| `d3d9Buffers.cpp` | Alocação, lock/unlock, atualização e descarte de buffers de vértice/índice DX9. | `Dx11Mesh` e buffers dinâmicos TL cobrem criação/submissão básica; falta a semântica completa de pools, DISCARD/NOOVERWRITE e consumidores do engine. |
| `d3d9Buffers.hpp` | Contratos e estruturas dos buffers DX9, formato/uso e helpers de upload. | Não há header de compatibilidade DX9→DX11; usar interfaces nativas `ID3D11Buffer` através do mesh DX11. |
| `d3d9Defs.hpp` | Definições e compatibilidade de formatos/estados D3D9 compartilhados pelo renderer. | `Dx11StateTranslator` e `Dx11Texture` cobrem subconjuntos; formatos/estados usados pelo engine precisam de mapeamento explícito. |
| `engdd9.cpp` | Implementação do renderer principal: device/presentation, estado, recursos, shaders, geometria, chamadas 2D/3D e serviços do engine. | `Dx11Device`, `Dx11Renderer`, `Dx11EngineCore` e helpers fazem apenas fatias. É o maior bloco sem paridade; há dependências compartilhadas e APIs de engine ainda faltantes. |
| `engdd9.hpp` | Declaração de `EngineDD9`, interfaces/estruturas DX9, buffers, constantes, estado de render e serviços. | O contrato-base `Engine` tem implementação DX11 inicial, ainda abstrata/incompleta; tipos e consumidores DX9 continuam existentes. Não remover antes de migrar dependências. |
| `engdd9Video.cpp` | Abertura/fechamento e avanço de quadros de vídeo no renderer D3D9. | Sem caminho equivalente de vídeo DX11 identificado. |
| `physHeap9.cpp` | Heap e contabilização de memória física/texturas, incluindo liberação/reutilização. | D3D11 gerencia alocação de recursos via device; não existe política equivalente de heap/LRU e contabilização da engine. Requer adaptação de política, não tradução literal. |
| `physHeap9.hpp` | Classes `PhysMemHeap9`, item de alocação e APIs Allocate/Free/Get de memória física/texturas. | Nenhuma API DX11 pública equivalente identificada; recursos devem ser criados/destruídos via `ID3D11Device` e memória monitorada separadamente. |
| `postProcess.cpp` | Efeitos, render targets, passes, queries e composição pós-processamento DX9. | Sem subsistema de pós-processamento DX11 completo. Render target/pass/shader chain ainda é lacuna. |
| `postProcess.hpp` | Tipos/contratos dos efeitos e handles, estados e recursos de pós-processamento. | Nenhum contrato DX11 completo; `PostProcessHndl` e consumidores em `Engine` ainda dependem do caminho legado. |
| `pushBuffer.cpp` | Gravação/reprodução e serialização de comandos de render, sincronização e submissão adiada. | D3D11 usa context imediato; não há tradução para deferred context/command list nem equivalente da gravação customizada. |
| `pushBuffer.hpp` | Contratos de command buffer, mensagens e tipos de replay/record usados pelo renderer. | Sem API DX11 correspondente. A migração precisa manter ordenação e lifetime de recursos antes de usar command lists. |
| `ShaderCompileCache.cpp` | Cache de shaders D3D9: valida fontes/seções, lê/grava bytecode e envolve `D3DXCompileShader`. | `Dx11ShaderCache` suporta macros e `ID3DInclude`; o cache persistente é usado sem handler de include. `ShaderCompileCache.obj` ainda é compilado/linkado em x64 e mantém imports D3DX9. FileBank, seções múltiplas e formato `.shdc` continuam sem equivalência DX11. |
| `ShaderCompileCache.h` | API `ShaderCompileCache`, `LazyLoadFile` e `D3DXCompileShader_Cached`. | `Dx11ShaderCache::Compile` e `Dx11ShaderCompiler` cobrem source em memória, macros e includes via API nativa; o contrato `LazyLoadFile` e o formato legado permanecem separados. |
| `txtD3D9.cpp` | Carregamento e gerenciamento de texturas DX9, mipmaps, surfaces, cache e uso do TextBank. | `Dx11Texture`, `Dx11VolumeTexture` e `Dx11TextBank` parciais; falta paridade de streaming/LRU, formatos e ligação completa ao material/render. |
| `txtd3d9.hpp` | `TextureD3D9`, `TextBankD3D9`, mip cache, surfaces, uso/lifetime e jobs de carregamento. | Implementações DX11 iniciais existem, mas assíncronismo/residência/vídeo/amostragem e integração total não estão completos. |

## Shaders e includes

| Arquivo | Papel do conteúdo | Correspondente DX11 / estado |
|---|---|---|
| `ShaderSources_3_0/common.h` | Declarações e macros compartilhados pelos shaders legados. | Precisa de revisão/conversão por perfil HLSL e layout de constantes; não há porte integral identificado. |
| `ShaderSources_3_0/FPShaders.hlsl` | Conjunto de pixel shaders de efeitos/funções do renderer legado. | Não há catálogo completo de pixel shaders DX11 equivalente. |
| `ShaderSources_3_0/Fxaa3_11.h` | Implementação/include do algoritmo FXAA 3.11. | FXAA não foi integrado ao pipeline DX11; shader requer adaptação de sintaxe/perfis e pass de pós-processamento. |
| `ShaderSources_3_0/FxaaShader.h` | Helpers/configuração do shader FXAA. | Sem equivalente ligado ao renderer DX11. |
| `ShaderSources_3_0/PPAA/AreaTex.h` | Tabela de área usada pelo SMAA. | Asset/tabela não está conectado ao DX11. |
| `ShaderSources_3_0/PPAA/SearchTex.h` | Tabela de busca usada pelo SMAA. | Asset/tabela não está conectado ao DX11. |
| `ShaderSources_3_0/PPAA/SMAA.h` | Funções/etapas do SMAA para anti-aliasing pós-processado. | Sem cadeia de passes SMAA DX11. |
| `ShaderSources_3_0/PS.h` | Código/declarações de pixel shader do renderer. | Não portado integralmente; materiais DX11 gerais estão pendentes. |
| `ShaderSources_3_0/PS.hlsl` | Implementação HLSL de pixel shader(s) do renderer. | HLSL DX11 existente é parcial e não substitui esse conjunto. |
| `ShaderSources_3_0/PSBasic.hlsl` | Pixel shader básico/fallback. | Há pixel shader básico em helpers DX11, mas equivalência de inputs/material não foi validada. |
| `ShaderSources_3_0/PSBasicSSSM.hlsl` | Pixel shader básico com caminho SSSM/sombreamento associado. | Sem porte equivalente confirmado. |
| `ShaderSources_3_0/PSSR_Default.hlsl` | Variante de pixel shader para caminho SSR padrão. | SSR e shader variante ainda não estão no pipeline DX11. |
| `ShaderSources_3_0/PSSR_nVidia.hlsl` | Variante SSR específica para NVIDIA. | Sem equivalente DX11; precisa decidir se mantém variante específica ou substitui por caminho comum validado. |
| `ShaderSources_3_0/PSThermal.hlsl` | Pixel shader de visão térmica. | Sem efeito térmico DX11 integrado. |
| `ShaderSources_3_0/psvs.h` | Definições/declarações compartilhadas VS/PS. | Contrato de constantes/semânticas tem de ser mapeado para layouts e constant buffers DX11. |
| `ShaderSources_3_0/VS.h` | Declarações e constantes de vertex shader. | `Dx11ConstantBuffer` é infraestrutura parcial; layout legado não foi convertido por completo. |
| `ShaderSources_3_0/VS.hlsl` | HLSL de vertex shader do renderer. | Shaders DX11 atuais cobrem alguns caminhos auxiliares; não há paridade de materiais/terreno. |
| `ShaderSources_3_0/VSCalmWater.hlsl` | Vertex shader de água calma. | Sem shader/renderer DX11 de água equivalente confirmado. |
| `ShaderSources_3_0/VSPoint.hlsl` | Vertex shader para primitivas/pontos. | Sem caminho específico portado identificado. |
| `ShaderSources_3_0/VSShadowVolume.hlsl` | Geometria/shader de volumes de sombra. | Sombras/volumes DX11 ainda ausentes do porte. |
| `ShaderSources_3_0/VSShore.hlsl` | Vertex shader da faixa costeira/shore. | Sem equivalente DX11 de shore. |
| `ShaderSources_3_0/VSSimulWeather.hlsl` | Vertex shader do sistema SimulWeather. | Há renderer/clima DX11 próprio e shaders HLSL, ainda com aproximações e validação visual pendente. |
| `ShaderSources_3_0/VSSprite.hlsl` | Vertex shader de sprites. | TL renderer DX11 cobre submissão transformada básica, sem paridade total de sprites/material. |
| `ShaderSources_3_0/VSTerrain.hlsl` | Vertex shader do terreno. | Terrain/material DX11 ainda incompleto; não há porte integral. |
| `ShaderSources_3_0/VSTerrainGrass.hlsl` | Vertex shader de grama/vegetação do terreno. | Grama e material de terreno ainda não portados integralmente. |
| `ShaderSources_3_0/VSWater.hlsl` | Vertex shader de água. | Água DX11 e seu conjunto de materiais ainda não estão em paridade. |

## Próximos subsistemas

1. Validar o cache DX11 em build Retail x64 e, quando executado, confirmar miss→compile→gravação e hit→leitura; confirmar invalidação por mudança de source/entry/profile/flags.
2. Migrar as variantes/macros e includes do compilador/cache D3D9 para uma API DX11 que aceite `D3D_SHADER_MACRO` e `ID3DInclude`/resolver de includes.
3. Implementar cache de estados DX11 a partir de `Dx11StateTranslator`, sem alterar comportamento legado até os estados terem paridade.
4. Definir render targets/pass chain DX11 para post-process e migrar efeitos individualmente (FXAA/SMAA primeiro são candidatos isoláveis).
5. Só depois integrar buffers/command recording, texturas/streaming e os materiais de terreno/água/sombra; manter DX9 no alvo até dependências e cenas terem cobertura comprovada.

## Limitações da análise

O inventário inclui cada arquivo encontrado por enumeração recursiva da pasta. Para os shaders, o papel foi inferido pelo nome e estrutura do conjunto; uma auditoria semântica linha a linha de cada shader ainda será necessária antes de converter ou excluir qualquer um. O backend D3D9 permanece intacto.

## Dependências de integração pendentes

- `lib/main.cpp:97` ainda inclui `d3d9/engdd9.hpp` sem guarda de plataforma, apesar da criação do renderer D3D11 sob `_WIN64`. Não remover a inclusão antes de rastrear os tipos/declarações compartilhados que ela fornece e validar o build x64.
- O build limpo `Retail|x64` produziu objetos para `d3d9/d3d9Buffers.cpp`, `d3d9/engdd9.cpp`, `d3d9/engdd9Video.cpp`, `d3d9/postProcess.cpp`, `d3d9/ShaderCompileCache.cpp` e `d3d9/txtD3D9.cpp`. O mapa `build/dx11-render-target-verify/DayZ.map` contém símbolos `EngineDD9`/post-process de `engdd9.obj`, portanto esse backend legado também entrou no link. `physHeap9.cpp` e `pushBuffer.cpp` não aparecem na lista de `ClCompile` do projeto nem produziram objetos neste alvo; isso não remove os tipos/consumidores que ainda dependem dessas interfaces.
- A tabela de imports do executável validado contém `d3d11.dll` (`D3D11CreateDeviceAndSwapChain`), `D3DCOMPILER_47.dll` (`D3DCompile`) e `d3dx9_41.dll`; não contém `d3d9.dll` como import direto. Isso confirma que o x64 inicia no caminho DX11, mas ainda inclui código `EngineDD9` e uma dependência D3DX9.
- `postProcess.cpp/.hpp` permanecem sem porte dos efeitos, pass graph, queries e transições de recursos. O novo `Dx11RenderTarget` não implementa FXAA, SMAA, SSR, térmico nem integrações com `Engine`.
- `Dx11RenderTarget` compila no x64, mas a combinação de estados/contextos e renderização visual em runtime ainda requer smoke test com um caller real do renderer.
- `d3d9Buffers.cpp` compila no x64 e ainda implementa criação/lock/unlock do `IDirect3DVertexBuffer9` via `GEngineDD->GetDirect3DDevice()`, incluindo flags/pools D3D9. `Dx11Mesh` é uma primitiva nativa separada; o wrapper/cache de user vertex buffers ainda não foi migrado nem conectado a todos os chamadores. `ShaderCompileCache.cpp` também compila e conserva `D3DXCompileShader`; o novo cache DX11 cobre só o fluxo usado por `Dx11ShaderCompiler`, não elimina o legado.
- Próxima fatia recomendada: substituir os user/dynamic vertex buffers para o caminho `Dx11Mesh` com estratégia de upload equivalente, mapear todos os chamadores e proteger o caminho 32-bit/legado antes de excluir qualquer fonte D3D9 do x64. Em paralelo, investigar a inclusão e símbolos de `engdd9.cpp`; só remover quando cada serviço tiver substituto e o mapa/import table/build confirmarem a remoção segura.
