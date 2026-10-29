# TVS_MOVE — varijanta sa račvama i prelazima

29.10.2026: pripremljen izvorni kod, **bez kompilacije**, po važećoj instrukciji
korisnika. Ova varijanta i novi izbor `<Izabrana>` još nisu u preuzetim ARX paketima.
Ne pokretati kompilaciju do novog odobrenja. Commit mora sadržati `[skip ci]`.

## Kako je predviđeno da radi

Koristi se ista komanda i novo dugme: TVS_MOVE. Izabrati pravu dionicu blizu
željene ivice, potvrditi `<Izabrana>` Enterom (ili izabrati Druga/Osa), pa zadati
novu bočnu poziciju.

Jednostavan niz sa dva koljena koristi prethodnu, potvrđenu geometriju, uključujući
kosa koljena. Ako niz sadrži račvu, prelaz ili dodatne povezane dionice, uključuje
se novi proračun mreže:

- Izabrana dionica pomjera se bočno, paralelno prethodnoj osi.
- Povezana koljena, račve i prelazi koji moraju da je prate pomjeraju se kao kruti
  elementi: dimenzije, radijusi, uglovi i oblik prelaza ostaju isti.
- Kanal paralelan pravcu pomjeranja mijenja dužinu na priključenom kraju; njegov
  udaljeni kraj ostaje fiksan. Tu se prenošenje pomjeranja zaustavlja.
- Kanal koji nije paralelan pomjeranju prenosi pomjeranje na naredni fiting.
  Tako se promjena može prenijeti kroz glavnu trasu, dok se ogranci prilagode.
- Slobodan kraj dionice koja se kruto prenosi prati tu dionicu. Slobodan udaljeni
  kraj dionice koja mijenja dužinu ostaje fiksan. Sama izabrana dionica mora imati
  vezu na oba kraja.

Primjer: pomjeriš horizontalni glavni kanal sa T-račvom nagore. Račva ga prati,
vertikalni ogranak se skraćuje/produžava do svog fiksnog kraja, a glavna trasa
prenosi pomjeranje do narednog mjesta gdje ga susjedna dionica može preuzeti
promjenom dužine. Prelaz se pomjera u cjelini, uz iste presjeke na oba kraja.

Podržani su odgovarajući pravougaoni i okrugli presjeci po pojedinačnom priključku,
kao i prelazi između različitih presjeka. Priznaju se postojeći spojevi po krajevima
sa tolerancijom 0,1 mm u mm/cm/m; ne mijenja se format DWG-a.

## Ograničenja i poništavanje

Svi pokrenuti elementi moraju biti u zajedničkoj XY ravni sa usklađenim osama i
elevacijom. Račva je u ovoj fazi pravougaona po osama (90°), sa presjecima iz
postojećeg TVS_WYE. Fleksibilni elementi, segmentna koljena ВСН i koljena
gore/nadolje ostaju izvan obuhvata.

Nisu obuhvaćeni priključci usred nepodijeljene dionice, pomjerane zatvorene petlje,
XREF/blok unutrašnjosti (ni priključci na opremu predstavljenu običnim blokovima), više od 256 promijenjenih elemenata, dupli spojevi i
promjene koje preokreću/skraćuju dionicu ispod 1 mm. Novi slučajni kontakt sa drugim
priključkom se odbija. Ovo nije provjera sudara kompletnog tijela sa građevinom,
opremom ili drugim kanalima.

Geometrija i TVS MText multileader oznake mijenjaju se unutar iste transakcije.
Zaključani/nedostupni elementi ili oznake prekidaju cijelu izmjenu. Masa se izvodi
iz nove geometrije, a statične specifikacije treba ponovo generisati. Predviđen je
jedan UNDO za cijelu promjenu; radni UNDO/REDO tek treba provjeriti.

## Provjera koja još predstoji

Pripremljen `tests/test_move_network.cpp` koristi stvarni matematički header.
Nije kompajliran niti izvršen zbog zabrane kompilacije. Ima mrežu sa T-račvom,
prelazom, različitim presjecima i tri dionice koje mijenjaju dužinu; obuhvata
mm/cm/m, rotacije, skraćivanje preko kraja, duple kontakte, neusaglašene presjeke,
netangentne ose, 3D, bočni priključak, novi kontakt i granicu 256 elemenata.

Po odobrenju: izvršiti sve testove, kompajlirati 2020/2022 i provjeriti stvarne
pravougaone/okrugle mreže, asimetrične prelaze, grane, ponovljena pomjeranja,
sačuvan/ponovo otvoren DWG, UCS, mase, oznake, zaključane slojeve i UNDO/REDO.
Ovaj izvorni kod još ne predstavlja potvrđenu radnu verziju.
