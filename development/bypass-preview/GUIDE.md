# TVS_BYPASS_PREVIEW — DEV, bez DWG upisa

Učitati odgovarajući DEV DBX, zatim ARX za AutoCAD 2020 ili 2022; ne miješati verzije. Komanda je uključena samo sa /p:TVSBypassPreview=true (TVS_BYPASS_PREVIEW). Bez flaga nijesu uključeni ni command ni numeric bridge.

1. U Model prostoru izabrati pravi horizontalni kruti pravougaoni TVS kanal nove trase, zatim drugi takav TVS kanal kao prepreku. Samostalni objekti, bez XREF/blokova ili označenih etažiranja. Oba su samo reference i otvaraju se read-only.
2. Izabrati Prvi/Posljednji aktivni kraj. Smjer +s ide ka tom kraju. s=0 je lokalni presjek stvarnih WCS osa; negativna/pozitivna granica prozora je fizička dužina u mm od tog presjeka. Početni predlog granica dolazi sa krajeva izabrane trase. Granice se mogu ručno promijeniti — izabrana trasa se ne skraćuje.
3. Jedinice: provjeriti TVS zapis ili izabrati ručno kada nedostaje/ispravan zapis nije dostupan. Izbor mijenja samo razvojni ulaz, ne DWG jedinice ili globalne postavke. Osne tačke i fizička kota nijesu ponovo korigovane za A/2 ili B/2. Crtački Z je odvojen od fizičke kote.
4. Unijeti obje izolacije (nedostaju dok se ne unesu, eksplicitno 0 kada ih nema). Provjeriti/potvrditi Ri, clearance, projektnu rezervu, minimume komada/projekcije, prozor i sve javne budžete. Gornja/donja granica je apsolutna fizička kota spoljnih gabarita; prazno polje izričito znači neaktivno. Izabrati 30/45/60°, Ispod/Iznad i potvrditi ručne podatke.
5. Pronađi prolaz staje na prvom potvrđenom kandidatu. Pokušaj kraći prolaz nastavlja ograničenu pretragu i za potpuno iste ulaze čuva prethodni bolji potvrđeni rezultat. Minimum se ne potvrđuje. Promjena bilo kog ulaza uklanja prethodni rezultat i onemogućava pregled dok se ponovo ne računa.
6. Presjek u dijalogu ima istu skalu s/z; konture i spojne tačke dolaze iz istog checked Result-a. Sivo označava izolaciju, boje lim, crveno prepreku. Lukovi su vizuelno teselirani sa hordinom greškom do .05 mm; taj prikaz se NE koristi za dokaz kolizije. Projekcija u osnovi prikazuje projektovane gabarite sedam tijela i spojne presjeke; njihovo međusobno preklapanje u osnovi je očekivano i nije fizička kolizija. To nije novi trajni TVS prikaz niti skrivenolinijsko renderovanje.
7. Pregled u crtežu zatvara modalni dijalog i prikazuje isključivo transient grafiku. Enter uklanja grafiku i vraća dijalog; Esc uklanja grafiku i završava. Nema entiteta upisanih pa obrisanih. Otkaži, zatvaranje i greška takođe ne mijenjaju objekte/stanje trase. Umetni je nedostupan.

## Numerika i odziv

BypassNumericBridge.cpp je odvojena compilation jedinica: /fp:strict, /GL-, bez ARX/MFC PCH, C++11 izvor (/std:c++14 na MSVC). Ostali ARX/DBX flagovi ostaju isti. Nepodržani FE/FTZ/DAZ uslovi se odbijaju, ne normalizuju. Računanje je sinhrono samo nakon pritiska tastera, bez worker niti i CAD API-ja iz numerike. Tokom računanja UI čeka; nema obećanja trenutnog otkazivanja. Rok se provjerava između checker poziva i jedan aktivan poziv može prekoračiti rok. Razvojni UI ograničava javne budžete: najviše 10000 ms, 128 checker poziva i 1000000 ćelija; nivoi i ostali limiti su vidljivi.

## Ručna AutoCAD matrica — NIJE izvršena u Linux/CI okruženju

Prije/poslije svake probe brojiti Model-space entitete, npr. (sslength (ssget "_X" '((410 . "Model")))); za prazan izbor koristiti 0. Takođe uporediti položaje/veličine/Properties oba izabrana kanala, DWGMOD i postojeći režim vođenja. Test raditi u kopiji DWG-a.

- Oba AutoCAD izdanja, mm/cm/m; WCS i pomjeren/rotiran UCS. A!=B. Sve tri ugla iznad/ispod; izolacija 0 i 25 mm.
- Izbor Prvi i Posljednji u 0°,37°,90° i obrnutom smjeru. Presjek, window s=0 i fizička kota ne smiju dobiti dupli pomak.
- Odbijanje kružnog/fleksibilnog/kosog kanala, kose prepreke, djelimičnog poprečnog preklopa i istog objekta dvaput. Bez djelimičnih izmjena.
- Pronađi / Pokušaj kraći, uski/asimetrični prozor, aktivne visinske granice, premali budžet i numerička granica. NoCandidate nije dokaz nemogućnosti.
- Promijeniti svaki unos poslije Found: status i presjek se moraju poništiti, pregled mora biti onemogućen. Promijeniti jedinice i provjeriti predložene granice prozora.
- Pregled: Enter nazad; ponoviti više puta; Esc i Otkaži/zatvaranje. REGEN/zoom za vrijeme privremenog pregleda gdje AutoCAD dozvoljava transparentni poziv. Nema trajnih entiteta ili ostataka grafike.
- Ponoviti komandu, otkazati pri oba izbora i pri pitanju o smjeru. Broj DWG entiteta prije/poslije mora ostati identičan. Nema novih podataka, specifikacije ili izmjene changeZ().

Praktični rezultat zapisati: verzija / jedinice / UCS / ulazi / status / vizuelni presjek / preview / broj entiteta prije-poslije / Esc-Cancel / prolaz-pad.

Snimci stvarnog AutoCAD dijaloga nijesu napravljeni bez dostupnog AutoCAD runtime-a. Izolovani i build testovi nijesu zamjena za ovu matricu.

## Revizija čitljivosti v2

Kote i Ose uključuju/isključuju samo grafičke oznake. Napredno / dijagnostika prikazuje razvojne parametre i puni rezultat, dok osnovni prikaz čuva projektantske ulaze i sažetak. Donji tasteri ostaju na stalnim mjestima. Režimi Konture / Sve ivice su izričito DIJAGNOSTIČKI, BEZ zaklanjanja; crtice su simbolika spojeva/pomoćnih gabarita, a ne skrivene ivice. Originalna trasa ostaje vidljiva. Detalji i matrica revizije: development/bypass-preview/UI_READABILITY.md u repozitorijumu.
