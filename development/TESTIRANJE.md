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
TVS_OFFSET, TVS_BYPASS i TVS_BYPASS_DETALJ rade samo u DEV paketu (DEV-BYPASS-PERSISTENT).

## Otvoreno – treba testirati i javiti nazad
**Prelaz – prenos dimenzija na susjedne kanale**
1. Promjena dimenzije kanala na ulazu u prelaz: prva dimenzija prelaza se NE mijenja.
2. Promjena druge dimenzije prelaza iz dijaloga: izlaznom kanalu se mijenja dimenzija.

Provjeriti oba slučaja i ostale kombinacije (izlazni kanal → druga dimenzija prelaza,
prva dimenzija prelaza → ulazni kanal) i javiti šta je očekivano ponašanje.

## Odloženo
- Koljeno direktno na priključku grane T komada.
- P2 – podužni presjek.
- Detalji koljena u presjeku (kao TVS koljena gore/dolje).
- Finese nastavka crtanja poslije umetanja T komada.
