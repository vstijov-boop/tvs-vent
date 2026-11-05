# TVS_VENT – plan razvoja

Redoslijed po dogovoru (5. 11. 2026). Stavke 1–3 su u radu, ostale idu redom.

## U radu
1. **Prelaz prati dimenzije susjednih kanala** – promjena ulaznog kanala mijenja prvu dimenziju prelaza, izlaznog drugu; promjena dimenzije prelaza mijenja samo kanal na tom kraju.
2. **Spajanje na master** – testirano stanje `razvoj-tkomad` (štucna, razbijanje, etaž, stil oznaka, TVS_STRETCH).
3. **Izvoz crteža bez TVS_VENT** – kopija DWG-a sa razbijenim TVS objektima; original ostaje netaknut.

## Otvoreno (poslije prioritetnih stavki)
- **TVS_IZVOZ sa obilaskom:** izvoz crteža koji ima TVS_BYPASS uspije, ali AutoCAD pri snimanju kopije javi „An error occurred during save. We recommend that you run RECOVER“ (AUDIT ne nalazi greške). Uklanjanje TVS zapisa iz NOD-a kopije nije pomoglo. Crtež bez obilaska se izvozi bez upozorenja. Sumnja: kloniranje članova obilaska (subWblockClone / remapClones) ili njihovi zapisi u kopiji.

## Sljedeće (redom)
4. **ASHRAE oznaka fitinga** (SR5-3 štucna, CR3-x koljeno, SR4-x prelaz …) u specifikaciji i izvozu – osnova za proračun pada pritiska (KanCalc).
5. **Etaž u TVS_MOVE i obilazak (TVS_BYPASS)** – isto ponašanje kao TVS_STRETCH i prilagođavanje kote.
6. **Odloženo iz ranijih faza** – podužni presjek P2, detalji koljena u presjeku, koljeno direktno na grani T komada, finese nastavka crtanja poslije T komada.
7. **Okrugla štucna** (konusna / 45° na okruglom kanalu).
8. **Podešavanja na jednom mjestu** – `Settings.h` statičke promjenljive (kopija po .cpp fajlu) u jedan zajednički objekat.
9. **Brzina na velikim crtežima** – jedan indeks elemenata (položaj, kote) umjesto prolaska kroz cijeli Model prostor u TVS_STRETCH, presjeku i provjeri etaža.
10. **Razbijanje `acrxEntryPoint.cpp`** (~4300 linija) – komande u zasebne fajlove.
11. **Instalacioni paket** – build pravi gotov `TVS.bundle` (PackageContents.xml, verzija, Resources\TVS_Oznake.dwg).
