# Stvarno stanje koda na pregledanom HEAD-u

Istorijski inventar pregledanog HEAD-a bd01b2e, sačuvan bez reinterpretacije.
U naknadno odobrenom INT-00 uvedena je DEV registracija `TVS_OFFSET` i zajednički
servis oba Z pozivaoca. Aktuelni ugovor i ograničenja: [INT-00 uputstvo](OFFSET_INT00_GUIDE.md).
Izvršni HEAD i stvarni build/test rezultati su u [INT-00 izvještaju](OFFSET_INT00_RESULTS.md).
Tvrdnja ispod da TVS_OFFSET nije registrovan važi za navedeni stari HEAD, ne novu isporuku.

Pregled: 01.11.2026. Tačan HEAD: `bd01b2e208a5ef58e2905ce9854d8a63dac75382`.
Grana `razvoj-bypass-candidate`, PR #1 draft. Sadržaj izvršnog r3 paketa potiče
iz `ee85e72cce0e805991d172a2c34cdc4306530cb3`. Ovaj inventar opisuje izvorni kod,
ne tvrdi da su svi ogranci praktično potvrđeni.

## Komande i opcije relevantne integraciji

`TVS_OFFSET` i `TVS_BYPASS` **nisu registrovane** na ovom HEAD-u. To su predlozi.
Provjereno u `TVS_Ventilation_ARX/acrxEntryPoint.cpp`, ne izvedeno iz naziva dokumenta.

- `TVS_PIPE`: crtanje nove trase; `TVS_DRAW`: nastavak iz izabranog konteksta i
  `drawStep3`. U oba toka registrovane opcije su `Dimenzija/D`, `Koljeno/K`,
  `Račva/R`, `Z`, `Gore/G`, `Nadolje/N`. Prva dva koraka izbora tačaka nude
  Dimenzija/D. `Obilazak`/bypass nije opcija. `Gore/Nadolje` koriste postojeći
  `UpDown`; nisu novi checked bypass. `TVS_DUCT` je zasebna legacy putanja, ne
  novi integracioni ulaz i nije obuhvaćena obećanjem istog Z toka.
- Uz `TVS_OFFSET_INTEGRATION`, opcija `Z` poziva
  `tvsOffsetIntegration::execute` iz `OffsetIntegrationCommand.h`; bez flaga
  ostaje legacy changeZ. `Applied` jedini ažurira nastavak i globalnu kotu/ugao.
- DEV offset registracije: `TVS_OFFSET_STATUS`, `TVS_OFFSET_POLICY`,
  `TVS_OFFSET_TEST_BAD`, `TVS_OFFSET_FAIL_NEXT` uz integracioni flag; proof:
  `TVS_TEST_OFFSET`, `TVS_TEST_OFFSET_CHECK`, `TVS_TEST_OFFSET_FAIL` uz proof flag.
- Uz `TVS_BYPASS_PREVIEW`: `TVS_BYPASS_PREVIEW`, `TVS_BYPASS_DETAIL_TEST`,
  `TVS_BYPASS_DETAIL_CHECK`, grupa `TVS_BYPASS_DEV`. PREVIEW sada može upisati
  standardni statički detalj; ne upisuje trajni TVS obilazak. Stariji GUIDE naslov
  „bez DWG upisa / Umetni nedostupan“ opisuje raniju fazu; `SIDE_DETAIL.md` i
  stvarni writer su novije mjerodavno stanje.

DEV workflow-i `dev-bypass-preview-2020.yml` i `dev-bypass-preview-2022.yml`
uključuju TVSBypassPreview, TVSOffsetProof i TVSOffsetIntegration. ARX projekti
2020/2022 uključuju i proof definiciju kad je integracija uključena; DBX mora
imati odgovarajući integracioni flag. To nisu podrazumijevana produkciona obećanja.

## Šta se trajno upisuje

**DEV Z offset:** skraćuje dolazni obični horizontalni pravougaoni kanal, dodaje
dva `TVS_TAP`, jedan kosi `TVS_Pipe` i obični izlazni kanal od početnih 300 mm.
Troje unutrašnjih članova nose `TVS_OFFSET_MEMBER` v1. Izlaz postaje aktivni kanal
na novoj koti. Transakcija uključuje članove, zapise i skraćivanje; fail-next
prekida prije završetka. To nije opšti standalone editor priključene mreže.

`appearance()` prenosi layer, color, linetype, linetypeScale, lineweight, Flow,
Grani, Wipeout i This1D. Novi pipe konektori inicijalizuju se bez connectionID.
Unutrašnje članstvo ne dokazuje kompletno prevezivanje spoljašnjih konektora ili
očuvanje proizvoljnih atributa; to zahtijeva eksplicitan novi ugovor.

**BYPASS statički detalj:** `BypassSideDetail.h` jednim transakcionim upisom pravi
standardnu block definition/reference, polilinije s kružnim bulge lukovima,
MTEXT i standardne neasocijativne kote, izdvojene stilove/sloj i XRecord.
Izvorna trasa/prepreka nisu otvorene za upis. Dokumentacija nije novi TVS fizički
sklop niti automatski asocijativni detalj. Pozicioniranje prije potvrde je transient.

**Samo rezultat/preview:** sedmočlani checked bypass kandidat (četiri koljena,
dva kosa, srednji horizontalni), ograničena pretraga i prikazi. Nema sedam trajnih
TVS članova, povezanog bypass AssemblyId, žive oznake, osvježavanja ili opcije u crtanju.

## Podržana geometrija, jedinice i izvedbe

Offset integracija odbija `ThisRound`, nekruti tip, nehorizontalni WCS izvor i
drugi prostor osim Model-a; zahtijeva A/B>0 i obični editabilni neoznačeni kanal.
Pravougaona glatka koljena: Ri konstantan ili k*B, Ri>0; specific režim odbijen.
Solver ima širi numerički domen od UI tipičnih 30/45/60; 90°/Unsupported90 se
odbija, bez zamjene približnim uglom. DEV Lmin=100 mm, minimumProjection=0.1 mm,
ostatak dolaznog kanala najmanje 1 mm; nisu proizvodni standardi. Nulti ravanski
međukomad nije podržan integracijom.

Bypass adapter (`PreviewModel.h::adapt`, `readPipe`): oba prava direktna ordinary
kruta pravougaona horizontalna TVS kanala u aktivnom Model prostoru, nisu isti
objekt ni označeni offset članovi; upravne XY ose (dot tolerancija 1e-10), pun
poprečni obuhvat. Nema XREF/blokova, kružnih/fleksibilnih, kosih ili proizvoljno
ukrštenih osa. UI bira 30/45/60°, iznad/ispod; izolacije su eksplicitne, uključujući 0.
Jedinice mm/cm/m (1/10/1000 mm po jedinici); fizička kota odvojena od crtačkog Z.

Legacy DBX inače ima `ThisRound`, `TypeRoundTap_TapRound` i `TapSection`, te
konstantni/variable/specific radius režime. To dokazuje postojanje klasa i ranijih
izvedbi, ne podršku u današnjem offset/bypass adapteru. A=B nije kružni presjek.
Pravougaoni kružni luk u profilu koljena ne znači kružni poprečni presjek.

## Podaci sklopa i identitet detalja

`OffsetAssemblyData.h/.cpp`: Record(version=1, guid, role, members[3], rise).
Uloge FirstElbow / SlopedStraight / SecondElbow; XRecord `TVS_OFFSET_MEMBER`:
DXF 90 verzija, 1 GUID, 70 uloga, tri 330 soft-pointer reference, 40 rise.
Rise je nenulti samo za kosi član. Nema bypass parent-a, vidljive OB oznake ili
detaljske reference u tom zapisu. Statusi Legacy/Valid/Incomplete/Corrupt/
UnsupportedVersion/GeometryMismatch; susjedna povezanost je odvojena od članstva.
Postoje zaštite parcijalnog editovanja/transforma i remap clone članstva.
COPY cijele trojke namjerava novi GUID; djelimična kopija Incomplete. Stvarno
runtime ponašanje cijele integracione matrice nije ovim inventarom potvrđeno.

`TVS_BYPASS_SIDE_DETAIL_V1` na standardnoj block reference: code-1 stringovi,
početak `TVS_SIDE_DETAIL/1`, vlastiti GUID, korisnička oznaka, STATIC_SNAPSHOT;
zatim writer/units/orientation i brojčani snapshot potvrđenog rezultata.
Handle-ovi trase/prepreke su informativni, nisu žive reference. Nema izvornog
AssemblyId/revizije, automatskog osvježavanja ili dovoljno podataka da se time
obeća rekonstrukcija operativnog TVS sklopa.

COPY standardnog detalja kopira isti GUID/oznaku. Inventory u lokalnim definicijama
blokova otkriva konflikt i blokira novo umetanje; XREF i globalni registry između
DWG-a nisu obuhvaćeni. Automatski novi identitet nije implementiran.

## Potvrđeno naspram neprovjerenog

Praktično AutoCAD 2022: r3 native 72-case matrica, kote/DIMLFAC/display blocks,
četiri rollback tačke, duple oznake i detekcija COPY identiteta. Raniji r2 standardni
snapshot je ponovo otvoren bez TVS-a; nije to isti interaktivni OB-01 slučaj.
Interaktivno izabrani stvarni objekti, checked kandidat, umetanje OB-01 i neposredni
Undo/Redo prolaze u dokumentovanom okruženju s navedenim startup isključenjima.
Drugi preview OB-02 je otkazan vizuelno; potpuna provjera ostataka nije završena.

Automatizovane numeričke/stub provjere i DEV buildovi 2020/2022 postoje; nisu
zamjena za runtime. Ranija korisnička AutoCAD 2020 potvrda odnosi se na geometrijski
proof, ne novu integraciju ili detail paket. Reference za offset količinu:
400x200, Ri=100, deltaZ=400, 45° daje P=riseStraight=282.842712 mm i L=400 mm;
predviđeno 0.4 m, 0.48 m² i 2.2608 kg u dokumentovanim uslovima. Ne predstavljati
ovo kao sada izmjeren prolaz svih fizičkih potrošača.

Potpuna praktična integraciona matrica, povezani konektori/atributi, puna
cancel/no-residue provjera, ponovno otvaranje interaktivnog detalja bez TVS-a,
masa/specifikacija prije-poslije, anotaciona/DPI matrica i AutoCAD 2020 runtime
ostaju u [checklisti](INTEGRATION_BACKLOG.md). Redo uzrok nije izolovan, ne pripisuje
se pojedinačnom dodatku i sada je odložen; nije riješen niti blokator planiranja.

## Potpuna registraciona lista

Sljedeći dodatak je prepis registrovanih globalnih/lokalnih imena iz izvora na
navedenom HEAD-u; prisustvo definicije nije tvrdnja da je korisna funkcija potpuno
implementirana ili praktično testirana. Compile-time uslovi iznad važe za DEV imena.

- `TVS_PropertyManager` (lokalno `TVS_PropertyManager`; grupa `Ventilation_ARX`).
- `TVS_AttributesToProps` (lokalno `TVS_AttributesToProps`; grupa `Ventilation_ARX`).
- `TVS_ExcelSp` (lokalno `TVS_ExcelSp`; grupa `Ventilation_ARX`).
- `TVS_Sp` (lokalno `TVS_Sp`; grupa `Ventilation_ARX`).
- `TVS_AddProperty` (lokalno `TVS_AddProperty`; grupa `Ventilation_ARX`).
- `TVS_MOVE` (lokalno `TVS_MOVE`; grupa `Ventilation_ARX`).
- `TVS_PIPE` (lokalno `TVS_PIPE`; grupa `Ventilation_ARX`).
- `TVS_DRAW` (lokalno `TVS_DRAW`; grupa `Ventilation_ARX`).
- `TVS_TEST` (lokalno `TVS_TEST`; grupa `Ventilation_ARX`).
- `TVS_TRANS` (lokalno `TVS_TRANS`; grupa `Ventilation_ARX`).
- `TVS_WYE` (lokalno `TVS_WYE`; grupa `Ventilation_ARX`).
- `TVS_CONNECTT` (lokalno `TVS_CONNECTT`; grupa `Ventilation_ARX`).
- `TVS_CONNECTTest` (lokalno `TVS_CONNECTTest`; grupa `Ventilation_ARX`).
- `TVS_CONNECTW` (lokalno `TVS_CONNECTW`; grupa `Ventilation_ARX`).
- `TVS_DUCT` (lokalno `TVS_DUCT`; grupa `Ventilation_ARX`).
- `TVS_1D2D` (lokalno `TVS_1D2D`; grupa `Ventilation_ARX`).
- `TVS_SPEC` (lokalno `TVS_SPEC`; grupa `Ventilation_ARX`).
- `TVS_BLCKMNGR` (lokalno `TVS_BLCKMNGR`; grupa `Ventilation_ARX`).
- `TVS_EXCELSPEC` (lokalno `TVS_EXCELSPEC`; grupa `Ventilation_ARX`).
- `TVS_SpecificationOld` (lokalno `TVS_SpecificationOld`; grupa `Ventilation_ARX`).
- `TVS_Flex` (lokalno `TVS_Flex`; grupa `Ventilation_ARX`).
- `TVS_Show` (lokalno `TVS_Show`; grupa `Ventilation_ARX`).
- `TVS_CUT` (lokalno `TVS_CUT`; grupa `Ventilation_ARX`).
- `TVS_Change` (lokalno `TVS_Change`; grupa `Ventilation_ARX`).
- `TVS_ERASE` (lokalno `TVS_ERASE`; grupa `Ventilation_ARX`).
- `TVS_LEAD` (lokalno `TVS_LEAD`; grupa `Ventilation_ARX`).
- `TVS_LEAD_Recalculate` (lokalno `TVS_LEAD_Recalculate`; grupa `Ventilation_ARX`).
- `TVS_SCALE` (lokalno `TVS_SCALE`; grupa `Ventilation_ARX`).
- `TVS_AddAtrib` (lokalno `TVS_AddAtrib`; grupa `Ventilation_ARX`).
- `TVSSomeParts` (lokalno `TVSSomeParts`; grupa `Ventilation_ARX`).
- `TVS_SETTINGS` (lokalno `TVS_SETTINGS`; grupa `Ventilation_ARX`).
- `TVS_HEATFLOOR` (lokalno `TVS_HEATFLOOR`; grupa `Ventilation_ARX`).
- `MyCommand` (lokalno `MyCommandLocal`; grupa `TVSMyGroup`).
- `MyPickFirst` (lokalno `MyPickFirstLocal`; grupa `TVSMyGroup`).
- `MySessionCmd` (lokalno `MySessionCmdLocal`; grupa `TVSMyGroup`).
- `TVS_STRETCH` (lokalno `TVS_STRETCH`; grupa `Ventilation_ARX`).
- `TVS_TEST_OFFSET` (lokalno `TVS_TEST_OFFSET`; grupa `Ventilation_ARX`).
- `TVS_TEST_OFFSET_CHECK` (lokalno `TVS_TEST_OFFSET_CHECK`; grupa `Ventilation_ARX`).
- `TVS_TEST_OFFSET_FAIL` (lokalno `TVS_TEST_OFFSET_FAIL`; grupa `Ventilation_ARX`).
- `TVS_OFFSET_STATUS` (lokalno `TVS_OFFSET_STATUS`; grupa `Ventilation_ARX`).
- `TVS_OFFSET_POLICY` (lokalno `TVS_OFFSET_POLICY`; grupa `Ventilation_ARX`).
- `TVS_OFFSET_TEST_BAD` (lokalno `TVS_OFFSET_TEST_BAD`; grupa `Ventilation_ARX`).
- `TVS_OFFSET_FAIL_NEXT` (lokalno `TVS_OFFSET_FAIL_NEXT`; grupa `Ventilation_ARX`).
- `TVS_BYPASS_DETAIL_TEST`, `TVS_BYPASS_DETAIL_CHECK`, `TVS_BYPASS_PREVIEW` — direktni addCommand, grupa `TVS_BYPASS_DEV`, uz TVS_BYPASS_PREVIEW.

ADSSYMBOL `MyLispFunction` je dodatna LISP registracija; nije nova offset/bypass komanda.
