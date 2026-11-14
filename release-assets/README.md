# Release assets

`common/` contains runtime files shared by both packages. Put client-only files
in `client/` and server-only files in `server/`; those directories are overlaid
on top of `common/` when a tagged release is packaged.

The current import from `E:\Dec` supplies `common/Addons`, `common/dll`, and
`common/dta`. It does not include the client/server-specific files needed to
prove either package complete. Add client `Keys` and the server's `BattlEye`,
`config`, `Keys`, and `MPMissions` directories before publishing.

The private client and server builds disable Steam and BattlEye identity
services; this does not add account authentication. A tag release is blocked
until `COMPATIBILITY-TESTED.md` records a successful client-to-server test.
Complete packages are split into sub-2-GiB ZIP parts to meet GitHub Releases'
per-file limit; download every part and concatenate them in numeric order to
recover each ZIP.

The imported common runtime files occupy about 6.46 GB. GitHub Actions fetches
these LFS objects only for a tagged release; check the repository's LFS
bandwidth quota before frequent releases.
