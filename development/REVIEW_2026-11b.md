# TVS_VENT – drugi, nezavisni pregled koda (novembar 2026, b)

Grana `razvoj-tkomad`, HEAD `66e1950`. Pregled čitanjem koda; ništa nije mijenjano, ništa nije pokretano u AutoCAD-u.
**[kod]** = vidi se u kodu, **[provjeriti]** = treba potvrditi u AutoCAD-u.

Napomene o osnovi:
- Commit `f9b0e08` ne postoji u klonu; ispravke su gledane kao `1f5bba7..HEAD` (commit prvog pregleda → HEAD).
- `python3 tests/run_all.py`: **52 testa, palo 0** (69 s).
- Obuhvat. Red po red: `commands/*.inl`, `acrxEntryPoint.cpp`, `SizeFollow.h`, `RunDialog.h`, `geometry/RunSketch.h`, `SizeFollowRules.h`, `TeeTypeDialog.h`, `TeePort.h`, `TeeNeighbours.h`, `DecimalText.h`, `TransformGuard.h`, `DrawingUnits.h`, `PortGrips.h`, `SpiroCatalog.h`, `HeatFloor.cpp`, `OffsetIntegrationCommand/Diagnostics/Reactor/SessionState/SelectionCheck.h`, `BypassService.h`, `TVSTestOffset.h`, `Func.cpp` (sve osim zakomentarisanog), `TVSCommand*.cpp`, `SPEC.cpp` (osim srednjeg dijela `add` za T komad/prelaz i `SpecWithAttrlist` ispisa).
  Samo tok i rizični obrasci (ne red po red): `TVS_TAP.cpp` crtanje 520–1836 i `DrawDirect`, `TVSMoveCommand.h` 640–859 (STRETCH), `MoveNetworkGeometry.h`, `AlignCommand.h` 1–150, `SectionCommand.h`, `ProfileCommand.h`, `OffsetService.h` 330–357, `OffsetRefit.h` 60–160, `BypassPreviewCommand.cpp` 1–395 (dijalog), `BypassSideDetail.h`, `geometry/bypass/*`, DBX `Offset/BypassAssemblyData.cpp`. Za njih presuda u tabeli (c) vrijedi samo za pročitani dio.

---

## (a) Provjera ispravki iz prvog pregleda

| Nalaz | Presuda | Gdje / napomena |
|---|---|---|
| K1 AttributesToProps | OK | `TVSCommandAttributesToProperties.cpp` – samo izabrani blokovi, potvrda, `acedSSFree`. Sitno: „Obrađeno blokova N“ broji i preskočene (zaključan sloj). |
| K2 konstantan radijus okruglog koljena | OK | `dgAllEdit.cpp:666` `%i` |
| K3 Excel nije dostupan | OK | `Specification.inl:188` |
| K4 rad sa zatvorenim objektima | **nepotpuno** | `reopen()` (`ConnectHelpers.inl:4`) provjerava upis, ali i sam čita `o->objectId()` kroz stari pokazivač; `Connect.inl:106/213` i dalje prosljeđuju zatvorene `pEnt1/pEnt2`, a `ConnectWithTap/Wye`, `addTrans`, `addtrans`, `nextpipe` čitaju `FirstPoint/SizeA/MA…` zatvorenih objekata; `Specification.inl:27–33` i `:106–112` (`pEnt->close()` pa `addBlock(pEnt)`); `SPEC.cpp:559–564`. Radi dok je objekat u memoriji – isto kao prije. Nije regresija. |
| `reopen()` – regresije | OK uz napomenu | Ako `reopen` ne uspije usred `ConnectWithWye` (`:446/451/461`) komanda izlazi sa već napravljenim T komadom (bez vraćanja). Kanali su prethodno otvoreni za upis u komandi, pa je scenario rijedak. |
| K5 Menadžer blokova, dug tekst | OK (dijalog) / **nepotpuno** (isti obrazac drugdje) | `dgBlockManager.cpp:135–142`. Ostalo: `SPEC.cpp:564` `wcscpy_s(pVal,512,…)` – atribut bloka ≥ 512 znakova ruši TVS_SPEC / TVS_EXCELSPEC / TVS_BLCKMNGR pri čitanju; `SPEC.cpp:608–609` (`sType+"-"+sSize` u 512); `MSExcel.cpp:976` (513). |
| K6 `=` umjesto `==` | OK | `Misc.inl:218–237` |
| K7 TVSSomeParts | OK | samo `TVS_DEV` |
| V1 razvojne komande | **nepotpuno** | U izdanju ostali: `TVS_HEATFLOOR` (`acrxEntryPoint.cpp:400`, vidi N6), `TVS_BYPASS_PREVIEW` (`:140`, grupa „TVS_BYPASS_DEV“, poruke „DEV:“), `TVS_OFFSET_POLICY`, `TVS_OFFSET_STATUS` (`:441–444`). `TVS_DEV` nije vezan ni za jedan MSBuild parametar/workflow – kod pod `TVS_DEV` se nikad ne kompajlira (truli neprimijećeno). |
| V2 MODAL | OK | `tests/test_release_commands.py` to čuva. `TVS_SETTINGS` ostaje TRANSPARENT (namjerno). |
| V3 Svojstva | **nepotpuno + regresija** | `Func.cpp:1134` čitanje `kForRead` OK; `:1316` Odustani OK; `:1397/1506` brojač OK. Nepotpuno: `func::Change(AcDbEntity*)` (dvoklik, `tdbl.cpp:76`) i dalje `dg.DoModal()` bez provjere (`Func.cpp:1827`) i bez poruke kad `upgradeOpen` ne uspije. Regresija: vidi **N1**. |
| V4 decimalni zarez | OK kao parser / **regresija** u Svojstvima | `DecimalText.h` ispravan. `Trim()` u `parse` mijenja ponašanje `DDX_MillimetreString` – **N1**. |
| V5 crtanje | OK | `Draw.inl:172–175, 214–217, 295–297`, `PipeTransWye.inl`, `connect2` (`DrawHelpers.inl:305–350`), 100 mm (`:642`), `SizeB` (`ConnectHelpers.inl:156, 322`). Ostaje: `Draw.inl:39–62` ne zatvara objekat ako nije kanal (teorijski); `nextpipe` tiho briše koljeno koje ne staje (`DrawHelpers.inl:262`). |
| V6 TVS_CUT | OK uz dopunu | `Edit.inl:251–302`. Ne prenosi podatke iz extension dictionary-ja: flex manžetne (`FlexCuffData`) – manžetna „kraj“ ostaje na prvom dijelu (na mjestu reza), novi dio je nema [kod]; skriveni rasponi (`TVS_HIDDEN_BY_V1`) ostaju samo na prvom dijelu. `setXData` kopira i poziciju/oznaku – oba dijela imaju istu poziciju (prihvatljivo, navesti u TESTIRANJE). |
| V7 TVS_ERASE / TVS_RAZBIJ | **nepotpuno** | `Edit.inl:362–426`: brisanje tek poslije uspješnog `explode` – OK. Nije urađeno: članovi etaža/obilaska se i dalje razbijaju pojedinačno (roditeljski zapis `TVS_BYPASS_<guid>` u NOD-u i V1 zapisi ostaju siročad). `postToDatabase` se zove poslije `erase()` – ako prostor ne može da se otvori, element je izgubljen (redoslijed obrnuti). |
| V8 TVS_DIM_PRATI (faza 2) | OK za navedeno | kota na spoju (`SizeFollow.h:715, 740`), svjež snimak (`:629`), po crtežu (`:76, 618`), limit 5000 (`:696`). Defekti dijaloga dionice: odjeljak **D**. |
| V9 SCALE / MIRROR | OK po dogovoru, ali vidi **N2** | `TransformGuard.h`; odbijanje vraća `eNotApplicable` za cijelu matricu (i translacioni dio). |
| V10 selekcije | OK | sve navedene komande oslobađaju selekciju. |
| V11 resource override | OK | `TVSCommandPropertyManager.cpp:33`, `TVSLeadCommand.cpp:124`, `TVSSpecTable.cpp:208`. `dgChangeZ` u `OffsetIntegrationCommand.h:72, 146` i dalje bez override-a (radi) [provjeriti]. |
| V12 AddAtrib / CheckAtt | **nepotpuno** | `Specification.inl` OK. Ostalo: `Misc.inl:119–121` `pBlkRef->close()` pa korišćenje + dvostruko zatvaranje u destruktoru `AcDbObjectPointer`; ista greška zagrada `_T("…%s", arg)` u `SPEC.cpp:570` i `:664` (`%s` bez argumenta – mogući pad na grani greške); `SPEC.cpp:546, 645` iterator atributa se ne briše. |
| S2 OnShowWindow | OK | `dgBlockManager.cpp:57`, `dgTVSPropertyManager.cpp:69` |
| S4 `setZ()` baca izuzetak | nije urađeno | `Edit.inl:483–486` (ne poziva se) |
| S5 nulta dužina | OK | `TVS_Pipe.cpp:739, 1098, 1114, 1472` |
| S7 CI testovi | OK | oba `build-autocad-*.yml`: posao `tests`, `build: needs: tests`. Samo `push` na `master`/`razvoj-tkomad`; `dev-*.yml` workflow-i bez testova. |
| TVS_DEV zaštita | OK | `acrxEntryPoint.cpp:417–454`; vidi (3) za `TVS_OFFSET_PROOF`. |

---

## (b) Novi nalazi

### Kritično

**N1. Svojstva (TVS_Change, dvoklik): OK bez ikakve izmjene mijenja ugao koljena, a u crtežu u metrima i dimenzije** [kod]
- `Func.cpp:2503` `strfil(double)`: necijeli broj se upisuje kao `"%.2f\n"` (dvije decimale + znak novog reda); `sequal` (`:2531`) poredi **tekst** prije/poslije.
- Ugao (stari kod, od originala): `Swectangle` je u radijanima → `"1.57\n"`; dijalog prikaže `89.9544°` (`dgAllEdit.cpp:318–321`), `OnBnClickedOk` (`:689–695`) vrati `"1.57"` ≠ `"1.57\n"` → `Func.cpp:1454 / 1952` `put_Swectangle(1.57)`.
  Posljedica: svako OK u Svojstvima nad koljenima istog ugla: 90° → 89,954°, 45° → 45,26°, 30° → 29,79°, 60° → 60,16°. Priključak koljena se pomjera za ≈ 0,0026·(R+A/2) kod 45° (1,5 mm za 600 mm), tj. preko tolerancija 0,1 mm (TVS_MOVE/ALIGN) i 1 mm (TVS_DIM_PRATI). To je vjerovatan uzrok „staje na koljenu“ i odbijanja „ose nisu tangentne“ na mrežama koje su mijenjane kroz Svojstva (uobičajen način: izaberi dionicu → Svojstva).
- Dimenzije (regresija faze 5): `DDX_MillimetreString` (`dgAllEdit.cpp:33–45`) sada koristi `tvsDecimal::parse`, koji radi `Trim()`. Ranije `"0.32\n"` nije prolazio parser i vraćao se nepromijenjen; sada se učita, prikaže kao mm i pri OK vrati kao `"0.32"` ≠ `"0.32\n"` → upis.
  Scenario (crtež u **m**): kanal Ø315 (SizeA = 0,315) → `"0.32\n"` → dijalog prikaže 320 → OK → `put_SizeA(0.32)`. Isto za Ø125 (→130), Ø355, kote (2,755 → 2,76), dužine prelaza, konstantni radijus, i sve to za sve izabrane elemente sa zajedničkom vrijednošću. U cm crtežu greška je ≤ 0,05 mm (bezopasno), u mm nema je.
- Popravka: `strfil` → `"%.15g"` bez `\n`; `sequal` poredi brojeve (`fabs(a-b) < 1e-9·max(1,|a|)`), a ne tekst; ugao čuvati kao `double` i pisati samo ako se prikazani stepeni promijene (`%.6g` stepeni, ne radijana); isto u `Change(AcDbEntity*)`. Test u `tests/` nad čistom funkcijom formatiranja/poređenja.
- [provjeriti] koljeno 45° → Svojstva → OK → Properties: ugao 45,26; m-crtež: Ø315 → Svojstva → OK → Ø320.

**N2. Kopiranje TVS elemenata između crteža sa različitim jedinicama (mm → cm): dimenzije ostaju „kao u mm“** [kod + provjeriti]
Uzrok:
1. `SizeA/SizeB`, radijusi, dužine fitinga, `Elev`, `WipeoutLength` i zapisi u extension dictionary-ju (`TeeData`: `l`, `second`, `radius2`; `HiddenSpan`) čuvaju se u **jedinicama crteža**. Faktor mm/jedinici je po crtežu (`TVS_DRAWING_UNITS` u NOD-u, `DrawingUnits.h:11`), nije u elementu i ne zavisi od `INSUNITS` (`JEDINICE_CRTEZA.md` izričito: konverzija pri COPY/PASTE nije implementirana).
2. Privremeni DWG clipboard-a nastaje WBLOCK-om izbora – zapis `TVS_DRAWING_UNITS` se ne prenosi (bez zapisa = mm).
3. Ako se `INSUNITS` izvora i cilja razlikuju, AutoCAD pri umetanju zove `transformBy` sa razmjerom 0,1; `TransformGuard` to odbija (`eNotApplicable`) – element ostaje sa starim brojevima (400 postaje 400 cm), a odbijen je i translacioni dio matrice [provjeriti gdje element završi]. Ako su `INSUNITS` isti/nepostavljeni, AutoCAD ne skalira ništa i brojevi se opet prenose doslovno. U oba slučaja 400 mm postaje 4000 mm.

Prijedlog popravke (dva koraka, prvi rješava prijavljeni slučaj):
- **Korak 1 (DBX):** u `subTransformBy` sva četiri elementa: ako je matrica **uniformna** razmjera `s` (tri ose iste dužine) – primijeniti je i pomnožiti sa `s` sve linearne veličine: `SizeA/B`, `SizeAp1..Bp2`, `SizeApr/Aotv…`, `Radius`, `RadiusConst`, `LengthTr`, `LengthPl`, `Elev/ElevUp/ElevDown`, `WipeoutLength`, te `TeeData (l, second, radius2)` i `HiddenSpan (first, last, width, elevation)`. Pošto su veličine u jedinicama crteža, to je tačno konverzija mm↔cm↔m kad su `INSUNITS` ispravni. Neuniformnu razmjeru i dalje odbiti. Članovi etaža/obilaska: dozvoliti samo uniformnu razmjeru cijelog sklopa ili odbiti kao sada.
- Ručni `SCALE` u istom crtežu: ili dozvoliti (mreža ostaje konzistentna, dimenzije se mijenjaju – poruka „dimenzije ×s“), ili zadržati zabranu preko ARX editor reaktora (`commandWillStart("SCALE")` postavlja zastavicu koju DBX čita). Odluka je korisnička; tehnički je prva varijanta jednostavnija.
- **Korak 2 (ARX):** `AcEditorReactor::otherWblock` – upisati `TVS_DRAWING_UNITS` izvora u ciljnu bazu (da clipboard nosi faktor); `otherInsert`/`endInsert` – uporediti `faktor(izvor)/faktor(cilj)` sa stvarno primijenjenom razmjerom; ako se ne slažu (npr. `INSUNITS` = 0), upozoriti i ponuditi skaliranje umetnutih TVS elemenata oko tačke umetanja. Bez ovoga korak 1 radi samo kad `INSUNITS` odgovaraju TVS jedinicama.
- Test: čista funkcija „uniformna razmjera + skaliranje polja“ u `geometry/` + ručni test u `TESTIRANJE.md` (mm→cm, cm→mm, cm→m, sa i bez `INSUNITS`).

### Važno

**N3. Globalne postavke se ne usklađuju sa jedinicama crteža u dijelu komandi** [kod]
`syncTVSGlobalUnits()` zovu samo stare komande i dio novih (`NewCommands.inl`). Ne zovu: `TVS_OFFSET` (`NewCommands.inl:3`), `TVS_BYPASS`, `TVS_BYPASS_DETALJ` (`acrxEntryPoint.cpp:111–112, 138–142`), `TVS_DIM_PRATI`, `TVS_ETAZ_KOTA`. Oni čitaju `globalTapRadiusConst` (u jedinicama zadnjeg usklađenog crteža): `OffsetIntegrationCommand.h:54, 77, 119`, `tvsBypassRadiusSettings()`, `SizeFollow.h:158, 493`.
Scenario: aktivan mm-crtež, prelazak u cm-crtež, prva komanda TVS_OFFSET ili TVS_BYPASS sa konstantnim radijusom → Ri = 150 jedinica = 1500 mm → „prekratko“ ili ogromna koljena.
Popravka: `syncTVSGlobalUnits()` na početku tih ulaza, ili jednom u `documentActivated`.

**N4. `OffsetRefit` (etaž prati kotu): stanje je globalno, bez čišćenja i bez isteka** [kod]
`OffsetRefit.h:22–28, 52, 189–199`: `pending()/opened()` nisu po crtežu (ispravka V8 urađena samo u `SizeFollow`); `goodbye` ne briše id-jeve zatvorenog crteža (kasnije otvaranje id-ja uništene baze – mogući pad); `posted()` nema istek kao `SizeFollow::post` (5 s) – ako poslata komanda ne stigne, `TVS_ETAZ_KOTA` se više ne šalje do restarta. Isto: `tvsOffsetSession::history()` (`OffsetSessionState.h:7`) ključ je `AcDbDatabase*` i nikad se ne briše.
Popravka: isti obrazac kao `SizeFollow` (`forget(db)` u `goodbye`, obrada samo svog crteža, istek).

**N5. `tvsBypassResize`: brisanje starog obilaska nije u transakciji** [kod]
`BypassPreviewCommand.cpp:544–560`: članovi se brišu jedan po jedan; ako npr. četvrti ne može da se otvori (zaključan sloj), poruka kaže „ništa nije promijenjeno“, a prva tri su obrisana i zapis sklopa ostaje. Ako upis novog ne uspije, stari je već uklonjen (to poruka kaže).
Popravka: brisanje + spajanje + novi upis u jednu transakciju (`BuildTransaction`), abort na prvoj grešci.

**N6. `TVS_HEATFLOOR` je nedovršen eksperiment u izdanju** [kod]
`Misc.inl:246–304`, `HeatFloor.cpp`: korak 200 u jedinicama crteža; `getOffsetCurves` rezultati se nikad ne brišu (`Misc.inl:282`, `HeatFloor.cpp:258–264, 287–288, 308–316` – do 10 000 iteracija); `getInsideOffset` vraća `-1` (`:270`), `getMaxOffset` nema `return` na kraju (`:298`) – nedefinisano ponašanje, moguće da se u `polySet` upiše korisnikova polilinija otvorena za čitanje, pa `setClosed`/`drawEntity`/`close` nad njom (`:23–24`) → pad; ispis debug brojeva. Nije u meniju (`TVS_VENT.cuix`), ali se može ukucati.
Popravka: pod `TVS_DEV` (dodati u `tests/test_release_commands.py`).

**N7. Specifikacija (TVS_SPEC / TVS_EXCELSPEC): blokovi na zaključanom sloju se tiho izostavljaju** [kod]
`SPEC.cpp:538` `GetAtt` otvara blok `kForWrite` samo da pročita atribut → `addBlock` vraća `false` → blok nije u tabeli, bez poruke. Popravka: `kForRead`.

**N8. TVS_OFFSET / Z: poslije „U redu“ u dijalogu traži se druga potvrda sa podrazumijevanim „Otkaži“** [kod]
`OffsetIntegrationCommand.h:87–95`: `[Primijeni/Otkazi] <Otkazi>` + ispis „DEV preview…“, „TVS OFFSET DEV: Lmin=…“. Enter poslije dijaloga poništava etaž. Ako je namjerno (pregled), podrazumijevano treba biti „Primijeni“; DEV poruke ukloniti.

**N9. Automatski poslate komande (`_TVS_DIM_PRATI`, `_TVS_ETAZ_KOTA`, `_TVS_PRELAZ_KOTA`) postaju „zadnja komanda“** [provjeriti]
`SizeFollow.h:87` šalje `TVS_DIM_PRATI` poslije svake komande koja je dirnula TVS element (zbog oznaka, podrazumijevano uključeno). Enter/razmak za ponavljanje (TVS_DRAW, MOVE, COPY…) tada ponavlja TVS_DIM_PRATI; uz to drugi korak u UNDO. Ako se potvrdi: oznake osvježavati direktno u `commandEnded` (kontekst dokumenta, `lockDocument`) bez slanja komande; komandu slati samo kad treba dijalog/pitanje.

**N10. Dužina fleksibilnog koljena u novoj specifikaciji** [kod, provjeriti brojem]
`TVS_TAP.cpp:2218`: `π·Radius·ugao/2/π` = `Radius·ugao/2`; osa luka je `(Radius + SizeA/2)·ugao` (tako se i crta, `:377`). Za Ø200, k = 0,5, 90°: 79 mm umjesto 314 mm.

### Sitno

- S-1 `Func.cpp:2490, 2515` `CString("Разные")` – uski literal sa ćirilicom; na build mašini (CP1252) postaje „??????“ u poljima sa različitim vrijednostima. → `L"*različito*"`.
- S-2 Svojstva ne provjeravaju opseg: ugao koljena 0, ≥ 180 ili negativan (`TVS_TAP.cpp:304` dijeli sa `cos(ugao/2)`), dimenzija 0 (`:487` dijeli sa `SizeA/2`).
- S-3 `TVS_TAP.cpp:3087, 3121` `int get_RadiusVariableParameter()`, `int get_RadiusConst()` – vraćaju `int` (0,5 → 0); trenutno se ne koriste – obrisati ili ispraviti tip.
- S-4 `SPEC.cpp:210` poredi `RadiusTypeRound` sa `TypeRoundTap_TapSection` (pogrešno polje); `SPEC::add` bez `return` na jednoj grani (`:421–425`); `SPEC::toChar` (`:495`) curi i vraća adresu lokalnog bafera (ne koristi se).
- S-5 `TVSCommandSpecification.cpp:34–47`: petlja bez izlaza za status različit od `RTNORM/RTCAN/RTKWORD`.
- S-6 `Misc.inl:102–111` (`TVS_AddAtrib`): pri praznom kliku čita se neinicijalizovan `ads_name`; ako blok nema ime, komanda izlazi bez poruke.
- S-7 `SpiroCatalog.h:26–34`: keš postavki je `static` u inline funkciji – ARX i DBX imaju svaki svoj; poslije TVS_SETTINGS odmah poslati REGENALL može crtati stare postavke do 1 s [provjeriti].
- S-8 `PortGrips.h:79–86`: „slobodan priključak“ se gleda samo u osnovi – element na drugoj koti na istoj XY tački sakriva konektor-grip.
- S-9 `SizeFollow.h:252, 461–464`: mrtav kod (grana TK-O nikad ne ulazi u `run.tees`); `:221` granica 2000 bez poruke; `:123` UNDO briše čekanja svih crteža.
- S-10 `runBypass` (`BypassPreviewCommand.cpp:422`) u Layout-u izlazi bez poruke.
- S-11 Izvoz u Excel piše brojeve kao tekst sa tačkom (`MSExcel.cpp:962–977`) – u Excel-u sa decimalnim zarezom nisu brojevi [provjeriti].
- S-12 `tests`: nema testa koji drži `TVS_HEATFLOOR`, `TVS_BYPASS_PREVIEW`, `TVS_OFFSET_POLICY/STATUS` van izdanja; grupa `TVS_BYPASS_DEV` se registruje van makroa pa je test ne vidi.

### D. Dijalog cijele dionice (TVS_DIM_PRATI, isključen) – konkretni defekti

| # | Prijava | Uzrok u kodu | Popravka |
|---|---|---|---|
| D1 | staje na koljenu (bez razloga) | `SizeFollow.h:225` (i `:395`): susjed se priznaje samo ako mu je `Elev` jednak ±1 mm; inače `count==0` = „slobodan kraj“, bez `stop()` i bez oznake. Prenos kanal↔prelaz (`:715`) koristi blaže pravilo `tvsFollowRules::joined` („jedini element na tački → prihvati“). | isto pravilo u `walk`/`joints`; kad kota ne odgovara → granica „razlika kota“ |
| D2 | staje na koljenu | koljeno čiji je ugao zaokružen kroz Svojstva (**N1**) ima priključke > 1 mm od krajeva kanala → opet `count==0` | N1 + alat za popravku postojećih koljena (zaokružiti ugao na 30/45/60/90 kad je unutar 0,5°) |
| D3 | staje na okruglom koljenu; okruglo koljeno ostaje između pravougaonih kanala | `TVSMoveCommand.h:58`: okruglo koljeno je „podržano“ samo sa radijusom k·D ili konstantnim; tip „specifični“ (segmentno, `RadiusTypeRound_RadiusSpecific`) dobija `kind = 0` → `walk` ga tretira kao granicu, `joints()` (`:398`) ga ne broji → ni prelaz ni upozorenje. Pri promjeni oblika kanali s obje strane postanu pravougaoni (ručno / više izabranih), koljeno ostane okruglo. | za promjenu oblika koljeno ne treba stare priključke: konvertovati ga po `elbowRule` i izračunati nove; inače obavezno upozorenje |
| D4 | ništa se ne javi | `SizeFollow.h:656`: kad je dijalog uključen a dionica prazna (`anything()==false`), nema ni dijaloga ni upozorenja o spoju bez prelaza (upozorenje je samo u grani „isključeno“, `:652–654`) | upozorenje `joints()` uvijek |
| D5 | prelaz na ogranku T komada umjesto prilagođenja | (a) TK-O/TK-G: dionica nikad ne mijenja T komad direktno (`:248–250`); „Nastavi“ otvara njegov dijalog, gdje priključak preuzima samo **širinu** (`TeeTypeDialog.h:93–102`, `lockB`) – ako se promijenila i visina, ostaje prelaz zbog visine; Otkaži / „Stani“ / „Prelaz iza“ → `tvsTeePort::afterDuct` umeće prelaz. (b) Promjena oblika: svaki T komad je granica (`:241`), iako štucna/TK0 imaju slobodan oblik ogranka (`shapeFree`) – ogranak se ne mijenja; kod TK0/štucne ne umeće se ni prelaz ni upozorenje (`afterDuct` radi samo za TK-O/TK-G, `TeePort.h:249`). (c) Dijalog isključen (sadašnje stanje): TK-O/TK-G uvijek prelaz; TK0/štucna ništa. | (a) u dijalogu dionice jasno reći „TK-O: mijenja se širina; visina ostaje visina ulaza“; kad dionica dolazi na **ulaz**, ponuditi da T komad preuzme i visinu (već postoji `newInlet`). (b) štucna/TK0: pri promjeni oblika postaviti `ThisRoundotv`, `SizeAotv/Botv`, novu `LengthPl` (kao `apply`, `TeeTypeDialog.h:433–434`). |
| D6 | više posla nego ručno | iz Properties palete svaka izmjena polja (A pa B) šalje posebnu komandu i poseban dijalog (`:96, 106`); „Otkaži“ ne vraća izmjenu kanala; dva koraka UNDO; ugniježđeni dijalozi (T komad → dionica → T komad, `followFromTee`) | skupiti izmjene (npr. 300 ms / do gubitka fokusa palete); jedna UNDO grupa; dijalog samo iz Svojstava, a iz palete samo upozorenje |
| D7 | djelimična primjena | `applyRun` (`:449, 456, 484`): otvaranje za upis koje ne uspije (zaključan sloj) se preskače bez poruke, ostalo se upiše; `etazResize` greška ostavlja dionicu promijenjenu do etaža (`:446`); nema transakcije | sve u jednoj transakciji, abort uz poruku |
| D8 | pogrešan razlog | flex kanal na trasi → `boundary()` vraća „kanal druge dimenzije“ (`:199`) | poseban tekst |

---

## (c) Komande i dijalozi

| Komanda | Presuda |
|---|---|
| TVS_PIPE, TVS_DRAW (+ konektor-grip) | radi; stari obrazac pokazivača (K4), bez novih grešaka |
| TVS_SETTINGS | radi; TRANSPARENT namjerno; decimalni zarez OK |
| TVS_CONNECTT, TVS_CONNECTW | radi; K4 nepotpun, izlaz usred operacije bez vraćanja ako `reopen` padne |
| TVS_CUT | OK; ne prenosi flex manžetne / skrivene raspone |
| TVS_Change (Svojstva) | **N1** – popraviti prije sljedećeg izdanja |
| TVS_1D2D | OK |
| TVS_ERASE / TVS_RAZBIJ | OK; sklopovi se razbijaju djelimično (V7), redoslijed brisanje/dodavanje |
| TVS_SCALE | OK (TVS elementi se namjerno preskaču) |
| TVS_AddAtrib | radi; `close()` pa korišćenje (V12) |
| TVS_SPEC, TVS_EXCELSPEC | radi; N7, baferi 512 (K5 obrazac) |
| TVS_Sp, TVS_ExcelSp | OK |
| TVS_BLCKMNGR, TVS_PropertyManager, TVS_AddProperty | OK |
| TVS_AttributesToProps | OK |
| TVS_LEAD, TVS_LEAD_Recalculate, TVS_LEAD_AUTO | OK |
| TVS_MOVE, TVS_STRETCH | OK (transakcija, provjera snimka); osjetljivi na N1 (tolerancija 0,1 mm) |
| TVS_ALIGN | OK; isto |
| TVS_TK_TIP | OK; ograničenje visine TK-O/TK-G (D5); lanac koljeno/prelaz bez transakcije (greška na pola ostavlja prelaz) |
| TVS_DIM_PRATI | prenos kanal↔prelaz OK; dijalog dionice – odjeljak D; N9 |
| TVS_PRELAZ_KOTA, TVS_MATCH, TVS_FLEX_MANZETNE/A, TVS_IZVOZ, TVS_KOTE, TVS_NATPIS | OK (pročitan tok) |
| TVS_PRESJEK (poprečni, uzdužni, osvježi) | OK (tok); `SectionCommand.h`/`ProfileCommand.h` nisu čitani red po red |
| TVS_OFFSET, opcija Z | radi; N3, N8, DEV poruke |
| TVS_ETAZ_KOTA | radi; N4 |
| TVS_BYPASS, opcija Obilazak, TVS_BYPASS_DETALJ | radi (strogi „fail closed“); N3, N5 |
| TVS_BYPASS_PREVIEW, TVS_OFFSET_STATUS, TVS_OFFSET_POLICY | razvojne – van izdanja |
| TVS_HEATFLOOR | **N6** – van izdanja |
| Razvojne pod `TVS_DEV` | ne grade se nigdje |

| Dijalog | Presuda |
|---|---|
| Podešavanja (`dgPipeSize`) | OK; decimalni zarez, kvačica „cijela dionica“ |
| Svojstva (`dgAllEdit` + `Func::Change`) | **N1**; S-1, S-2 |
| Tip T komada (`TeeTypeDialog.h`) | OK |
| Promjena dimenzije dionice (`RunDialog.h`) | sam dijalog OK; logika iza njega – D |
| Etaž / Z (`dgChangeZ`) | OK; druga potvrda u komandnoj liniji (N8) |
| Obilazak (`BypassPreviewCommand.cpp`) | OK (tok) |
| Presjek (`SectionDialog.h`) | OK |
| Menadžer blokova, Menadžer svojstava | OK |
| Oznake (`dgMLeadProperty`), podešavanja specifikacije (`TVSSpecTable`) | OK |
| TVS_MATCH dijalog | OK (tok) |

---

## (3) Definicije u izdanju (`TVS_Ventilation_ARX20xx.vcxproj:152–172`, workflow `/p:TVSBypassPreview=true /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true`)

| Define | Šta uključuje | Rizik | Isključiti? |
|---|---|---|---|
| `TVS_BYPASS_FAULT_INJECTION` | `BypassService.h:43–49, 133, 143, 148`: ako je prije pokretanja AutoCAD-a postavljena sistemska promjenljiva `TVS_BYPASS_FAIL` = 1/2/3, TVS_BYPASS namjerno pada u toj fazi („[DEV] Proba prekida“) i poništava transakciju | nizak – bez te promjenljive nema efekta | **Da, bezbjedno**: izbaciti iz `PreprocessorDefinitions` (2020 i 2022); `inject()` tada uvijek vraća `false`. Nijedan test je ne traži. |
| `TVS_OFFSET_PROOF` | (1) `#include "TVSTestOffset.h"` – u njemu su `BuildTransaction`, `modelId`, `pt`, `initialize`, koje koristi **produkcijski** kod (`OffsetService.h`, `BypassService.h`, `SizeFollow.h:320`, `OffsetIntegrationCommand.h`); (2) `commands/NewCommands.inl:1` – unutar tog `#ifdef` su i funkcije `TVS_OFFSET`, `TVS_ETAZ_KOTA`, `TVS_OFFSET_STATUS/POLICY`; (3) test komande `TVS_TEST_OFFSET*` – registrovane samo uz `TVS_DEV` | nizak (test komande nisu dostupne) | **Ne može se samo isključiti** – build puca (`#error` u `TVSTestOffset.h:2–4`, nedefinisane funkcije komandi). Prvo: pomoćne klase preseliti u neutralno zaglavlje (npr. `OffsetBuild.h`), u `NewCommands.inl` pod `TVS_OFFSET_PROOF` ostaviti samo tri `TVS_TEST_OFFSET*` funkcije; tek onda ukloniti define iz izdanja. |

---

## (d) Plan otklanjanja (redoslijed)

1. **N1** – Svojstva: formatiranje/poređenje brojeva i ugla (+ test); alat/komanda za popravku već zaokruženih uglova koljena. Mali obim, najveći učinak (rješava i dio D1/D2 i odbijanja TVS_MOVE/ALIGN).
2. **N2 korak 1** – uniformna razmjera u `subTransformBy` sa skaliranjem svih veličina (DBX, pažljivo: stari crteži se ne mijenjaju); ručni test mm↔cm↔m.
3. **N3, N4, N6, N7, N8** – mali, mehanički; `TVS_HEATFLOOR`, `TVS_BYPASS_PREVIEW`, `TVS_OFFSET_POLICY/STATUS` pod `TVS_DEV`; `TVS_BYPASS_FAULT_INJECTION` van izdanja; proširiti `test_release_commands.py`.
4. **N5, V7** – transakcije (obilazak pri promjeni dimenzije; razbijanje sklopova cijelih).
5. **N9** [poslije provjere] – oznake bez poslate komande.
6. **N2 korak 2** – prenos jedinica kroz WBLOCK/INSERT i upozorenje kad `INSUNITS` ne odgovara.
7. **K4/K5/V12 ostaci** – `SPEC.cpp`, `Specification.inl`, `Misc.inl`.
8. **TVS_OFFSET_PROOF** razdvajanje; `TVS_DEV` kao MSBuild parametar + jedan dev workflow da se taj kod bar kompajlira.
9. **Dijalog dionice (D1–D8)** – tek poslije 1; prvo pravila kao čista geometrija sa testovima (D1, D3, D5b), pa jedna transakcija (D7).

## (e) Prijedlozi unapređenja (najviše 5)

1. **TVS_PROVJERA** (iz prvog pregleda, sada sa konkretnim pravilima): koljena čiji ugao odstupa od 30/45/60/90 za < 0,5°, krajevi kanala koji promašuju priključak za 0,1–5 mm, različite kote na spoju, članovi sklopa bez roditelja, okruglo koljeno između pravougaonih kanala. Lista sa „zumiraj“ i „popravi“.
2. **Jedinice u elementu**: pri sljedećoj promjeni verzije zapisa upisati faktor mm/jedinici u svaki TVS element – kopiranje između crteža tada ne zavisi od `INSUNITS` ni od NOD zapisa.
3. **Jedna transakcija + jedna UNDO grupa po radnji** kao zajednički pomoćnik (već postoji `tvsMove::Transaction`) za TVS_TK_TIP, dionicu, obilazak, razbijanje.
4. **Numerička polja dijaloga kroz jedan sloj** (`double` + prikaz u mm/stepenima, poređenje brojeva) – uklanja cijelu klasu grešaka N1 iz Svojstava i olakšava novi izgled dijaloga (stavka 22 plana).
5. **Dionica bez dijaloga**: umjesto modalnog dijaloga poslije svake izmjene – upozorenje u komandnoj liniji + posebna komanda „TVS_DIONICA“ koju korisnik pokrene kad želi (izabere kanal, vidi pregled, potvrdi).
