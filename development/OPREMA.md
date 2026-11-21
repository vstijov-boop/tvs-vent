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
