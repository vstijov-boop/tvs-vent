# Redo isolation and interactive acceptance — preserved checkpoint

Priority change, 2026-11-01: investigation is deferred at the user's request.
It is unresolved and is not a blocker for further development planning. No new
Redo probes, reactor changes or additional Redo analysis are authorized in the
current planning step. The sections below preserve the chronological checkpoint;
later completed B/C results and startup restoration supersede earlier pending text.
Current status and outstanding checks: [integration backlog](../INTEGRATION_BACKLOG.md).

Source under review: bd01b2e (executable ee85e72). No functional changes made during initial inspection.

## Controlled runtime plan

A: new AutoCAD process, no manually loaded TVS pair; decline optional third-party startup scripts, inventory modules before the sequence. Ordinary interactive _LINE, finish LINE, _U, immediately _REDO. No LISP/audit/other command between U and REDO.
B: separate fresh process with matched 2022-r3 DEV DBX/ARX, before any DEV command repeat the same ordinary interactive sequence. Record all remaining third-party modules; do not claim TVS-only unless verified.
C: working copy of real source DWG, actual interactive checked candidate and one detail insertion; cancel second insertion during position selection, compare before/after state; Undo/Redo detail and repeat LINE. Save/reopen standard detail without TVS.

Pending user startup security choice: new process window 1575600 requested pg-spaces.lsp from ArtermoPodno.bundle. No security choice was automated. Existing session window 1772796 was left intact. No hot unload/reload performed.

## Source copy

Original: C:/Users/Win/Desktop/Ukrstanje kanala.dwg
SHA256 before: 6BD33BC1F2126A48A4DC40B5E88C354A453A2AC28EA3ECE688415F3A83848133
Working copy: DEV-side-detail/runtime/interactive-source-copy.dwg (outside repository). Original was copied, not opened for modification.

## Targeted static inspection (not runtime proof)

- OffsetIntegrationReactor.h: registered once in On_kInitAppMsg under TVS_OFFSET_INTEGRATION; removed only on unload. commandEnded calls clearClonePermits and afterUndoRedo; cancelled/failed only clearClonePermits.
- OffsetAssemblyData.cpp clearClonePermits: std::set clear plus in-memory revision increment. No database open, transaction or DWG write.
- OffsetSessionState.h afterUndoRedo: matches U/UNDO/REDO/MREDO. Checks existing in-memory history; opens stored TVS_Pipe IDs kForRead, updates globalElev/globalElevMid only. No startTransaction, kForWrite or initialization. Empty history returns immediately.
- endDeepClone -> remapClones: clone-specific write scope, reads source metadata; Legacy objects are skipped before destination kForWrite. Marked TVS clones can receive remapped metadata. This is a real write path, not disabled. No evidence yet that U/REDO invokes it in this reproduction.
- TVS_Entity, TVS_Pipe, TVS_TAP, TVS_TRANS, TVS_WYE persistent openedForModify/cancelled/modified/erased/modifyUndone/reappended/unappended callbacks: assertReadEnabled and delegate to AcDbCurve; no explicit additional transaction or write open in those bodies.
- TVS_Entity subOpen/subClose/subCancel/dwgInFields invalidate in-memory sheet-metal revision; base filing restores entity data through the supplied filer. No extra database object transaction is started there.
- PropertyAdmin initialization registers OPM COM properties on app/drawing load, not on commandEnded. Selection mass/read-only sheet callbacks open native entities kForRead, with memory caches. No explicit DWG write found in reviewed selection getters.
- MleaderController registerApp / TVSPropertyController registerApp perform regapp registration on document load. Mleader text setters and ordinary editing/grip connection paths do have kForWrite opens, but are not called by the reviewed command end or Undo callbacks.
- BypassSideDetail writer starts a transaction only inside confirmed write(), after interactive positioning confirmation. Preview and inventory are read-only/transient. No detail-specific Undo reactor is installed.

These findings neither identify the cause nor exonerate TVS. No reactor has been disabled and no speculative patch has been made. Runtime A/B/C remains required. Previous entmakex observations are not substituted for the requested interactive baseline.

## Executed A0 — fresh process with residual third-party environment

New process window 1575600. Security dialog for pg-spaces.lsp disappeared following the user handoff; its choice was not observed, so do not claim it was declined. ARX inventory before LINE contains no TVS module. It does contain geomprops2021x64.arx and selsim2021x64.arx, along with Autodesk modules. VLX inventory nil. Ribbon shows ARTERMO PG. Installed manifests configure ArtermoPg_024.dll, 2Duct.AutoCAD.dll and Kgh.Cad.dll to load at startup; manifest settings alone do not prove each managed assembly loaded.

Sequence actually executed through native command input and mouse: _LINE, pick two points, Enter to finish; _U; immediately _REDO. No LISP, audit or other command between U and REDO. LINE vanished after U. REDO output: Nothing to redo. Exact native transcript: DEV-side-detail/runtime/redo-isolation/A-command-log.txt. Screenshot: A-redo-failed.jpg.

This reproduces without the TVS ARX/DBX pair but does NOT establish a pure AutoCAD environment or identify a responsible component. Previous report's warning about resident reactors remains valid.

The first attempt to move the ArtermoPodno bundle failed because files were in use. Contrary to the initial report, this operation partially moved 14 files into ArtermoPodno.bundle.baseline-disabled. Reconnection revealed both folders. All 14 files were restored by copying only missing files back, with SHA-256 verification for every restored file and no overwriting. The original six remaining files were left in place. The partial backup is retained. The earlier statement that no restoration was necessary was incorrect.

## Executed A — three startup manifests excluded

After restoring the partial move, only PackageContents.xml was atomically renamed to PackageContents.xml.redo-test-disabled in ArtermoPodno.bundle, 2Duct.bundle and KGH-Ducts.bundle. Original paths and SHA-256 values were saved in runtime/redo-isolation/startup-manifest-state.json. These temporary names must be restored before finishing this investigation.

Fresh process window 69736 started with those three packages excluded and no TVS pair loaded. The ARX list was saved before LINE; TVS absent, remaining geomprops2021x64.arx / selsim2021x64.arx recorded. ModPlus UI remains. (vlr-reactors) returned nil; this inventories LISP reactors only, not native/.NET callbacks.

Exact sequence: _LINE, two mouse-picked points, Enter; _U; immediately _REDO. No intervening command, LISP or audit. **Passed:** line disappeared on U and reappeared on REDO; output Everything has been redone. Native log A-clean-command-log.txt and A-clean-redo-pass.jpg saved outside the repository under DEV-side-detail/runtime/redo-isolation.

This is a controlled environment difference, not yet identification of one responsible package. No TVS functional patch justified by this finding alone. B launched separately as window 70252 with the same startup exclusions; matched 2022-r3 DBX/ARX loading currently awaits manual security choices. No development command has been invoked in B.


## Confirmed B and C results — 2026-11-01 checkpoint

B completed in fresh process 70252 with the same three startup manifests excluded and the matched AutoCAD 2022-r3 TVS DBX/ARX loaded. Before any development command, interactive _LINE (two picked points, Enter), _U, immediate _REDO passed: Everything has been redone. The log inventories both TVS modules. Two Failed to register COM server messages occurred on load; the native detail command subsequently worked. This is not a verified TVS-only environment: geomprops2021x64.arx, selsim2021x64.arx and ModPlus UI remained. Evidence: B-command-log.txt, B-line-redo-pass.jpg.

C used interactive-source-copy.dwg, copied from the original. Real route 2A3 and obstacle 2A4 were selected through the native command. Candidate was checked, OB-01 was inserted at (1753.89, 876.284, 0), basic dimensions, physical text and arrow size 10 mm. Immediately _U followed by _REDO restored the detail with Everything has been redone. No diagnostic command was run between them. Evidence: C-checked-candidate.jpg, C-inserted-OB01.jpg, C-detail-undo.jpg, C-detail-redo-pass.jpg, C-command-log.txt.

A later read-only comparison returned SOURCE_HEADERS_UNCHANGED=T for the original two pipes. This compares exposed entget headers and XData, not the full custom native payload. The original source DWG SHA-256 was rechecked unchanged: 6BD33BC1F2126A48A4DC40B5E88C354A453A2AC28EA3ECE688415F3A83848133. Custom pipe COM inspection failed with Automation Error; full native payload invariance has not been proven by that comparison.

A second native insertion offered OB-02 and was cancelled at the placement-preview confirmation. The transient second detail disappeared visually; its log records *Cancel*. The planned post-cancel entity/symbol-table equality check has NOT been executed. The native log now also contains subsequent EXPLODE, zoom/pan and U commands after cancellation; do not use the present drawing as an untouched cancellation snapshot. Do not attribute those later actions to the controlled sequence.

Still outstanding: formal no-residue comparison after cancellation in a new controlled run; ordinary interactive LINE/U/REDO after detail insertion; saving and reopening this interactive detail without TVS; full source custom-payload comparison if required. The earlier 72-case matrix and earlier save/reopen tests do not replace these steps.

Conclusion: A0 failed with the residual startup environment; A and B passed with three startup bundles excluded; C detail Undo/Redo passed in that controlled environment. This narrows a reproducible environment difference but does not identify a specific bundle, reactor, interaction or TVS defect. No causal attribution and no functional fix are justified yet. No code changes, blanket reactor disabling, solver/generator changes, production release, hot unload/reload or AutoCAD 2020 runtime certification were performed. COPY duplicate snapshot identity remains a documented limitation.

Cleanup: all three PackageContents.xml startup manifests were restored to their exact original names and their recorded SHA-256 values verified. The earlier partial ArtermoPodno.bundle.baseline-disabled backup remains retained; no original bundle files were overwritten during recovery. Existing AutoCAD processes retain their already loaded modules until restart.
