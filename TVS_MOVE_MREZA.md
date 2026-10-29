# TVS_MOVE — varijanta sa račvama i prelazima

29.10.2026: korisnik je odobrio kompilaciju i build. Prošli su svi pripremljeni
testovi (devet Python testova i osam C++ programa). Praktična provjera proširene
mreže i Enter-default ponašanja u AutoCAD-u još predstoji.

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
Kompajliran je uz C++98 i stroga upozorenja i uspješno izvršen. Ima mrežu sa T-račvom,
prelazom, različitim presjecima i tri dionice koje mijenjaju dužinu; obuhvata
mm/cm/m, rotacije, skraćivanje preko kraja, duple kontakte, neusaglašene presjeke,
netangentne ose, 3D, bočni priključak, novi kontakt i granicu 256 elemenata.

Poslije kompilacije 2020/2022 u AutoCAD-u provjeriti stvarne
pravougaone/okrugle mreže, asimetrične prelaze, grane, ponovljena pomjeranja,
sačuvan/ponovo otvoren DWG, UCS, mase, oznake, zaključane slojeve i UNDO/REDO.
Ovaj izvorni kod još ne predstavlja potvrđenu radnu verziju.

## Provjereni paketi

Izvor: `5677500d2ebeaaabf2dc621359fb0d07f9a5eb29`.
Novi `test_move_reference_input.py` kompajlira stvarni unos reference sa simuliranim
AutoCAD API-jem: Enter, Izabrana, Druga, Osa, prekid prije tačke i prekid unosa tačke.
Prošireni C++ test uključuje okrugle mreže i prelaz okruglo/pravougaono, kao i
suprotan smjer pomjeranja. Svi testovi prolaze.

- [AutoCAD 2020 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40869321805/artifacts/12895261724)
  SHA256 `62e4a1f4f34c4ce7c0ddcccdd6edb0147a1925708f227b2e5e7ea04b0d5f9618`.
  0 grešaka, 2 uobičajena LNK4099 upozorenja za razvojni rxapi.pdb.
  ZIP CRC, x64 PE/acdb23.dll, BUILD commit, novi prompt i proširenje mreže u ARX-u,
  najnoviji CUIX sa ikonom i nepromijenjeni KGTEZ potvrđeni.

Instalacija: zatvoriti AutoCAD, sačuvati prethodni ARX/DBX par i CUIX, zamijeniti
oba binarna fajla odgovarajućom verzijom i TVS_TOOLS.cuix novim iz paketa ako
ikona još nije instalirana. KGTEZ i TVS_VENT.cuix ostaju nepromijenjeni.
Praktično ponašanje nove mrežne varijante u AutoCAD-u još treba potvrditi.

- [AutoCAD 2022 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40869321602/artifacts/12895282695)
  SHA256 `d31bf315b433149e0977d38bb8cd13161806abe6f409a19d5030ee476e9693c9`.
  0 grešaka, 2 uobičajena LNK4099 upozorenja za razvojni rxapi.pdb.
  ZIP CRC, x64 PE/acdb24.dll, BUILD commit, novi prompt i proširenje mreže u ARX-u,
  najnoviji CUIX sa ikonom i nepromijenjeni KGTEZ potvrđeni.
