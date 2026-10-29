# TVS_MOVE — prvi korak povezanog pomjeranja

Status 29.10.2026: korisnik je odobrio kompilaciju. Matematički i topološki
testovi prolaze. Radni test u AutoCAD-u još nije izvršen. Novi paketi uključuju
i početne radijuse 0,5; prethodni UCS paketi ih nemaju.

## Upotreba i ponašanje

1. `TVS_MOVE` — izabrati krutu pravu TVS dionicu između dva koljena u osnovi.
2. Zadati tačku nove bočne pozicije ose; uzdužna i vertikalna komponenta unosa
   se ne koriste. Bazna tačka je projekcija klika na osu izabranog kanala.
3. Oba koljena klize duž osa susjednih kanala, njihovi udaljeni krajevi ostaju
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
