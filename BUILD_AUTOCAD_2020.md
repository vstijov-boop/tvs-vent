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
