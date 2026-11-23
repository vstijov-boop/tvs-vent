# Oprema na mreži (TVS_OPREMA)

Obični AutoCAD 2D blokovi (nisu dinamički) postaju dio TVS kanalske mreže:
- **u liniji, 2 priključka:** regulator protoka, PP klapna, CAV, VAV, prigušivač buke – kanal je presječen, blok stoji između dva komada kanala;
- **terminal, 1 priključak:** rešetka, difuzor, krovna kapa, protivkišna žaluzina – na slobodnom kraju kanala ili flex crijeva (bočna ugradnja kasnije).

Zajednički zapis (osnova, `geometry/Equipment.h`, `TVS_Ventilation_ARX/EquipmentStore.h`): XRecord `TVS_OPREMA_V1` u bloku (vrsta, oznaka, opis, priključci na osi kanala, kota = kota ose kanala, dimenzija kanala, dužina, model gubitka) i podrazumijevani zapis po nazivu bloka u `TVS_OPREMA_TIPOVI`. Samo horizontalno (osnova).

## Povezivanje – TVS_OPREMA / TVS_OPREMA_ODVEZI
Već nacrtani blokovi se povezuju NA MJESTU, blok se nikad ne pomjera:
1. **kanali već naliježu** na lica bloka (tolerancija 1 mm) → samo zapis;
2. **mali razmak / preklop** → krajevi kanala se skrate / produže do lica bloka (razmak do 150 mm, opcija `Tolerancija`; kraj kanala ispod bloka uvijek);
3. **neprekinut kanal ispod bloka** → kanal se presiječe za dužinu bloka (novi komad sa svim svojstvima kanala, kao TVS_CUT); komad ne smije ostati kraći od 50 mm.

- Dužina bloka = granice definicije bloka (bez definicija atributa) preslikane u crtež (blockTransform, pa radi i zarotiran blok), projektovane na osu kanala; ili dužina zadata za naziv bloka (centrirano na blok).
- Osa kanala mora proći kroz blok, najviše 25 mm od sredine bloka. Kanali sa dvije strane iste dimenzije i kote. Kraj na licu bloka ne smije biti spojen sa trećim elementom.
- U liniji ili terminal odlučuje geometrija; ako za naziv bloka postoji zapis, vrsta ga potvrđuje (rešetka preko kontinualnog kanala ili regulator sa kanalom samo s jedne strane → odbijeno).
- Zapis: iz ranijeg povezivanja (ponovo) ili iz `TVS_OPREMA_TIPOVI`, inače vrsta Rešetka (terminal) / Ostala oprema (u liniji); priključci, `mmPerUnit`, dimenzija i oblik kanala se uvijek upisuju iznova.
- Svaki blok: provjera (zaključan sloj, etaž / obilazak), pa jedna transakcija – odbijen blok ne mijenja ništa. Na kraju: povezano (u liniji / terminali), skraćeni / produženi krajevi, presječeni kanali, odbijeni po razlogu (svaki sa handle-om; odbijeni ostaju izabrani).
- `Sve` = svi blokovi trenutnog prostora koji leže na kanalu; xref blokovi se preskaču. Već povezani se preskaču, osim ako se odgovori `Da` na pitanje o ponovnom povezivanju.
- `TVS_OPREMA_ODVEZI`: briše zapis iz izabranih blokova; kanali ostaju kakvi jesu.

Kod: `geometry/EquipmentLink.h` (geometrija i odluka, test `tests/test_equipment_link.cpp`), `TVS_Ventilation_ARX/EquipmentLinkCommand.h` (komande).

## Oprema u proračunima mreže (TVS_PROTOK, TVS_PAD, TVS_DPD_IZVOZ, TVS_DIM_PRATI, TVS_PROVJERA)
- Čitanje: TVS_Ventilation_ARX/EquipmentNodes.h dodaje povezane blokove trenutnog prostora mreži; priključci se preračunavaju u jedinice crteža, a z priključka uzima z kraja kanala na koji naliježe u osnovi (kanali nose kotu u Elev). Pravila: geometry/EquipmentFlow.h.
- **TVS_PROTOK**: terminal ima protok iz zapisa; bez njega protok kanala na kome stoji (ranije upisan); oba i različita → upozorenje, važi oprema. Protok terminala se upisuje u zapis opreme, ne u kanal. Inline oprema nema protok.
- **TVS_PAD**: inline oprema je lokalni gubitak unutar dionice (kao koljeno, ne počinje novu dionicu, bez dužine), Δp po modelu (tvsEquipment::loss, pd kanala na ulazu). Terminal završava dionicu, njegov Δp je „Δp kraj. elem.“ (Fitinzi red bez ζ, Izvor „Oprema“), bez rezerve; oznaka terminala je natpis kraja u Šemi. „Za ventilator“: rezerva se ne primjenjuje na Δp krajnjih elemenata.
- **TVS_DPD_IZVOZ**: oprema ide kao oprema na DuctPressDrop dionici (Fiting.oprema = tip iz palete, položaj, oznaka): terminal na slobodnom kraju, inline na svom mjestu; fiksni Δp → dp_fix, ζ → konst / ručni ζ, katalog → dp_fix izračunat pri izvezenom Q (u DuctPressDrop-u fiksan, poruka).
- **TVS_DIM_PRATI**: povezan blok je granica dionice („oprema“), dimenzija se kroz njega ne prenosi.
- **TVS_PROVJERA**: „oprema: priključak ne naliježe na kanal“ – priključak nije na kraju kanala iste dimenzije i kote (1 mm), sa zumom.
- Ne radi (još): TVS_MOVE / TVS_STRETCH ne vide blokove; TVS_PROTOK „Primijeni“ (dimenzionisanje) ne mijenja opremu – granica kao nepoznat element.

Obični AutoCAD 2D blokovi opreme (fiksni, nisu dinamički) postaju dio TVS mreže kanala:
- u liniji kanala, 2 priključka: regulator protoka, PP klapna, CAV, VAV, prigušivač buke – kanal je presječen, blok stoji između dva komada kanala;
- krajnji element, 1 priključak na slobodnom kraju kanala / flexa: rešetka, difuzor, krovna kapa, protivkišna žaluzina (bočne rešetke kasnije).

Već nacrtani blokovi se povezuju na mjestu (ne umeću se ponovo). Kota priključka = kota ose kanala. Za sada samo horizontalno.
Zapis u bloku: `geometry/Equipment.h` (vrsta, oznaka, opis, priključci, dimenzija, dužina, model pada pritiska, protok), čuva se u bloku (XRecord `TVS_OPREMA_V1`) i po nazivu bloka (`TVS_OPREMA_TIPOVI`), `TVS_Ventilation_ARX/EquipmentStore.h`.

## Svojstva, specifikacija i meni

**TVS_OPREMA_SVOJSTVA** (`TVS_Ventilation_ARX/EquipmentDialog.h`, dijalog `IDD_EQUIPMENT`; čista logika `geometry/EquipmentEdit.h`, test `tests/test_equipment_edit.cpp/.py`):
- jedan ili više povezanih blokova (izbor prije ili poslije komande); nepovezan blok: „Blok nije povezan u mrežu (TVS_OPREMA).“;
- polja: Vrsta, Oznaka (+ „Nova“ = najveći broj te vrste u crtežu + 1, npr. PPK-03; prefiks po vrsti), Opis / tip, Model pada pritiska (Bez / Fiksni Δp / ζ / Δp_n pri Q_n, aktivna samo polja modela), Protok m³/h (samo krajnji element sa 1 priključkom); priključci, dimenzija i dužina samo za čitanje;
- više blokova: polje koje se razlikuje pokazuje „Različito“ i ostaje kakvo jeste u svakom bloku dok se ne promijeni (brojevi se porede kao brojevi, kao u dijalogu Svojstva);
- provjera: Δp, ζ, Δp_n ≥ 0; model Δp_n pri Q_n traži Q_n > 0;
- „Primijeni na sve blokove ovog naziva“: zapis bez priključaka, oznake i protoka postaje podrazumijevani za naziv bloka (`writeDefault`), a vrsta, opis, model i vrijednosti idu u sve povezane blokove tog naziva u crtežu (oznaka i protok ostaju svoji);
- svi blokovi se upisuju u jednoj transakciji poslije provjere zaključanih slojeva (jedan UNDO korak, sve ili ništa); podrazumijevana svojstva po nazivu poslije toga.

**Specifikacija** (TVS_SPEC / TVS_EXCELSPEC, `SPEC::addEquipment`): povezan blok je stavka tabele blokova – Pozicija = oznaka, Naziv = opis / tip ili naziv vrste, Tip i dimenzije = `a×b` / `Øa` mm, jedinica kom; grupiše se kao ostali blokovi (oznaka, naziv, dimenzija …). Šifra, proizvođač, masa i napomena i dalje iz atributa ako ih blok ima. Nepovezani blokovi – kao ranije (atributi); TVS_BLCKMNGR bez promjene.

**Meni / ribbon** (`tools/add_new_commands_cuix.py`, `Installer/TVS_VENT/TVS_VENT.cuix`, ikone `Installer/TVS_VENT/icons/TVS_OPREMA*_16/32.png`): „Oprema na mreži“ (TVS_OPREMA), „Odveži opremu“ (TVS_OPREMA_ODVEZI), „Svojstva opreme“ (TVS_OPREMA_SVOJSTVA) u panelu / meniju „Crtanje kanala“.

## Priključenje rešetke – TVS_RESETKA
Crta priključak rešetke koji se do sada crtao ručno (primjeri korisnika: rešetka na bočnoj strani i u dnu kanala) i povezuje blok rešetke u mrežu (korisnik 2026-11-23).
1. Blok rešetke (jedan; izbor prije komande važi), 2. kanal – Enter / `Najblizi` = kanal čija je osa najbliža tački umetanja bloka u osnovi (istaknut), 3. dijalog „Priključenje rešetke“ (`IDD_GRILLE`):
- **Strana:** Lijevo / Desno (gledano u smjeru crtanja kanala, početak → kraj) / Dno; predizbor: Dno kad blok leži preko kanala (sredina ili tačka umetanja bliže osi od pola širine kanala), inače strana ose na kojoj je tačka umetanja.
- **Način:** Ravni T komad (TK0, dužina kao TVS_CONNECTW iz TVS_SETTINGS) / Štucna (SR5-3, okrugli ogranak SR5-11; samo pravougaoni kanal) / Konusna štucna (SR5-12, samo okrugli ogranak) / Direktno na stranicu (samo Dno). Za Dno su T komad i štucna isključeni (ogranak naniže horizontalna mreža još ne podržava – piše u dijalogu).
- **Dužina ravnog dijela ogranka** [mm], podrazumijevano 100, pamti se (`HKCU\Software\TVS_Vent`, vrijednost `ResetkaOgranak`).
- **Dimenzija ogranka** A × B ili okrugli D: iz zapisa bloka (priključak rešetke), inače iz podrazumijevanog zapisa za naziv bloka, inače širina bloka × visina kanala (strana) / širina × dubina bloka (dno).
- Provjera (dijalog ostaje otvoren): ogranak nije viši od bočne stranice kanala (konusna: baza konusa), priključak na dnu nije širi od kanala (manja stranica poprijeko), dužina ogranka > 0, tačka priključenja najmanje pola T komada / štucne + 50 mm od oba kraja kanala – osim kad je bliži kraj **slobodan** (vidi dolje); dno: priključak naspram kanala, pola priključka od krajeva.

Šta se radi (jedna transakcija, jedan UNDO korak, sve ili ništa; prije toga provjera zaključanog sloja / etaža / obilaska):
- **Lijevo / Desno:** tačka priključenja = projekcija tačke umetanja na osu. Kanal se dijeli na T komadu: original ide do ulaza T komada, novi komad (sva svojstva kanala, kao TVS_OPREMA / TVS_CUT) od izlaza do starog kraja. T komad TVS_WYE: ravni TK0 (LengthPl = dužina T komada) ili štucna sa zapisom TVS_TEE_V1 (L = 0,25 × širina, min. 75 mm; konusna L = 100 mm i baza konusa; vod samo 25 mm preko otiska – kao TVS_TK_TIP), kosi ulaz štucne uzvodno u smjeru crtanja kanala. **Rešetka kod slobodnog kraja kanala** (tipičan slučaj, primjer p.dxf: kraj kanala 17603, T komad 17603–18153, osa ogranka 17878): kad je projekcija bliža od pola T komada + 50 mm slobodnom kraju (na njemu nema drugog elementa), T komad se stavlja osom ogranka na projekciju, kanal se skraćuje / produžava do ulaza T komada, novi komad se ne pravi, a prolazni izlaz T komada ostaje slobodan (mreža ga vidi kao čep); T komad smije preći stari kraj kanala. Odbija se samo ako taj kraj nije slobodan ili je kanal prekratak i sa druge strane. Ogranak: prav kanal dimenzije ogranka od priključka T komada napolje, iste kote kao kanal, protok 0. Blok se pomjera i okreće (`transformBy`, atributi idu sa njim) tako da mu je **bazna tačka na slobodnom kraju ogranka**.
- **Pravilo orijentacije bloka** (iz primjera p.dxf: Res_2 bočno, ogranak prema −y, rotacija 0; Res_2 na kraju kanala nacrtanog prema +x, rotacija 90; granice bloka lokalno y −10…60 leže na ogranku): **lokalna −y osa bloka gleda napolje** (od kanala), +y u ogranak. Neogledalan blok u osnovi: rotacija = atan2(oₓ, −o_y) za jedinični vektor napolje o; komanda okreće blok za ugao od trenutnog pravca lokalne −y do o, pa radi i ogledan / skaliran blok.
- Zapis rešetke: 1 priključak na slobodnom kraju ogranka (mount AtEnd, smjer u ogranak), dimenzija = ogranak, `mmPerUnit` crteža; vrsta / opis / model / oznaka / protok iz ranijeg zapisa bloka ili podrazumijevanog za naziv bloka, inače Rešetka. Blok zapisan kao oprema u liniji (regulator …) se odbija.
- **Dno + direktno:** ništa se ne crta; zapis mount Bottom, priključak = projekcija sredine bloka na osu (kota ose, bez smjera), dimenzija = kanal, priključak rešetke (`neck`) = dimenzija iz dijaloga.

Kod: `geometry/GrilleConnect.h` (tačka priključenja, strana, raspored T komada / ogranka / bloka, provjera; test `tests/test_grille_connect.cpp` sa geometrijom primjera), `TVS_Ventilation_ARX/GrilleCommand.h` (komanda i dijalog), meni / ribbon „Priključenje rešetke“ (`tools/add_new_commands_cuix.py`, ikone `TVS_RESETKA_16/32.png`).
Ne radi (još): ogranak naniže (T komad na dnu), ogranak drugačije kote, linijski difuzori sa dva priključka.
