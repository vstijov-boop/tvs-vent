# DEV: statički kotirani bočni detalj, zapis v1

Korisnik je potvrdio prethodni preview i UI v2 u AutoCAD-u 2022. Ova faza dodaje samo dokumentacioni blok iz trenutno potvrđenog rezultata. Nema TVS proxy objekata, trajnog TVS obilaska, veze sa parent sklopom, osvježavanja, promjene solvera/generatora, changeZ, V1 etažiranja ili specifikacije.

## Pokretanje

U izdvojenoj testnoj sesiji učitati DEV DBX, pa ARX iste verzije. Pokrenuti `TVS_BYPASS_PREVIEW`, izabrati trasu/prepreku/smjer, potvrditi jedinice i ulaze, izračunati kandidat. `Umetni detalj…` dostupan je samo dok rezultat važi. Izmjena numeričkih ulaza ga onemogućava. Promjena Kote/Ose/Napredno ne mijenja rezultat.

Taster zatvara dijalog i traži oznaku (`OB-01`, sljedeći slobodan broj, ili vlastita oznaka), Osnovno/Detaljno, visinu teksta i veličinu strelica u mm, te tačku umetanja. ASCII oznaka: 1–32 slova/cifre/crtica/podvlaka, bez razlike velikih i malih slova. U preview-u izabrati **Umetni**, **Pozicija** ili **Otkazi**; Enter je Otkazi. Ovo je preview položaja, s pomoćnim prikazom kotnih vodilica; konačne kote su standardni DIMENSION entiteti. Esc ili neuspjeh prije potvrde ne upisuje ništa. Komanda se završava poslije jednog umetanja: jedan command Undo treba da ukloni kompletan upis, Redo da ga vrati. To zahtijeva praktičnu potvrdu u AutoCAD-u.

## Geometrija, kote i jedinice

Iz istog checked Result-a bridge izvozi sedam tijela s tačnim kružnim bulge lukovima: četiri koljena, dva kosa i jedan srednji pravi komad. Nema novog poziva generatora pri umetanju. Metal, izolacija i osa imaju odvojene standardne polilinije. Prepreka dolazi iz istog potvrđenog projektnog konteksta. Osni lukovi su kružni, ne tetive kroz koljena.

Bočni prikaz: lokalno +s desno, fizičko +Z gore. Lokalni početak je početna osa; od svih visina oduzima se fizički z0. Blok se umeće translacijom u WCS XY, na WCS visini odabrane tačke. Tačka se iz trenutnog UCS-a pretvara u WCS. Visina na papiru nije fizička WCS Z. Apsolutne kote početka, kraja i prolaza ispisane su zasebno s datumom.

Numerički podaci ostaju mm. Pri DWG upisu svaka geometrijska koordinata i veličina anotacije dijeli se potvrđenim faktorom mm/jedinici DWG: mm=1, cm=10, m=1000. Blok ima jediničnu skalu. Linearne kote imaju DIMLFAC=faktor i `<> mm`: mjera dolazi iz geometrije standardne kote, ne hardkodiranog broja. Odvojene veličine teksta/strelica ne skaliraju geometriju. Ne mijenja se INSUNITS niti korisnikov DIMSTYLE.

Osnovno: stvarne aligned kote W, H (raniji C) i D; tabela navodi fizičke S1/H/S2, K1–K4/ugao/Ri, dimenzije kanala/prepreke, izolacije, apsolutne kote, potvrđeni interval kritičnog razmaka i odvojeni vertikalni zazor. Detaljno dodaje stvarne aligned kote fizičkih S1 i S2, pokazne linije i pojedinačne oznake sedam komada, te potvrđeni kritični par. Vertikalni zazor je izričito zazor između horizontalnog srednjeg komada H i prepreke u njihovom horizontalnom preklopu, sa izolacijama; nije predstavljen kao najmanji geometrijski razmak. Ako preklopa nema, ova veličina je označena kao nedefinisana.

Kotne linije/tekst imaju odvojene spoljne pojaseve, tekstualna tabela svoj gornji pojas, oznake komada zasebnu bočnu kolonu. Vrlo veliki tekst i neobičan font/AutoCAD fit mogu zahtijevati pregled; automatizovan geometrijski test nije dokaz prikaza fontova. Nema hidden-line zavisnosti ni prenosa dijagnostičkih ivica osnove.

Za svako umetanje kreiraju se izdvojeni standardni blok, sloj, text style (txt.shx) i dimstyle s GUID imenom. Postojeći stilovi/slojevi se ne prepisuju, aktivni stil se ne mijenja. DIMENSION entiteti ostaju u bloku, njihovi anonimni prikazni blokovi se računaju prije commita. Kote su **neasocijativne**; nema AcDbDimAssoc objekta ni automatskog osvježavanja. Blok sadrži samo standardne entitete i XRecord, tako da ne zavisi od TVS klase za prikaz. Save/reopen/bez TVS-a i stvarno ponašanje kota u bloku moraju se praktično provjeriti prije prihvatanja.

## Minimalni ugovor identiteta

Extension dictionary standardne blok reference: `TVS_BYPASS_SIDE_DETAIL_V1`. XRecord je niz DXF code 1 stringova, invariant format, doubles sa 17 značajnih cifara. Prva četiri polja su fiksna: `TVS_SIDE_DETAIL/1`, GUID, vidljiva korisnička oznaka, `STATIC_SNAPSHOT`. Dalje su writer/source/copy/orientation/units oznake, informativni handle-ovi originalne trase i prepreke (nisu žive reference), faktor, nivo kotiranja, tekst/strelice, redoslijed numeričkih polja i vrijednosti: dimenzije/datum, fizički frame i porijeklo, D/H/W/s0/prolaz, interval potvrđenog razmaka, Ri/izolacije/razmaci/ugao/strana/prozor/granice; zatim osam osnih tačaka i kritični par. Sama standardna blok geometrija čuva nacrtani snimak. Ovo nije zapis dovoljan za regenerisanje živog TVS sklopa niti obećanje nove pretrage identičnog rezultata. Postojeća V1 šema etažiranja je netaknuta.

COPY: standardni COPY kopira XRecord i time ostavlja isti identitet **istog statičkog snimka**, ne stvara novu nezavisnu operativnu jedinicu. Nema reactor-a niti automatskog preimenovanja. Prije svakog umetanja skeniraju se direktne blok reference u svim lokalnim block definition zapisima (Model/Paper i ugniježđene definicije), provjeravaju oznake i GUID-i. Duplikati blokiraju novo umetanje s jasnom porukom; korisnik treba da ukloni kopiju, a za novi nezavisni detalj ponovo upotrijebi Umetni detalj. XREF sadržaj nije skeniran. Višestruko umetanje spoljne roditeljske definicije ili zaseban DWG/WBLOCK nije globalni registry identiteta; puna COPY/INSERT/WBLOCK podrška nije obećana. Ne koristiti kopije kao različite identifikovane detalje. Eksplicitna provjera je dostupna i kroz `TVS_BYPASS_DETAIL_CHECK`.

## Atomski upis

Prije potvrde koriste se samo transient objekti. Poslije potvrde jedna transakcija obuhvata nove stilove/sloj, definiciju bloka, entitete i reference/XRecord. Svi API statusi koji utiču na upis se provjeravaju; izuzetak abortira transakciju. Nema otvaranja trase/prepreke za pisanje. Standardni objekti nisu TVS oprema i ne dodaju se u TVS mase/količine. Praktično provjeriti specifikaciju, a ne samo broj Model space entiteta.

## Obavezna praktična matrica — neizvršena dok se posebno ne evidentira

1. Iznad/ispod, 30/45/60°, izolacija 0/25, početni datum 0/negativan/pozitivan, mm/cm/m. DIST/LIST u lokalnom bloku ili kopiji za EXPLODE: fizička dužina u DWG jedinicama × faktor = prikazana mm kota. W/H/D/S1/S2, četiri stvarna kružna koljena i osni lukovi.
2. Osnovno/Detaljno: čitljivost oznaka, odvojeni geometrijski i vertikalni razmak, tekst/strelice 0.5/3.5/10/100 mm. Povećanje anotacija ne smije promijeniti fizičke mjere.
3. Promjena stvarnog ulaza poništava umetanje; Kote/Ose ne pokreću novi proračun. Dupla oznaka različitog case-a odbijena. COPY pa CHECK/novo umetanje mora prijaviti kopirani identitet; brisanje kopije uklanja blokadu.
4. Esc u svakom promptu, Otkazi i promjena pozicije: nema novih blokova/stilova/slojeva ili Model entiteta. Potvrda: tačno jedna standardna blok referenca; Undo vraća pređašnje stanje, Redo vraća kompletan detalj i kote.
5. Snimiti DWG, zatvoriti AutoCAD, otvoriti u sesiji bez TVS-a; detalj i standardne kote ostaju vidljivi, mjere ostaju tačne. REGEN, pomjeranje/rotacija cijelog bloka. Ne skalirati blok ako treba sačuvati pravilo 1:1.
6. Prije/poslije provjeriti originalne handle-ove/podatke trase i prepreke, TVS specifikaciju i masu. DWGMOD poslije uspješnog umetanja treba da se promijeni jer je standardna dokumentacija sada odobren upis.
7. DEV injekcija greške tokom upisa dokumentovana u završnom izvještaju; abort ne smije ostaviti ni reference ni definicije ni stilove. Ne proglašavati transakcijsko ponašanje/Undo runtime prolazom na osnovu kompajliranja.

Automatizovane testove, DEV buildove i praktične AutoCAD provjere izvještavati odvojeno. Po isporuci stati radi pregleda. Produkciona grana/bundle se ne mijenjaju.
