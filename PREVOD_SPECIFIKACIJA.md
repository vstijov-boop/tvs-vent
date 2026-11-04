# Prevod specifikacija — AutoCAD i Excel

Grana: `srpski-prevod`. Prevod je unesen u izvorni kod. Nije napravljena
nova Windows ARX/DBX verzija niti izvršen izvoz kroz AutoCAD/Excel.

## Podrazumijevana zaglavlja

| Kolona | Srpski latinicom |
| --- | --- |
| Pozicija | Pozicija |
| Naziv i opis | Naziv i tehničke karakteristike |
| Tip/model/dokumentacija | Tip, model i oznaka dokumentacije |
| Kod proizvoda | Šifra proizvoda |
| Dobavljač | Dobavljač |
| Jedinica mjere | Jed. mjere |
| Količina | Količina |
| Masa jedne jedinice | Masa/jed., kg |
| Napomena | Napomena |

Isti šablon koristi se za crtanje tabele u AutoCAD-u i izvoz u Excel.
U starijim specifikacijama prevedena su i odgovarajuća zaglavlja, jedinice,
automatski nazivi elemenata i zbirni natpis `Ukupno:`.

## Automatski nazivi i jedinice

| Izvorni naziv / jedinica | Prevod |
| --- | --- |
| Воздуховод | Kanal |
| Воздуховод гибкий | Fleksibilni kanal |
| Воздуховод круглый | Okrugli kanal |
| Воздуховод прямоугольный | Pravougaoni kanal |
| Отвод | Koljeno |
| Отвод круглый | Okruglo koljeno |
| Отвод прямоугольный | Pravougaono koljeno |
| Тройник | T-komad |
| Переход | Prelaz |
| шт / шт. | kom |
| м | m |
| м2 / м² | m² |
| м3 / м³ | m³ |
| кг | kg |

Aktivni getter-i automatskih naziva i jedinica koriste prevedene vrijednosti
iz `commonDef.h`. Za starije specifikacije prevedeni su odgovarajući literali
u `SPEC.cpp`. Konverzije količina, dužina, površina i mase nijesu mijenjane.

U novom izvozu, `translateUnit` prevodi samo tačno prepoznate oznake jedinica
za prikaz; ne upisuje nove vrijednosti u postojeće XData zapise. Ostale
jedinice, npr. `komplet`, ostaju takve kakve ih je korisnik unio.

## Sačuvani i prilagođeni šabloni

`getRowSettings` učitava postojeći šablon kao i ranije. Prije prikaza i
upotrebe, `translateTemplate` mijenja samo tačno prepoznato rusko zaglavlje
ispred prvog znaka `;` u svakom redu. Širina i ostatak reda ostaju isti,
uključujući funkcionalne oznake `[Имя]`, `[Тип]`, `[Размер]` itd.

Primjer ranije prilagođene širine:

```text
Наименование; 4321.5; [Имя]-Poseban tekst
```

postaje:

```text
Naziv; 4321.5; [Имя]-Poseban tekst
```

Proizvoljni naslovi kao `Moj naslov` ili `Наименование opreme` ne mijenjaju
se. Čitanje samo po sebi ne prepisuje sačuvani šablon u AutoCAD podešavanjima;
postojeća funkcija čuvanja iz dijaloga podešavanja ostaje ista.

Nazivi/proizvođači/modeli/napomene iz atributa blokova i slobodni tekstovi
korisnika ne prevode se automatski. Za njih bi trebalo posebno dogovoriti
rječnik. Već nacrtane tabele i prethodno izvezeni Excel fajlovi se ne mijenjaju.

## Provjera

Iz korijena repoa, u okruženju sa GNU C++ kompajlerom:

```sh
g++ -std=c++98 -Wall -Wextra -Werror -pedantic tests/test_specification_text.cpp -o /tmp/test_specification_text
/tmp/test_specification_text
```

Testovi prolaze za fabrička zaglavlja, prilagođene širine i izraze, mješovite
završetke redova, prepoznate/nepoznate jedinice, prazne ulaze i idempotentnost
(prevod se ne mijenja pri ponovnom pozivu). Dodatni test nad stvarnim starim
i novim fabričkim šablonom takođe je kompajliran i izvršen uspješno.

Poređenje izvora potvrđuje da su u `SPEC.cpp` mijenjani samo tekstualni
literali, a u `commonDef.h` samo vrijednosti naziva i jedinica. `SPEC.h`,
`TVSProperty.h` i `TVSPropertyController.cpp` ostaju nepromijenjeni, kao i
nazivi/tagovi atributa, XData ključevi i funkcionalne oznake šablona.

Prije upotrebe treba ponovo izgraditi ARX i DBX, pa na kopiji projekta
provjeriti AutoCAD i Excel specifikaciju za okrugli/pravougaoni i fleksibilni
kanal, koljeno, T-komad, prelaz i blok s korisničkim podacima. Uporediti
količine, dimenzije, površine i zbirove; provjeriti novi i ranije sačuvani
šablon. Praktičan test obuhvata i korisnikov Excel 2013; postojeći COM način
izvoza nije mijenjan.
