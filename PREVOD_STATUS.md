# Status prevoda TVS interfejsa i specifikacija — srpski latinicom

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

## Dodatno: AutoCAD Properties paleta (Ctrl+1)

Na zahtjev korisnika obuhvat je proširen i na TVS svojstva u Properties paleti.
Prevedeno u izvoru:

- Deset dinamičkih svojstava: Pozicija, Naziv, Tip, Dimenzije, Šifra,
  Proizvođač, Jedinica mjere, Količina, Masa i Napomena; prevedeni su i njihovi opisi.
- Grupa svojstava prikazuje se kao `TVS svojstva`, bez promjene internog
  `CTVSPropertyCategoryName` ili numeričkog identifikatora kategorije.
- MLeader svojstvo: `Format teksta`, sa opisom na srpskom.
- Natpisi svojstava kanala, koljena, prelaza i T-komada u DBX COM omotačima:
  dimenzije, protok, brzina, dužina, ugao, kote, maskiranje, ivice i oznake.
- Zajednički natpisi koje TVS prikazuje za Boju, Sloj, Tip linije, Razmjeru
  tipa linije, Stil štampe, Debljinu linije, Hipervezu i Materijal.
- Opisi svojstava (`helpstring`) u izvornom DBX IDL fajlu. Opisi gornje/donje
  kote usklađeni su s postojećim natpisima `GetDisplayName`; u starom IDL-u
  ti opisi bili su međusobno zamijenjeni. Povezivanje s podacima nije mijenjano.

U `GetDisplayName`, `GetDescription` i `GetCategoryName` korišćeni su posebni
tekstovi za prikaz. Interni ruski ključevi ostaju u `TVSProperty.h` i koriste
se kao i ranije. Ne mijenjaju se standardni nazivi u drugim AutoCAD objektima
niti jezik ostatka AutoCAD interfejsa.

Statička provjera ovog proširenja potvrđuje da je u 16 C++ fajlova promijenjen
samo tekst dodijeljen za prikaz (97 provjerenih poziva `SysAllocString`,
uključujući postojeće komentarisane primjere), uz BOM i završne razmake.
U IDL fajlu promijenjeni su samo `helpstring` tekstovi: GUID-ovi, DISPID-jevi,
imena metoda i tipovi podataka identični su prethodnom commitu.
Interni ključevi, kontroler podataka i funkcionalni šabloni su nepromijenjeni.

Dodatna provjera u Windows-u ostaje obavezna: ponovo izgraditi i ARX i DBX,
uključujući MIDL generisanje biblioteke tipova iz IDL izvora. Generisani
`*_i.h`/`*_i.c` i postojeći binarni fajlovi nijesu ručno mijenjani. U AutoCAD-u
provjeriti prikaz i uređivanje TVS svojstava na kanalu, koljenu, prelazu,
T-komadu, bloku sa TVS svojstvima i MLeader oznaci, te izbor više objekata.

## Dodatno: toolbarovi, hintovi i ribbon

Na zahtjev korisnika prevedeni su aktivni CUIX paketi:

- `Installer/TVS_VENT/TVS_VENT.cuix`: 119 tekstualnih polja.
- `Installer/TVS_TOOLS/TVS_TOOLS.cuix`: 75 tekstualnih polja.

Obuhvaćeni su nazivi dugmadi, tekstovi pomoći pri prelasku mišem, naslovi
hintova, povezani naslovi ribbon panela i komande u menijima. Prevedeni su
prikazani nazivi komandi; stvarni izvršni makroi (`Command`) nijesu mijenjani.

Potvrđeni su ispravnost ZIP paketa i parsiranje XML sadržaja, nepromijenjena
XML struktura, identifikatori, reference i svih 38 izvršnih makroa. Svih 38
ugrađenih slika ikona, kao i ostali netekstualni resursi, identični su prethodnoj
verziji. Mijenjani su samo dozvoljeni tekstovi u `MenuGroup.cui`,
`ToolbarRoot.cui`, `RibbonRoot.cui` i `PopMenuRoot.cui`.

Interni aliasi toolbarova, naziv grupe makroa, nazivi fajlova i ruski nazivi
komandi unutar izvršnih makroa ostaju sačuvani radi kompatibilnosti. Ruski
komentari u XML-u nijesu tekst interfejsa. Arhivske kopije u `Installer/Versions`
i postojeći EXE instalateri nijesu mijenjani.

Za ovaj dio prevoda nije potrebna C++ izgradnja. Potrebno je učitati prevedene
CUIX fajlove u AutoCAD-u i vizuelno provjeriti hintove i ribbon. Prevod dijaloga
i Properties palete i dalje zahtijeva novu ARX/DBX izgradnju. Detalji i rječnik:
`PREVOD_TOOLBAROVA.md`.

## Dodatno: specifikacije u AutoCAD-u i Excelu

Na zahtjev korisnika prevedene su i nove generisane specifikacije:

- Devet zaglavlja podrazumijevanog šablona zajedničkog za AutoCAD i Excel.
- Prepoznata ruska zaglavlja u ranije sačuvanim šablonima, bez promjene
  širina kolona, oznaka svojstava, izraza, razmaka i korisničkih naslova.
- Zaglavlja i zbirni redovi oba starija načina izrade specifikacija (`SPEC.cpp`).
- Automatski nazivi elemenata: Kanal, Fleksibilni kanal, Koljeno, T-komad i
  Prelaz; u staroj specifikaciji i oznake okruglog/pravougaonog oblika.
- Automatske jedinice `m`, `m²` i `kom`. U novom izvozu se prepoznate ruske
  oznake jedinica iz postojećih podataka prikazuju na latinici; izvorni podaci
  nijesu prepisani. Proizvoljne jedinice ostaju sačuvane.
- Poruke o napretku izvoza koje se nalaze u kodu specifikacije.

Promjena `commonDef.h` utiče i na prikaz automatskih naziva i jedinica TVS
objekata u Properties paleti. Nazivi i opisi koje je korisnik upisao u blokove,
šifre, proizvođači i napomene ne prevode se automatski.

Novi pomoćni kod `SpecificationText.h` ima nezavisne C++ testove u
`tests/test_specification_text.cpp`. Testovi su kompajlirani i izvršeni uz
`g++ -std=c++98 -Wall -Wextra -Werror -pedantic`. Prošli su slučajevi fabričkog
šablona, prilagođenih širina i naslova, očuvanja funkcionalnih oznaka,
kombinovanih završetaka redova, praznih ulaza i ponovljenog poziva prevoda.
Dodatno je kompajliran test koji poredi stvarni stari fabrički šablon iz Git-a
s novim šablonom u izvoru. Statički je potvrđeno da proračuni u `SPEC.cpp`,
kontroler XData i nazivi/tagovi atributa nijesu promijenjeni.

Ovo ne potvrđuje cijelu Windows/ObjectARX izgradnju ni Excel COM izvoz.
Preostalo: kompajlirati ARX/DBX i provjeriti oba izlaza na istom skupu
objekata. Postojeće nacrtane tabele i ranije izvezeni Excel fajlovi se ne
mijenjaju; potrebno ih je ponovo generisati. Vidi `PREVOD_SPECIFIKACIJA.md`.

## Kompatibilnost

Interni nazivi `CTVSProperty_*`, XData identifikatori, ID-jevi resursa i
oznake svojstava unutar funkcionalnih šablona nijesu mijenjani. Proračunski kod nije
mijenjan. Nazivi i vrijednosti atributa blokova, slojeva i podaci korisnika
ostaju kakvi su bili. Funkcionalne oznake kao `[Тип]` i `[Размер]` ostaju na
ruskom, a uputstvo objašnjava njihov smisao na srpskom. Njihov prevod bi zahtijevao
poseban rad na kompatibilnosti i nije dio ovog prevoda dijaloga.

U resursima je sačuvana referenca `ВСН`. Oznake dimenzija u natpisima su
transliterisane (`Dpr`, `Hpr`, `Dotv`, `Hotv`), bez promjene pripadajućih polja
ili formula. Zaglavlja i automatski tekstovi novih specifikacija sada su
prevedeni. Postojeće tabele u crtežima nijesu prepravljane.

## Obavljena provjera izvora

- Svih osam definicija zadržalo je identitet resursa.
- Svih 223 kontrola zadržalo je tip, ID i stilove; mijenjani su tekstovi i
  samo navedene dimenzije/položaji kontrola.
- U tekstovima dijaloga nema preostale ćirilice osim reference `ВСН`.
- Interni `TVSProperty.h`, `TVSPropertyController.cpp` i `resource.h` su
  bajt-po-bajt identični polaznoj verziji.
- U `MLeaderSettings.h` promijenjena su samo objašnjenja. U `TVSSpecTable.h`
  prevedena su objašnjenja i zaglavlja šablona, uz iste širine i funkcionalne oznake.
- `git diff --check` prolazi.

Ovo su statičke provjere, ne potvrda kompajliranja ili rada dodatka.

## Preostalo prije upotrebe

- [ ] Kompajliranje u Windows-u uz odgovarajući Visual Studio/MFC i ObjectARX SDK.
- [ ] Potvrda da Windows Resource Compiler ispravno obrađuje UTF-8 resurs.
- [ ] Otvaranje sedam aktivnih dijaloga i provjera svih stanja i opcija.
- [ ] Provjera Properties palete, opisa svojstava i uređivanja vrijednosti.
- [ ] Učitavanje prevedenih CUIX paketa i provjera toolbar/ribbon hintova i menija.
- [ ] MIDL izgradnja i provjera slova u opisima DBX svojstava.
- [ ] Provjera `č ć š ž đ`, preloma redova, dugih natpisa i tabela.
- [ ] Vizuelna provjera na 100%, 125% i 150% skaliranja ekrana.
- [ ] Provjera U redu/Otkaži i čuvanja podešavanja na kopiji DWG-a.
- [ ] Potvrda da postojeći šabloni, atributi i proračuni daju iste rezultate.
- [ ] Poređenje nove i stare specifikacije u AutoCAD-u i Excelu: iste količine, dimenzije i zbirovi.

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
