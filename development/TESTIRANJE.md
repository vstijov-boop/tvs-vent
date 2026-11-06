# TVS_VENT – period testiranja (objedinjeno na master)

## Novi meni (TVS_VENT.cuix)
Učitajte novi TVS_VENT.cuix (CUILOAD / Menuload ili zamjena postojećeg fajla).
Nove komande su u meniju "Crtanje kanala", na traci TVS_VENT i na ribbonu:

| Komanda | Naziv | Ribbon panel |
|---|---|---|
| TVS_OFFSET | Promjena kote | Crtanje kanala |
| TVS_BYPASS | Obilazak prepreke | Crtanje kanala |
| TVS_TK_TIP | Tip T komada | Crtanje kanala |
| TVS_PRELAZ_KOTA | Poravnanje prelaza | Crtanje kanala |
| TVS_FLEX_MANZETNE | Flex manžetne | Crtanje kanala |
| TVS_FLEX_MANZETNA | Flex manžetna uklj/isklj | Crtanje kanala |
| TVS_MATCH | Prenos svojstava | Atributi |
| TVS_PRESJEK | Poprečni presjek | Oznake |
| TVS_KOTE | Prikaz kota | Oznake |
| TVS_BYPASS_DETALJ | Detalj obilaska | Oznake |

TVS_MOVE i TVS_STRETCH imaju nove ikone.
Regularni paket (tvs-vent-autocad-20xx-x64) i DEV paket sada imaju iste komande, uključujući TVS_OFFSET, TVS_BYPASS i TVS_BYPASS_DETALJ.

## Otvoreno – treba testirati i javiti nazad
**Prelaz – prenos dimenzija na susjedne kanale:** ispravljeno (vidi „Dimenzije na spojevima prelaz–kanal“ niže); provjeriti oba smjera.

## Novo – testirati
- **Štucna 45° (ASHRAE SR5-3):** opcija `Stucna` u TVS_CONNECTW, pri spajanju tokom crtanja, u opciji Tkomad i u TVS_TK_TIP. L = 0,25 × širina ogranka, min. 75 mm; kosi ulaz uzvodno (oKreni ga prebacuje); glavni kanal se crta neprekinut.
- **TVS_ERASE:** TK-O, TK-G, štucna i bočno poravnati prelazi se razbijaju u nacrtani oblik.
- **Etaž prati kotu kanala:** promjena kote kanala uz etaž (Properties, Svojstva, TVS_MATCH) pita „Prilagoditi etaž? [Da/Ne]“. Ugao, radijus i dimenzije ostaju; mijenja se dužina etaža u osnovi, a promijenjeni kanal prati koljeno (komanda TVS_ETAZ_KOTA se pokreće sama). Obilazak (TVS_BYPASS) se ne mijenja ovim putem.
- **Stil oznaka:** TVS_MleaderStyle iz Resources\TVS_Oznake.dwg se učitava sam.

- **TVS_STRETCH i etaž:** etaž (TVS_OFFSET) se pomjera cijeli kad ga okvir obuhvati; ako ga okvir presijeca, komanda to javi. Susjedni kanali se produžavaju/skraćuju.
- **Maskiranje:** zadana vrijednost za nove elemente je 1 mm (umjesto 50).

- **Dimenzije na spojevima prelaz–kanal:** promjena dimenzije kanala (Svojstva, dvoklik, Properties) mijenja dimenziju prelaza na tom kraju; promjena dimenzije prelaza mijenja kanal na tom kraju (i dalje do sljedećeg prelaza na drugom kraju tog kanala). Komande TVS (crtanje, TVS_MATCH ...) ovo ne pokreću. Pokreće se samo (TVS_DIM_PRATI).
- **TVS_IZVOZ (Izvoz bez TVS):** pravi kopiju crteža u kojoj su TVS elementi zamijenjeni linijama; otvoreni crtež ostaje netaknut. Elementi na zamrznutim slojevima ostaju TVS objekti (komanda javi broj).

- **Kota u dijalogu dimenzija tokom crtanja → etaž:** kad u TVS_DRAW/TVS_PIPE opcijom Dimenzija promijeniš kotu, otvara se prošireni dijalog etaža sa predloženom novom kotom (U redu = etaž, Otkaži = bez etaža); etaž je na postojećoj dimenziji; ako se mijenja i dimenzija, poslije etaža ide prelaz, a nova kota (Vrh/Osa/Dno iz dijaloga) važi za novi kanal. Ne: kao do sada.

- **Prošireni dijalog Z (etaž):** opcija Z / TVS_OFFSET: postojeći vrh, osa i dno kanala; novo stanje biraš kao Vrh/Osa/Dno i upisuješ vrijednost (ostale dvije se prikazuju); ugao 30/45/60 ili upisan; odmah razlika kota, kosi dio i dužina etaža u osnovi ili razlog zašto nije izvodljivo; skica etaža u presjeku. U redu ne prolazi dok etaž nije izvodljiv.

- **TVS_STRETCH i obilazak (TVS_BYPASS):** kad okvir presijeca kanal prije i poslije obilaska i obuhvata cijeli obilazak, obilazak se pomjera cio (7 članova) duž kanala; ulazni kanal se produži, izlazni skrati. Ako okvir presijeca obilazak, komanda to javi. Prepreka se ne pomjera.
- **TVS_MOVE i etaž/obilazak:** bočno pomjeranje kanala u liniji sa etažom ili obilaskom pomjera i etaž/obilazak cio; kanali iza koljena se produže/skrate. Kanal koji je dio etaža ili obilaska se ne bira (komanda javi).
- **Oznake (MLeader)** na članovima pomjerenog etaža/obilaska idu zajedno sa njima.

- **Štucna 45° za okrugli ogranak na pravougaonom kanalu:** TVS_CONNECTW (i spajanje tokom crtanja) pita „Tip T komada, okrugli ogranak [Stucna/Ravni]“; TVS_TK_TIP na takvom T komadu nudi Stucna/Ravni/oKreni. Ista pravila kao SR5-3: L = 0,25 × D, min. 75 mm; kosi ulaz uzvodno. U osnovi se crta kao pravougaona štucna širine D. Specifikacija: „Štucna 45°, okrugli ogranak“ A×B / ØD (L=). Osa okruglog kanala mora biti na koti ose pravougaonog: inače TVS_CONNECTW (sa štucnom) javi dijalog i ne pravi spoj, a TVS_STRETCH/TVS_MOVE javi dijalog sa obje kote.

- **Uzdužni presjek (P2):** TVS_PRESJEK → opcija **Uzduzni**: klikni prvi i zadnji element trase; program nađe put kroz mrežu (i kroz T komad), istakne trasu i razvije je u ravan (ista razmjera po dužini i visini). Crta: kanale, kose dijelove i koljena gore/dolje Z prelaza i obilaska, koljena u osnovi i T komade na svojoj koti (oznaka „koljeno“, „T komad“, „prelaz“), prelaze kao trapez, presječene kanale drugih trasa (pravougaonik sa X / krug, dimenzija, kota dna), linije etaža (pita kote, kao P1). Ispod: tabela Stacionaža [m] / Kota dna [m] / Dimenzija. Blok TVS_UZDUZNI_<oznaka>; **Osvjezi** ga ponovo generiše; u osnovi oznaka slova na oba kraja trase.

- **oKreni uz prikaz:** pri spajanju sa štucnom / TK-O / TK-G (TVS_CONNECTW, spajanje tokom crtanja) i u TVS_TK_TIP T komad je nacrtan i istaknut prije pitanja „[oKreni] <Gotovo>“; oKreni se može ponavljati dok se ne pritisne Enter.

- **Dimenzija cijele dionice do prelaza (plan 14, faza 1):** promijeni dimenziju kanala (Svojstva / Properties) u dionici sa koljenima u osnovi; program pita „Promijeniti cijelu dionicu do prelaza (koljena N, kanala M)? [Da/Ne] <Da>“. Na Da: koljena dobiju novu dimenziju i radijus, kanali iste stare dimenzije novu dimenziju, krajevi kanala prate priključke koljena; prelaz na kraju preuzme dimenziju kao do sada. Staje na etažu, obilasku, T komadu, koljenu gore/dolje, elementu druge dimenzije (ispiše gdje). Ako bi kanal postao prekratak – ništa se ne mijenja.

- **MIRROR fitinga:** koljena u osnovi i prelazi poravnati uz jednu stranu se ispravno preslikavaju (koljeno obrne luk, prelaz zamijeni stranu); T komadi i kanali kao i do sada. Etaž i obilazak se i dalje ne mogu preslikati (poruka) – samo pomjeriti / rotirati.

## Odloženo
- Koljeno direktno na priključku grane T komada.
- Detalji koljena u presjeku (kao TVS koljena gore/dolje).
- Finese nastavka crtanja poslije umetanja T komada.
