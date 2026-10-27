# Online izgradnja za AutoCAD 2022 x64

Workflow: `.github/workflows/build-autocad-2022.yml`.
Radna grana: `srpski-prevod`. Glavna grana se ne mijenja.

## Namjena

GitHub Actions koristi Windows Server 2022, Microsoft MSBuild i alatni skup
v142. Potrebne MFC/ATL komponente instaliraju se na privremeni runner.
ObjectARX SDK 2022 preuzima se direktno sa Autodesk servera, provjerava se
Autodesk digitalni potpis i raspakuje se njegovim SFX alatom. SDK se ne
objavljuje u repozitorijumu ni u izlaznom paketu.

Izgradnja cilja `TVS_Ventilation 2022.sln`, `Release`, `x64`.
U radnoj kopiji runnera stari IDL import `axdb20enu.tlb` prilagođava se na
`axdb24enu.tlb` iz SDK-a, a MIDL putanje se dopunjavaju. Ova promjena se ne
upisuje u zajedničke IDL fajlove koji služe i za druge verzije AutoCAD-a.

## Pokretanje i rezultati

Relevantni commit-i na grani automatski pokreću workflow. Podržan je i
`workflow_dispatch`; dostupnost dugmeta za ručno pokretanje na GitHub-u
zavisi od toga da li GitHub prepoznaje workflow na podrazumijevanoj grani.
Za dosadašnje pokušaje korišćen je automatski `push` okidač.

Na stranici Actions, uspješna izgradnja treba da ponudi artefakt
`tvs-vent-srpski-autocad-2022-x64`, sa novim ARX/DBX fajlovima, prevedenim
CUIX menijima i oznakom izvornog commita. Logovi se čuvaju i nakon greške
u artefaktu `autocad-2022-build-logs`.

Uspješna kompilacija nije potvrda rada u AutoCAD-u. Izlazni paket služi za
probno učitavanje uz postojeću TVS instalaciju. Potrebno je provjeriti
učitavanje dodataka, dijaloge, Properties paletu, toolbarove i specifikacije.

## Evidencija početnog podešavanja

- Prvi pokušaj: 7-Zip je prepoznao samo PE omotač Autodesk paketa, bez SDK-a.
  Riješeno korišćenjem Autodesk SFX alata uz `-suppresslaunch -d`.
- Drugi pokušaj: SDK je preuzet, raspakovan i pronađena je biblioteka tipova.
  MSBuild je prijavio nedostajuće MFC biblioteke za v142 (MSB8041).
- Dodata je instalacija odgovarajućih v142 MFC i ATL komponenti na runneru.

## Prva uspješna izgradnja — 27.10.2026.

- Run: https://github.com/vstijov-boop/tvs-vent/actions/runs/40512583361
- Izvorni commit: `628efbdd712e85fd65bd16ce6d4ccf05e6a70f09`.
- Rezultat: **Build succeeded, 0 grešaka, 2 upozorenja**.
- Upozorenja su LNK4099 za nedostajući Autodesk `rxapi.pdb` (debug simboli).
- Napravljeni su `TVSTVS_Ventilation_ARX.arx` i `TVSTVS_Ventilation_DBX.dbx`.
- Paket: https://github.com/vstijov-boop/tvs-vent/actions/runs/40512583361/artifacts/12737743525
- ZIP SHA256: `ec2222554e35a8eb7b25ea13c76c2dc851836618c32a29f22bb4896b733b5a53`.

Paket sadrži oba nova dodatka, dva prevedena CUIX menija i BUILD.txt.
Namijenjen je **AutoCAD-u 2022, 64-bit**, uz postojeću instalaciju TVS-a.
Nije instalacioni EXE. Provjera u AutoCAD-u i Excelu još nije obavljena.
Ovaj pokušaj nije mijenjao obradu MLeader parametra `[P]`; njegovo povezivanje
s jediničnim padom pritiska u Pa/m ostaje zasebna ispravka.

Aktuelan rezultat uvijek provjeriti na GitHub Actions stranici. Stari
instalacioni EXE fajlovi u `Installer` nijesu dokaz nove izgradnje.
