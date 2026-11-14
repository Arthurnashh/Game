# Private multiplayer identity and authentication design

## Scope

This design is for a separately operated private server and clients that are
authorized to run this source. It replaces Steam matchmaking, Steam player IDs,
and Steam session tickets for that private network. It does **not** replace the
game's ownership/licence checks, imitate Steamworks, or enable connection to
official Steam/BattlEye services. Those remain outside this private auth layer.

## What the current source does

The current connection path has several distinct Steam dependencies:

1. `lib/main.cpp` initializes Steam (`InitSteam`, around line 10385) and the
   normal client startup treats failure to initialize it as fatal (around
   lines 12055–12064).
2. `lib/Network/networkClient.cpp` attaches the SteamID and an auth-session
   ticket to `PlayerIdentity` during connection (around lines 3255–3288).
3. `lib/Network/network.cpp` validates that ticket using
   `SteamGameServer()->BeginAuthSession` and callbacks (`SteamValidation`,
   around lines 2545–2637).
4. `lib/Network/networkServer.cpp` initializes Steam GameServer and publishes
   server metadata (around lines 2673–2827).
5. The SteamID is also passed to BattlEye GUID validation and Steam stats.
   Matchmaking, friends, favorites, overlay, and Steam Cloud are separate UI
   and convenience integrations; they are not the core game transport.

`PlayerIdentity::id` is already a separate string identity used by other
features. The private-server build also has `GetPrivateServerUID()` in
`networkClient.cpp`, but this alone is not authentication: a client-generated
or locally stored ID can be copied or spoofed. The protocol currently carries
`_steamID` and `_steamSessionTicket` as separate fields in `network.hpp`.

## Proposed private identity and auth flow

- Give each private-network account a server-issued random 128-bit account ID.
  Use that stable ID for saves, bans, and server-side player records; never use
  an IP address, hardware fingerprint, or client-chosen GUID as identity.
- Authenticate accounts through a small HTTPS auth service operated by the
  private server owner. Store password verifiers with Argon2id; never store
  passwords in the game-server database or send them in the game UDP protocol.
- After login, the auth service issues a short-lived signed session token
  containing account ID, audience/server ID, issue/expiry times, and a unique
  token ID. Sign with Ed25519; keep the signing private key only on the auth
  service and pin the public key in the private server configuration.
- The game client presents the token in the connection identity message. The
  private game server verifies signature, expiry, audience, and one-time token
  ID before admitting the player. Reject duplicates and expired tokens.
- Keep anti-cheat as a separate integration. Until a private BattlEye contract
  and supported identity format are confirmed, do not feed fabricated SteamID
  values into BattlEye callbacks; disable that integration only in an explicit
  private-server configuration or replace it with an independently authorized
  anti-cheat.
- Use the account ID for private persistence. Steam stats, friends, Steam
  browser listings, overlays, favorites, and Steam Cloud can be removed from
  the private build independently or replaced with private equivalents.

## Implementation sequence

1. Add an explicit `PRIVATE_NETWORK_AUTH` build configuration and keep it
   separate from official/release configurations.
2. Define a versioned auth-token field in the client/server handshake and
   update both ends together. Keep the field-size and protocol-version checks
   strict; do not silently reinterpret the existing SteamID/ticket fields.
3. Add HTTPS login/token acquisition in the client and signature verification
   in the private server. Make failed, expired, replayed, or wrong-server
   tokens reject the connection.
4. Replace persistence and private-server admin records with the server-issued
   account ID. Migrate existing records explicitly; do not derive IDs from a
   SteamID if the goal is an independent account system.
5. Remove Steam-only UI features from this configuration and provide direct
   address/LAN/private-list discovery. Retain the existing Steam integration
   in the official configuration.
6. Build and validate with two clients against the private server, including
   valid login, invalid password, expired token, token replay, duplicate
   account, disconnect/reconnect, and save/load identity continuity.

## Current status

This is an impact analysis and implementation design, not an implemented auth
system. The private client/server build settings now disable Steam and
BattlEye integrations on both targets so their network structures match. This
does not implement account authentication, replace anti-cheat, or change the
general Steam implementation. The handshake still crosses identity
serialization and persistence, so a successful runtime connection test is
required before publishing packages.
