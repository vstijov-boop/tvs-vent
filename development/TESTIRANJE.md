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
**Prelaz – prenos dimenzija na susjedne kanale**
1. Promjena dimenzije kanala na ulazu u prelaz: prva dimenzija prelaza se NE mijenja.
2. Promjena druge dimenzije prelaza iz dijaloga: izlaznom kanalu se mijenja dimenzija.

Provjeriti oba slučaja i ostale kombinacije (izlazni kanal → druga dimenzija prelaza,
prva dimenzija prelaza → ulazni kanal) i javiti šta je očekivano ponašanje.

## Novo – testirati
- **Štucna 45° (ASHRAE SR5-3):** opcija `Stucna` u TVS_CONNECTW, pri spajanju tokom crtanja, u opciji Tkomad i u TVS_TK_TIP. L = 0,25 × širina ogranka, min. 75 mm; kosi ulaz uzvodno (oKreni ga prebacuje); glavni kanal se crta neprekinut.
- **TVS_ERASE:** TK-O, TK-G, štucna i bočno poravnati prelazi se razbijaju u nacrtani oblik.
- **Etaž prati kotu kanala:** promjena kote kanala uz etaž (Properties, Svojstva, TVS_MATCH) pita „Prilagoditi etaž? [Da/Ne]“. Ugao, radijus i dimenzije ostaju; mijenja se dužina etaža u osnovi, a promijenjeni kanal prati koljeno (komanda TVS_ETAZ_KOTA se pokreće sama). Obilazak (TVS_BYPASS) se ne mijenja ovim putem.
- **Stil oznaka:** TVS_MleaderStyle iz Resources\TVS_Oznake.dwg se učitava sam.

- **TVS_STRETCH i etaž:** etaž (TVS_OFFSET) se pomjera cijeli kad ga okvir obuhvati; ako ga okvir presijeca, komanda to javi. Susjedni kanali se produžavaju/skraćuju.
- **Maskiranje:** zadana vrijednost za nove elemente je 1 mm (umjesto 50).

## Odloženo
- Koljeno direktno na priključku grane T komada.
- P2 – podužni presjek.
- Detalji koljena u presjeku (kao TVS koljena gore/dolje).
- Finese nastavka crtanja poslije umetanja T komada.
