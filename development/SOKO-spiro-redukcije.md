# Spiro redukcije (okrugli kanali): dužine po katalogu SOKO

Uputstvo za crtanje koncentrične (R) i ekscentrične (RA) redukcije, te varijanti sa gumenom zaptivkom (RG, RAG).
Izvor: SOKO inženjering, katalog „Kanali za razvod vazduha v12", spiro program (poglavlje 2.4, redukcije R/RA i RG/RAG).

## Oznake

- **d1** je veći prečnik, **d2** manji prečnik (mm).
- **a** je dužina cilindričnog priključka (nastavka) na svakom kraju, zavisi od prečnika tog kraja.
- **Lc** je dužina konusa, mjerena **između** priključaka.
- **L** je ukupna dužina elementa: **L = a(d1) + Lc + a(d2)**.

## Formule

| Element | Dužina konusa Lc |
| --- | --- |
| R, RA (bez zaptivke) | Lc = 2 · (d1 − d2) + 100 mm |
| RG, RAG (sa gumenom zaptivkom) | Lc = 2 · (d1 − d2) + 50 mm |

Ukupna dužina: **L = a(d1) + Lc + a(d2)**.

## Dužina priključka a

| Prečnik | a (mm) |
| --- | --- |
| Ø80 – Ø224 | 40 |
| Ø250 – Ø355 | 55 |
| Ø400 – Ø630 | 75 |
| Ø710 – Ø900 | 100 |
| Ø1000 – Ø1400 | 115 |

## Priključak, sićka i ravni dio (25 mm)

Na crtežu iz kataloga svaki priključak ima sićku (pertlu) blizu konusa. Sićka je graničnik: susjedni kanal ide na priključak
**do sićke**. Dio između sićke i konusa je ravni dio **c ≈ 25 mm** (izmjereno na kataloškom crtežu R-400-300: oko 27 mm na
većem i 23 mm na manjem kraju; crtež nije kotiran, pa je 25 mm procjena, a ne podatak iz kataloga).

Zato se u crtežu (kanal siječe na sićki):

- dužina elementa u crtežu = **Lc + 2 · 25** (25 mm ravno sa svake strane konusa);
- proizvodna ukupna dužina L = a(d1) + Lc + a(d2) ostaje ista (za spisak materijala i naručivanje).

Primjer R-400-300: Lc = 300, dužina u crtežu 300 + 50 = **350 mm**, ukupno L = 75 + 300 + 55 = **430 mm**.
Vrijednost 25 mm treba potvrditi kod proizvođača ili montažera.

## Kako nacrtati (bočni pogled, osa horizontalno)

1. Nacrtati osu. Od početne tačke nacrtati priključak dužine **a(d1)**, visine d1 (pravougaonik d1 × a).
2. Od kraja tog priključka nacrtati konus dužine **Lc**: početak prečnika d1, kraj prečnika d2.
3. Na kraju konusa nacrtati priključak dužine **a(d2)**, visine d2.
4. Ukupna dužina od prvog do zadnjeg ruba je **L**.

**Koncentrična (R, RG):** obje kružnice na istoj osi, konus simetričan oko ose.

**Ekscentrična (RA, RAG):** isto, samo se osa manjeg priključka pomjera tako da je jedna strana (gornja ili donja)
**ravna**, tj. izvodnica konusa na toj strani je paralelna osi većeg priključka. Pomak osa je (d1 − d2) / 2.
Dužine Lc i L su iste kao kod koncentrične.

## Primjer

R-400-300 (d1 = 400, d2 = 300):

- a(400) = 75 mm, a(300) = 55 mm
- Lc = 2 · (400 − 300) + 100 = **300 mm**
- L = 75 + 300 + 55 = **430 mm**

RG-400-300: Lc = 2 · 100 + 50 = **250 mm**, L = 75 + 250 + 55 = **380 mm**.

## Šifra za naručivanje

`R-400-300`, `RA-400-300`, `RG-400-300`, `RAG-400-300` (oznaka, Ød1, Ød2).

## Standardni prečnici

Ø80, Ø100, Ø125, Ø140, Ø150, Ø160, Ø180, Ø200, Ø224, Ø250, Ø280, Ø300, Ø315, Ø355, Ø400, Ø450, Ø500, Ø560, Ø630, Ø710, Ø800, Ø900, Ø1000, Ø1120, Ø1250, Ø1400.

Katalog ne daje sve kombinacije: ispod je za svaki d1 spisak d2 koji postoje u katalogu.

## Kombinacije iz kataloga (d1 → d2) i dužina Lc

Lc u mm za R / RA. Za RG / RAG oduzeti 50 mm (u tabeli RG nema nekih redova koji postoje u R, vidi zadnji stupac).

| d1 | a(d1) | d2 koji postoje u R (Lc) | d2 samo u R, ne u RG |
| --- | --- | --- | --- |
| 140 | 40 | 125 (130) | — |
| 150 | 40 | 125 (150), 140 (120) | — |
| 160 | 40 | 125 (170), 140 (140), 150 (120) | — |
| 180 | 40 | 125 (210), 140 (180), 150 (160), 160 (140) | — |
| 200 | 40 | 125 (250), 140 (220), 150 (200), 160 (180), 180 (140) | — |
| 224 | 40 | 125 (298), 140 (268), 150 (248), 160 (228), 180 (188), 200 (148) | — |
| 250 | 55 | 125 (350), 140 (320), 150 (300), 160 (280), 180 (240), 200 (200), 224 (152) | — |
| 280 | 55 | 125 (410), 140 (380), 150 (360), 160 (340), 180 (300), 200 (260), 224 (212), 250 (160) | — |
| 300 | 55 | 125 (450), 140 (420), 150 (400), 160 (380), 180 (340), 200 (300), 224 (252), 250 (200), 280 (140) | — |
| 315 | 55 | 125 (480), 140 (450), 150 (430), 160 (410), 180 (370), 200 (330), 224 (282), 250 (230), 280 (170), 300 (130) | — |
| 355 | 55 | 150 (510), 160 (490), 180 (450), 200 (410), 224 (362), 250 (310), 280 (250), 300 (210), 315 (180) | — |
| 400 | 75 | 160 (580), 180 (540), 200 (500), 224 (452), 250 (400), 280 (340), 300 (300), 315 (270), 355 (190) | — |
| 450 | 75 | 200 (600), 224 (552), 250 (500), 280 (440), 300 (400), 315 (370), 355 (290), 400 (200) | — |
| 500 | 75 | 250 (600), 280 (540), 300 (500), 315 (470), 355 (390), 400 (300), 450 (200) | — |
| 560 | 75 | 300 (620), 315 (590), 355 (510), 400 (420), 450 (320), 500 (220) | — |
| 630 | 75 | 400 (560), 450 (460), 500 (360), 560 (240) | — |
| 710 | 100 | 450 (620), 500 (520), 560 (400), 630 (260) | — |
| 800 | 100 | 560 (580), 630 (440), 710 (280) | — |
| 900 | 100 | 710 (480), 800 (300) | — |
| 1000 | 115 | 800 (500), 900 (300) | — |
| 1120 | 115 | 900 (540), 1000 (340) | — |
| 1250 | 115 | 1000 (600), 1120 (360) | — |
| 1400 | 115 | 1250 (400) | 1250 |

## Napomene

- Formule i dužine priključaka su iz kataloga; tabela kombinacija je izvučena iz kataloških tabela masa.
- Ako tražena kombinacija d1/d2 nije u tabeli, formula daje dužinu, ali je element po katalogu nestandardan
  (provjeriti kod proizvođača).
- Gotov program (KGH Ducts) koristi iste formule za tipove SPIRO_R, SPIRO_RA, SPIRO_RG, SPIRO_RAG.
