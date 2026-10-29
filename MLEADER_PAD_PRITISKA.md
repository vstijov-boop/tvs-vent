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
