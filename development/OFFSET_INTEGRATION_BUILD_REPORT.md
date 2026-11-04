# DEV integracija promjene kote — izvještaj

Datum: 30.10.2026. Grana: `razvoj-offset-arx`.
Izvorni commit paketa: `da3e2c288aaa7848465a9df368b808e41f2b80e4`.
Ovo je paket za pregled i praktičnu provjeru, ne produkciono izdanje.

## Status provjera

| Provjera | AutoCAD 2020 | AutoCAD 2022 |
|---|---|---|
| Završno kompajliranje i linkovanje | Uspješno: 0 grešaka, 3 LNK4099 upozorenja | Uspješno: 0 grešaka, 3 LNK4099 upozorenja |
| Učitavanje nove integracije u AutoCAD | Agent nije izvršio | Agent nije izvršio |
| Interaktivni Z, novi podaci, COPY i količine u AutoCAD-u | Agent nije izvršio | Agent nije izvršio |

AutoCAD nije dostupan u lokalnom Linux okruženju niti se pokreće u Windows CI-ju.
Raniji korisnički izvještaj sa 12 slučajeva/24 TAP/216 objekata evidentiran je u
`USER_VALIDATION_2020.md`, isključivo kao korisnički izvršena provjera ranijih
razvojnih dokaznih komandi u AutoCAD 2020/mm/Kontura. Ne potvrđuje novu integraciju.

## Commit-i i redosljed

- A `d141433`: minimalni verzionisani podaci, validacija i razvojni flagovi.
- B `9396577`: fizička količina, COM i preflight specifikacije.
- C `360d525`: postojeći dijalog i oba pozivaoca nove Z putanje.
- D `c002aec`: zaštite, remap klonova, lokalizovana projekcija i dijagnostika.
- `8ff344f`: rollback i stanje nastavka, čitanje bez write-open, dodatne zaštite.
- `ae68b17`: oštećene recipročne zapise ne klasifikovati kao stare objekte.
- `899120b`: ciljani test članstva, lokalna maska i obrada greške registracije objekta.
- `da3e2c2`: ne prikazivati djelimični zbir mase izbora sa neispravnim članom.

Osnova clone lifecycle-a uvedena je uz C jer kreiranje novih zapisa i callback-i
moraju zajedno ostati izgradivi; kompletna politika zaštite i dijagnostika su u D.
Prihvaćeni `OffsetGeometry.h` i `OffsetAdapter.h` ostali su nepromijenjeni.

## Implementirano ponašanje

`TVSOffsetIntegration=true` definiše `TVS_OFFSET_INTEGRATION` u ARX/DBX i uključuje
razvojne dokazne komande. Bez flaga ostaje ranija Z putanja, raniji get_Length i
obračun starih objekata. Produkcioni workflow-i i paketi nisu prepisani; nema merge-a
u produkcionu granu, migracije starih DWG-a niti novog dijaloga.

Oba pozivaoca `changeZ()` koriste ishode Applied/NoChange/Cancelled/Failed. Postojeći
dijalog prikazuje kote u fizičkim mm, Na kotu je apsolutna završna kota ose. Stvarne
dimenzije, smjer, crtački Z i početna fizička kota čitaju se sa aktivnog kanala.
Z crtačke ravni ne zamjenjuje fizičku kotu. Podržani su Model prostor, pravougaoni
kruti kanal, jednaka glatka kružna koljena bez ravnih nastavaka, pozitivan Ri,
horizontalni ulaz/izlaz i domen solve-a bez 90°. Nepodržani slučajevi nemaju stari
fallback. Promjenljivi Ri koristi visinu B u ravni savijanja.

Prije izmjena provjerava se raspoloživa dužina: skraćivanje dolaznog segmenta mora
ostaviti barem 1 mm. Razvojna sesijska politika, podesiva kroz TVS_OFFSET_POLICY:
Lmin=100 mm, minimumProjection=0.1 mm. To nisu norme niti konačna proizvodna pravila.
Strogi solve i korisnička završna kota ne mijenjaju se radi numeričke granice.
NoChange tolerancija komandne putanje je 0.001 mm; nema pomjeranja geometrije.

U jednoj transakciji nastaju dva koljena, nagibni međukomad i obični izlazni komad
od 300 mm; dolazni kanal se skrati. Globalna kota/ugao i aktivni nastavak ažuriraju
se tek poslije uspješnog commita. Cancel/NoChange/Failed ne ažuriraju nastavak.
`TVS_OFFSET_FAIL_NEXT` izaziva rollback poslije upisa sva tri zapisa, prije
skraćivanja ulaza. Proof komande ostaju zasebne i ne daju fizički označene sklopove.

## Podaci, valjanost i kopiranje

Na svakom od tri člana: extension dictionary / Xrecord `TVS_OFFSET_MEMBER`:
`90=version(1), 1=GUID, 70=role, 330=FirstElbow, 330=SlopedStraight,
330=SecondElbow, 40=riseStraightMm`. Sve reference su recipročne soft-pointer
reference; setXlateReferences(true). Rise je nula na koljenima, potpisan na ravnom
međukomadu: axis[2].z-axis[1].z. Fizička dužina nije nezavisno uskladištena.
Ulaz i izlaz su susjedi, ne članovi.

Statusi: Legacy, Valid, Incomplete, Corrupt, UnsupportedVersion, GeometryMismatch.
Null/erased član daje Incomplete. Postojeći član bez svog očekivanog zapisa,
pogrešna klasa, nerecipročne reference, GUID/uloga nesaglasnost ili nepoznata
verzija nisu horizontalni fallback. Čitanje i REGEN ne popravljaju DWG. Provjera
geometrije koristi postojeće parametre i prihvaćeni solve/adapter, ne MA/MC redraw keš.
TVS_OFFSET_STATUS odvojeno ispisuje INTERNAL i NEIGHBORS, bez upisa susjednih ID-jeva.

Prije implementacije pregledani su stvarni ObjectARX 2020 SDK header-i sa CI-ja:
AcDbObject::subDeepClone/subWblockClone; AcRxEventReactor::endDeepClone; AcDbIdMapping;
AcDbXrecord::setXlateReferences. Konkretno: dbmain.h:2060/2066, rxevent.h:100,
dbxrecrd.h:40 iz stvarnog CI SDK-a. endDeepClone se koristi nakon prevođenja ID-jeva;
ne pišu se završne reference u beginDeepCloneXlation, gdje su još u prelaznom stanju.
Potpuni COPY dobija novi GUID i reference kopija; djelimični COPY nulira nekopirane
članove i ostaje Incomplete. Vlastiti validni rise nepotpunog komada dopušta njegov
samostalni obračun, ali ga ne proglašava povezanim sklopom. Greška završetka klona
ispisuje se; nema automatskog popravljanja pri otvaranju.

Postojeći setteri/gripovi/transformacije originalnih članova blokirani su. Samo
novo evidentirane kopije i nerazidentne scratch kopije smiju se pozicionirati čistom
translacijom tokom clone komande; dozvole se gase na commandEnded/Cancelled/Failed.
SCALE/ROTATE/MIRROR članova nisu podržani. MOVE kompletne originalne trojke takođe
nije podržan u ovoj prvoj verziji. Pomjeranje običnih susjeda može prekinuti vanjsku
vezu, dok unutrašnji sklop ostane validan; STATUS te dvije stvari ne poistovjećuje.

COPY u crtež sa drugim faktorom jedinica nije podržan: rezervisana verzija -1
blokira obračun kopije. WBLOCK/INSERT i vanjski clone API zahtijevaju zasebnu provjeru.
Nisu obećane zaštite protiv dodatka koji direktno prepiše javna C++ polja ili obriše
sve tragove Xrecord-a. Potpuno uklanjanje svih zapisa nije moguće razlikovati od
starog neoznačenog objekta bez dodatnog markera; takvo ručno uklanjanje nije podržano.

## Grafika i potrošači fizičke dužine

Novi označeni međukomad projektuje četiri ugla oba stvarna spojna presjeka. Fizičke
uzdužne ivice, gornje vidljive i donje skrivene linije odvojene su od simboličke
osne linije. Maska koristi postojeći široki osni stroke preko ispravnog projektovanog
gabarita, umjesto širokog stroke-a zatvorene konture. Obični kanali nisu promijenjeni.
Konture, skrivena grafika, maska i OSNAP nove integracije još nisu praktično potvrđeni.

| Potrošač | Novo označeni međukomad | Stari/obični kanal |
|---|---|---|
| TVS_Pipe::get_Length | XY projekcija za geometriju | raniji XY |
| getPhysicalLength | hypot(XY,riseStraightMm/faktor) | raniji get_Length |
| getCount | fizička dužina u metrima | ranije |
| Površina lima / masa | fizička dužina bez rezerve | ranije |
| Stari SPEC / EXCELSPEC | fizička količina, preflight izbora | ranije |
| Novi AutoCAD / Excel SPEC i menadžer | fizička količina i masa, preflight | ranije |
| COM Length | fizička dužina u mm, E_FAIL za neispravno | ranija mm semantika |
| KGTEZ | prima fizički COM Length; njegova rezerva ostaje njegova | ranije |
| Properties, masa/jed. i ukupna masa | fizički obračun; neispravan član odbija zbir | ranije |
| Količinski tekstovi oznaka | preko getCount/controller | ranije |

[L] u postojećem multileader-u označava protok, ne dužinu; ta semantika nije mijenjana.
Tabele koje već postoje treba ponovo generisati; nisu uvedeni reaktori za njihov live update.

Referenca 400x200 / Ri100 / deltaZ400 / 45°: projekcija=282.842712 mm,
riseStraight=282.842712 mm, Lphysical=400 mm, količina=0.4 m,
površina=0.48 m², debljina=0.6 mm, masa=2.2608 kg bez rezerve.

## Lokalni automatizovani testovi — stvarno izvršeni

Linux, Python 3.12.14, g++ 13.3.0, C++98 sa -Wall -Wextra -Werror -pedantic.
Komanda: `python -m unittest discover -s tests -p 'test_*.py' -v`.
Stvarni izlaz: `OFFSET_INTEGRATION_TEST_RESULTS.txt` (16 testova, OK, exit 0).
Novi testovi: skalarna pravila zapisa, nezavisni numerički količinski oracle,
2756 provjera projekcije/obračuna, stvarni izdvojeni predikat transform dozvole i
stvarni izdvojeni prefiks recipročne validacije sa API zamjenama. Provjera reflektuje
null/erased, izgubljen zapis, pogrešnu klasu, strani DB, duplirani ID i pokvarene reference.
To nisu ObjectARX testovi serijalizacije, COPY callback-a ili transakcija.
Postojeće regresije geometrije/adaptera, radijusa, KGTEZ, tabele, P i MOVE takođe prolaze.

## Windows buildovi

| Cilj | Okruženje / toolchain | SDK / konfiguracija | Stvarni run |
|---|---|---|---|
| AutoCAD 2020 x64 | GitHub windows-2022, MSBuild 15.9.30, VS2017 v141 / MSVC14.16.27023, MFC/ATL | ObjectARX2020, Windows SDK10.0.17763.0, Release x64 | [41113599791](https://github.com/vstijov-boop/tvs-vent/actions/runs/41113599791) |
| AutoCAD 2022 x64 | GitHub windows-2022, MSBuild17.14.60, VS2022 host sa v142 / MSVC14.29.30133, MFC/ATL | ObjectARX2022, WindowsTargetPlatformVersion10.0 iz postojećeg workflow-a, Release x64 | [41113599723](https://github.com/vstijov-boop/tvs-vent/actions/runs/41113599723) |

Stvarni završni izlaz CI-ja (sačuvan iz job loga):

```text
2026-10-30T19:15:52.1091465Z Microsoft (R) Build Engine version 15.9.30+gc0e4b0e683 for .NET Framework
2026-10-30T19:16:14.8840377Z Build succeeded.
2026-10-30T19:16:14.8859740Z     3 Warning(s)
2026-10-30T19:16:14.8859884Z     0 Error(s)

2026-10-30T19:18:35.7311231Z MSBuild version 17.14.60+43b635718 for .NET Framework
2026-10-30T19:19:16.7088144Z Build succeeded.
2026-10-30T19:19:16.7103210Z     3 Warning(s)
2026-10-30T19:19:16.7103500Z     0 Error(s)
```

Sva tri upozorenja po buildu su LNK4099: SDK rxapi.lib nema isporučeni rxapi.pdb;
nema preostalih grešaka kompajliranja/linkovanja ovog koraka. Potpuni logovi ostaju
među artefaktima odgovarajućeg run-a, zajedno sa binlog-om. To nije dokaz učitavanja u CAD.


Komande nakon iste postojeće pripreme SDK/MIDL/MFC u DEV workflow-ima:

```powershell
& $env:TVS_MSBUILD 'TVS_Ventilation 2020.sln' /m /t:Build /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0.17763.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
& msbuild 'TVS_Ventilation 2022.sln' /m /t:Build /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
```

Razvojni paketi (iz istog izvornog commita da3e2c2):

- [DEV AutoCAD 2020 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/41113599791/artifacts/13004248728), 438085 B; SHA256 458ec3c7e4a13cc8ee587854f57a36822290ab36956e4eb68597ce5e3f11febc.
- [DEV AutoCAD 2022 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/41113599723/artifacts/13005466192), 429606 B; SHA256 2033582140c9da669430499aa8a0b55dd544570240a8ff958fe2add85a1d315d.

Svaki ZIP: BUILD.txt, OFFSET_INTEGRATION_GUIDE.md, istorijski OFFSET_PROOF_GUIDE.md,
TVSTVS_Ventilation_DBX.dbx i TVSTVS_Ventilation_ARX.arx.
CI putanje: x64/Release/2020 odnosno x64/Release/2022; pakovanje dist-offset-dev.
Produkcioni bundle nije prepisan. Artefakti ističu 28.01.2027. prema GitHub metapodacima.

Oba stvarno preuzeta ZIP-a prošla su CRC provjeru; SHA256 odgovara GitHub digest-u.
PE machine=0x8664 (AMD64). Uvoz biblioteka: 2020 acdb23/AcGe23/ac1st23, 2022
acdb24/AcGe24/ac1st24; oba ARX-a uvoze odgovarajući TVSTVS_Ventilation_DBX.dbx.
BUILD.txt oba paketa potvrđuje da3e2c2 i razvojne flagove. Ove provjere nisu APPLOAD.


## Praktična provjera koja ostaje korisniku

Učitati DEV DBX pa ARX odgovarajuće godine u čistom AutoCAD procesu, iz zasebnog
foldera bez produkcionog TVS-a. Ne zamjenjivati produkcioni bundle. Upute i matrica:
`OFFSET_INTEGRATION_GUIDE.md`. Referentni prvi test: mm, osa -400, 400x200, k=0.5,
TVS_PIPE, prava dionica >1500 mm, Z / Na kotu 0 / 45°. Nastaviti crtanje pa Esc.
TVS_OFFSET_STATUS na srednjem komadu mora prikazati Valid, navedene količine i
zaseban status veza sa susjedima. COM Length očekivano 400 mm.

Obavezno popuniti matricu: interaktivni tok kroz oba pozivaoca; Cancel/NoChange;
premala visina/ulaz/90°; podizanje/spuštanje 30/45/60°; Undo/Redo i nastavak;
FAIL_NEXT rollback; snimanje/ponovno otvaranje; potpuni/djelimični COPY; brisanje;
nepoznati zapisi; površina/masa/sve tabele/COM/KGTEZ; mm/cm/m; pomjeren/rotiran UCS
u horizontalnoj ravni; stari objekti. Nagnuta crtačka ravan sa nehorizontanim ulazom
nije dio podržanog opsega. Posebno posmatrati krajnje konture nagibnog komada.

Nezavisni referentni brojevi za A×B=400×200, Ri=100 i |deltaZ|=400 mm:

| Ugao | P međukomada mm | Apsolutni riseStraight mm | Lphysical mm | Ukupni XY zahvat mm |
|---|---:|---:|---:|---:|
| 30° | 600.000000 | 346.410162 | 692.820323 | 800.000000 |
| 45° | 282.842712 | 282.842712 | 400.000000 | 565.685425 |
| 60° | 115.470054 | 200.000000 | 230.940108 | 461.880215 |

Pri spuštanju rise je negativan, dužine ostaju iste. Pri početnoj koti -400 mm,
završna kota za podizanje je 0, za spuštanje -800 mm. Pri prelasku na cm/m geometrijske
koordinate u crtežu dijele se sa 10/1000; COM Length, dijalog i dijagnostika i dalje
navode fizičke mm. Koristiti probni crtež pripremljen u odgovarajućim jedinicama;
ovaj korak ne obećava automatsko skaliranje postojećeg sklopa naknadnom promjenom
jedinica bez transformacije geometrije.

Format mjerenja: slučaj / očekivano / izmjereno / odstupanje u mm / prolaz-pad;
navesti AutoCAD, jedinice, režim i BUILD commit. Nijedna od ovih integracionih
praktičnih provjera nije predstavljena kao izvršena.

Dodatne otvorene provjere: Windows build bez flaga nije pokretan u ovom ciklusu
(izolacija pregledana u izvoru); performanse čitljive provjere GUID kolizije na velikoj
mreži; višedokumentni Undo/Redo i reload dodatka; stvarno trajanje/reda clone callback-a.
Numerička politika granice ostaje za odluku prije produkcije.

## Datoteke u integracionim commit-ima (prema stanju prije A)

```text
M	.github/workflows/dev-offset-2020.yml
M	.github/workflows/dev-offset-2022.yml
A	TVS_Ventilation_ARX/OffsetIntegrationCommand.h
A	TVS_Ventilation_ARX/OffsetIntegrationDiagnostics.h
A	TVS_Ventilation_ARX/OffsetIntegrationReactor.h
A	TVS_Ventilation_ARX/OffsetSelectionCheck.h
A	TVS_Ventilation_ARX/OffsetSessionState.h
M	TVS_Ventilation_ARX/SPEC.cpp
M	TVS_Ventilation_ARX/TVSCommandExcelSpecification.cpp
M	TVS_Ventilation_ARX/TVSCommandPropertyManager.cpp
M	TVS_Ventilation_ARX/TVSCommandSpecification.cpp
M	TVS_Ventilation_ARX/TVSMoveCommand.h
M	TVS_Ventilation_ARX/TVSPropertyController.cpp
M	TVS_Ventilation_ARX/TVSPropertyMassDyn.cpp
M	TVS_Ventilation_ARX/TVSPropertySpec.cpp
M	TVS_Ventilation_ARX/TVSTestOffset.h
M	TVS_Ventilation_ARX/TVS_Ventilation_ARX2020.vcxproj
M	TVS_Ventilation_ARX/TVS_Ventilation_ARX2022.vcxproj
M	TVS_Ventilation_ARX/acrxEntryPoint.cpp
M	TVS_Ventilation_DBX/ComPipe.cpp
A	TVS_Ventilation_DBX/OffsetAssemblyData.cpp
A	TVS_Ventilation_DBX/OffsetAssemblyData.h
M	TVS_Ventilation_DBX/TVS_Entity.cpp
M	TVS_Ventilation_DBX/TVS_Entity.h
M	TVS_Ventilation_DBX/TVS_Pipe.cpp
M	TVS_Ventilation_DBX/TVS_Pipe.h
M	TVS_Ventilation_DBX/TVS_TAP.cpp
M	TVS_Ventilation_DBX/TVS_Ventilation_DBX2020.vcxproj
M	TVS_Ventilation_DBX/TVS_Ventilation_DBX2022.vcxproj
A	development/OFFSET_INTEGRATION_GUIDE.md
A	development/USER_VALIDATION_2020.md
A	geometry/OffsetRecordRules.h
A	geometry/SlopeProjection.h
A	tests/test_offset_integration.cpp
A	tests/test_offset_integration.py
A	tests/test_offset_membership_guard.py
A	tests/test_offset_transform_guard.py
```

Uz ovu listu dodati su ovaj izvještaj i stvarni izlaz testova. Automatski obilazak,
novi dijalog presjeka i detalji nisu razvijani. Ovdje se staje za korisnički pregled.
