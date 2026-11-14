# DX11 nativo: estado do porte

## Objetivo

Fazer o cliente x64 usar Direct3D 11 como renderer nativo e, ao concluir a
paridade, retirar o caminho Direct3D 9 do cliente. Não basta trocar a API de
criação do dispositivo: todos os recursos de desenho e serviços gráficos
precisam ter implementação DX11 antes de remover o renderer antigo.

## Estado confirmado nesta source

- `lib/d3d11/` já contém dispositivo/swap chain, compilação HLSL, texturas,
  malhas, estados, alpha test, desenho TL e um `Dx11EngineCore` inicial.
- Foi adicionado `Dx11VolumeTexture` com criação de `Texture3D`, SRV, upload
  completo e atualização por região via `UpdateSubresource`; o arquivo entrou
  no alvo Retail x64 e compilou. É a base de recursos 3D para o porte do clima,
  ainda não é por si só o renderer de nuvens.
- Foi adicionada `TextureSysGPUDX11` em `SimulCommonDX11.*`: mantém buffer CPU
  com acesso protegido, offsets lineares, marcação dirty e upload preguiçoso ao
  volume D3D11. Os callbacks Simul do céu e das nuvens agora usam esse recurso.
- Foi adicionado `SimulSkyRendererDX11` em `SimulSkyRendererDX11.*`. Ele escolhe
  formato de volume float16/float32 com suporte D3D11, aloca os seis volumes de
  loss/inscatter e converte os callbacks de float4 do SimulWeather para o
  formato escolhido. Está ligado ao clima DX11 e compartilha esses volumes com
  o renderer de nuvens.
- Foi adicionado `SimulCloudRendererDX11` em `SimulCloudRendererDX11.*`: cria
  volumes de densidade/iluminação, compila VS/PS HLSL, envia a malha de nuvens,
  aplica blend/depth e amostra ruído, densidade, iluminação e volumes de fade.
  O shader foi compilado com FXC e a implementação foi compilada e linkada em
  Retail x64. A iluminação e o fog ainda são uma aproximação; falta validar
  visualmente céu e nuvens em uma sessão do jogo.
- `SimulWeatherDX11` agora substitui a fábrica Simul no `Dx11GameEngine` e
  conecta sky, clouds, loss, inscatter e fade. O carregamento de
  `noise_raw.paa` foi encaminhado pelo `Dx11TextBank` do engine, em vez de abrir
  a fonte de textura diretamente no renderer.
- A source também contém um renderer Simul para DirectX 11 em
  `extern/SimulWeather/Simul/Platform/Windows/DirectX 1x`; ele depende do
  Effects11/D3DX11 e de arquivos `.fxo` pré-compilados, que não estão presentes
  nessa árvore. Não foi ligado diretamente ao cliente; o porte próprio usará
  `D3DCompile`/shaders HLSL e o device/context do engine DX11.
- `lib/main.cpp` envia o cliente x64 diretamente para `CreateEngineD3D11`; a
  opção x64 de escolher D3D9 foi removida.
- O build x64 de `lib/lib2010.vcxproj` gerou
  `build/port-x64/dist-private-no-steam/DayZ.exe` em 2026-11-13.
- A compilação Retail x64 de `DayZ2010.sln` após a integração do clima terminou
  com 0 erros e 9.704 avisos e gerou `build/port-x64/dist/DayZ.exe`. Isso prova
  que o backend climático linka; não prova paridade visual nem execução completa
  do jogo.
- O executável abriu o device DX11 em feature level 11_0. Durante a carga de
  Chernarus, o RPT registrou `Cannot load material file physics/materials.h` e
  uma access violation; o endereço da falha mapeia para
  `GroundClutterCache::Calculate` em `landClutter.obj`. Não houve log indicando
  que `SimulWeatherDX11` tivesse sido inicializado, então a aparência do clima
  continua sem validação. Evidências em
  `build/port-x64/game-private-x64/profile-x64-weather-loader-check-30s/`.
- O caminho `Dx11GameEngine::DrawLine` deixou de ser no-op: agora forma uma
  faixa de 3 pixels a partir dos vértices TL e a envia ao `Dx11TLRenderer`;
  a compilação x64 passou, mas falta validar em runtime.
- `Dx11GameEngine::DrawDecal` foi conectado ao pipeline TL: cria o quad,
  preserva UV/rhw/cor e recorta a imagem nas bordas da viewport antes do draw.
  A compilação x64 mais recente linkou `DayZ.exe`; decals ainda precisam de
  validação visual no runtime.
- A rota comum `FaceArray::Draw` agora tem suporte DX11 para
  `PrepareMesh/BeginMesh/EndMesh`, `PrepareTriangle`, `DrawPolygon` e
  `DrawSection`: recebe os vértices TL já transformados/iluminados pelo motor,
  carrega a textura primária da seção e envia cada polígono ao renderer DX11.
  O build x64 linkou após essa mudança. Materiais multi-stage, shaders
  específicos, instancing e qualidade visual ainda não têm paridade.
- O cliente x64 já seleciona D3D11 diretamente em `lib/main.cpp`; isso ainda
  não significa que todo o engine foi convertido. O backend D3D9 continua
  compilado no projeto e tipos/caminhos antigos ainda aparecem em código
  compartilhado, `camera.cpp`, `landscape.cpp`, `shapeDraw.cpp`, clima e
  módulos Enfusion.
- Foram encontrados 30 arquivos-fonte com referências diretas a `EngineDD9`
  ou `GEngineDD`. O contrato `Engine` contém cerca de 60 métodos virtuais puros,
  além de serviços de renderização opcionais que também precisam ser
  implementados ou deliberadamente desativados no DX11.
- `lib/lib2010.vcxproj` já liga `d3d11.lib`, `dxgi.lib` e `d3dcompiler.lib` no
  alvo x64. As referências a D3D9 em outras configurações incluem alvos
  antigos/Xbox e não devem ser removidas indiscriminadamente.
- Um teste do cliente x64 iniciado sem opção de renderer criou o dispositivo
  DX11 em feature level 11_0 e continuou carregando assets (aprox. 428 MB após
  35 s); `d3d9.dll` não apareceu entre os módulos carregados nesse processo.
- O executável ainda importa `d3dx9_41.dll` para rotas legadas de compilação
  de shaders. Tentar excluir em bloco os seis arquivos `lib/d3d9/*.cpp` do
  alvo x64 não linka: restam 31 referências, incluindo post-process handles,
  renderização Enfusion e clima DX9. As exclusões foram desfeitas para manter
  o projeto buildável enquanto esses sistemas ganham implementações DX11.
- O módulo `SimulWeatherDX9` continua no projeto e intacto. A fábrica x64 usa o
  caminho DX11; a migração de funcionalidades ainda está incompleta e o build
  conserva dependências DX9 por compatibilidade enquanto o restante é portado.

## Pendências para DX11 nativo

1. Completar a implementação do contrato `Engine` no backend DX11, eliminando
   implementações vazias para desenho de mundo, geometria, materiais e clima.
2. Migrar o carregamento/gerência de texturas do `TextBankD3D9` para recursos
   D3D11 com mipmaps, formatos comprimidos, streaming e recuperação de device.
3. Portar o pipeline de materiais/shaders: iluminação, transparência, alpha
   test, fog, terrain, instancing, skinning, sombras e pós-processamento.
4. Migrar UI, sprites, linhas, vídeo/screenshot, resolução, janela, gamma e
   device lost/restore.
5. Remover dependências `EngineDD9`/`GEngineDD` compartilhadas pelo cliente e
   implementar substitutos DX11 neutros onde necessário.
6. Completar o sistema SimulWeather DX11: paridade de iluminação/fog,
   temporais, restauração de device, todos os estados e validação visual com
   assets reais. O desenho básico de nuvens e os volumes sky/cloud já estão
   ligados, mas ainda não são equivalentes completos ao renderer DX9.
7. Converter todos os demais subsistemas e dependências D3D9 referenciados pelo
   cliente x64; só então retirar fontes, headers, bibliotecas e imports D3D9 do
   alvo x64. O renderer DX11 já está selecionado no cliente x64, mas ainda não
   tem paridade nem validação suficiente para substituir integralmente o
   backend antigo.

## Critério de conclusão

Build x64 limpo com DX11 selecionado por padrão; nenhum import de runtime D3D9
no executável cliente; menu e cena 3D funcionais com assets reais; troca de
resolução/janela, device restore, texturas e materiais validados; servidor
dedicado continua sem dependência de renderer gráfico.

## Próxima fatia de implementação

O build x64 foi repetido após proteger o cache da `Dx11TextBank` contra objetos
`Texture` de outro tipo: agora `Load` valida a classe com `dynamic_cast`, e a
criação GPU rejeita assets incompatíveis em vez de reinterpretá-los. O build
terminou com código 0 e relinkou `build/port-x64/dist/DayZ.exe`.

No teste de inicialização, o dispositivo D3D11 foi criado em feature level
11_0, os addons/configurações foram carregados e o crash anterior em
`TextureSourcePacBase::Init` não se repetiu. A falha seguinte ocorreu ao abrir
a cena `intro`, dentro de `_aligned_msize` da CRT (`DayZ.RPT` aponta para
`file: intro`), com nova access violation. A leitura do contexto de exceção e
dos endereços de retorno do minidump localizou a pilha em
`PacLevelMem::GetPixelInt` / `DXTBlock128::GetColor4`, chamado por
`LayerMaskCacheItem::GetLayer` enquanto `GroundClutterPosMap::GenerateLocked`
é executado a partir de `GroundClutterCache::Calculate`. O ponteiro recebido
pela rotina de tamanho/alocação é `0x3f800000` (representação float de 1.0),
portanto a falha atual é uma liberação/tamanho de memória com ponteiro inválido
durante a amostragem da máscara de terreno. O startup avançou além do parser
PAC, mas a hipótese anterior de um objeto legado no cache não foi confirmada
como causa desse crash. Evidência: o RPT/minidump em
`build/port-x64/game-private-x64/profile-x64-texture-type-guard-check2/`.

Esse lançamento foi feito com o executável do build e o diretório de trabalho
`build/port-x64/game-private-x64`, que contém a estrutura completa do cliente
(Addons, dta, dll, Keys e BattlEye), para evitar conclusões de um pacote parcial.

O runtime também continua registrando `Cannot load material file
physics/materials.h`. Esse asset está ausente do pacote de teste e deve ser
localizado na árvore de dados/PBOs e incluído no runtime, mas o log sozinho não
prova que ele causou a access violation.

Próxima ação: rastrear a origem do ponteiro inválido ao amostrar a máscara DXT
no caminho de clutter/terreno e corrigir seu ciclo de vida/formato no caminho
DX11. Em seguida testar inicialização até o menu e uma cena 3D usando sempre a
estrutura completa do cliente. Depois, fechar paridade do clima, materiais/
terrain e dos outros serviços que continuam ligados a `EngineDD9`. Não declarar
o porte completo nem retirar dependências DX9 antes de passar pelo critério de
conclusão acima.

No teste seguinte, com diagnóstico RPT explícito e pacote completo novamente,
as duas máscaras do tile inicial foram identificadas e lidas com sucesso:
`dz\worlds\chernarusplus\data\layers\m_007_014_lca.paa` (512x512, formato
10, 262144 bytes) e `s_007_014_lco.paa` (512x512, formato 6, 131072 bytes).
`GetRect` retornou sucesso para ambas e os buffers tinham o tamanho esperado.
Mesmo assim, a access violation repetiu-se em `_aligned_msize`, com `RCX` do
contexto de exceção igual a `0x3f800000`; os endereços de retorno ainda passam
por `GetPixelInt`/`DXTBlock128::GetColor4` e o cálculo do clutter do terreno.
Isso afasta arquivo truncado como causa imediata e mantém em aberto uma
incompatibilidade de ownership/allocator ou corrupção no caminho de amostragem.
Evidência: `build/port-x64/game-private-x64/profile-x64-terrain-mask-rpt-check/`.

Atualização de 13/11/2026: com WARP e o pacote completo, o dispositivo D3D11
inicializa em feature level 11_0. Os testes, porém, ainda ficam pretos e falham
em pontos diferentes. A falha mais recente em
`profile-x64-mesh-lock-check` foi remapeada corretamente pelo `DayZ.map` para
`TextureSourcePacBase::Init` em `pactext.cpp` (offset `004ed91e`), e não para
`TextureSourceSimple::Init`, como registrado antes. O RPT mostra cinco
inicializações `TextureSourceSimple` com ponteiros e contagens aparentes
válidos antes da falha. O acesso é uma escrita inválida durante a inicialização
de textura PAC/PAA; ainda não há evidência suficiente para atribuí-la ao array
de mipmaps, a um arquivo de textura específico ou ao backend D3D11.

Foi adicionado diagnóstico de entrada e por mip em `TextureSourcePacBase::Init`
para registrar nome do recurso, endereço do array e índices, além de rejeitar
argumentos nulos ou contagens fora do limite do array local de offsets. O build
passou por pelo menos 128 texturas PAC/PAA do mapa e milhares de chamadas de
clipping com endereços de buffers válidos. A falha antiga de clipping não se
reproduziu com a proteção do ciclo `BeginMeshTL`/`EndMeshTL`, embora isso ainda
precise de confirmação em mais execuções.

O mesmo teste WARP terminou depois com access violation em `TListBidir::Move`,
instanciado no código de `ShapeBank` (`shapeDraw.obj`, endereço indicado no
RPT em `profile-x64-pac-init-diagnostic`). Portanto a tela preta segue sem
resolução; a falha PAC/PAA anterior também não foi reproduzida nesse run. O
próximo passo é usar o contexto do dump e os símbolos do map para identificar a
lista/ponteiro inválido em `ShapeBank::Move`, sem presumir que seja um defeito do
driver ou do carregador de texturas. Validar menu e cena 3D em hardware e WARP
antes de declarar o DX11 funcional.

Atualização posterior — 13/11/2026: o dump do teste em hardware confirmou que
a falha em `TListBidir::Move` escrevia no endereço nulo `0x8`. O crash acontecia
no deslocamento das listas do cache de geometria durante o desenho. Foi
adicionado um `CriticalSection` em `shapeDraw.cpp` para serializar a rotação por
frame com operações `Add`, `Delete` e leituras da lista `_geometryLoaded`, além
de uma guarda diagnóstica em `TListBidir::Move`. A recompilação Retail x64
terminou com sucesso. No novo teste pelo driver de hardware, o processo ficou
responsivo por mais de 60 segundos e chegou a mais de 400 mil chamadas de
clipping sem registrar a guarda nem repetir o crash. Isso sustenta a hipótese
de corrida e mostra que a sincronização melhorou a estabilidade, mas ainda
precisa de uma sessão mais longa.

A captura fornecida pelo usuário mostra saída visual parcial no menu, mas as
logos da Bohemia/DayZ e a tela de carregamento continuam ausentes. O avanço no
caminho 3D não comprova que o renderer 2D/UI, materiais e texturas estejam
completos. Estado atual: progresso confirmado, menu completo e sessão jogável
ainda não validados. Próximo: validar a renderização visual no hardware,
completar UI/materiais, repetir o teste prolongado e depois comparar hardware
com WARP. Não remover o backend DX9 antes da paridade necessária.

## Validação visual do cliente 0.45.124252 — 14/11/2026

- O build Retail x64 mais recente terminou com código 0 em
  `build/dx11-terrain-lod-x64/futura-renderer-build-attempt19.log` e gerou
  `build/port-x64/dist/DayZ.exe`.
- Resolvido o diálogo `Shaders not valid (mismatch of exe and data?)`. A causa
  observada era a leitura do cache por `QIFStreamB`: o caminho `bin\\` era
  tratado pelo sistema de PBO/bancos e retornava 0 bytes embora o arquivo solto
  estivesse presente. `ShaderCompileCacheDX11` agora lê esse artefato de disco
  com `QIFStream`; o diagnóstico registra caminho, bytes lidos e CWD se a leitura
  falhar novamente.
- `-generateshaders` produziu os três caches no pacote completo:
  `bin/Shaders_PS_DX11.shdc` (1.377.199 bytes),
  `bin/Shaders_VS_DX11.shdc` (14.771.717 bytes) e
  `bin/Shaders_PP_DX11.shdc` (185.684 bytes). A geração passou pela permutação
  térmica que falhava anteriormente.
- O lançamento Retail normal ficou ativo e carregou assets do mundo; o usuário
  confirmou visualmente que o terreno e os modelos 3D aparecem corretamente,
  sem o diálogo de cache. Esta é a primeira validação visual positiva do mundo
  3D com o renderer DX11 integrado.
- Ainda aparecem mensagens `DX11 Error: Not handled` vindas de conversões de
  estados em `lib/d3d11/StateLists.cpp`. Não impediram a cena validada, mas os
  fallbacks de blend/rasterizer/sampler/depth precisam ser rastreados para
  garantir paridade visual antes de considerar o renderer completo.
- O executável continua mantendo imports/caminhos legados D3D9/D3DX9 para
  componentes ainda não migrados. Esta validação não significa que DX9 foi
  removido nem que o porte inteiro está concluído.

Próxima etapa: identificar os estados DX9 que chegam às conversões DX11 sem
mapeamento, corrigir apenas os casos comprovados e validar novamente a cena;
depois continuar a auditoria de caminhos gráficos legados e remover DX9 só
quando não houver consumidores restantes.