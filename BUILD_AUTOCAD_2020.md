# Online izgradnja i proba — AutoCAD 2020 x64

Workflow: `.github/workflows/build-autocad-2020.yml`.
Grana: `srpski-prevod`.

Za AutoCAD 2020 koristi se ObjectARX SDK 2020 i VS 2017 alatni skup v141,
sa odgovarajućim MFC/ATL bibliotekama. Instalacija razvojnih alata i SDK-a
obavlja se na privremenom GitHub Windows runneru.

Rješenje: `TVS_Ventilation 2020.sln`, konfiguracija `Release / x64`.
Stari IDL import `axdb20enu.tlb` u kopiji na runneru mijenja se na
`axdb23enu.tlb` iz SDK-a 2020. Zajednički izvorni IDL fajlovi se ne mijenjaju.
SDK i Microsoft instalateri ne ulaze u izlazni ZIP paket.

## Zamjena u postojećoj TVS instalaciji

Zatvoriti AutoCAD i sačuvati cijeli folder
`C:\ProgramData\Autodesk\ApplicationPlugins\TVS.bundle` na lokaciju izvan
`ApplicationPlugins`, na primjer `C:\TVS_BACKUP\20261027\TVS.bundle`.

Iz uspješno napravljenog paketa za **2020 x64** zamijeniti:

| Fajl | Odredište unutar TVS.bundle\Contents |
| --- | --- |
| TVSTVS_Ventilation_ARX.arx | TVS_VENT\2020\win64 |
| TVSTVS_Ventilation_DBX.dbx | TVS_VENT\2020\win64 |
| TVS_VENT.cuix | TVS_VENT\Resources |
| TVS_TOOLS.cuix | TVS_TOOLS\Resources |

Oba binarna dodatka zamijeniti zajedno. Ostale fajlove postojeće instalacije
zadržati. Paket za 2022 koristiti isključivo za odgovarajuću verziju AutoCAD-a.

Poslije zamjene pokrenuti AutoCAD 2020 i isprobati prevod na kopiji DWG-a.
Ako ostanu stari natpisi menija, zatvoriti AutoCAD i preimenovati odgovarajuće
`TVS_VENT.mnr` i `TVS_TOOLS.mnr` u `.mnr.old`, pa ponovo pokrenuti program.

Za povratak na original zatvoriti AutoCAD i vratiti sačuvane fajlove.
Kompajliranje ne zamjenjuje praktičnu provjeru u AutoCAD-u i Excelu.
MLeader parametar `[P]` još nije implementiran; ovaj postupak samo gradi
trenutni izvorni kod prevoda.

## Prilagođavanja kompatibilnosti

- Dva poziva `setCOLORREF` u `TVS_Entity.cpp` zamijenjena su ekvivalentnim
  `setRGB(GetRValue(...), GetGValue(...), GetBValue(...))` pozivima. SDK 2020
  nema noviji metod; SDK 2022 podržava RGB alternativu.
- Samo u radnoj kopiji runnera prevedeni ARX `.rc` fajl prepisuje se kao
  UTF-16LE sa BOM-om, radi starijeg Windows resource compiler-a. Izvorni
  resurs u Git-u ostaje u UTF-8 kodiranju.

## Uspješna izgradnja — 27.10.2026.

- Run: https://github.com/vstijov-boop/tvs-vent/actions/runs/40519200341
- Izvorni commit: `419857ffbc9612b3c99dfc843debf9e13b76944c`.
- Rezultat: **0 grešaka, 2 upozorenja LNK4099** o Autodesk debug simbolima.
- ZIP: https://github.com/vstijov-boop/tvs-vent/actions/runs/40519200341/artifacts/12740736538
- ZIP SHA256: `ab2946444825f86ea9bc727c4ee805b5bf6edfc507d81ac901ecb39b4fc36ee8`.

Provjereni su ZIP integritet, x64 PE zaglavlja oba dodatka i njihove stvarne
zavisnosti od `acdb23.dll` za AutoCAD 2020. Potvrđeni su i prevedeni naslovi
sa srpskim dijakriticima u kompajliranom ARX resursu. Praktična proba u
AutoCAD-u i Excelu i dalje predstoji.

Izmjena RGB poziva prošla je i ponovnu izgradnju za AutoCAD 2022:
https://github.com/vstijov-boop/tvs-vent/actions/runs/40518456427
Ranije preuzeti paket za 2022 i dalje je namijenjen laptopu sa AutoCAD-om 2022.
