# [P] — jedinični pad pritiska u oznaci kanala

Grana: unapredjenja-masa.

Token `[P]` u višestrukoj oznaci sada daje jedinični pad pritiska pravog kanala
u Pa/m, sa dvije decimale. `[V]` sada takođe ima dvije decimale (m/s). Ranije je grana za `[P]` vraćala uspjeh bez vrijednosti.

Koristi se postojeći TVS_Calculate::getI, sa istom osnovnom formulom kao svojstvo
kanala `i, Pa/m`. Dimenzije se pretvaraju u fizičke milimetre iz mm/cm/m DWG-a.
Za okrugli kanal koristi se prečnik, za pravougaoni obje stranice. Protok se
računa po apsolutnoj vrijednosti: kopije ulaza sprečavaju da kalkulator promijeni
smjer protoka sačuvan na objektu. Nulti protok daje 0.00. Neispravne dimenzije i
nebrojčane vrijednosti ne upisuju NaN/Infinity u oznaku.

Ovo nije ukupni pad pritiska u Pa niti lokalni gubitak koljena/račve/prelaza.
Za fitinge `[P]` ostaje prazan. Postojeći model trenja, gustina i hrapavost nijesu
mijenjani. Vrijednost nije provjera usklađenosti sa standardom niti novi proračun
cijele ventilacione mreže.

Primjer formata:

`[D]\Pi=[P] Pa/m`

Opšti format određuje nove oznake. Za postojeće oznake promijeniti njihovo
svojstvo `Format teksta` i pokrenuti `TVS_LEAD_Recalculate` nad njima.
Oznaka sa već sačuvanim `[P]` dobija vrijednost pri ponovnom preračunavanju.

Test `tests/test_mleader_pressure.py` kompajlira stvarnu granu `[P]` i postojeći
kalkulator uz zamjene za CAD tipove: okrugli/pravougaoni kanal u mm/cm/m, protok
720.5 / -720.5 / 0, očuvanje podataka objekta i odbacivanje neispravnih ulaza.
CAD prikaz i interakcija zahtijevaju provjeru u AutoCAD-u.

Za ugradnju zatvoriti AutoCAD, napraviti backup i zamijeniti ARX i DBX zajedno
iz paketa za odgovarajuću verziju. KGTEZ i CUIX ostaju iz prethodne provjerene
verzije. Sačuvane funkcije razmjere tabela, mase i zbira izbora ostaju uključene.

Kontrolni primjer prema postojećem TVS modelu: 400×200 mm i 720 m³/h daju
`[V]=2.50` m/s i `[P]=0.32` Pa/m, jednako u mm/cm/m. Za oba tokena
prikazuju se tačno dvije decimale.

## Windows paketi — 29.10.2026.

Kod paketa: `85894ef5e10c54851b169a0ba4a4506c434bcdd7`.
Obje Release x64 kompilacije: 0 grešaka i po 2 SDK PDB upozorenja LNK4099.
ZIP CRC, PE x64, acdb23/acdb24 importi i novi tekst pomoći su provjereni;
KGTEZ i oba CUIX fajla odgovaraju prethodnoj verziji. Prikaz novih [P]/[V]
oznaka u AutoCAD-u još treba potvrditi kod korisnika.

- [AutoCAD 2020 paket](https://github.com/vstijov-boop/tvs-vent/actions/runs/40828785931/artifacts/12877428023)
  SHA256 `393c2dfe974d09a71700541eac34b3641e5e1932c9868e8dd56663b1a1b33cbd`
- [AutoCAD 2022 paket](https://github.com/vstijov-boop/tvs-vent/actions/runs/40828785910/artifacts/12878240832)
  SHA256 `7ed15b8324d80b67fc3c6284b83fc6a35e8d5af5b123d8edd4fe6a471fb5d4c8`
