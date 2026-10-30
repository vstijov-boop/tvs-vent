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
biranja/oduzimanja pojedinačnih elemenata i bez živog pregleda pomjeranja.

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
