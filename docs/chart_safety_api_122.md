# Experimental chart-safety provider

This branch implements the provider side of OpenCPN's draft `HostApi122`
chart-safety contract. It is intended to be built and tested with the matching
OpenCPN API 1.22 development branch.

The provider development branch explicitly declares API 1.22 and pins the
matching provisional header from `pob220/opencpn-libs`. This avoids an
undeclared runtime dependency on `GetHostApi()` in binaries which claim to
support much older OpenCPN cores. Released o-charts branches are unchanged;
the provisional dependency is replaced by the official API package if the
contract is accepted.

The plugin registers its callback during `Init()` and unregisters it during
`DeInit()`. OpenCPN invokes the callback synchronously for an o-chart instance
and a bounded grid of active cells. The provider uses its in-memory licensed
chart representation and immutable spatial index to return only:

- semantic flags for land, drying areas and unknown-depth dangers;
- minimum charted depth where the source object supplies one; and
- a bitmap identifying the requested cells intersected by each semantic
  feature.

It does not return or persist decrypted chart geometry. The result explicitly
permits OpenCPN or a consumer plugin to persist the compact, identity-scoped
derived safety raster. Updating the installed chart set changes the host cache
identity and invalidates those derived tiles.

The old proof-of-concept exported-symbol protocol is deliberately removed.
Registration, callback ownership and automatic teardown are now managed by
the versioned host API.
