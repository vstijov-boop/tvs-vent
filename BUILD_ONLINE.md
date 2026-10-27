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

Aktuelan rezultat uvijek provjeriti na GitHub Actions stranici. Stari
instalacioni EXE fajlovi u `Installer` nijesu dokaz nove izgradnje.
