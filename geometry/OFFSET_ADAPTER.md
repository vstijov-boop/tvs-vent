# Tanak adapter Result → parametri postojećih TVS entiteta

30.10.2026. Izolovani korak bez povezivanja sa aplikacijom. OffsetGeometry.h je
nepromijenjen. Nema XRecord-a, COM/specifikacione izmjene, dijaloga ili DWG operacija.

## Interfejs

`OffsetAdapter.h`, namespace tvsOffsetAdapter:

```cpp
Placement p(Q, headingRadians, mmPerModelUnit, startElevationMm);
Result parameters = adapt(originalInput, solvedGeometry, p);
```

Input i solvedGeometry moraju biti par iz ISTOG poziva solve(). Adapter ne poziva
solve i ne ponavlja proračun etažiranja. Pošto su Input/Result javne strukture bez
identifikatora porijekla, nekoliko odbrambenih provjera nije dokaz njihove
provenijencije. Pozivalac mora održati taj ugovor; proizvoljno prepravljeni Result
nije podržan ulaz.

Q je MiddlePoint prvog koljena u modelskim koordinatama, odnosno tačka završetka
trase na kojoj postojeća komanda traži Z. Lokalni horizontalni početak dobija se
oduzimanjem axis[1].s duž trase od Q. Crtački Z ostaje Q.z za SVE plan tačke.
Fizička početna kota u mm je nezavisan ulaz i ne upisuje se u crtački Z.

DTO sadrži oba koljena: MiddlePoint, Startvector, Normvector, A/B, razriješeni
konstantni Ri, Swectangle, referentnu Elev i Form Up/Down. Dimenzije, Ri i Elev
koljena su u modelskim jedinicama; referentne kote međukomada i fizička/projektovana
dužina su eksplicitno u mm. Enum Form je nezavisan od SDK brojeva: budući DBX
most mora ga eksplicitno prevesti u Form_Up/Form_Down. Ovo nisu živi entiteti. Budući DBX writer mora eksplicitno postaviti ThisRound=false,
kruti DuctTypeStill i konstantni režim Ri iz DTO-a; ne ostavljati ih na globalnim
vrijednostima. Prikaz 1D/wipeout je odluka pozivaoca i ne utiče na ovaj DTO.

Ri je razriješen iz ulaza; DTO predlaže konstantni radijus za reprodukciju istog
rezultata. Varijabilni režim je u testu dodatno provjeren sa istim k*B=Ri. Adapter
ne preuzima globalna podešavanja. Ne predlaže niti nameće Lmin=100 / projekciju 0.1.

| Priključak | Prvo koljeno | Drugo koljeno |
|---|---|---|
| MiddlePoint i očekivani MC | plan(axis[1]) | plan(axis[2]) |
| Očekivani MA | plan(axis[0]) | plan(axis[3]) |
| Startvector uz trasu (Ux,Uy) | (Uy,-Ux,0) | (-Uy,Ux,0) |
| Form kod podizanja | Up | Down |
| Form kod spuštanja | Down | Up |
| Referentna Elev | početna horizontalna kota | završna horizontalna kota |

Nagibni ravanski kanal: FirstPoint=plan(axis[1]), LastPoint=plan(axis[2]).
Ulazni kanal dobija novi LastPoint=plan(axis[0]), izlazni FirstPoint=plan(axis[3]).
Adapter NE definiše ostale krajeve ulaznog/izlaznog kanala niti ih modifikuje.
Kote krajeva pravog međukomada su startElevationMm + axis[1/2].z; njihov pomak
nije ukupni deltaZ etažiranja. Fizička dužina kopira se iz Result.straightLength,
projekcija iz Result.projectedLength. MiddlePoint nije centar kružnog luka.

Status Ready daje upotrebljiv DTO. NoGeometry propagira odsustvo Ok geometrije
(NoChange/greška solve se razlikuju u ulaznom geometrijskom rezultatu).
InvalidParameters označava nevažeći položaj/jedinice/parametre; InconsistentResult
uočenu neusaglašenost para; UnrepresentablePlacement prelivanje/gubitak preciznosti
pozicioniranja. Neuspjeh ne vraća djelimični DTO i ne pravi upis.

## Izvršena provjera i njena granica

Python test čita STVARNI trenutni TVS_TAP.cpp i izdvaja get_radius, setRadius,
put_Form, računanje startangle iz Startvector i pravougaoni Up/Down prefiks
subWorldDraw koji računa CenterPoint/A/D/MA/MC. Ti izrazi se kompajliraju i izvršavaju;
ne postoji druga ručno prepisana formula MA/MC kao zamjena za taj prefiks.
Početni Radius, MA i MC se namjerno kvare prije izvršavanja.

Zamjene su AcGePoint3d::rotateBy, jednostavna vektorska struktura, assert metode,
unit helper i omotač klase. Odabir odgovarajuće grane subWorldDraw je u testu
prinudan; nije testirana kompletna ObjectARX dispatch/render putanja. Nema stvarnog
AcGe, AcDb baze, transakcije, crtanja ili serijalizacije. To NIJE dokaz stvarnog
ponašanja dodatka u AutoCAD-u.

540 slučajeva: 3 jedinice × 5 smjerova × 3 ugla × 2 smjera visine × 3 A!=B
presjeka × 2 režima Ri. Provjerena su oba koljena svakog slučaja. Dodatno fiksni
numerički oracle sa Q=(1000,2000,37) i početnom kotom -400, razdvajanje dvije Z
reference, odsustvo geometrije, nevažeći faktor, nesaglasni podaci i gubitak XY
preciznosti pri vrlo velikoj WCS poziciji.

Lmin=0 i Ri=0 nisu zabranjeni adapterom kao proizvodno pravilo; zero-length DTO
ne znači da je bezbjedno kreirati nulti TVS_Pipe u aplikaciji. Šema B i podaci nisu
implementirani. Dokumentovan predlog je u OFFSET_ASSEMBLY_B_PROPOSAL.md.

## Obavezno provjeriti u AutoCAD-u nakon posebnog odobrenja integracije

- Stvarni setter/redoslijed kreiranja i ponovljeni REGEN: MA/MC naspram Result,
  bez čitanja prethodnog redraw keša; 30/45/60°, oba smjera visine, A!=B.
- Stvarno AcGe rotiranje, pomjeren/rotiran UCS, različite WCS ravni, mm/cm/m.
- Puni prikaz kontura, skrivene linije, wipeout, 1D režim i izbor/OSNAP tačaka.
- Kote u Properties i MLeader: horizontalne reference odvojene od crtačkog Z;
  fizičke kote kosih spojeva posebno označene.
- Zaključan sloj, otkazivanje i greška pisanja: nema djelimične operacije.
- COPY, brisanje, integritet članstva, Undo/Redo i snimanje/otvaranje tek uz kasnije
  odobrene podatke B; fizička dužina, tabele, COM i KGTEZ tek uz tu integraciju.

Stvarni izlaz testova: tests/OFFSET_ADAPTER_TEST_REPORT.md. Windows build nije
pokrenut i nema promjene ponašanja postojećih komandi/crteža.
