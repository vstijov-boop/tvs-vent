# DEV side detail — results, 2026-11-01

Later checkpoint / priority change: controlled AutoCAD 2022 LINE and interactive
detail Undo/Redo passed with the documented startup exclusions; another startup
environment reproduced the failure. Cause remains unisolated and no individual
add-on is blamed. Investigation is deferred to final stabilization, not a blocker
for integration planning. No new Redo testing is requested now. The original
report below is historical; later results and incomplete interactive checks are
recorded in [REDO_INVESTIGATION.md](REDO_INVESTIGATION.md) and the authoritative
[backlog/checklist](../INTEGRATION_BACKLOG.md). Production is not approved.

Executable source: `ee85e72cce0e805991d172a2c34cdc4306530cb3`. Corrected packages: **r3**. PR #1 remains draft on `razvoj-bypass-candidate`; no production merge. CI checkout `a8c445b693930666021d1accc69a7d3db3ed318f` has the identical executable tree (GitHub compare: zero changed files).

## Commits and scope

- c80f020: checked-body circular paths, static detail model, native blocks/dimensions, insertion and identity.
- 9792c28: exterior slope dimension rails and SDK compatibility.
- b41b2c6 / 25efc93: native fixture, validated ASCII conversion without warning suppression.
- fc84871: inventory/fixture now cast the owned pointer after AcDbObjectPointer::acquire clears the raw source pointer.
- 8929e9d: local bounds start at zero instead of absolute s0; local-bound regressions added.
- ee85e72: native fixture accounts for measurement() already including DIMLFAC; independently verifies raw endpoint distance times unit factor. Wide-character error diagnostics fixed.

Changed: PreviewModel, SideDetail, checked geometry export in BypassNumericBridge, BypassPreviewCommand, BypassSideDetail/BypassSideDetailTest, DEV registration, tests and DEV packaging/docs. No solver/generator, changeZ, DBX entity, offset V1 schema, specification or production workflow changes.

## Automated checks

- Local v143 strict-FP: five methods passed; final targeted model test passed, 23432 assertions (15.239 s).
- [GCC full suite](https://github.com/vstijov-boop/tvs-vent/actions/runs/41314882886): 21 methods passed.
- [Windows v141/v142](https://github.com/vstijov-boop/tvs-vent/actions/runs/41314882838): five methods passed per compiler.
- The 23432 assertions include earlier preview checks; they are not independent AutoCAD runtime cases. New model matrix covers mm/cm/m, both sides, 30/45/60 degrees, insulation 0/25, datum -400/+12000, both dimension levels, bulge sweep/radii, physical/projected lengths, absolute/local coordinates, labels and stale-result refusal. Existing Fraction/checker/FP/generator tests remain included unchanged.

## DEV packages

- [AutoCAD 2020 build](https://github.com/vstijov-boop/tvs-vent/actions/runs/41314882785): build/link passed. [Artifact](https://github.com/vstijov-boop/tvs-vent/actions/runs/41314882785/artifacts/13086876763). ZIP 573652 bytes; SHA-256 `a052900928ac4bfd0c73a849250c99875087d8ce8a95d7d1f5e2d14af73a7357`.
- [AutoCAD 2022 build](https://github.com/vstijov-boop/tvs-vent/actions/runs/41314882941): build/link passed. [Artifact](https://github.com/vstijov-boop/tvs-vent/actions/runs/41314882941/artifacts/13087685323). ZIP 564619 bytes; SHA-256 `edf13fc7c3c3abc33dd35f42a4daf704cf4299e199cb2f0db1e1b75bd2102dad`.

Downloaded hashes matched metadata. PE x64 verified: 2020 ARX 1008128 / DBX 427008 bytes; 2022 ARX 956928 / DBX 449024 bytes. Load a matched pair in a freshly started test process. Earlier packages are superseded.

## Executed AutoCAD 2022 checks

The user handled DBX/ARX security prompts. Computer Use operated disposable drawings.

- **r3 native fixture passed repeatedly:** 72 synthetic checked cases (mm/cm/m, above/below, 30/45/60, insulation 0/25, basic/detailed), native dimension measurements, raw endpoints, DIMLFAC, dimension display blocks and metadata. Four injected failures left no additional references, block definitions, layers or styles. Case-insensitive duplicate label refused.
- r2 investigation independently read 169.235 raw cm against 1692.35 mm native measurement, and 36 cm against 360 mm. Writer was correct; old fixture multiplied DIMLFAC twice. r3 fixes the expectation and passes all units.
- **COPY:** a copied reference caused CHECK to explicitly report duplicate identity. No automatic new identity/renaming is implemented.
- **Save/reopen without TVS:** saved r2 standard snapshot reopened after restarting AutoCAD, before loading TVS. Read-only result: `TVS_MODULES=nil DETAIL_REFS=25`; geometry and native dimensions remained visible. r3 writer is unchanged from r2; only fixture expectation and diagnostics changed.
- **Undo:** one Undo of the fixture command removed the matrix; separate read-only count returned zero references.
- Final r3 matrix saved outside the repository as `DEV-side-detail/runtime/r3-native-72.dwg`. Mixed units in one diagnostic layout are intentional.

Local evidence in `DEV-side-detail/runtime/`: native-72-pass.jpg, cm-measurement-evidence.jpg, copy-duplicate-detected.jpg, reopened-without-tvs.jpg, undo-zero-refs.jpg, redo-baseline-failure.jpg, plus saved DWGs.

## Open acceptance issues

**Redo is NOT accepted.** Immediate Redo after Undo reported Nothing to redo, by command and Ctrl+Y. The same failure reproduced with an ordinary LINE created through entmakex, including in new empty Drawing2 before any detail command there. UNDOCTL was 53 in Drawing1. TVS and other installed add-ons remained resident, so the cause is not isolated. Do not attribute it to a particular add-on or claim clean-installation success. Retest ordinary LINE and a single interactive detail in a controlled clean environment. Autodesk documents writes in Undo callbacks as one possible source of lost redo history; that is only a diagnostic lead: https://blog.autodesk.io/nothing-to-redo-message-appears-straight-after-undoing-in-autocad-for-net-and-objectarx/

**Hot unload/reload is not validated.** Earlier repeatedly loaded session produced c0000027 on unloading; evidence arx-hot-unload-exception.jpg. Testing continued after restarting, with one matched r3 pair. Restart when changing DEV binaries. Cause remains unisolated.

Not executed: AutoCAD 2020 runtime; full interactive insertion/position/Cancel/Esc on real TVS objects; complete font/annotation-size/DPI matrix; original route/obstacle/specification/mass before-and-after checks. Synthetic writer tests do not substitute for these. User-confirmed v2 dialog does not validate the new insertion prompts.

This is a **DEV review candidate with open acceptance issues**, not a production release. See SIDE_DETAIL.md for instructions, XRecord contract, units, nonassociative dimensions and COPY limitations. inspect-detail.lsp is read-only and usable without TVS. Stop for review after delivery; no production merge.
