# TVS_VENT – plan razvoja

Redoslijed po dogovoru (5. 11. 2026). Stavke 1–6 su urađene i na masteru; u 7 je urađen P2, ostalo iz 7 slijedi.

## Urađeno
1. **Prelaz prati dimenzije susjednih kanala** – promjena ulaznog kanala mijenja prvu dimenziju prelaza, izlaznog drugu; promjena dimenzije prelaza mijenja samo kanal na tom kraju.
2. **Spajanje na master** – testirano stanje `razvoj-tkomad` (štucna, razbijanje, etaž, stil oznaka, TVS_STRETCH).
3. **Izvoz crteža bez TVS_VENT** – kopija DWG-a sa razbijenim TVS objektima; original ostaje netaknut.

## Otvoreno (poslije prioritetnih stavki)
- **TVS_IZVOZ sa obilaskom:** izvoz crteža koji ima TVS_BYPASS uspije, ali AutoCAD pri snimanju kopije javi „An error occurred during save. We recommend that you run RECOVER“ (AUDIT ne nalazi greške). Uklanjanje TVS zapisa iz NOD-a kopije nije pomoglo. Crtež bez obilaska se izvozi bez upozorenja. Sumnja: kloniranje članova obilaska (subWblockClone / remapClones) ili njihovi zapisi u kopiji.

## Sljedeće (redom)
4. *(urađeno, potvrđeno 5. 11.)* **Promjena kote tokom crtanja → etaž** – kad se u dijalogu dimenzija tokom TVS_DRAW/TVS_PIPE promijeni kota, ponuditi „Umetnuti etaž? [Da/Ne]“ (isti put kao opcija Z: tvsOffsetIntegration); ako se mijenja i dimenzija, prvo etaž na postojećoj dimenziji, pa prelaz.
5. *(urađeno, potvrđeno 5. 11.; otvara se i iz dijaloga dimenzija tokom crtanja)* **Prošireni dijalog promjene kote (Z)** – postojeće stanje: vrh / osa / dno kanala (samo prikaz); novo stanje: izbor reference (Vrh/Osa/Dno) i vrijednost (npr. vrh 2950 → 2750); ugao (30/45/60°); odmah prikaz razlike kota, kosog dijela, dužine u osnovi i da li je izvodljivo (premala razlika za dva koljena); skica kao kod prelaza.
6. *(urađeno, potvrđeno 6. 11.)* **Etaž u TVS_MOVE i obilazak (TVS_BYPASS)** – isto ponašanje kao TVS_STRETCH i prilagođavanje kote.
7. **Odloženo iz ranijih faza** – uzdužni presjek P2 *(urađeno, potvrđeno 6. 11.: TVS_PRESJEK → Uzduzni)*, detalji koljena u presjeku, koljeno direktno na grani T komada, finese nastavka crtanja poslije T komada.
8. **Okrugla štucna** (konusna / 45° na okruglom kanalu). *(Okrugli ogranak na pravougaonom kanalu sa štucnom 45° urađen 6. 11.; ostaje okrugli – okrugli: konusna / 45°.)*
9. **ASHRAE oznaka fitinga** (SR5-3 štucna, CR3-x koljeno, SR4-x prelaz …) u specifikaciji i izvozu – osnova za proračun pada pritiska (KanCalc).
10. *(urađeno 6. 11., u testiranju)* **Podešavanja na jednom mjestu** – `Settings.h` statičke promjenljive (kopija po .cpp fajlu) u jedan zajednički objekat.
11. *(urađeno 6. 11., u testiranju: indeks identiteta etaža sa reaktorom baze; trasa P2 kroz mrežu ćelija)* **Brzina na velikim crtežima** – jedan indeks elemenata (položaj, kote) umjesto prolaska kroz cijeli Model prostor u TVS_STRETCH, presjeku i provjeri etaža.
12. *(urađeno 6. 11., u testiranju: komande u `TVS_Ventilation_ARX/commands/*.inl`)* **Razbijanje `acrxEntryPoint.cpp`** (~4300 linija) – komande u zasebne fajlove.
13. **Instalacioni paket** – build pravi gotov `TVS.bundle` (PackageContents.xml, verzija, Resources\TVS_Oznake.dwg).
14. **Dimenzija cijele dionice do prelaza** *(dogovoreno 6. 11.; faza 1 potvrđena 6. 11.; faza 2 potvrđena 6. 11.; faza 3 potvrđena 6. 11.)* – danas promjena dimenzije kanala (Svojstva, Properties) mijenja samo taj kanal (i prelaz na njegovom kraju); koljena, etaž i kanali iza njih ostaju stari. Novo: pitanje „Promijeniti cijelu dionicu do prelaza? [Da/Ne]“; na Da program ide mrežom u oba smjera i mijenja sve do granice. Granice: prelaz (preuzme novu dimenziju na svom kraju), slobodan kraj, T komad (njegov priključak na toj strani preuzme dimenziju; ogranak / štucna se ne mijenja).
    - **Faza 1:** kanali i koljena (u osnovi i gore/dolje) između dva prelaza ili slobodna kraja; koljenima se mijenja radijus/dužina, susjedni kanali se produže/skrate. Na etažu, obilasku i T komadu program stane i javi zašto.
    - **Faza 2:** etaž (ponovni proračun kosog dijela kao kod TVS_ETAZ_KOTA; ako više ne staje – poruka) i T komad kao granica.
    - **Faza 3:** obilazak – provjera razmaka do prepreke sa novom dimenzijom (upozorenje, bez automatske izmjene); kasnije opcija da se nastavi kroz prolaz T komada.
15. **TVS_LEAD_Recalculate – sve oznake odjednom** *(predloženo 6. 11.)* – danas ažurira izabrane MLeader oznake (običan AutoCAD izbor; prozorom se već mogu izabrati sve oznake u pravougaoniku). Novo: opcija **Sve** (sve TVS oznake u crtežu / aktivnom prostoru), sa ispisom koliko je ažurirano i koje se ne mogu ažurirati (element obrisan). Kasnije, po izboru: automatsko ažuriranje oznaka elemenata koje su promijenile TVS komande (dimenzija dionice, TVS_STRETCH, MIRROR).
