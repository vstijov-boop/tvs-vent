# DEV pravougaoni bypass

Radni tok: **nacrtaj → izaberi prepreku → pronađi → prihvati → nastavi crtanje**.

Učitati DBX pa ARX iste godine iz ovog izdvojenog paketa u novoj AutoCAD sesiji.
Ne prepisivati produkcioni bundle i ne koristiti hot unload/reload.

1. TVS postavke: mm/cm/m; kruti pravougaoni kanal. Nacrtati pravolinijski kanal
   kroz područje prepreke i dovoljno iza nje: kandidat mora stati unutar njegovih
   krajeva, uz ostatak najmanje 1 mm na oba kraja.
2. Samostalno: `TVS_BYPASS`, izabrati kanal, prepreku i Prvi/Posljednji smjer.
   Tokom `TVS_PIPE` ili `TVS_DRAW`: opcija `Obilazak` / `O`; izabrati samo prepreku.
3. Postojeći dijalog: potvrditi stvarne jedinice, eksplicitno unijeti obje izolacije
   (0 ako ih nema), Ri, razmak i ostale ulaze; `Pronađi prolaz`.
4. `Pregled u crtežu` je privremen; Enter vraća dijalog. **`Prihvati obilazak`**
   trajno upisuje isti provjereni kandidat, bez nove pretrage. Izmjena ulaza uklanja
   važenje rezultata; promijenjen kanal/prepreka ili veze odbijaju upis.
5. U crtanju poslije uspjeha odmah zadati sljedeću tačku. Nastavak je sa sačuvanog
   spoljašnjeg kraja i početne fizičke kote. Cancel/neuspjeh čuva dotadašnji nastavak.

Nastaju četiri koljena, dva kosa člana i H, dvije neizmijenjene V1 trojke i njihov
roditelj. Originalni kanal ostaje jedan spoljašnji ostatak; drugi ostatak dobija
novi objekt. Oba spoljašnja položaja/kota/presjek ostaju isti. Eksplicitni izlazni
pipe susjed dobija samo nužnu recipročnu referencu novog ostatka; geometrija susjeda
i prepreka se ne mijenjaju. Roditelj čuva obje granične reference. Legacy vektori
konektora i dalje nisu zasebno filed podaci običnih kanala; novi roditelj ne migrira
globalnu mrežu niti popravlja druge objekte pri otvaranju.

Fizički obračun kosih članova ostaje postojeći V1 (sopstveni rise svakog člana).
H je horizontalni fizički kanal. Roditelj nema količinu; sedam članova se ne broje
ponovo kroz njega. `Umetni detalj…` ostaje poseban statički nepovezan snimak.
`TVS_BYPASS_PREVIEW` zadržava raniji preview i statički detalj; za trajno prihvatanje
koristiti TVS_BYPASS ili Obilazak tokom crtanja.

Ograničenja: obični direktni horizontalni pravougaoni kruti TVS kanali, Model,
upravna prepreka i pun poprečni obuhvat postojećeg checker-a, 30/45/60, mm/cm/m.
Bez pomjeranja mreže, redukcija ili kružnih kombinacija. Dvosmislene veze,
nepoznati atributi i zaključani objekti odbijaju se kao kod offset servisa.
COPY/WBLOCK/INSERT kloniranje novih bypass članova je odbijeno u ovoj DEV verziji;
time se izbjegava kopija koja bi ostala vezana za originalnog roditelja. Stari
samostalni offseti zadržavaju svoj postojeći clone ugovor.
Kraj ostatka koji već dodiruje postojeći bypass roditelj ne smije se mijenjati;
njegov slobodni spoljašnji kraj može se koristiti za nastavak.

**Prikaz ne računa tačnu vidljivost.** Pune linije su projektovane konture,
crtice mogu označavati simboličke spojeve/pomoćne gabarite, a ne potvrđene skrivene
ivice. Puni hidden-line prikaz ostaje otvoren nedostatak GFX-01 i nije dio ovog
paketa. Statički bočni detalj je dostupan; povezani/osvježavani detalj nije uveden.

Za rollback probu u potrošnoj kopiji: prije pokretanja AutoCAD-a PROCESS env
`TVS_BYPASS_FAIL` postaviti na 1 (sedam članova), 2 (ostatak i prevezivanje),
3 (roditelj/backlink-ovi prije commita). Svaki prekid mora ostaviti izvor i susjede
bez promjena i bez novih članova/zapisa. Ukloniti env i restartovati poslije probe.
To nije Undo/Redo proba; STAB-01 ostaje odložen.

Korisnik je prijavio da INT-00 Z funkcioniše dobro. Verzija AutoCAD-a i pojedinačne
runtime stavke te korisničke probe nisu potvrđeni i ne pretpostavljaju se.
