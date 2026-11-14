# DayZ 0.45 x64 / Direct3D 11 port status

This file records the current renderer migration state from source and binary
inspection. It is a work checklist, not a claim that the game is already using
Direct3D 11.

## Current verified state

- `lib/d3d11/` contains standalone D3D11 device, shader, mesh, texture, state,
  constant-buffer, alpha-test, and frame-rendering helpers.
- `lib/lib2010.vcxproj` compiles those helpers for `Retail|x64` and links the
  Windows SDK D3D11, DXGI, and shader compiler libraries.
- The x64 `lib2010.vcxproj` build completed and produced
  `build/port-x64/dist/DayZ.exe` (verified in
  `build/port-x64/x64-dx11-alpha-test-build.log`).
- The produced executable still imports `d3d9.dll` and `d3dx9_41.dll`.
- `dumpbin` confirms the produced executable is PE x64 (`8664`), but its
  import table contains `d3d9.dll` and `d3dx9_41.dll` and no D3D11/DXGI imports.
- `lib/main.cpp` still selects `CreateEngineD3D9()` when `_ENABLE_DX10` is 0.
- An incomplete, dormant D3D10 renderer is present as `EngineDDT` in
  `lib/d3dT/engddT.hpp` and `lib/d3dT/engddT.cpp`. It derives from `Engine`,
  but it is not D3D11; it is not included in `lib/lib2010.vcxproj`,
  `_ENABLE_DX10` is 0, and it contains 55 explicit `Fail("DX10 TODO")` sites.
- `EngineDDT` uses real D3D10 interfaces (`ID3D10Device`, `ID3D10Texture2D`,
  etc.). Names such as `IDirect3DVertexBuffer9Old` are typedefs to D3D10 types,
  not compatibility wrappers around D3D9 or D3D11. It can inform a port of
  game rendering behavior, but cannot be selected as a ready D3D11 renderer.
- `Engine` has roughly 30 pure virtual requirements, including the full 2D/3D
  submission path and `TextBank()`. A new class with only swap-chain methods
  would not start the game correctly. The practical source for that contract
  is the existing `EngineDDT` material/shader/mesh flow, ported from its
  D3D10 device/context and resource types to D3D11; this is the next renderer
  implementation task.
- Source-level port boundary: `EngineDDT` uses `ID3D10Device` both to create
  resources and to issue immediate rendering commands (`IASet*`, `VS/PSSet*`,
  `DrawIndexed`, clear and query operations). D3D11 splits these responsibilities
  between `ID3D11Device` and `ID3D11DeviceContext`, so a type rename would be
  invalid. `Dx11Device` already owns a D3D11 device/context/swap chain; next work
  must migrate the engine's resource creation and draw/state submission to
  those two interfaces while keeping the material/shader contract intact.
- Removed the D3D9 SDK header from `lib/d3dT/d3dDefsT.hpp` and deleted two
  unused D3D9 vertex-type macros from `lib/d3dT/engddT.cpp`. The remaining
  D3D9 vertex declaration mentions found in this backend are in commented code.
- `Dx11Mesh` now optionally creates dynamic vertex/index buffers and can replace
  their complete contents per frame with `WRITE_DISCARD`; it rejects partial
  updates because the discarded remainder would be undefined.
- `Dx11Texture` now uses `D3D11_USAGE_DEFAULT` and exposes `UpdateMip()` for
  streaming mip data into an existing 2D texture with `UpdateSubresource`.
- `Dx11Texture` maps PAC AI88, RGB565, ARGB1555/4444/8888 and DXT1-5 formats to
  DXGI formats, computes overflow-checked row pitches, and reports P8 as
  unsupported for direct upload because its palette must first be expanded.
  The x64 Retail target rebuilt successfully after this change; see
  `build/port-x64/x64-dx11-pac-texture-format-build.log`. PAC conversion is
  still only a helper and is not yet wired into the game's `Texture`/text-bank
  loading path.
- Added `Dx11TLRenderer`, which accepts the engine's `TLVertex` layout, converts
  transformed screen coordinates and RHW into D3D11 clip space, expands convex
  triangle fans into indexed triangle lists, and streams vertices/indices via
  dynamic buffers. It supports optional `Dx11Texture` sampling and alpha blend;
  `Dx11Renderer` now owns and exposes this path as `DrawTLVertices`. Retail x64
  build passed in `build/port-x64/dx11-tl-geometry-build.log`. This is a usable
  2D/transformed-vertex submission component; `Engine`/`DrawPolyDo` still do not
  call it, and the game's material, texture-bank, and world-render paths remain
  unported.
- Added `Dx11ShapeMesh` as a geometry conversion layer from a loaded game
  `Shape` to static D3D11 vertex/index buffers. It copies position, normal and
  UV data, triangulates polygon faces, validates indices and buffer-size bounds,
  and retains index ranges per shape section for later material binding. Retail
  `Dx11Mesh::DrawIndexedRange`, `Dx11ShapeMesh::DrawSection` and
  `Dx11Renderer::DrawShapeSection` now submit a selected section with a caller-
  bound texture. Retail x64 build passed in
  `build/port-x64/dx11-shape-section-submit-build.log`. This conversion and
  submission still are not called by `Engine::PrepareMesh`/`DrawSectionTL`;
  animated, instanced and actual shape-material binding remain to be connected.
- The x64 Retail target keeps its legacy include order for the old audio and
  input sources, while the eight `d3d11/*.cpp` files put Windows SDK `shared`
  and `um` headers first at the item level. The full x64 build passes, and the
  DX11 translation units no longer emit the DXGI `C4005` redefinition warnings.
  Other legacy sources still include DXSDK headers and retain their own warnings.
- The x64 Retail build passed after these edits. The DX11 helper files are
  included in that target; the dormant D3D10 source files are not compiled.
- Built BulletCollision, BulletDynamics, and LinearMath as Release x64 with
  VS2017/v141 and SDK 10.0.19041. `dumpbin /headers` reports machine `8664
  (x64)` in each resulting library under `build/port-x64/deps/`. The solution
  file itself still has no x64 solution configuration, so these dependencies
  were built through their project files directly.
- Fixed pointer-width defects found in the x64 build: `FastAlloc`
  now uses `uintptr_t` for chunk-alignment masks and pointer-derived offsets,
  and `EventToCallback` now keys its handle map with `INT_PTR` instead of
  truncating 64-bit `HANDLE` values to `int`. `MemCommit::ExtendEnd/ExtendBeg`
  now keep page indexes and rounded addresses pointer-width. The UI object's
  sort comparator now compares full pointer-width addresses instead of
  subtracting truncated `int` values. The x64 linker now places the image at
  `0x140000000` and enables ASLR. The Retail x64 target rebuilt successfully
  (`x64-aslr-and-pointer-fixes-build.log`); `dumpbin` confirms PE machine
  `8664`, image base `0x140000000`, High Entropy VA and Dynamic Base. Other
  C4311/C4312 diagnostics in `svnAutoUpdateClient.cpp`, `imexhnd.cpp`,
  `mapFile.cpp`, and thread support still need review.
- The next handle-related 64-bit hazards were fixed as well: mini-critical
  section handles and the event-recycling pool now publish/store HANDLE values
  with pointer-sized Win32 interlocked operations instead of `long` or
  `AtomicInt`. The client rebuilt successfully in
  `build/port-x64/x64-interlocked-handle-width-build.log`; that build contains
  no C4311/C4312 warnings from `CriticalSection.cpp` or `activeObject.cpp`.
- Fixed `ThreadBase::Start` to keep `_beginthreadex`'s returned handle in
  `uintptr_t`; the old `unsigned long` local truncated the handle on x64 and
  used the wrong failure sentinel. Incremental x64 rebuild passed in
  `build/port-x64/x64-thread-handle-return-build.log`. `dumpbin` again confirms
  x64/ASLR properties. The unrelated C4267 size-to-int warning in that file
  remains for review.
- `SVNAutoUpdaterClient` no longer stores `_spawnl`'s process identifier in a
  `HANDLE`; it checks the `intptr_t` result, validates `OpenProcess`, checks
  both pipe-handle duplications, and closes local pipe ends after transfer.
  Its Retail x64 build passed in `x64-svn-handle-width-build.log`. The final
  executable remains x64/ASLR but still imports D3D9, so this ABI cleanup does
  not represent renderer migration.
- `MapFile` now stores physical/logical symbol addresses as `uintptr_t`, parses
  map addresses with `strtoull`, and uses pointer-width lookup APIs and
  arithmetic. Memory callstack symbol lookup and the relevant crash-report
  formatting no longer narrow these addresses to `int`. The Retail x64 build
  passed in `build/port-x64/mapfile-stacktrace-width-build.log`. The older
  Intel stack-walk implementation still carries `DWORD` stack/register fields
  and emits C4311/C4312 warnings; it is not yet a correct x64 unwinder.
- The x64 exception-report path now calls DbgHelp `StackWalk64` dynamically
  using the saved AMD64 `CONTEXT`, with 64-bit PC/frame/stack values and
  pointer-width `.map` symbol lookup. It prints up to 128 frames and loads
  `dbghelp.dll` dynamically, so this does not add a static DbgHelp import.
  The current-thread generic callstack methods now use dynamically resolved
  `RtlCaptureStackBackTrace`, avoiding the legacy zero-valued x64 `GetEIP`.
  Retail x64 build passed in `build/port-x64/x64-current-thread-capture-build.log`.
  Cross-thread capture now suspends the target, reads its AMD64 context, and
  uses DbgHelp `StackWalk64` to fill pointer-width `CallInfo` records; it then
  resumes the thread and releases DbgHelp. Retail x64 build passed in
  `build/port-x64/x64-thread-stackwalk-build.log`. Runtime/concurrency behavior
  still requires validation in a crash-reporting session.
- The shared `Engine` contract is in `lib/engine.hpp`. Its required pure
  virtuals span device lifecycle, frame presentation, 2D/3D drawing, mesh
  preparation, mode switching, gamma, and weather creation. A working renderer
  must implement the complete contract rather than only clear/present.

## Migration sequence

1. Keep the existing x64 build working while extracting API-independent engine
   state from `EngineDDT`.
2. Port the `EngineDDT` device/resource layer to D3D11: device/swap chain,
   textures, buffers, input layouts, shader reflection/bindings, queries, and
   depth/stencil/blend/rasterizer state translation.
3. Port scene submissions and the game's shader/material paths, including
   skinned and instanced meshes, shadow maps, post-processing, weather, and
   2D UI. The standalone `Dx11Renderer` is not a substitute for these paths.
4. Add `CreateEngineD3D11` and make it the only Windows x64 client factory in
   `lib/main.cpp`; remove the x64 call path to `CreateEngineD3D9`.
5. Port texture and simulation/weather consumers that currently include
   `lib/d3d9/engdd9.hpp`, then remove D3D9/D3DX9 headers, source files, libraries,
   and runtime dependencies from the x64 client target.
6. Build the entire x64 client and server targets; run the client through menu,
   map loading, representative world rendering, resize/minimize/restore, and
   shutdown.

## Completion checks

- The x64 client selects `CreateEngineD3D11` unconditionally and initializes a
  D3D11 feature level supported by the target machine.
- A loaded world renders terrain, objects, characters, vegetation, UI, shadows,
  and post-processing with their game materials.
- The final executable import table has no `d3d9.dll`, `d3dx9_*.dll`, or other
  Direct3D 9 runtime dependency.
- The x64 target's source and linker inputs contain no D3D9/D3DX9 runtime code
  or libraries.
- A clean x64 build and runtime smoke test pass after the old renderer is
  excluded.
- Texture-path audit: `TextureD3D9` owns the actual `ITextureSource` and mip metadata privately. `TextBankD3D9::UseMipmap` selects/streams levels, while `LoadLevelsJob` asks the source to decode each mip into a D3D9-selected destination format and pitch. The DX11 helper currently knows PAC/DXT-to-DXGI mappings, but it has no safe access to that private source, mip residency policy, palette expansion for P8, or engine texture/material lifetime. Calling `GetMipmapData` from an isolated helper would bypass the existing streaming and format negotiation, so no such speculative bridge was added. Correct integration belongs in a D3D11 texture-bank implementation of `AbstractTextBank`/`Texture`, with a shared or extracted source/mip-loading layer; doing only GPU upload is insufficient.
- Fresh-build renderer audit (2026-11-13): compiled Retail|x64 from a new, empty intermediate directory `build/port-x64/obj-lib-cleanrenderer/` into `build/port-x64/dist-cleanrenderer/`. Build passed, PE is x64/ASLR, but the clean link still contains `d3d9.dll` and `d3dx9_41.dll`; the map attributes engine symbols to `engdd9.obj`. This proves the DX9 dependency is present in the current Retail x64 source target, not merely a stale object from the prior shared intermediate directory. `lib2010.vcxproj` compiles `d3d9/engdd9.cpp` for Retail|x64; excluding it safely requires first routing the client factory and all dependent texture, video, weather, and post-process paths through a complete DX11 engine. The fresh executable therefore remains a DX9 runtime and must not be described or shipped as DX11.
- Backend-selection audit: `lib/d3dT/engddT.cpp` is an abandoned Direct3D 10 backend, not a ready DX11 renderer. It is guarded by `_ENABLE_DX10`, which the x64 Retail configuration inherits as `0` from `lib/retailConfig.h`; `lib/lib2010.vcxproj` does not list the `d3dT` implementation sources, and startup still routes to `CreateEngineD3D9` when the flag is off. The dormant source has 55 `Fail("DX10 TODO")` sites and hundreds of ID3D10/device-call references. Therefore enabling its old switch would neither select DX11 nor provide a complete runtime. Current path remains the D3D9 `EngineDD9` implementation. The correct next architecture step is a deliberate D3D11 engine implementing the shared `Engine` contract and shared resource/material loading; neither a macro flip nor the standalone helpers can replace that contract.

- The clean-link map also shows only incidental D3D11 object data from `Dx11TLRenderer.obj`/`Dx11ShapeMesh.obj`; the executable import table has no `d3d11.dll` or `dxgi.dll`. The helpers compile but are not reachable from the game's startup or render call graph. This is the exact integration gap: wire a game-facing `Engine` implementation to `Dx11Device` before expecting D3D11 imports or runtime selection.
- Added `Dx11EngineCore`, an intentionally abstract class derived from the game's `Engine`. It now owns the D3D11 renderer and implements actual device initialization, frame begin/present, selective color/depth/stencil clear, pause/restore, resize, dimensions, and fog-color storage. `Dx11Device::BeginFrame` now delegates to the same selective clear implementation. The source is built only for Retail|x64. Incremental x64 build passed (`build/port-x64/dx11-engine-core-integration-build.log`). It remains abstract because game mesh/material, TextBank, skinning, weather, gamma, and other `Engine` operations are not implemented, and `main.cpp` is not switched to it. This is a game-contract integration seam, not a usable game renderer or claim of DX11 runtime.
- Extended `Dx11EngineCore` with the shared `Engine` 2D polygon/line entry points. Untextured `Vertex2DAbs` and `Vertex2DPixel` polygons now use the game's `Clip2D` interpolation and the existing D3D11 TL vertex renderer; pixel-space offsets and packed colors are converted into `TLVertex`. Solid-color line calls create clipped 3-pixel quads. Textured draws are intentionally rejected until the DX11 text bank can resolve `MipInfo`; silently substituting a white texture would corrupt UI/material behavior. Retail x64 build passed in `build/port-x64/dx11-engine-core-2d-build.log`. This path is compiled but the abstract core remains unselected by `main.cpp`; the final executable still imports DX9.
- Added a direct DX11 PAC/PAA upload path: `Dx11Texture::InitializeFromSource` obtains a fresh `ITextureSource` through `SelectTextureSourceFactory`, initializes up to 12 mip descriptors, decodes each mip into a DXGI-supported destination format (legacy P8 is expanded to ARGB1555), and creates a D3D11 texture/sampler. `Dx11EngineCore::DrawPolyPrepare` now resolves and binds the `MipInfo` texture for its 2D draws. A bounded 32-entry LRU avoids re-decoding every polygon; entries are released when the device is paused/recreated. Retail x64 build passed (`build/port-x64/dx11-pac-paa-texture-path-build.log`). This does not yet implement the game's `AbstractTextBank`, async mip streaming, video textures, or 3D material binding, and the abstract engine is still not selected in `main.cpp`. The linked executable still imports D3D9; no runtime claim is made.
- Follow-up integration (2026-11-13): added `Dx11TextBank` and `Dx11TextureAsset` as a synchronous implementation of the `AbstractTextBank`/`Texture` interfaces, wired it into `Dx11EngineCore::TextBank()`, and changed GPU upload to consume the exact `ITextureSource` and mip descriptors owned by the game texture object instead of reopening the texture by filename. The Retail x64 build passed in `build/port-x64/dx11-textbank-source-bridge-build.log`. This closes the previous source-ownership gap for static/PAC-like textures, but this bank does not yet implement asynchronous residency, full cache/mipmap policies, video textures, CPU pixel sampling, or 3D material binding. The clean output remains x64 and still imports `d3d9.dll` and `d3dx9_41.dll`; there is still no D3D11 import because `main.cpp` does not construct this still-abstract engine core.
- World-section path (2026-11-13): `Dx11EngineCore` now advertises its TL path and connects `BeginInstanceTL`/`BeginMeshTL`/`DrawSectionTL` to the source `TLVertexTable`, using the camera transform, vertex-animation context, legacy lighting/projection routines, per-section bias, and stage-0 texture. Sections are submitted through the D3D11 transformed-vertex renderer with depth testing enabled. Retail x64 build passed in `build/port-x64/dx11-tl-world-bias-build.log`. This is an initial CPU-transformed path: it submits each polygon separately, drops faces that cross clip planes, and does not yet support GPU instancing, multi-stage material shaders, GPU skinning, shadow passes, or the complete Engine contract. `Dx11EngineCore` remains abstract and unselected; the produced executable still imports D3D9.
