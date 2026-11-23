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
- **Terminal na vertikalnom izlazu koljena gore / dolje** (TVS_TAP Form_Up / Form_Down): priključak u osnovi na izlazu koljena (1 mm; pravougaono MiddlePoint, okruglo tvsRoundElbow::ports, inače MC), kota izlaza = kota koljena ∓ rc·(1 − cos ugla) → mreža ga spaja sa izlazom koljena (EquipmentNodes.h). TVS_PROTOK sada (kao TVS_PAD) spaja koljeno gore / dolje sa kanalom u osnovi i kad z nije isti. Gubitak koljena ostaje kao ranije; terminal je kraj dionice. TVS_PROVJERA poredi kotu priključka sa kotom izlaza koljena.
- **Rešetka na dnu kanala** (mount Bottom): priključak je tačka na osi kanala (1 mm, unutar kanala, najbliža kota), kanal se NE siječe u crtežu. Čitači (TVS_PROTOK / TVS_PAD / TVS_DPD_IZVOZ, FlowCommand.h splitBottom) ga za proračun dijele na dva komada i čvor odvajanja; rešetka je ogranak bez dužine, presjek = priključak (neckA × neckB / okrugli; 0 = kanal). Gubitak: T po KanCalc / Idelchik tabelama (dovod razdvajanje 0.7 / 0.6, odvod sabiranje 1.7 / 1.6; ogranak = priključak, Qb = protok rešetke, prolaz = kanal), C na Vc, plus gubitak same rešetke (Δp kraj. elem., bez rezerve). Zatvoren kraj iza rešetke nema protok i nije u mreži (kao čep T komada), u DuctPressDrop ide kao dionica bez protoka da bi se izveo T komad. TVS_PROTOK: protok rešetke u blok, na kanal protok ispred rešetke; dimenzionisanje vidi kanal kao jedan. TVS_DPD_IZVOZ: čvor na kanalu, ogranak smjer (0, 0, −0,10) m. Šema: crtica preko kanala sa oznakom. TVS_PROVJERA: „oprema: rešetka na dnu nije na osi kanala“. Zatvoren kraj nema protok – ako je otvoreni kraj kanala iza rešetke krajnji element, mora biti povezan terminal.
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
