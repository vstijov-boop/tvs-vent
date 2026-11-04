# Razvojni ObjectARX dokaz — build i granice provjere

Datum: 30.10.2026. Grana: `razvoj-offset-arx`, odvojena od `unapredjenja-masa`.
Izvor binarnih fajlova: `87c82e17a7f3cd281f3c6ce325f15293889c080a`.
Početna implementacija: `899629cab48837f00cef3dc550918c54c0461489`.
Kompletni izlaz regresija: OFFSET_PROOF_REGRESSIONS.txt.

## A/B/C — stvarno stanje

| Provjera | Rezultat |
|---|---|
| A. Windows kompajliranje i linkovanje 2020 | Uspješno, 0 grešaka, 3 LNK4099 upozorenja za SDK rxapi.pdb |
| A. Windows kompajliranje i linkovanje 2022 | Uspješno, 0 grešaka, 3 LNK4099 upozorenja za SDK rxapi.pdb |
| B. Učitavanje DBX/ARX u AutoCAD | NIJE IZVRŠENO: dostupno izvršno okruženje je Linux bez AutoCAD-a; CI runner gradi pomoću SDK-a |
| C. Izvršavanje komande, REGEN/MA/MC, izgled/OSNAP, Undo/Redo/rollback | NIJE IZVRŠENO: potrebna korisnikova praktična provjera |

Uspješan build nije dokaz runtime geometrije niti atomskog Undo/rollback-a.

## Datoteke

Dodate:
- TVS_Ventilation_ARX/TVSTestOffset.h
- .github/workflows/dev-offset-2020.yml
- .github/workflows/dev-offset-2022.yml
- development/OFFSET_PROOF_GUIDE.md
- development/OFFSET_PROOF_BUILD_REPORT.md
- development/OFFSET_PROOF_REGRESSIONS.txt

Izmijenjene:
- TVS_Ventilation_ARX/acrxEntryPoint.cpp: samo ifdef include, metode i registracije tri DEV komande.
- TVS_Ventilation_ARX/TVS_Ventilation_ARX2020.vcxproj: uslovni flag i prikaz headera u projektu.
- TVS_Ventilation_ARX/TVS_Ventilation_ARX2022.vcxproj: isti razvojni uslov.

Nijedna DBX datoteka, geometrijski modul/adapter, COM ili specifikacija nisu mijenjani.
Potvrđena byte-identičnost OffsetGeometry.h i OffsetAdapter.h sa prihvaćenim 4d31593;
changeZ tekstualno identičan; produkcioni workflow-i byte-identični.

## Toolchain, SDK i stvarne komande

Oba: GitHub Actions windows-2022 runner, Release x64, službeni ObjectARX SDK
preuzet/raspakovan postojećom verifikovanom procedurom, bez promjene ciljnih verzija.

| Cilj | Toolchain | SDK |
|---|---|---|
| AutoCAD 2020 x64 | MSBuild 15, v141 / MSVC 14.16.27023, MFC/ATL | ObjectARX 2020, WindowsTargetPlatformVersion=10.0.17763.0 |
| AutoCAD 2022 x64 | VS2022 MSBuild, v142 / MSVC 14.29.30133, MFC/ATL | ObjectARX 2022, WindowsTargetPlatformVersion=10.0 |

Izvršene PowerShell komande u pripremljenom CI checkout-u:

```powershell
& $env:TVS_MSBUILD 'TVS_Ventilation 2020.sln' /m /t:Build /p:TVSOffsetProof=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0.17763.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
& msbuild 'TVS_Ventilation 2022.sln' /m /t:Build /p:TVSOffsetProof=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
```

TVSOffsetProof=true uslovno dodaje TVS_OFFSET_PROOF samo ARX kompilaciji. Bez flaga
include i sve tri registracije ne postoje u kompajliranoj jedinici. U produkcionim
workflow-ima flag nije dodat. Nije pravljen novi produkcioni paket radi ovog testa.

Sirovi outputs runnera: x64/Release/2020 i x64/Release/2022; DEV packaging direktorijum
je dist-offset-dev. Runner je izolovani checkout, ne korisnikov instalirani bundle.

## Otklonjena greška ovog koraka

U prvim pokušajima postojao je suvišan/pogrešan include dbtext.h. Stvarni izlaz:

```text
tvstestoffset.h(7): fatal error C1083: Cannot open include file: 'dbtext.h': No such file or directory
```

Uklonjen u 87c82e1: AcDbText je dostupan kroz dbents.h u ciljnom SDK-u.
U međukoraku fc197a4 eksplicitno inicijalizovana postojeća filed polja probnih
entiteta, čiji konstruktori sami to ne rade. Konačni oba builda gore su uspješna.
Nije mijenjan SDK/toolset niti rađena nepovezana produkciona popravka.

## Razvojni artefakti i provjera preuzimanja

- [DEV AutoCAD 2020 ZIP](https://github.com/vstijov-boop/tvs-vent/actions/runs/41063691962/artifacts/12981766739)
  406334 bytes, SHA256 `bb07564705f02fd6403e1075c51ac0e621e7bf746ddb08355a94a5b97414ff18`.
  ARX 688128 bytes, DBX 394240 bytes.
- [DEV AutoCAD 2022 ZIP](https://github.com/vstijov-boop/tvs-vent/actions/runs/41063692350/artifacts/12980923233)
  402082 bytes, SHA256 `d093a3b4f0965efc4d3cb7dfdcf9f00ce90666d1f51ccd61e5a15ac6da0380be`.
  ARX 657408 bytes, DBX 419840 bytes.
- [2020 logs](https://github.com/vstijov-boop/tvs-vent/actions/runs/41063691962/artifacts/12982251290)
- [2022 logs](https://github.com/vstijov-boop/tvs-vent/actions/runs/41063692350/artifacts/12981147956)

Oba ZIP-a stvarno preuzeta i provjerena: ZIP CRC, SHA256, x64 PE (machine 8664,
PE32+), import acdb23.dll / acdb24.dll, BUILD commit/flag, sve tri komande i upozorenje
u ARX-u, sadržaj uputstva prema izvoru uz normalizaciju CRLF-a.
Sadrže samo ARX, DBX, BUILD.txt i OFFSET_PROOF_GUIDE.md. Bez produkcionih CUIX/LISP
fajlova, bez pisanja u postojeći produkcioni paket.

## Testovi koji su zaista izvršeni

Linux x86_64, g++ 13.3.0, Python 3.12.14. Izolovani C++ programi koriste
-std=c++98 -Wall -Wextra -Werror -pedantic. Komanda:

```sh
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests -p 'test_*.py' -v
```

Rezultat: svih 13 Python testova prošlo; unutar njih 28641 provjera adaptera i
589 provjera geometrijskog modula. Stvarni stdout/stderr i vrijeme izvršavanja
sačuvani su u OFFSET_PROOF_REGRESSIONS.txt. Ovo nisu testovi stvarnog ObjectARX-a.

## Učitavanje, pokretanje i mjerenje

Detaljno u OFFSET_PROOF_GUIDE.md (nalazi se i u ZIP-u). Obavezno novi čist AutoCAD
proces bez automatski učitanog produkcionog TVS-a; DEV paket van ApplicationPlugins.
APPLOAD prvo DBX, zatim ARX iste godine. Ne prepisivati postojeći bundle.
Prazan probni DWG, Model, jedinice odabrati prije kreiranja kroz TVS_SETTINGS.

1. TVS_TEST_OFFSET → Kontura/Maska/Osa. Kreira 12 slučajeva, 24 TAP-a i 216 ukupnih
   testnih objekata. Navedeni broj je prema kodu; stvarni runtime broj tek izmjeriti.
2. ZOOM Extents, REGEN, TVS_TEST_OFFSET_CHECK. Očekivano 24/24 PASS za MA/MC uz
   toleranciju 0.001 mm. Ovo je očekivanje za korisnikovu provjeru, ne izvršen rezultat.
3. Ponoviti REGEN/CHECK; upisati rezultate. Vizuelno provjeriti konture/skrivene
   linije/wipeout i OSNAP. Numerički PASS ne potvrđuje automatski ove stavke.
4. UNDO → CHECK: očekivano 0 živih objekata; REDO/REGEN/CHECK: očekivan povratak svih.
5. Novi prazan DWG: TVS_TEST_OFFSET_FAIL. Očekivan prazan model poslije aborta.
6. Ponoviti za mm/cm/m i pomjeren/rotiran UCS, pa na drugoj CAD verziji.

Obrazac: CAD/BUILD SHA/jedinice/UCS/režim/REGEN broj, zatim
slučaj / mjera / očekivano / izmjereno / odstupanje mm / prolaz-pad.
Popunjiv obrazac i očekivane dužine za 30/45/60° su u vodiču.

## Ograničenja i stop

Komande rade samo u praznom modelu; postojeća geometrija se ne mijenja. Manifest
mjerenja je memorijska sesija, ne članstvo sklopa. CHECK dijagnostički čita redraw
MA/MC isključivo poslije korisnikovog REGEN-a; konstrukcija i solve ne zavise od keša.
Referentna fizička kota je -400 mm, završna 0/-800 mm; crtački Z=37 mm nezavisan.
Nagibni TVS_Pipe i dalje ima XY obračun: nije za specifikaciju. Nema nove korisničke
politike Lmin=100 ili minimumProjection=0.1; probni solve koristi eksplicitni Lmin=0.

Test koristi postojeće klase i standardni BTR append + transakciju, a ne add_new
helper koji interno postuje i zatvara objekat. Stvarni Undo/Redo i abort moraju
biti potvrđeni u CAD-u. Postojeća grafika pojedinih AcDbPolyline/wipeout djelova može
imati svoje ravanske konvencije; tek praktična provjera potvrđuje njen izgled/Z.

Šema B je predlog. Razlikovanje COPY pozicioniranja i zabranjenih transformacija,
kao i unutrašnje valjanosti sklopa i njegove veze sa susjedima, nije implementirano.
Nema XRecord-a, kloniranja, zaštite editovanja ili nove fizičke dužine u obračunu.
Nakon ove isporuke stati prije povezivanja sa changeZ.
