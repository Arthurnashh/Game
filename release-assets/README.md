# Private release assets

The release uses two independent runtime trees: `client/` and `server/`. There
is no shared/common overlay. Keep the large runtime files in Git LFS under the
matching role directory.

## Runtime contract

- Client: `Addons/`, `dta/`, `Keys/dayz.bikey`, candidate `DayZ.exe`, and the
  Microsoft-signed x64 `d3dx10_41.dll` / `d3dx11_41.dll` runtime files at the
  package root. `bin/` contains the three generated DX11 shader caches. There
  is no client `dll/` directory.
- Server: `Addons/`, `dta/`, `dll/` containing the four x86 allocator DLLs,
  `Keys/dayz.bikey`, `MPMissions/dayz_Auto.ChernarusPlus/`, and root
  `serverDZ.cfg`, plus candidate `DayZServer.exe` and its runtime DLLs at the
  package root.

Every PBO must have its matching `.pbo.dayz.bisign`. Both trees must contain
the public `Keys/dayz.bikey` whose SHA-256 is
`C1B027CCCC9136767AC11E66AEDB347030CAEB444502BF5CC300F17D7C544B22`.
Server startup selects the root config with `-config=serverDZ.cfg`;
`config/config.vdf` is not a server configuration file.

## Candidate and local packaging

Run the `Build and Release` workflow with **Run workflow** to build both
targets and create the untagged `dayz-candidate-<run ID>` artifact (kept for 30
days). Download and extract that artifact; it contains `client/`,
`server/`, and `candidate-metadata.json`, but not the large LFS runtime
payload. From a clean worktree checked out at `<candidate-revision>`, fetch the
LFS objects into the local cache and package either role with the shared
Windows PowerShell script:

```powershell
git lfs fetch --include='release-assets/client/**,release-assets/server/**' origin <candidate-revision>
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .github/scripts/package-release.ps1 `
  -Role client `
  -BinaryDirectory .\candidate\client `
  -StageDirectory .\out\client `
  -ManifestPath .\out\DayZ-Client-content-manifest.txt `
  -ArchivePath .\out\DayZ-Client.zip `
  -ChecksumPath .\out\DayZ-Client.parts.sha256
```

Use `-Role server`, `.\candidate\server`, and distinct output paths for the
server. The script resolves LFS pointers from `.git/lfs/objects` without
checking the payload out and requires hardlinks into the stage directory. Put
the stage directory on the same volume as the checkout/LFS cache so the full
payload is not copied. The script prints the content-manifest SHA-256. Extract
each generated split archive from its `.zip.001` file into a separate clean
test directory and run the compatibility test there, not in the staging
directory. The manifest uses sorted UTF-8/LF rows of
`SHA256<TAB>size<TAB>relative/path`; it excludes itself and is also placed in
the staged package. To generate the compatibility report's executable hashes,
run `Get-FileHash .\candidate\client\DayZ.exe -Algorithm SHA256` and the
equivalent command for `DayZServer.exe`. Use the script's returned hash for
each package manifest hash.

Run the small fixture check (no full runtime archive required):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .github/scripts/test-package-release.ps1
```

## Release and extraction

The tag workflow promotes the exact candidate binaries that were tested and
packages the role-specific LFS assets. Each package release includes numbered
split ZIP parts, its content manifest, and a SHA-256 checksum file for every
part. Download all parts for a role into one directory, verify them against
the checksum file, then use 7-Zip to extract from that role's `.zip.001` file.
Keep all numbered parts together; concatenating them is not required.

These builds are for private/LAN use only. Steam identity/runtime integration
and BattlEye anti-cheat are disabled, no account authentication exists, and
there is no anti-cheat enforcement. Do not expose the server as a public or
trusted service. A successful compatibility report is an operator-attested
record, not automated proof.
