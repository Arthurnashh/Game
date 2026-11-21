# Estado de builds e testes — 21/11/2026

## Alvo de release do repositório

- A base desta execução é `50fa2a66609e3508d66a0f679f8dd7eb6e03ef86`. `Game.sln` usa Visual Studio 2026, toolset MSVC `v145`, SDK `10.0.26100.0` e plataforma x64.
- O workflow `.github/workflows/build-release.yml` compila as configurações `Retail` e `Retail Server` no runner `windows-2025-vs2026`, e coleta `DayZ.exe`, `DayZServer.exe` e DLLs.
- A execução `44626990961` em `main` passou no build de cliente e servidor para a revisão base. Como foi disparada por push, não produziu artefato candidato.
- O workflow agora também roda `cargo fmt`, 15 testes Rust, Clippy e build release do crate PBO em Windows x64. Para pull requests, publica `dayz-pr-candidate-<run_id>` com os binários da candidata. A validação desse novo fluxo ainda está pendente.
- O Linux local não tem Visual Studio/MSBuild/`link.exe`; o build oficial x64 deve ser validado pela Action, não inferido a partir dos testes Linux.

## Resultado local

As mudanças de fila e o crate isolado do codec SSCompress foram implementados sem remover arquivos, conteúdo ou comportamento do jogo. O helper de fila passa em debug, otimizado e com AddressSanitizer/UndefinedBehaviorSanitizer. O crate Rust passa em debug e release, e a ABI C foi chamada por executáveis C++ nos alvos x86_64 e i686 sob QEMU.

Ainda não há release final da candidata. Os testes locais não provam paridade de runtime nem ganho de desempenho no jogo. O formulário `release-assets/COMPATIBILITY-TESTED.md` permanece `Result: NOT RUN`; builds bem-sucedidos sozinhos não demonstram conexão, entrada em mundo jogável, desconexão e reconexão.

## Ambiente de validação local

- Linux x86_64, Debian 13, kernel `6.18.57`, GCC/G++ 14.2.0 e GNU Make 4.4.1.
- Rust `1.100.0`, instalado localmente sob `/tmp/dayz-rustup` e `/tmp/dayz-cargo`.
- Para validar Rust i686, bibliotecas Debian foram extraídas sob `/tmp/dayz-multilib` e QEMU sob `/tmp/dayz-qemu`; nada foi instalado no sistema. Esses testes não são benchmarks representativos.
- O ambiente não executa o cliente Direct3D 9 nem a sessão cliente-dedicado do Windows.

## Estado dos Makefiles Linux legados

Os comentários `//export` foram corrigidos para comentários GNU Make. `make -n -f Makefiles/Makefile server` avança até o caminho `RSA/lib`, ausente quando chamado da raiz; `make -n -C Makefiles server` para em `logger.c`, também ausente na árvore atual. O link mínimo `g++ -m32` não funciona com o runtime instalado. Esses Makefiles não são o fluxo de release x64 configurado na solução e não bloqueiam o build Windows oficial.

## Testes concluídos localmente

| Área | Verificação | Resultado |
| --- | --- | --- |
| Fila de rede | `networkQueueUtilsTest.cpp`, debug e `-O2`, C++11 | Passou |
| Fila de rede | Mesmo teste com `-fsanitize=address,undefined` | Passou |
| Codec Rust | 15 testes em debug e release, incluindo chamadas paralelas | Passou |
| Codec Rust | `cargo fmt --check` e Clippy com `-D warnings` | Passou |
| ABI Rust | `cargo check` para `x86_64-pc-windows-msvc`, `i686-unknown-linux-gnu` e `i686-pc-windows-msvc` | Passou |
| Codec Rust i686 | Biblioteca release e 15 testes debug/release sob QEMU | Passou |
| ABI Rust | Smoke test C++ contra biblioteca release x86_64 | Passou |
| ABI Rust i686 | Smoke test C++ contra biblioteca release i686 sob QEMU | Passou |
| Cliente/servidor x64 da revisão base | GitHub Actions `44626990961`, runner VS2026 | Passou |
| Candidata Windows x64 | Build, Rust e upload de artefato pelo PR | Pendente da Action candidata |
| Runtime cliente-servidor | Extração limpa, entrada na missão, rede e reconexão | Pendente |
| Desempenho do jogo | Captura F2 em cenários reais | Pendente; requer executar a candidata e a referência |
| Codec com assets reais | Igualdade de saída e comparador C++/Rust com corpus PBO | Pendente; sem corpus validado |

Comandos usados para validar Rust localmente:

```sh
cargo fmt --manifest-path rust/Cargo.toml --all -- --check
cargo test --manifest-path rust/Cargo.toml --locked -p pbo_codec
cargo test --manifest-path rust/Cargo.toml --locked --release -p pbo_codec
cargo clippy --manifest-path rust/Cargo.toml --locked --all-targets -- -D warnings
cargo check --manifest-path rust/Cargo.toml --locked --target x86_64-pc-windows-msvc -p pbo_codec
cargo check --manifest-path rust/Cargo.toml --locked --target i686-unknown-linux-gnu -p pbo_codec
cargo check --manifest-path rust/Cargo.toml --locked --target i686-pc-windows-msvc -p pbo_codec
```

Para compilar e executar o alvo i686 Linux nesta sessão, o linker wrapper e o runner QEMU foram criados em `/tmp` sobre pacotes extraídos, sem alterar o sistema:

```sh
CARGO_TARGET_I686_UNKNOWN_LINUX_GNU_LINKER=/tmp/dayz-i686-linker \
  cargo build --manifest-path rust/Cargo.toml --locked --release \
  --target i686-unknown-linux-gnu -p pbo_codec
CARGO_TARGET_I686_UNKNOWN_LINUX_GNU_LINKER=/tmp/dayz-i686-linker \
CARGO_TARGET_I686_UNKNOWN_LINUX_GNU_RUNNER=/tmp/dayz-run-i686 \
  cargo test --manifest-path rust/Cargo.toml --locked \
  --target i686-unknown-linux-gnu -p pbo_codec
```

## Próximos passos para releases

1. Passar a Action do PR e guardar o artefato candidato client/server x64.
2. Testar exatamente esse artefato em extrações limpas, preencher o formulário de compatibilidade com hashes e evidências reais e executar o cenário cliente-dedicado.
3. Capturar a linha de base e a candidata com cargas equivalentes; registrar p50/p95/p99 de quadros, tick, CPU e memória antes de adotar otimizações ou backend Rust.
4. Só então liberar a revisão de cliente e servidor. A publicação atual exige `Result: PASS` e valida que os hashes e manifestos correspondam ao candidato testado.

O codec segue como piloto isolado, sem integração ao carregamento do jogo e sem alegação de ganho. A otimização das filas segue sem aceite de runtime até comparar a sequência real de envios e medir carga representativa.
