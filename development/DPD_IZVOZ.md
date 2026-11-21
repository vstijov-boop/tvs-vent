# TVS_DPD_IZVOZ – mreža kao sistem za DuctPressDrop

Komanda: `TVS_DPD_IZVOZ` (`TVS_Ventilation_ARX/DpdExportCommand.h`), čista logika `geometry/DpdExport.h`,
testovi `tests/test_dpd_export.cpp` + `tests/test_dpd_export.py`. U crtež se ništa ne upisuje.

## Tok
1. Kraj mreže kod jedinice: jedan unaprijed izabran element sa tačno jednim slobodnim krajem, inače klik
   blizu slobodnog kraja (kao TVS_PAD / TVS_PROTOK). Protoci se sabiraju od krajnjih elemenata (u memoriji).
2. `Sistem [Dovod/Odvod]` (pamti se do kraja sesije).
3. Dionice kao u TVS_PAD (`tvsPressure::Calculator`, bez ASHRAE podataka), pa `tvsDpd::segments` i `tvsDpd::json`.
4. Snimanje: `acedGetFileD` (json, pitanje za prepis), podrazumijevano `<crtež>_Dovod.json` / `_Odvod.json`;
   UTF-8 bez BOM-a.

## Format (DuctPressDrop „Uvezi sistem…“, Ctrl+I – `services/system_import.py`)
Ravni (flat) format, isti kao Revit/Dynamo izvoz DuctPressDrop-a; u DuctPressDrop-u ništa ne treba mijenjati.

```json
{
  "naziv": "Dovod crtez", "projekat": "crtez.dwg", "inzenjer": "", "izvor": "TVS_VENT TVS_DPD_IZVOZ",
  "temp_vazduha": 20, "sig_factor_pct": "10",
  "dionice": [
    {"id": "D-01", "opis": "TVS dionica 1 (1/2)", "cvor_od": "J", "cvor_do": "KO1", "tip": "P", "sistem": "supply",
     "Q": 3600, "W": 600, "H": 300, "D": 0, "L": 5.6, "smjer": [5.6, 0, 0],
     "materijal": "Pocinkovani čelični lim", "fitinzi": []}
  ]
}
```

- `naziv` = ime novog sistema („Dovod“/„Odvod“ + ime crteža). `izvor` DuctPressDrop ignoriše.
- `sistem`: `supply` (Dovod) / `extract` (Odvod); id `D-01…` / `O-01…`.
- `Q` m³/h (DuctPressDrop: v = Q/3600/A), `W`×`H` ili `D` u mm (`tip` P / K; W = SizeA, širina u osnovi).
- `smjer` [dx, dy, dz] u m od `cvor_od` ka `cvor_do` (E = +x, N = +y, U = +z); crtež → mm preko
  `tvsUnits::factor`, pa /1000. OD → DO = od jedinice (i za odvod; DuctPressDrop sam okreće tok za extract).
- `L` = |smjer|: DuctPressDrop uvijek računa L iz smjera (osa između čvorova, fitinzi uključeni), pa L nije
  „samo nacrtani kanali“ kao u TVS_PAD-u.
- `materijal`: lim „Pocinkovani čelični lim“, fleks „Fleksibilni izolovani“ (nazivi iz MATERIAL_PRESETS).
- Brojevi uvijek sa tačkom (i uz lokal sa zarezom), stringovi JSON-escape, UTF-8.

## Geometrija
- Jedna DuctPressDrop dionica = jedan PRAVI komad. TVS dionica sa koljenima u osnovi dijeli se na tjemenima
  koljena (`MiddlePoint` TVS_TAP-a = presjek osa), novi čvorovi `KO1, KO2…`; opis „TVS dionica n (k/m)“.
- Čvorovi: imena iz TVS_PAD (J jedinica, T T komad, P promjena presjeka/vrste, K kraj). Čvor T komada je na
  osi prolaza (projekcija priključka ogranka), pa prolaz ostaje prav, a ogranak okomit.
- Koljeno gore / dolje i etaž / obilazak: bez loma u osnovi (prava linija između krajeva).
- Samo osnova: dz = 0 (krajevi TVS kanala nemaju pouzdanu kotu u z).
- Komad kraći od 1 mm se izostavlja, njegov krajnji čvor se spaja sa početnim (upozorenje).

## Šta se ne izvozi
- Fitinzi: DuctPressDrop ih izvodi iz geometrije (koljena, T komadi, prelazi; `services/izvedeni_fitinzi.py`).
- Krajnji elementi (rešetke, difuzori, ventili): nisu u TVS mreži kao objekti – dodaju se u DuctPressDrop-u.
- Kote / vertikale (dz), etaže, temperatura (20 °C, mijenja se u DuctPressDrop-u).

## Round-trip test
`tests/test_dpd_export.py` pravi uzorak (mreža iz test_pressure_drop: koljeno, TK-O, prelaz, fleks), učita ga
`parse_system_file` iz DuctPressDrop-a (`TVS_DPD_APP=…/hvac_app` ili podrazumijevana putanja; bez njega se
preskače) i provjerava Q / dimenzije / L / smjer, zatvaranje geometrije (`izracunaj_geometriju`) i – uz
PySide6 – `ProjectService.dodaj_uvezeni_sistem`: izvedeno koljeno u KO1, T komad u T1, prelazi u P1 / P2.
