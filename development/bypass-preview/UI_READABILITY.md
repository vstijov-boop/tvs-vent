# DEV pregled v2 — čitljivost i kotiranje

## Prvi korisnički funkcionalni test — 1. novembar 2026.

Korisnik je prijavio da je probao novu promjenu Z i BYPASS preview i da osnovno ponašanje djeluje ispravno. Dostavljen je stvarni snimak dijaloga; korisnik takođe navodi pregled u osnovi. Na snimku se vide mm, trasa 400×250, prepreka 250×250, obje fizičke kote ose 0, Ri=100, ugao45°, izolacije0, clearance50 i rezerva5. Rezultat D315, C360, W1362.792206, kota prolaza−315 i zazor oko65 mm postao je fiksni regresioni primjer.

Ovo je prvi korisnički funkcionalni test, ne potvrda kompletne test-matrice. AutoCAD izdanje i svi drugi režimi nijesu ovim izvještajem identifikovani/potvrđeni. Nove UI promjene u ovoj reviziji nijesu praktično testirane u AutoCAD-u od strane agenta i nijesu napravljeni novi stvarni AutoCAD snimci.

## Dijalog

Osnovni panel: iznad/ispod, ugao, Ri, izolacije, obavezan razmak, odvojena rezerva, minimum kosog/srednjeg pravog komada, prozor i opcione fizičke visinske granice. Napredno / dijagnostika otvara panel razvojnih parametara i puni ispis. Sva skrivena polja ostaju aktivna, čuvaju vrijednosti i prolaze istu validaciju. Ne mijenjaju se numerički ugovor, generator, solver, adapter Z, trajni upis ili DWG podaci.

U osnovnom režimu panel presjeka je proširen sa 590 na 710 projektnih piksela; pri otvaranju naprednih polja pomjera se desno. W, C i D su kotne linije; početna/završna kota ose su u gornjem pojasu. Osa prolaza, ugao/Ri, fizičke/projektovane dužine oba nagiba, visina kanala/prepreke, vrh/dno sa izolacijom i aktivne granice su u trokolonskoj legendi izvan geometrije. Kritični par je zelen sa pokaznom linijom i kružićima; to je najmanji geometrijski razmak između spoljnih gabarita, ne nužno vertikalni zazor. D je razlika kota osa, a ne zazor.

Kote i Ose su nezavisni prekidači prikaza; ne mijenjaju niti ponovo pokreću proračun. Obične mjere imaju jednu decimalu; ograda razmaka se za glavni ispis proširuje na dvije decimale, a puni dijagnostički ispis ostaje dostupan. Potvrđeni status/interval se ne izračunavaju iz zaokruženog teksta. Sve vrijednosti i referentne tačke dolaze iz istog checked Result-a ili potvrđenih ulaznih podataka. Skala s i z ostaje jednaka.

Kontrole i donje poruke su raspoređene unutar skalirane klijentske površine. Tekstovi imaju odvojene pojaseve van geometrije; praktična provjera Windows DPI/fontova i vrlo malih ekrana ostaje potrebna.

## Osnova — namjerno ograničen dijagnostički režim

Pogled je projekcija odozgo duž fizičke vertikale. Puna provjera vidljivosti nad zakrivljenim tijelima NIJE implementirana u ovoj reviziji. Nema heuristike koja cijeli niži kanal crta isprekidano. Nema tvrdnje o tačnom hidden-line prikazu.

- **Konture (dijagnostika, bez zaklanjanja)**: ukupni projektovani gabariti obilaska, izolacija i izabrana prepreka. Ose su opcione.
- **Sve ivice — dijagnostika (bez zaklanjanja)**: dodaje simboličke spojne ravni i pomoćne gabarite pojedinačnih fitinga. Crtice označavaju ove kategorije, NIKADA skrivenost.
- Pune zelene/crvene linije su konture obilaska/prepreke; sivo je izolacija. Cyan kratke crtice su simbolički spojevi. Sitno tačkaste sive linije su pomoćni gabariti. Žuta dugo–kratko linija je osa.
- Fizičke konture imaju prednost nad tačno poklopljenim pomoćnim segmentima i crtaju se posljednje. Ne prikazuju se granice pojedinačnih ćelija teselacije kao fizičke ivice.

**A. Samozaklanjanje** nije riješeno; **B. zaklanjanje preprekom** nije riješeno. Nema klasifikacije vidljivo/skriveno ili dijeljenja ivica pri ulasku/izlasku iz zone zaklanjanja. Za to je potreban zaseban razvojni korak nad površinama, a ne samo poređenje Z centara. Crtački Z služi samo smještanju transient prikaza, ne odlučuje o fizičkoj vidljivosti. Sve ivice su transparentni dijagnostički pregled.

Originalna izabrana trasa ostaje vidljiva ispod preview-a. Ne mijenjaju se njen izgled, podaci ni objekti; nema upisa-pa-brisanja. Transient crte/dash-obrasci su samo heap objekti. Limit prikaza je 10000 drawables; prevelik dijagnostički prikaz se odbija i čisti.

## Testovi i ručni pregled

Automatizovano: raspored kotnih pojaseva i odsustvo preklapanja njihovih pravougaonika sa geometrijom/međusobno pri skalama .65/.8/1/1.25/1.5/2 i oba panela (ovo nije stvarni Windows DPI/font test); korisnički numerički primjer, prijenos osa/dužina/kritičnog para, zaokruživanje samo za prikaz, klase linija, prednost poklopljene fizičke konture, iznad/ispod × 30/45/60° × smjer0/37° × izolacija0/25mm. Postojeće numeričke regresije ostaju obavezne. Test klase fizička/pomoćna linija nije dokaz hidden-line algoritma — takav algoritam nije dio isporuke.

U AutoCAD-u provjeriti:
1. U osnovnom panelu izračunati primjer sa slike; provjeriti kote, L/projekciju, W/D/C i zeleni kritični par. U Napredno provjeriti da su svi raniji budžeti/intervali i dalje dostupni.
2. Kote/Ose/Napredno uključiti/isključiti bez gubitka rezultata. Izmjena stvarnog ulaza mora poništiti rezultat i onemogućiti pregled do novog računanja.
3. Prebaciti Konture/Sve ivice i provjeriti legendu: isprekidano nije skriveno. Iznad/ispod, 30/45/60°, oba smjera i izolacija25mm. Ponašanje pri zaklanjanju je namjerno dijagnostičko, ne hidden-line prolaz.
4. Provjeriti 100/125/150/200% DPI i manje radne površine: tastere, polja, legendu, donje poruke, odsustvo preklapanja teksta sa geometrijom i čitljivost.
5. Enter vraća dijalog; Esc/Cancel/zatvaranje i ponavljanje čiste grafiku. Broj DWG entiteta, izabrani objekti, stanje trase i DWGMOD treba da ostanu nepromijenjeni.

Ove interaktivne provjere nijesu izvršene u CI-u. Korisnikov prvi test potvrđuje prethodnu preview verziju, ne ovu novu UI reviziju ili sve AutoCAD konfiguracije.
