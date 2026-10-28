# Masa lima kanala i fitinga

Grana: `unapredjenja-masa`, nastala iz provjerene grane `unapredjenja-jedinice`.

## Obračun

Jedan obračun u DBX-u služi Properties paleti, oznakama i svim TVS specifikacijama.
Ulazna geometrija pretvara se u fizičke milimetre prema jedinicama svog DWG-a.
Rezultati se ne upisuju kao trajna masa, već se računaju pri čitanju: promjena
presjeka, dužine, radijusa ili ugla daje novu masu. DWG format ostaje isti.

Preuzeto iz KGTEZ.lsp:

- Čelik, gustina 7850 kg/m³.
- Debljina prema najvećoj stranici svih relevantnih presjeka:
  do 500 mm uključivo: 0,6 mm; do 1000: 0,8 mm; do 1500: 1,0 mm; preko: 1,2 mm.
- Masa = površina [m²] × debljina [mm] / 1000 × gustina.
- Pravougaoni kanal: 2(a+b)L / 10⁶.
- Pravougaono koljeno: 2(a+b)(Ri+a/2)θ / 10⁶; θ u radijanima.
  Ri je stvarni unutrašnji radijus TVS objekta, a širina a pripada ravni savijanja.
  Ista formula kao KGTEZ, bez potrebe za kopiranjem i eksplodiranjem objekta.
- Pravougaoni prelaz: (a+b+a₂+b₂)L / 10⁶, aproksimacija iz KGTEZ-a.
- Pravougaona račva: [2(a+b)(a₂+2L)+2(a₂+b₂)L−a₂b₂] / 10⁶,
  sa oduzimanjem otvora ogranka kao u KGTEZ-u.

Ispravljena je stara formula površine pravougaonog koljena u SPEC.cpp.
Za najstariju komandu TVS_SpecificationOld uklonjen je zaseban obračun i koristi
se postojeći TVS_SPEC tok, uključujući njegov izgled i grupisanje tabele.
Tako ova komanda više ne može dati različitu površinu i masu.

KGTEZ računa samo pravougaone elemente. Za krute okrugle elemente TVS koristi
obim πD, radijus ose Ri+D/2 za koljeno, a za okrugle/mješovite prelaze postojeću
TVS aproksimaciju ekvivalentnim konusom. Račve koriste obime oba presjeka i otvor
ogranka. Ista usvojena tabela debljina primjenjuje se na najveći prečnik/stranicu.
To je usvojeni projektantski obračun; ne tvrdi se da ova tabela dokazuje
usklađenost sa određenom EN/DIN normom ili predstavlja tačan proizvodni raskroj.
Segmentirana koljena računaju se po glatkoj ekvivalentnoj površini.

Bez mase prirubnica, spojnica, vijaka, dodataka za šavove i bez rezerve.
Rezerva i ručni izbor debljine 2 mm ostaju samo u KGTEZ.lsp.
Fleksibilni kanali nemaju automatski obračun čeličnog lima; nova polja su prazna.
Postojeće ručno upisane mase blokova i opreme ostaju dostupne.

## Gdje se vidi

- Properties za kanal, koljeno, prelaz i račvu: `Površina lima, m²`,
  `Debljina lima, mm`, `Masa elementa, kg` — samo za čitanje.
  COM imena: `SheetArea`, `SheetThickness`, `SheetMass`.
- Upravljač TVS svojstvima: postojeća masa po jedinici količine i nova ukupna
  masa reda. Automatska masa ne može se ručno prepisati.
- TVS_SPECIFICATION / Excel: postojeći `[Масса]` znači kg po jedinici količine:
  kg/m za prave kanale, kg/kom za fitinge. Nova kolona `Masa, kg` prikazuje
  zbirnu masu reda. Ona se dodaje i ako sačuvani prilagođeni šablon nema masu.
  Originalni sačuvani šablon se ne prepisuje.
- Dodatni tokeni šablona: `[UkupnaMasa]`, `[Povrsina]`, `[Debljina]`.
- TVS_SPEC, stari Excel izvoz i TVS_SpecificationOld: nova kolona ukupne mase,
  sa zbirom za cijelu tabelu.
- Multileader: `[M]` kg cijelog elementa, `[A]` m², `[T]` mm.
  Primjer: `[D]\\PM=[M] kg`.

Kanali jednakog presjeka, ali različitih dužina, grupišu se po kg/m, uz zbir
stvarnih masa i površina. Koljena sa različitim radijusima/masama ne stapaju se
u isti red mase po komadu. Ne zaokružuje se prije sabiranja.

## Provjere i ugradnja

`tests/test_sheet_metal.cpp`: nezavisno razvijena površina koljena (dvije bočne
ploče i unutrašnji/spoljašnji pojas), pragovi debljina, dužine, uglovi, okrugli
presjeci, pretvaranje mm/cm/m i sabiranje masa.
`tests/test_sheet_metal_kgtez.py`: kompajlira C++98 obračun i poredi 56 primjera
sa izvršavanjem stvarnih numeričkih funkcija iz KGTEZ.lsp kroz testni interpreter.
`tests/test_sheet_metal_adapter.py` kompajlira stvarni DBX adapter sa malim
zamjenama za CAD tipove: provjerava polja, jedinice mm/cm/m, ravan savijanja,
okrugle/mješovite presjeke, najveću dimenziju ogranka i isključivanje fleksibilnih
kanala. CAD/COM prikaz i proizvodna tačnost aproksimacija time nijesu simulirani.

Poslije uspješnog Windows build-a zamijeniti ARX i DBX zajedno, uz zatvoren
AutoCAD i prethodni backup. Koristiti paket koji odgovara verziji AutoCAD-a.
KGTEZ i CUIX nije potrebno mijenjati za ovu funkciju.

Ručna provjera u AutoCAD-u (mm, cm i m):

1. Kanal 400×200, L=3000 mm: 3,6 m², 0,6 mm, 16,956 kg; u specifikaciji
   količina 3 m i masa po metru 5,652 kg/m.
2. Dodati isti kanal dužine 1000 mm: grupisani red 4 m i 22,608 kg.
3. Koljeno 400×200, Ri=150 mm, ugao 90°: 0,659734457 m² i 3,107349293 kg.
   Sa uglom 45° površina i masa su upola manji. Provjeriti i Gore/Nadolje.
4. Porediti sa KGTEZ bez rezerve i bez opcije 2 mm.
5. Promijeniti dužinu/ugao/radijus; provjeriti Properties i vezanu oznaku.
   Specifikacije su snimak izbora i moraju se ponovo generisati poslije izmjena.
6. Provjeriti sva tri CAD izlaza, oba Excel toka, stari sačuvani šablon i
   upravljač svojstvima; sačuvati/ponovo otvoriti DWG.
7. Provjeriti pragove 500/1000/1500 mm, račvu sa većim ogrankom, okrugli i
   mješoviti prelaz, fleksibilni kanal i blok sa ručno upisanom masom.
