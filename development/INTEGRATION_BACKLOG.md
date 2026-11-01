# Integracija — backlog i provjere

Datum 01.11.2026; osnova koda `bd01b2e208a5ef58e2905ce9854d8a63dac75382`.
Aktuelni plan: [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md).
Istorijsko stanje koda: [CODE_STATE.md](CODE_STATE.md). Naknadno je odobrena i
implementirana razvojna faza INT-00; ugovor: [OFFSET_INT00_GUIDE.md](OFFSET_INT00_GUIDE.md).
Rezultati, commit-i i paketi: [OFFSET_INT00_RESULTS.md](OFFSET_INT00_RESULTS.md).
Praktično prihvatanje nove isporuke je odvojeno od kompilacije i testova sa zamjenama.

## STAB-01 — otvoreno, odloženo u završnu stabilizaciju

**„Undo/Redo i kompatibilnost sa startup dodacima — završna stabilizacija prije
produkcionog prihvatanja.“**

Nije riješeno. Ne blokira naredno razvojno planiranje. Po zahtjevu korisnika sada
ne pokretati nove probe, ne analizirati dalje i ne mijenjati reaktore. Vraća se u
fazu 5, prije produkcionog prihvatanja. Postojeći rollback ostaje obavezan.

Sačuvani nalaz iz već izvršenih proba:

- U dokumentovanom AutoCAD 2022 okruženju sa startup manifestima ArtermoPodno,
  2Duct i KGH-Ducts privremeno isključenim prolaze obični interaktivni LINE Undo/Redo
  bez TVS-a i sa odgovarajućim DEV DBX/ARX parom; prolazi i interaktivni OB-01
  detalj Undo/Redo. Preostali moduli su evidentirani; to nije apsolutno čista sesija.
- U drugom startup okruženju problem je reprodukovan.
- Konkretan uzrok nije izolovan; nema dokaza za pripisivanje krivice pojedinačnom
  dodatku. Razlika između okruženja nije uzročna identifikacija.
- Sva tri startup manifesta su vraćena i hash provjeren. Ranija djelimična
  rezervna kopija ArtermoPodno ostala je sačuvana.
- Detalji i putanje dokaza: [sačuvani izvještaj](bypass-preview/REDO_INVESTIGATION.md).
  Nove probe nisu izvršene za potrebe ovog plana.

## Razvojne stavke — status

- [x] INT-00 DEV implementacija: zajednički servis/snapshot/writer, očuvana oba Z
  pozivaoca i registrovan TVS_OFFSET. Nije produkciono niti praktično runtime prihvatanje.
- [x] INT-01 DEV kod: trajni pravougaoni bypass, TVS_BYPASS i spoljni konektori.
  Praktično prihvatanje novog paketa ostaje otvoreno.
- [x] INT-02 DEV kod: isti servis kroz Obilazak/O u TVS_PIPE i TVS_DRAW.
  Praktičan nastavak oba toka u AutoCAD-u još nije potvrđen.
- [ ] INT-03: AssemblyId → OB-xx → DetailId, izvor/revizija, ručno osvježavanje.
- [ ] INT-04a: kružna trasa / pravougaona prepreka; glatka kružna koljena i offset.
- [ ] INT-04b: pravougaona trasa / stvarna cilindrična prepreka.
- [ ] INT-04c: kružna trasa / cilindrična prepreka.
- [ ] INT-05: završna stabilizacija i zasebno produkciono prihvatanje, uključujući STAB-01.

Zavisnosti, datoteke, potrebni podaci i kriterijumi svake stavke su u planu.

## INT-00 — preostali praktični pregled nove isporuke

Sljedeće nije izvršeno u AutoCAD-u tokom INT-00 i ostaje otvoreno za pregled:

- [ ] AutoCAD 2020 i 2022, odvojeni restartovani DEV procesi i odgovarajući DBX/ARX.
- [ ] TVS_OFFSET Prvi/Posljednji, obrnut pravac, podizanje/spuštanje, 30/45/60,
  A!=B, mm/cm/m i stvarno rotiran UCS; vizuelni preview i potvrda.
- [ ] Povezan izabrani kraj odbijen; suprotni priključak, originalni identitet,
  MLeader veza, inženjerski/prikazni i podržani korisnički podaci očuvani.
- [ ] Cancel/NoChange/Failed, premala visina/ulaz, stale snapshot, zaključan sloj,
  nepodržani atributi i fail-next bez ostataka ili promjene susjeda.
- [ ] Z kroz TVS_PIPE i TVS_DRAW: iste fizičke mjere i ispravan naredni segment,
  na starom kraju pri neuspjehu i na novom izlazu poslije uspjeha.
- [ ] Stvarni fizički potrošači (COM, Properties, TVS/Excel specifikacija, KGTEZ),
  materijal/prikaz i save/reopen. Legacy neupisivanje konektorskog vektora ostaje
  ograničenje; INT-00 ne uvodi novu trajnu topološku šemu.

Numerička matrica i testovi stvarnih C++ funkcija sa SDK zamjenama ne označavaju
ove stavke kao prošle. STAB-01/Redo se sada ne testira. Korisnik je naknadno
odobrio zaokruženu DEV implementaciju INT-01/02: roditelj nad dvije neizmijenjene
V1 trojke i H; bez povezanih detalja, kružnih kombinacija i pune hidden-line slike.

## Preostale provjere prethodne isporuke — NISU prošle

Ovo je evidencija za buduću provjeru; nije nalog da ih sada izvršavamo.

- [ ] Formalno poređenje svih relevantnih entiteta i block/layer/style/dimstyle
  zapisa poslije interaktivnog otkazivanja drugog detalja. Vizuelni nestanak
  transient pregleda OB-02 i *Cancel* su zabilježeni; potpuna provjera ostataka nije.
- [ ] Esc/Cancel pri svakom promptu, promjena pozicije i ponovljeni preview.
- [ ] Obična interaktivna LINE Undo/Redo proba poslije umetanja detalja — STAB-01.
- [ ] Snimiti i ponovo otvoriti upravo taj interaktivno umetnuti detalj bez TVS-a.
  Ranije otvaranje r2 standardnih snimaka bez TVS-a je prošlo, ali nije ova proba.
- [ ] Potpuno poređenje internog sadržaja izvorne trase/prepreke u radnoj kopiji.
  Entget zaglavlja su bila jednaka; originalni DWG hash je nepromijenjen.
  To nije puna provjera native payload-a. COM pregled custom pipe-a nije uspio.
- [ ] TVS specifikacija, masa i količine prije/poslije interaktivnog detalja.
- [ ] Puna matrica anotacija/fontova/DPI, naročito granične visine i strelice.
- [ ] Preostala ručna preview matrica: Prvi/Posljednji, UCS/smjerovi, invalidacija
  svakog ulaza, granice/budžeti, izbori/odbijanja i DWGMOD. Ne proširivati tvrdnju
  iz sintetičke matrice na sve ove interaktivne tokove.
- [ ] AutoCAD 2020 runtime za novi integracioni i side-detail paket. Build prolaz
  i stari geometrijski proof nisu ovaj runtime prolaz.
- [ ] Offset integraciona praktična matrica iz `OFFSET_INTEGRATION_GUIDE.md`:
  Z i nastavak trase, fizički potrošači, podaci/COPY, save/reopen, mm/cm/m i UCS,
  rollback i višedokumentni tok. Postojeća automatizovana i stara proof potvrda
  ne znače da je cijela ova matrica prošla.

## Već evidentiran razvojni napredak — ograničeni domet

- [x] AutoCAD 2022 r3 sintetička 72-case native matrica, mjere/DIMLFAC/prikazni
  blokovi/podaci, četiri fail-injection tačke i odbijanje duple oznake.
- [x] Detekcija COPY dupliranog identiteta statičkog detalja; automatski novi
  identitet nije implementiran i ostaje jasno ograničenje.
- [x] Raniji standardni r2 snimci otvoreni bez TVS-a; nije završni interaktivni slučaj.
- [x] Interaktivni izbor stvarne trase/prepreke, kandidat, umetanje OB-01 i njegov
  neposredni Undo/Redo u dokumentovanom AutoCAD 2022 okruženju.

Referenca: `bypass-preview/SIDE_DETAIL_RESULTS.md` i dopuna
`bypass-preview/REDO_INVESTIGATION.md`. Ovo nije produkciono odobrenje.

## Stalne granice

- Produkcija se ne mijenja, PR #1 ostaje draft; DEV isporuka čeka korisnički pregled.
- Ne uklanjati transakcijsku zaštitu/rollback; ne uvoditi DWG upise iz dijagnostike.
- Nema automatskog pomjeranja nizvodne mreže ili spajanja starih detalja po oznaci.
- COPY statičkog detalja zadržava duplirani identitet; prijaviti konflikt.
- Hot unload/reload nije podržan testni tok; nova verzija poslije restartovanja.
- Segmentna/fleksibilna koljena, promjene presjeka, proizvoljna 3D mreža i automatsko
  osvježavanje detalja ostaju zaseban obim.

## Korisnička proba i novi DEV bypass

- Korisnik funkcionalno potvrđuje da INT-00 promjena Z radi dobro. Verzija AutoCAD-a,
  jedinice i pojedinačne runtime stavke nisu navedeni; nije potvrda cijele matrice.
- Primjedba da bypass samo prikazuje kandidat bila je tačna za prethodni paket.
  Novi kod dodaje trajno prihvatanje istog kandidata i nastavak oba toka crtanja.
- GFX-01 ostaje otvoren: nepotpune vidljive/skrivene fizičke ivice offseta i bypassa.
  Novi paket ne računa tačnu vidljivost; to je izričito označeno u dijalogu/uputstvu.
- [ ] Novi bypass runtime: oba smjera, jedinice/UCS/uglovi/strane, spoljašnji priključci,
  Cancel, stale/lock, rollback, stvarni fizički potrošači i save/reopen.
- [ ] Nova roditeljska XRecord/backlink validacija i filing u stvarnom AutoCAD-u.
- [ ] Obilazak/O i sljedeći segment kroz TVS_PIPE i TVS_DRAW u obje ciljane verzije.

Uputstvo: [BYPASS_DEV_GUIDE.md](BYPASS_DEV_GUIDE.md). Testovi sa SDK zamjenama
ne potvrđuju native filing ili praktičan prolazak ovih stavki. STAB-01 ostaje odložen.

## Ciljana provjera poslije aa12514 — roditeljski scope

- [x] Regresija starog pinned(): nevažeći roditelj blokira nepovezan kanal.
- [x] DEV read-only ispravka: potpun poznati raspored referenci određuje nezavisnost
  prije odbijanja zbog nevažećeg članstva; stvarne/pozitivne granice ostaju zaštićene.
- [x] Zaostali roditelj nakon brisanja svih članova ne blokira nepovezan kanal kada
  su reference čitljive; nema automatskog čišćenja DWG zapisa. SDK zamjene, ne native proba.
- [ ] Praktično: valjan bypass, obrisani članovi/cijeli obilazak, nepovezan kanal i
  zaštićena granica; provjeriti poruke i odsustvo DWG promjena pri odbijanju.
- Ograničenje: potpuno nečitljiv/nepoznat raspored nema dokaz nezavisnosti;
  VEZA NEUTVRDJENA navodi ključ/status i konzervativno odbija operaciju.
