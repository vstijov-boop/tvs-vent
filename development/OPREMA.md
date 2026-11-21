# Oprema na mreži (TVS_OPREMA)

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
