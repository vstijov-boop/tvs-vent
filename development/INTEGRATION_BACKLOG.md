# Integracija — backlog i provjere

Datum 01.11.2026; osnova koda `bd01b2e208a5ef58e2905ce9854d8a63dac75382`.
Aktuelni plan: [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md).
Stanje koda: [CODE_STATE.md](CODE_STATE.md). Nema nove implementacije u ovoj isporuci.

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

## Razvojne stavke — predloženo, nije implementirano

- [ ] INT-00: zajednički snapshot/ugovor upisa i samostalni pravougaoni offset.
- [ ] INT-01: trajni pravougaoni bypass, samostalna komanda i spoljni konektori.
- [ ] INT-02: isti servis kao opcija tokom crtanja.
- [ ] INT-03: AssemblyId → OB-xx → DetailId, izvor/revizija, ručno osvježavanje.
- [ ] INT-04a: kružna trasa / pravougaona prepreka; glatka kružna koljena i offset.
- [ ] INT-04b: pravougaona trasa / stvarna cilindrična prepreka.
- [ ] INT-04c: kružna trasa / cilindrična prepreka.
- [ ] INT-05: završna stabilizacija i zasebno produkciono prihvatanje, uključujući STAB-01.

Zavisnosti, datoteke, potrebni podaci i kriterijumi svake stavke su u planu.

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

- Produkcija se ne mijenja, PR #1 ostaje draft; implementacija čeka pregled plana.
- Ne uklanjati transakcijsku zaštitu/rollback; ne uvoditi DWG upise iz dijagnostike.
- Nema automatskog pomjeranja nizvodne mreže ili spajanja starih detalja po oznaci.
- COPY statičkog detalja zadržava duplirani identitet; prijaviti konflikt.
- Hot unload/reload nije podržan testni tok; nova verzija poslije restartovanja.
- Segmentna/fleksibilna koljena, promjene presjeka, proizvoljna 3D mreža i automatsko
  osvježavanje detalja ostaju zaseban obim.
