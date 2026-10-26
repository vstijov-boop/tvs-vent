# Status prevoda dijaloga — srpski latinicom

Datum: 26.10.2026. Grana: `srpski-prevod`. Osnova: `master` na `5aa1ac5`.

## Završeno u izvoru

- Prevedena 154 pojavljivanja tekstova u osam definicija dijaloga.
- Prevedena zaglavlja tabela u `dgBlockManager.cpp` i `dgTVSPropertyManager.cpp`.
- Prevedena uputstva u `MLeaderSettings.h` i `TVSSpecTable.h`.
- Zamijenjen oštećeni naslov upravljača svojstvima i uklonjeni besmisleni
  podrazumijevani natpisi prilagođenih grid kontrola.
- `.rc` koristi UTF-8 sa BOM-om i obje direktive `code_page(65001)`.
  Postojeće jezičke oznake i uslovi izbora resursa su sačuvani.
- Proširen prostor pravougaonog koljena i formule radijusa; proširen dijalog
  kote i povećan dijalog specifikacije radi objašnjenja; proširena kolona Izabrano.

`IDD_DGALLEDIT1` je preveden kao postojeći resurs; u pregledanim C++/H fajlovima
nije pronađen njegov aktivni poziv. Ostalih sedam ID-jeva povezano je s klasama
odgovarajućih dijaloga. Dostupnost iz komandi provjeriti u AutoCAD-u.

## Kompatibilnost

Interni nazivi `CTVSProperty_*`, XData identifikatori, ID-jevi resursa i
podrazumijevani funkcionalni šabloni nijesu mijenjani. Proračunski kod nije
mijenjan. Nazivi i vrijednosti atributa blokova, slojeva i podaci korisnika
ostaju kakvi su bili. Funkcionalne oznake kao `[Тип]` i `[Размер]` ostaju na
ruskom, a uputstvo objašnjava njihov smisao na srpskom. Njihov prevod bi zahtijevao
poseban rad na kompatibilnosti i nije dio ovog prevoda dijaloga.

U resursima je sačuvana referenca `ВСН`. Oznake dimenzija u natpisima su
transliterisane (`Dpr`, `Hpr`, `Dotv`, `Hotv`), bez promjene pripadajućih polja
ili formula. Podrazumijevani sadržaj specifikacije i crteža nije preveden.

## Obavljena provjera izvora

- Svih osam definicija zadržalo je identitet resursa.
- Svih 223 kontrola zadržalo je tip, ID i stilove; mijenjani su tekstovi i
  samo navedene dimenzije/položaji kontrola.
- U tekstovima dijaloga nema preostale ćirilice osim reference `ВСН`.
- Interni `TVSProperty.h`, `TVSPropertyController.cpp` i `resource.h` su
  bajt-po-bajt identični polaznoj verziji.
- Potvrđeno je da su u `MLeaderSettings.h` i `TVSSpecTable.h` promijenjena
  samo objašnjenja; podrazumijevani šabloni i ostali kod su sačuvani.
- `git diff --check` prolazi.

Ovo su statičke provjere, ne potvrda kompajliranja ili rada dodatka.

## Preostalo prije upotrebe

- [ ] Kompajliranje u Windows-u uz odgovarajući Visual Studio/MFC i ObjectARX SDK.
- [ ] Potvrda da Windows Resource Compiler ispravno obrađuje UTF-8 resurs.
- [ ] Otvaranje sedam aktivnih dijaloga i provjera svih stanja i opcija.
- [ ] Provjera `č ć š ž đ`, preloma redova, dugih natpisa i tabela.
- [ ] Vizuelna provjera na 100%, 125% i 150% skaliranja ekrana.
- [ ] Provjera U redu/Otkaži i čuvanja podešavanja na kopiji DWG-a.
- [ ] Potvrda da postojeći šabloni, atributi i proračuni daju iste rezultate.

Prvi praktični test pripremiti za korisnikovu ciljnu verziju AutoCAD-a
(2017 ili 2022). U ovom Linux okruženju nema Windows/ObjectARX alata ni
AutoCAD-a; ARX/DBX i instalacioni EXE fajlovi nijesu napravljeni niti mijenjani.

## Rječnik tekstova resursa

Ponavljanja istog natpisa spojena su u jedan red. Za naslove, dugmad i
natpise iz glavnog `.rc` resursa korišćeni su sljedeći prevodi:

| Izvorni tekst | Srpski latinicom |
| --- | --- |
| Размер | Dimenzije kanala |
| OK | U redu |
| Cancel | Otkaži |
| Ширина Dпр | Širina Dpr |
| Высота Hпр | Visina Hpr |
| Скорость, м/с | Brzina, m/s |
| Расход, м3/ч | Protok, m³/h |
| Отвод | Koljeno |
| Прямоугольный | Pravougaoni |
| Круглый | Okrugli |
| Переход | Prelaz |
| Тройник | T-komad |
| L=Dпр* | L=Dpr* |
| i,  Па/м | i, Pa/m |
| Отметка | Visinska kota |
| Основные параметры | Osnovni parametri |
| Режим отображения | Način prikaza |
| Маскировка | Maskiranje |
| Грани | Ivice |
| Низ | Dno |
| Верх | Vrh |
| Центр | Centar |
| Вырвнивание | Poravnanje |
| R=Dпр* | R=Dpr* |
| Секционный (ВСН) | Segmentno (ВСН) |
| Тип | Tip |
| Гибкий | Fleksibilni |
| Свойства | Svojstva |
| Dпр2 (Dотв) | Dpr2 (Dotv) |
| Hпр2 (Hотв) | Hpr2 (Hotv) |
| dP,  Па | dP, Pa |
| Тэг 2 | Oznaka 2 |
| Тэг 1 | Oznaka 1 |
| Текстовое поле | Tekstualno polje |
| Направление | Smjer |
| Угол, град | Ugao, ° |
| Вниз | Nadolje |
| Вверх | Nagore |
| Прямо | Pravo |
| Привязка черчения | Referenca crtanja |
| Отступ* | Odmak* |
| * не работает для центра | * ne važi za centar |
| Z | Promjena kote Z |
| C отметки | Sa kote |
| На отметку | Na kotu |
| Угол | Ugao |
| Менеджер блоков TVS | Upravljač blokovima TVS |
| ОК | U redu |
| Отмена | Otkaži |
| Настраиваемый1 | (prazan natpis grid kontrole) |
| Настройка мультивыноски | Podešavanje višestruke oznake |
| Ok | U redu |
| Выноска воздуховода | Oznaka kanala |
| Выноска блока | Oznaka bloka |
| Условные обозначения | Objašnjenje oznaka |
| Настройка спецификации | Podešavanje specifikacije |
| Шаблон спецификации | Šablon specifikacije |
| ???????? ??????? | Upravljač svojstvima TVS |
| ok | U redu |
| cancel | Otkaži |
| ?????????????1 | (prazan natpis grid kontrole) |
