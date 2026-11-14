# Status do port Direct3D 11 — DayZ 0.45.124252

**Data:** 14/11/2026  
**Estado:** backend D3D11 inicial funcional; port do renderer do mundo incompleto.

## Resumo

O cliente x64 já inicializa um dispositivo e uma swap chain D3D11, abre o menu e desenha a interface 2D. Também há submissão de meshes e materiais 3D no fundo do menu. Isso comprova que existe um caminho real de renderização D3D11, mas ainda não significa que o renderer do jogo foi portado por completo.

Na captura mais recente enviada pelo usuário, os quadrados brancos não aparecem, mas o cenário continua roxo/rosa e o terreno não está sendo exibido corretamente. A captura é do menu; o terreno, o personagem e uma sessão jogável ainda precisam de validação visual.

## O que já está implementado

- Cliente compilado para x64 em `build/dx11-material-color-fix-x64/DayZ.exe`.
- Inicialização do dispositivo D3D11, swap chain, render target e depth buffer.
- Caminho de apresentação com `Present` e tratamento básico de redimensionamento.
- Pipeline de shaders e buffers para desenho 2D e para meshes transformados pelo caminho TL do engine.
- Carregamento/cache de texturas e submissão de múltiplas texturas de material.
- Tradução parcial de estados de renderização, alpha test, transparência, depth e fog.
- Implementações parciais de alguns modos de material, incluindo árvores e materiais multi-stage.
- Correção recente para não tratar os stages SMDI de `PSMulti` como textura diffuse de detalhe.
- Suporte inicial às fontes de UV local, mundial e UV1; outras fontes especiais ainda precisam ser cobertas.
- Alteração em andamento no caminho de terreno: `DrawSectionTL` reaplica a seleção/mistura de alturas dos LODs de `VSTerrain`/`VSTerrainGrass` e o fade de altura da grama. A alteração está sendo compilada em diretório isolado; ainda não foi confirmada por build concluído nem por inspeção visual.

## Evidência disponível

- O build x64 foi concluído sem erros e gerou o executável acima.
- O RPT registra `feature level 0xb000` (D3D 11.0) e a inicialização do renderer em 800×600.
- O RPT instrumentado registra meshes, seções 3D, árvores e materiais sendo enviados ao renderer. Os diagnósticos estão limitados às primeiras chamadas, portanto esses números não representam toda a cena.
- O RPT dessa execução não registra falhas de `Draw` nas chamadas instrumentadas. Isso confirma submissão sem erro de API; não comprova que o resultado visual esteja correto.
- A imagem mais recente mostra o menu funcional e parte da vegetação/objetos, ainda com cor incorreta e sem o terreno adequado.

## Nova referência local: `D:\Futura_PhysX3\Futura_PhysX3`

A pasta contém uma implementação D3D11 de engine completa o bastante para ser a principal referência deste port, e não apenas shaders soltos. Em `lib/d3d11` há `EngineDD11 : Engine`, criação de dispositivo/swap chain (`engdd11.cpp`), buffers e estados, cache/compilação de shaders, backend de texturas, pós-processamento e fontes de shaders SM4.0. `ShaderSources_4_0` inclui `VSTerrain.hlsl`, `VSTerrainGrass.hlsl`, shaders de água/estradas/nuvens/sprites e shaders de pixel com variantes de terreno. O renderer seleciona `PSTerrainX` e as famílias de materiais de terreno, e implementa tanto `VSTerrain` quanto `VSTerrainGrass`.

O desenho parece compatível com a orientação do desenvolvedor: `EngineDD11` adapta os comandos e estados tradicionais do engine para objetos `ID3D11*`, preservando boa parte do contrato antigo. A semelhança por linhas com os headers do DayZ é alta: `engine.hpp` 91%, `scene.hpp` 88%, `landImpl.hpp` 92%, `tlVertex.hpp` 98%, `Shape/shape.hpp` 95%, `Shape/material.hpp` 97%, `camera.hpp` 96%. Isso é evidência de linhagem comum e torna a adaptação promissora, mas não prova compatibilidade binária ou compilação direta.

Há limites concretos para o reaproveitamento. O projeto de referência (`lib/lib2005.vcxproj`) só declara `Win32` e `Xbox 360`, enquanto o alvo atual é x64; será necessário criar a configuração x64 e validar tamanhos de ponteiro e dependências. O renderer inclui `D3DX11`/`D3DX10` do DirectX SDK antigo. A source DayZ já tem headers e bibliotecas x64 correspondentes em `extern/DXSDK`, então essa dependência existe localmente, embora as bibliotecas precisem ser adicionadas à configuração correta do projeto e os requisitos de runtime conferidos. Portanto, a pasta não é um drop-in x64.

As macros também têm papéis diferentes. `_ENABLE_DX11` é a chave estrutural usada para compilar e selecionar o renderer. Nesta cópia específica, `_ARMA3_FSAA_MODES` aparece no `normalConfig.h` e no código do renderer D3D9, mas não é referenciado em `lib/d3d11`; não é, portanto, requisito demonstrado para iniciar o backend D3D11. Caustics, refraction, radial post-process e a modificação de shader de terreno são recursos adicionais; cada macro só deve ser habilitada depois de confirmar que o código e os recursos correspondentes estão sendo portados. A implementação também contém alguns `Fail("DX11 TODO")` em funções de relatório/teste de textura e TODOs de pós-processamento; isso impede afirmar que toda função periférica está pronta, mas não apaga a existência do pipeline principal de mundo/terreno.

**Direção atualizada:** usar `D:\Futura_PhysX3\Futura_PhysX3\lib\d3d11` como referência principal para substituir gradualmente o renderer simplificado do DayZ, começando pelo caminho nativo de geometria/material/terreno e mantendo o contrato público de `Engine`. Portar em módulos e compilar x64 a cada integração; não copiar o backend inteiro às cegas. A morphagem CPU experimental registrada acima deve ser reavaliada quando o caminho `VSTerrain` completo estiver integrado.

### Integração em andamento

- Os 40 arquivos do backend e dos shaders SM4.0 foram adicionados a `lib/d3d11` sem substituir os arquivos `Dx11*` existentes.
- Os sete módulos C++ da referência estão registrados em `lib/lib2010.vcxproj` somente na configuração `Retail|x64`, com `_ENABLE_DX11=1` local a esses módulos.
- O build isolado está em andamento em `build/dx11-terrain-lod-x64`; até o último registro, os objetos comuns do engine compilavam com avisos preexistentes e ainda não havia erro reportado. O novo `EngineDD11` ainda não foi selecionado em `main.cpp`.
- A primeira incompatibilidade conhecida é o clima: a referência constrói `SimulWeatherDX11` para `EngineDD11`, mas a implementação atual do DayZ recebe `Dx11EngineCore*`. O renderer principal e o clima precisam ser integrados em etapas, sem mascarar essa diferença com um stub.

## O que ainda falta

1. **Terreno:** a morphagem de altura por LOD e o deslocamento/fade de grama têm uma primeira implementação CPU em `DrawSectionTL`, aguardando o resultado da compilação isolada. Isso trata apenas parte da geometria: falta o shader de pixel D3D11 e o pipeline completo de `PSTerrain`/`PSTerrainGrass`, incluindo camadas, mistura de texturas, detalhe, macro, normais, luz e fog. O caminho legado usa dados especiais (`VertexCustomInfo`, UV de satélite/máscara, normal/tangentes comprimidos, luz e distância); é necessário confirmar que a adaptação CPU preserva esses dados e as transições entre patches. `DrawSectionTL` ainda não mapeia `PSTerrainX`/`PSTerrainGrassX` para shaders D3D11 próprios. Portanto, a morphagem não equivale a terreno renderizado corretamente.
2. **Materiais e iluminação:** reproduzir a semântica dos shaders antigos em vez de usar aproximações. `PSTreeAdv` ainda não tem no D3D11 toda a iluminação, normais, translucência e parâmetros do shader legado; `PSMulti` também é parcial.
3. **Cor roxa/rosa:** causa exata ainda não isolada. A textura diffuse de alguns materiais de árvore chega pelo mip preparado, apesar de o stage 0 do material estar sem textura direta. Os probes de algumas árvores mostram cor de vértice branca; isso afasta essa entrada como causa nesses casos, mas não identifica sozinho a origem da dominante roxa.
4. **Conteúdo 3D completo:** validar personagem e animação, instâncias, objetos, água/céu, sombras, iluminação e transparências na cena de jogo.
5. **Paridade e estabilidade:** validar mapa e gameplay, corrigir artefatos, testar modos de tela/resolução e medir desempenho.
6. **Remoção do D3D9:** não foi feita. As partes D3D9 continuam na source; só devem ser removidas depois de a paridade D3D11 estar comprovada.

## Leitura honesta do avanço

Estamos na fase de **protótipo funcional do backend e integração inicial de materiais**. Já superamos a etapa em que o jogo só abria numa tela preta ou não desenhava nada, mas ainda estamos longe de um port completo do renderer. Não há uma porcentagem confiável: o menu e alguns meshes funcionam, enquanto subsistemas grandes — principalmente terreno, shaders de iluminação/material e validação do mundo — continuam incompletos.

## Próxima sequência técnica

1. Comparar `EngineDD11` da referência com a interface `Engine` e as estruturas de vértice/material da versão DayZ; mapear incompatibilidades específicas antes de integrar.
2. Integrar o caminho `EngineDD11`/D3D11 para geometria, materiais, buffers e estados na configuração x64 do DayZ, preservando a seleção do renderer e os recursos comuns do jogo.
3. Portar e compilar os shaders SM4.0 correspondentes, começando pelo terreno (`VSTerrain`, `VSTerrainGrass` e shaders de pixel de terreno); remover a morphagem CPU experimental se o caminho GPU a substituir.
4. Resolver as dependências do DirectX SDK antigo e obter um build x64 isolado sem regressões; validar menu, terreno, vegetação, objetos e personagem por RPT e capturas.
5. Portar pós-processamento, água/clima e recursos opcionais; só depois da paridade funcional revisar dependências e remover D3D9.

## Arquivos principais

- `lib/d3d11/Dx11EngineCore.cpp` — integração do engine e caminho principal de mesh/material.
- `lib/d3d11/Dx11TLRenderer.cpp` — shaders e submissão de geometria TL/material.
- `lib/d3d11/Dx11ShapeMesh.cpp` — malha D3D11 estática com posição, normal, UV e índices; ainda não substitui o caminho TL especializado do terreno.
- `lib/d3d11/Dx11Renderer.cpp` e `lib/d3d11/Dx11Device.cpp` — renderer e dispositivo D3D11.
- `lib/d3dT/engddT.cpp`, `lib/d3dT/engddT.hpp`, `lib/d3dT/ShaderSources` — referência histórica D3D10; útil para portar algoritmos, mas usa `ID3D10Device` e não substitui o trabalho D3D11.
- `lib/d3dT/ShaderSources/VSTerrain.hlsl`, `lib/d3dT/ShaderSources/PS.hlsl` e `lib/landImpl.hpp` — referência para entradas do terreno e organização das camadas.
- `build/port-x64/game-private-x64/profile-dx11-color-fix/DayZ.RPT` — log da execução citada nesta documentação.
