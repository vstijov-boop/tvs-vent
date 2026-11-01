# DEV side detail — results, 2026-11-01

Executable source: `25efc93801810218e8b5cb225e2dafe3a1b44fdf`.
CI checkout: `86c05d800ac26c3d8e13ab53ca3256253e03c2b4`, identical tree (GitHub compare: zero changed files). PR remains draft; this is not a production merge.

## Implementation commits

- `c80f020` — exact checked-body circular paths, static detail model, native DWG block/dimensions, insertion and identity contract.
- `9792c28` — slope dimensions outside section bounds; SDK setClosed/min/max compatibility.
- `b41b2c6` — isolated native fixture command with dimension and rollback verification.
- `25efc93` — explicit validated ASCII conversion for XRecord labels; v142 warning fixed without suppression.

Files: PreviewModel / new SideDetail model; checked geometry export in BypassNumericBridge; BypassPreviewCommand and new BypassSideDetail / BypassSideDetailTest; DEV registration, existing preview tests, DEV packaging/docs. No solver/generator, changeZ, DBX entity, offset V1 schema, specification or production workflow changes.

## Executed automated checks

- Local v143 strict-FP suite: 5 test methods, OK (36.281 s); targeted final side-detail model test: OK (20.322 s).
- [GCC full suite](https://github.com/vstijov-boop/tvs-vent/actions/runs/41311394248): 21 test methods, 32.208 s, OK.
- [Windows v141 / v142](https://github.com/vstijov-boop/tvs-vent/actions/runs/41311394225): 5 test methods each, 47.632 / 47.947 s, OK.
- Preview method: 23144 assertions per compiler. This includes the previous adapter/layout matrix and the new side-detail geometry matrix; it is not 23144 independent geometries or AutoCAD runtime cases. The new detail matrix uses mm/cm/m, both sides, 30/45/60, 0/25 insulation, -400/+12000 datum and both dimension levels. Checks include true bulge sweeps/radii, dimensional endpoints and physical/projected length distinction, local vs absolute datum, label validity and stale result refusal.
- Previous 452 Fraction, 70 checker, 30 FP environment and 274 generator checks remain included unchanged.

## DEV builds and binary provenance

- [AutoCAD 2020 build](https://github.com/vstijov-boop/tvs-vent/actions/runs/41311394318): build/link succeeded. [Package](https://github.com/vstijov-boop/tvs-vent/actions/runs/41311394318/artifacts/13085224156), ZIP 573334 bytes, SHA-256 `536e7b437e15582b6611f7907dd723faa4ecb6ef11bb44575d226ce58bff596f`.
- [AutoCAD 2022 build](https://github.com/vstijov-boop/tvs-vent/actions/runs/41311394219): build/link succeeded. [Package](https://github.com/vstijov-boop/tvs-vent/actions/runs/41311394219/artifacts/13086200276), ZIP 564398 bytes, SHA-256 `f76627de752f5c7254befd8e6147b3030d71f0a6e5463850c7edccbd0cee654e`.

Both ZIPs downloaded and hashes matched GitHub metadata. BUILD.txt targets and checkout checked. Both ARX/DBX pairs inspected as PE x64: 2020 ARX 1007104 / DBX 427008 bytes; 2022 ARX 955904 / DBX 449024 bytes. SDK tools and production packages were not changed. No compiler warnings suppressed to pass this change.

## Practical AutoCAD state — pending

AutoCAD 2022 actually opened through Computer Use in an isolated Drawing1 test session. Read-only loaded-module inspection showed no TVS ARX/DBX pair. Standard entity audit helper was loaded after the user handled AutoCAD's security prompt; initial result: `DETAIL_AUDIT refs=0 dimensions=0 polylines=0 mtext=0 unknown=0`.

Loading the new 2022 DBX reached AutoCAD's security prompt, which requires user action. At this point the native fixture, insertion, dimension-block display, rollback, Undo/Redo, COPY, save/reopen and visibility without TVS have **not** been executed. Compilation and numeric tests are not a substitute for those runtime checks. No runtime pass is claimed.

See [SIDE_DETAIL.md](SIDE_DETAIL.md) for the complete contract, commands, units, COPY limits and practical matrix. `inspect-detail.lsp` is a read-only standard-DWG audit usable without the plug-in.

