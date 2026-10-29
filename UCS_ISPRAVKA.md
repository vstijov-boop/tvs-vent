# UCS: unos tačaka bez pomjeranja kanala

29.10.2026 — grana `unapredjenja-masa`.

## Uzrok i ispravka

`acedGetPoint` i tačka izbora iz `acedEntSel` pripadaju aktivnom UCS-u,
a TVS geometrija i projekcije na kanale koriste WCS. Ranije su te vrijednosti
miješane. Novi `CoordinateInput.h` pretvara uspješan unos UCS → WCS, a baznu
tačku za gumenu liniju WCS → UCS. Svaki poziv koristi trenutno aktivni UCS.
Transformacije su tačke (uključuju pomjeren početak), ne samo vektori.
Otkazivanje, Enter i ključne riječi zadržavaju izvorni status i ne transformišu
nepostojeću tačku. Neuspjela transformacija prekida unos bez zapisivanja rezultata.

Obuhvaćeni su unos kanala i nastavak, biranje kanala za povezivanje/prekid,
samostalni fitinzi, multileader i tačka umetanja specifikacije.
`TVS_SCALE` namjerno zadržava UCS tačku jer je prosljeđuje ugrađenoj AutoCAD
komandi SCALE, koja očekuje UCS. Postojeći objekti i DWG format se ne mijenjaju.

Ovo rješava koordinatni unos. Geometrija TVS fitinga i dalje je zasnovana na
svjetskoj XY ravni: ova izmjena ne dodaje crtanje čitavog sistema u proizvoljnoj
nagnutoj 3D ravni. Primarni test je UCS rotiran oko svjetske Z ose, sa ili bez
pomjerenog početka i elevacije.

## Provjera

`tests/test_coordinate_input.cpp` kompajlira stvarni header sa simuliranim
AutoCAD API-jem. Provjerava 24 kombinacije četiri rotacije, dva pomjeraja i tri
jedinice, povrat bazne tačke, isti ulazni/izlazni bafer, tačku izbora, statuse
RTCAN/RTKWORD/RTNONE/greške i neuspjelu transformaciju. To nije zamjena za radni
AutoCAD test.

U AutoCAD-u 2020 i 2022 provjeriti:

1. U kopiji crteža nacrtati referentnu poliliniju sa dvije krivine.
2. UCS → World: TVS_PIPE prati zadate OSNAP tačke.
3. UCS → Z → 30, zatim 90 i -45: ponoviti iste svjetske tačke. Kanal, koljena
   i gumena linija moraju ostati na referenci; ORTHO prati aktivni UCS.
4. Ponoviti sa UCS početkom na poznatoj udaljenoj tački; provjeriti i mm/cm/m.
5. Nastaviti postojeći kanal; provjeriti promjenu dimenzije, račvu i povezivanje
   dvije dionice izborom blizu njihovih krajeva, te prekid kanala na izabranoj tački.
6. Dodati multileader, umetnuti specifikaciju i probati TVS_SCALE.
7. Esc i opcija Dimenzija ne smiju praviti kanal na neinicijalizovanoj tački.
8. Vratiti UCS → World: postojeći TVS objekti moraju ostati na istom mjestu.

Zatvoriti AutoCAD prije zamjene binarnih fajlova. Sačuvati prethodni ARX/DBX par
i koristiti isključivo paket za odgovarajuću verziju AutoCAD-a.

## Provjereni paketi

Izvor: `3f12fb7baea219750517665abae9bcef2749a68d`.
Obje Windows kompilacije uspješne: 0 grešaka, 2 uobičajena LNK4099 upozorenja
za nedostajući razvojni `rxapi.pdb`. ZIP CRC i x64 PE/AutoCAD zavisnosti provjereni.
Šest C++ test programa i svih sedam Python testova prolaze.
Radna provjera u AutoCAD-u još nije izvršena.

- [AutoCAD 2020 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40842023986/artifacts/12882948086)
  SHA256 `ec20da6dbfd0262c1a7258a67bfd058ecbf93abbccf6fd9989240b36c040c876`
- [AutoCAD 2022 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/40842024103/artifacts/12884015679)
  SHA256 `0e31ff38d447d132400ea5fa65b1c499b49b838911c223e47ce10ef42377cd7f`

Zamijeniti ARX i DBX zajedno u direktorijumu odgovarajuće verzije.
KGTEZ.lsp i oba CUIX fajla nisu mijenjani ovom ispravkom.
