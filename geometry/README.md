# Izolovani proračun promjene kote — korak 13.5

Ovaj modul nema pozivaoce u aplikaciji. Ne uključuje ObjectARX, changeZ, DBX/COM,
specifikaciju, XRecord ili dijalog. Ne mijenja postojeće entitete niti njihov format.

## Interfejs

`OffsetGeometry.h`, namespace `tvsOffset`, samostalan C++98 header.

```cpp
Input in(deltaZmm, widthMm, bendSizeMm, innerRadiusMm, angleRadians, minimumStraightMm);
Result r = solve(in); // ili solve(in, eksplicitnaPolicy)
```

Input namjerno nema konstruktor bez argumenata: `minimumStraight` mora biti zadat.
Nema podrazumijevanog proizvodnog pravila od 100 mm. Lmin=0 je podržan; dopušta
spajanje tangentnih lukova bez fizičkog međukomada. Numerički ostatak oko nule može
dati veoma malu pozitivnu dužinu; ne dodaje se vještački minimum.

Unutrašnji Ri<0 se odbija. Ri=0 je dozvoljen granični slučaj glatkog kružnog
modela sa unutrašnjom konturom svedenom na tačku. R=Ri+bendSize/2 ostaje pozitivan.
To ne modeluje oštro mitersko/segmentno koljeno i ne potvrđuje proizvodnu izvedbu.

Modul računa dva jednaka kružna koljena i tangentni međukomad istog presjeka,
bez ravnih nastavaka. `width` je poprečna širina; `bendSize` visina u ravni savijanja.
Konture su četiri ugla pravougaonog presjeka na četiri spojne ravni. Konture
okruglog presjeka nisu implementirane; ni unos D×D nije kružna kontura.
Različiti radijusi para, segmentni tip, fleksibilni kanal, promjenljiv presjek i
nehorizontalni ulaz/izlaz nisu podržani modeli. Rezultat nema pune mreže površina
ni diskretizaciju lukova: centri, radijus, ugao i spojevi određuju lukove.

Geometrija zavisi samo od potpisane deltaZ i lokalnih dimenzija. Prvi tangentni
spoj je (s,z,t)=(0,0,0). Ne oduzimaju se dvije velike apsolutne WCS kote.
Odvojeni `place` prima lokalnu tačku, modelsku početnu tačku, smjer trase i faktor
mm/model-jedinici. Apsolutna početna kota utiče samo na pozicioniranje.
`plan` vraća (s,t), `profile` (s,z) iz iste tačke, bez nezavisnih formula.

## Rezultat

`hasGeometry()` je true samo za Ok. Ostali statusi ne vraćaju djelimičnu geometriju.

| Status | Značenje |
|---|---|
| Ok | Validne tačke axis[4], centri centers[2], corners[4][4], dužine |
| NoChange | Apsolutna deltaZ u granici dužinske tolerancije |
| InvalidInput | Nevažeći ulazi/politika, negativan Ri/Lmin, nepozitivni presjek ili ugao van domena |
| Unsupported90 | 90° ili unutar eksplicitnog ugaonog pojasa; nema zamjene sa 89,4° |
| TooShort | Nedovoljna visina za lukove ili izračunati fizički L<Lmin |
| ProjectionTooSmall | Projekcija ispod eksplicitnog policy.minimumProjection |
| NumericalFailure | Prelivanje računanja ili nekonačan rezultat |

`straightLength` je fizička dužina međukomada, `projectedLength` XY projekcija,
`totalRun` ukupni horizontalni zahvat, `arcLength` dužina JEDNOG luka ose.
Sve dužine su u mm; pretvaranje količine u metre je dijeljenje fizičke dužine sa 1000.
Ovdje nije povezan nijedan potrošač specifikacije.

## Tolerancije i odluke u odnosu na plan

Policy defaults: absoluteTolerance=0.001 mm, relativeTolerance=1e-10,
angleTolerance=1e-8 rad, minimumAngle=0.1°, minimumProjection=0 mm.
Sve su eksplicitno izmjenjivi parametri; ne predstavljaju proizvodnu politiku.

Skala relativne tolerancije je najveća lokalna dužina među |deltaZ|, R, širinom,
visinom i Lmin. Apsolutna WCS pozicija ne utiče na toleranciju. Hluk manji od epsilon
vraća NoChange. Geometrijski ulazi se prvo validiraju; Unsupported90 se provjerava
prije NoChange, pa deltaZ=0 uz 90° daje Unsupported90.

**Preciziranje predloga:** fizički Lmin je stroga donja granica, bez prihvatanja
nešto kraćeg komada u tolerancijskom pojasu. Negativan ostatak visine se odbija,
ne zaokružuje u nulu. Time se ne mijenja zadati deltaZ da bi se napravila geometrija.
Na samoj granici decimalnog ulaza moguće je odbijanje usljed posljednjeg bita;
to je dokumentovano, a ne prikriveno proizvoljnim pomjeranjem.

Minimum projekcije podrazumijevano je 0 da bi Lmin=0 mogao dati nulti međukomad.
Kasnija politika integracije može eksplicitno zadati npr. 0.1 mm i fizički Lmin.
Tačno 90° je nezavisno od toga uvijek odbijeno u ovom koraku.

## Pokretanje

```sh
PYTHONDONTWRITEBYTECODE=1 python tests/test_offset_geometry.py
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests -p 'test_*.py' -v
```

Python runner u privremenom direktorijumu zaista kompajlira i izvršava:

```sh
g++ -std=c++98 -Wall -Wextra -Werror -pedantic tests/test_offset_geometry.cpp -o TEST_BINARY
TEST_BINARY
```

Poznati primjeri imaju unaprijed upisane brojeve za 30/45/60°, tačke ose i konture
iz prihvaćenog nalaza i zamijenjene A/B dimenzije. Odvojeni invarijantni testovi
provjeravaju tangentnost, radijuse, rastojanja i ogledanje spuštanja. Granični testovi
Lmin koriste izračunati rezultat samo za metamorfni test praga, ne kao numerički
oracle za osnovni proračun. Jedinice/rotacije testiraju adapter place, ne AutoCAD UCS.

Stvarni izvještaj: `tests/OFFSET_GEOMETRY_TEST_REPORT.md`.
Windows/ObjectARX, COPY, Undo/Redo, snimanje/otvaranje i tabele nisu testirani ovim
izolovanim korakom. Ovdje nema ni podataka ni integracije na kojima bi se ti testovi
mogli izvesti. Prije integracije rezultat treba posebno pregledati i odobriti.
