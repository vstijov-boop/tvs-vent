# TVS_STRETCH — pomjeranje izabranog dijela mreže

30.10.2026. Prva implementacija, bez Windows kompilacije i AutoCAD potvrde.

1. Pokrenuti TVS_STRETCH.
2. Zadati dva suprotna ugla okvira u aktivnom UCS-u (smjer izbora nije bitan).
3. Obuhvatiti priključke pokretnih fitinga i krajeve dionica koje treba pomjeriti.
4. Zadati baznu i novu tačku.

Izbor se zasniva na priključnim tačkama unutar okvira, ne na konturi tijela.
Oba kraja prave dionice u okviru pomjeraju cijelu dionicu. Jedan kraj u okviru
mijenja dužinu, dok drugi ostaje fiksan. Fiting mora imati sve priključke u okviru
ili sve van njega. Djelimično obuhvaćen fiting prekida komandu prije izmjene.
Potpuno neizabrani elementi ostaju nepromijenjeni. Komanda ne prenosi izbor
na ostatak mreže. Trenutna verzija ima jedan pravougaoni okvir, bez dodatnog
biranja/oduzimanja pojedinačnih elemenata sa živim šematskim pregledom pomjeranja.

Granični kanali moraju biti paralelni pomjeranju: promjena pravca/ugla nije
podržana. Koriste se podržani horizontalni kruti TVS elementi u trenutnom prostoru;
3D, fleksibilni i vertikalni/segmentni fitinzi nisu obuhvaćeni. Nema trajnih
konektora ni izmjene DWG formata. Mase se izvode iz nove geometrije; povezane
TVS MLeader oznake osvježavaju se uz geometriju, a statične tabele treba obnoviti.

Prije pisanja provjeravaju se spojevi, presjeci, tangentnost, minimalna dužina
1 mm, dupli/novi kontakti i priključci usred nepodijeljene dionice. Najviše 256
promijenjenih elemenata. Ovo nije potpuna provjera sudara tijela.
Izmjene se izvršavaju u jednoj transakciji modalne komande; zaključani element
ili oznaka prekidaju cijelu izmjenu. Praktično UNDO/REDO ponašanje treba potvrditi.

Test test_stretch_selection.cpp provjerava mm/cm/m i četiri rotacije, mrežu sa
račvom/prelazom, nepomjerene krajeve, nepomjerenu susjednu dionicu, nepotpun
izbor fitinga, bočni pomjeraj, preokretanje dionice i prazan izbor. Uključuje
postojeće mrežne regresione testove. Prošlo i 11 Python testova.
U AutoCAD-u još provjeriti okvir u rotiranom/pomjerenom UCS-u, Esc, zaključane
slojeve, oznake, UNDO/REDO i sačuvan/ponovo otvoren DWG.

## Vizuelna povratna informacija

Izabrani elementi privremeno se ističu prije unosa bazne tačke. AcEdJig prikazuje
konture pravih kanala i veze priključaka fitinga tokom pomjeranja miša. Fiting je
šematski prikaz, ne puni izgled lima/radijusa. Zeleno znači dozvoljen položaj;
crveni element označava prvu pronađenu geometrijsku prepreku, ostali su žuti dok
potez nije dozvoljen. Pregled ne upisuje objekte ni boje u DWG.

Ako okvir presiječe fiting ili je potvrđen nedozvoljen geometrijski potez,
problematični original se privremeno ističe uz opis razloga; Enter/Esc vraća
u crtež. Isticanje se čisti pri svakom izlasku. Prikaz koristi WCS položaj koji
vraća acquirePoint; izbor okvira ostaje u aktivnom UCS-u. ORTHO je podržan.
Nema ponovnog unosa u istoj komandi nakon nedozvoljene potvrde: ispraviti izbor
ili pomjeraj i pokrenuti ponovo. Greške otvaranja/zaključavanja i oznaka i dalje
se prijavljuju porukom uz poništavanje transakcije.

Windows kompilacija i praktična provjera prikaza, osnap/ortho, Esc i UNDO/REDO
još nisu urađene. Build ostaje odložen po zahtjevu korisnika.

## Provjereni buildovi 30.10.2026

Kod: `4fe8e17abb59ec8c2c40755bf4da1cda73703be3`.
Oba builda: 0 grešaka, 3 LNK4099 upozorenja za razvojni SDK rxapi.pdb.
Prošlo svih 11 Python testova i prošireni C++ test_stretch_selection.
ZIP CRC, x64/acdb23 odnosno acdb24, izvorni commit, oba CUIX, KGTEZ i
prisustvo komande TVS_STRETCH u ARX-u provjereni. Praktična AutoCAD potvrda
prikaza, pomjeranja i UNDO/REDO još je potrebna.

- [AutoCAD 2020](https://github.com/vstijov-boop/tvs-vent/actions/runs/41024339332/artifacts/12962748735)
  SHA256: `4c25096fc7594225928f41755a56d1f899270cc08710b30ce9009a2ca1e3e66d`.
- [AutoCAD 2022](https://github.com/vstijov-boop/tvs-vent/actions/runs/41024339331/artifacts/12963291607)
  SHA256: `dd46d7e77ea848fab32d61487cccec968f31d63da71cab3172a42a8c10dffdb7`.

Zatvoriti AutoCAD, sačuvati prethodne fajlove i zamijeniti odgovarajući ARX/DBX
par u Contents/TVS_VENT/2020/win64 ili 2022/win64. Zamijeniti i
Contents/TVS_VENT/Resources/TVS_VENT.cuix i
Contents/TVS_TOOLS/Resources/TVS_TOOLS.cuix te KGTEZ.lsp u istom Resources.
Oba dugmeta nalaze se u TVS_VENT. Novi build uključuje i izmjenu Tab redoslijeda.
