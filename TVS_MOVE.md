# TVS_MOVE — prvi korak povezanog pomjeranja

Status 29.10.2026: korisnik je potvrdio da prva verzija TVS_MOVE dobro radi.
Dodata je dorada za izbor ivice; njena radna provjera u AutoCAD-u još predstoji.
Matematički i topološki testovi prolaze. Novi paketi uključuju
i početne radijuse 0,5; prethodni UCS paketi ih nemaju.

## Upotreba i ponašanje

1. `TVS_MOVE` — izabrati krutu pravu TVS dionicu između dva koljena u osnovi.
   Kliknuti blizu ivice koja treba da dođe na novu poziciju.
2. Na upit `Referenca pomjeranja [Izabrana/Druga/Osa] <Izabrana>:` pritisnuti
   Enter za ivicu bližu prvom kliku, ili zadati `Druga` (`D`) / `Osa` (`O`).
3. Zadati novu poziciju potvrđene reference. Ivica je pola širine od ose;
   uzdužna i vertikalna komponenta unosa se ne koriste. Gumena linija polazi
   od potvrđene ivice ili ose. Izbor reference ne mijenja geometriju prije tačke.
4. Oba koljena klize duž osa susjednih kanala, njihovi udaljeni krajevi ostaju
   fiksni. Presjeci, radijusi i uglovi koljena ostaju isti.

Za dva koljena od 90° izabrani kanal zadržava dužinu. Za kosa koljena njegovi
krajevi klize različito pa se može promijeniti i njegova dužina, dok pravac
ostaje paralelan prethodnom. Minimalna preostala dužina svake od tri dionice
je 1 mm; prelazak preko fiksnog kraja i obrtanje dionice nisu dozvoljeni.

Prepoznaje se niz: susjedni kanal — koljeno — izabrani kanal — koljeno — susjedni
kanal. Radi nad stvarnim krajevima u trenutnom prostoru, uključujući postojeće
crteže. Tolerancija kontakta je 0,1 mm u fizičkim jedinicama, jednaka za mm/cm/m.
UCS unos koristi prethodnu ispravku UCS ↔ WCS.

Masa i dužine koriste izmijenjenu izvornu geometriju. Postojeće TVS MText
multileader oznake vezane za ovih pet elemenata dobijaju novi sadržaj i položaj
strelice; tekst ostaje na svom mjestu. Strelica susjednog kanala se po potrebi
ograničava na preostalu dužinu. Zaključana povezana oznaka prekida cijelu izmjenu.
Statične ranije nacrtane specifikacije se ne precrtavaju automatski.

## Ograničenja prve verzije

- Samo kruti kanali i koljena u zajedničkoj svjetskoj XY ravni, sa odgovarajućim
  presjecima i elevacijom. Okrugla koljena sa konstantnim ili proporcionalnim
  radijusom jesu obuhvaćena. Segmentna okrugla koljena sa posebnim ВСН radijusom,
  fleksibilni elementi i koljena gore/nadolje nisu obuhvaćeni.
- Račve, prelazi i bočni priključci nisu elementi koji se pomjeraju ovim korakom.
  Dvosmisleni/duplirani spojevi i lokalne petlje se odbijaju. Udaljeni krajevi
  susjednih kanala smiju nastavljati ostatak mreže: ostaju na istom mjestu.
- Detekcija drugih priključaka nije provjera sudara kompletnog tijela kanala sa
  drugim kanalima, građevinom ili opremom. Prije prihvatanja pregledati raspored.
- Obični AutoCAD MOVE i gripovi još ne prilagođavaju povezane elemente.
- Postojeći eksperimentalni `TVS_Connector` nije aktiviran: njegovo pisanje/čitanje
  u kanalu je isključeno u izvornom kodu. TVS_MOVE gradi radne veze iz geometrije
  pri svakom pozivu, bez novih trajnih veza i promjene formata DWG-a.
- XREF i elementi unutar blokova nisu direktno dostupni ovoj komandi.

## Atomicitet i Undo

Komanda je modalna, a svih pet elemenata i povezanih oznaka mijenjaju se unutar
jedne AutoCAD transakcije. Neuspjeh otvaranja, zaključan sloj, nepodržana geometrija
ili greška oznake vraćaju izmjenu kroz `abortTransaction`. Cilj je jedan UNDO za
cijelu komandu; to obavezno provjeriti u stvarnom AutoCAD-u, zajedno sa REDO.

## Provjere prije sljedeće distribucije

`tests/test_move_geometry.cpp` kompajlira stvarni matematički header i prolazi.
Pokriva mm/cm/m,
rotacije i pomjerene koordinate, obrnuti redosljed krajeva, prava i kosa koljena,
skraćivanje preko kraja, obrtanje centralnog kanala, 3D unos i nevažeće vrijednosti.

`tests/test_move_topology.py` kompajlira stvarne funkcije za prepoznavanje
kontakata, presjeka i bočnih priključaka. Provjerava duple kontakte, toleranciju
u fizičkim jedinicama, elevaciju i razliku presjeka. Prolazi svih osam Python
testova i sedam samostalnih C++ test-programa.

U AutoCAD-u 2020/2022 još provjeriti
pravougaoni i okrugli niz sa 90° i 45° koljenima, obje orijentacije elemenata,
ponovljena pomjeranja, SAVE/ponovno otvaranje, UCS World/rotiran/pomjeren, mm/cm/m,
masu i oznake, UNDO/REDO, Esc, zaključane slojeve/oznake, duple kontakte, prekratke
dionice, račve/prelaze i XREF. Testirati i stare crteže bez sačuvanih konektora.

## Paketi prve verzije — korisnik potvrdio rad

Izvor paketa: `79981e451275ec418632767d137fe5dc90e6fe5c`.
Prvi Windows pokušaj otkrio je sukob naziva `near` sa Windows makroom. Funkcija
je preimenovana u `pointsNear`, a C++ test sada definiše isti makro prije headera
kako bi provjerio da se sukob ne ponovi.

- [AutoCAD 2020 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40850661942/artifacts/12886893011)
  — uspješno, 0 grešaka, 2 uobičajena LNK4099 upozorenja za razvojni rxapi.pdb.
  SHA256 `e98671107b8a7f5780a3ae5f5714097c7d0757fbf3df7ff1c33213ec98c49ebf`.
  ZIP CRC, PE x64, zavisnost acdb23.dll, BUILD commit i nova komanda/prompt u ARX-u
  provjereni. KGTEZ i CUIX fajlovi odgovaraju nepromijenjenim izvorima.

Poslije zatvaranja AutoCAD-a sačuvati prethodni ARX/DBX par i zamijeniti oba fajla
iz paketa odgovarajuće verzije. KGTEZ i CUIX nisu mijenjani ovom doradom.
Kompilacija i testovi ne potvrđuju rad u AutoCAD-u; praktična provjera, naročito
UNDO/REDO i ažuriranje vezanih oznaka, još ostaje da se izvrši.

- [AutoCAD 2022 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40850662131/artifacts/12887440826)
  — uspješno, 0 grešaka, 2 uobičajena LNK4099 upozorenja za razvojni rxapi.pdb.
  SHA256 `14b6ab7ae3084431be15feef4142b1d10b33a8d500c52261a3412b55a4a45a61`.
  ZIP CRC, PE x64, zavisnost acdb24.dll, BUILD commit i nova komanda/prompt u ARX-u
  provjereni. KGTEZ i CUIX fajlovi odgovaraju nepromijenjenim izvorima.

## Dorada: pomjeranje prema ivici

`referencePoint` iz prvog klika i širine kanala određuje fizičku ivicu. Isti
proračun se koristi za pravougaoni kanal (širina A) i okrugli kanal (prečnik).
Izbor ivice je nezavisan od redosljeda krajeva kanala i aktivnog UCS-a.

Novi test obuhvata 144 kombinacije: obje ivice, tri reference, oba redosljeda
krajeva, četiri rotacije i mm/cm/m. Provjerava poznatu konačnu osu kada se na
zadatu tačku dovede ivica, umjesto samo poređenja sa kopijom istog algoritma.
Praktično provjeriti obje ivice i opcije I/D/O, OSNAP na susjedni kanal i UNDO.

## Paketi dorade sa izborom ivice

Izvor: `75f4cc2ddaf636706bafba161e9202d635dc48f1`. Osam Python testova i prošireni
C++ test geometrije prolaze. Korisnik je potvrdio prethodnu verziju sa osom;
novi izbor ivice tek treba potvrditi u AutoCAD-u.

- [AutoCAD 2020 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40862249796/artifacts/12891728822)
  SHA256 `bfcabf6232ed5c019c6447ea0a84726a456b14d5d27e0ee39d18fa5966f140fe`.
  Uspješno, 0 grešaka, 2 uobičajena LNK4099 upozorenja. ZIP CRC, x64 PE,
  acdb23.dll, BUILD commit i novi prompt sa opcijama ivica provjereni.

KGTEZ i oba CUIX fajla ostaju nepromijenjeni. Za instalaciju zatvoriti AutoCAD,
sačuvati prethodni ARX/DBX par i zamijeniti oba fajla iz odgovarajućeg paketa.

- [AutoCAD 2022 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40862249787/artifacts/12892491545)
  SHA256 `f560650278c0bb0a8037bd6c79ca21a202ebfbeb2fbb8ebd32c1db8eaf79d1e2`.
  Uspješno, 0 grešaka, 2 uobičajena LNK4099 upozorenja. ZIP CRC, x64 PE,
  acdb24.dll, BUILD commit i novi prompt sa opcijama ivica provjereni.

## Podrazumijevani izbor potvrđen Enterom — čeka kompilaciju

Korisnik je potvrdio da dorada sa ivicama radi. Na njegov zahtjev je zatim
izbor reference izdvojen prije zadavanja tačke:
`Referenca pomjeranja [Izabrana/Druga/Osa] <Izabrana>:`.
Enter prihvata `Izabrana`; zatim se zadaje nova pozicija ivice.
`Druga`/`D` i `Osa`/`O` biraju drugu ivicu ili osu. Esc prekida bez izmjene.

Ova posljednja dorada je sačuvana **bez kompilacije**, na izričit zahtjev korisnika.
Ne pokretati novu kompilaciju dok je korisnik ne odobri. Prethodni paketi sa
ivicama rade, ali još nemaju novi prompt sa podrazumijevanom opcijom.
Commit mora imati `[skip ci]` da spriječi automatski build.
