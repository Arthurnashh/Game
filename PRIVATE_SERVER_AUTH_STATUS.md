# Private server identity and Steam status

## Current x64 build

- `Retail|x64` defines `DAYZ_PRIVATE_SERVER=1` in `lib/lib2010.vcxproj`.
- `lib/retailConfig.h` uses that define to set `_ENABLE_STEAM`,
  `_ENABLE_STEAM_MATCHMAKING`, and `_ENABLE_STEAM_OWNERSHIP` to `0`.
- The x64 client now compiles without Steam-specific BattlEye GUID callback
  signatures. The callback declarations and forwarding methods are conditional
  on Steam ownership.
- The client uses a stable locally generated Windows GUID from
  `private-server-id.txt` as its private player identity. That ID is sent as
  `PlayerIdentity::id`; it is not a SteamID and is not compatible with official
  Steam or BattlEye identity services.
- `GetUserCertificate` and the RSA CD-key registration/challenge path are
  guarded by `_VERIFY_CLIENT_KEY`. Retail private-server configuration sets it
  to `0`, so this licensing registration flow is not used by the private build.
- `build/port-x64/dist-private-no-steam/DayZ.exe` built successfully with VS
  2017/v141. `dumpbin` confirms PE x64/ASLR and shows no `steam_api.dll`,
  Steam client, or Steamworks import. It still imports `d3d9.dll` and
  `d3dx9_41.dll`; this build is not the DX11 renderer port.

## Authentication boundary

The generated GUID is a persistent identifier, not proof of account ownership.
The private server should authenticate access with its server password and
server-side rules. A per-player account login, signed session token, and
administrative allowlist would be a separate feature if the project needs them.

RSA code and `rsa_x64.lib` remain in the wider source/build because cryptographic
signatures can also protect files and data. The legacy CD-key registration RSA
path is disabled in this private configuration; removing every cryptographic
primitive would require auditing those other consumers first.

## Build evidence

- Log: `build/port-x64/private-no-steam-parallel-build.log`
- Output: `build/port-x64/dist-private-no-steam/DayZ.exe`
