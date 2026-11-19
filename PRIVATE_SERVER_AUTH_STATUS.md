# Private server identity

## x64 client and dedicated server

- The VS2026 solution builds \`Retail|x64\` and \`Retail Server|x64\` using
  \`DAYZ_PRIVATE_SERVER=1\`.
- Private builds use direct connections and a locally generated Windows GUID
  persisted in \`private-server-id.txt\`. The client sends this GUID as
  \`PlayerIdentity::id\`.
- Both targets disable BattlEye and the legacy CD-key registration flow.
- See [release assets](release-assets/README.md) for build candidate packaging
  and [the compatibility record](release-assets/COMPATIBILITY-TESTED.md) for
  end-to-end verification.

## Authentication boundary

The generated GUID is a persistent identifier, not proof of account ownership.
The private server should authenticate access with its server password and
server-side rules. A per-player account login, signed session token, and
administrative allowlist would be a separate feature if the project needs them.

RSA code and `rsa_x64.lib` remain in the wider source/build because cryptographic
signatures can also protect files and data. The legacy CD-key registration RSA
path is disabled in this private configuration; removing every cryptographic
primitive would require auditing those other consumers first.
