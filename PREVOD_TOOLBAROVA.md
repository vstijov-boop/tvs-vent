# Prevod toolbarova i hintova na srpski latinicom

Grana: `srpski-prevod`. Prevedeni su aktivni paketi `TVS_VENT.cuix` i
`TVS_TOOLS.cuix` iz odgovarajućih foldera u `Installer`.

## Rezultat provjere

| Paket | Prevedena polja | Nepromijenjeni izvršni makroi | Nepromijenjene slike ikona |
| --- | ---: | ---: | ---: |
| TVS_VENT | 119 | 24 | 23 |
| TVS_TOOLS | 75 | 14 | 15 |
| Ukupno | 194 | 38 | 38 |

Oba ZIP paketa prolaze provjeru integriteta. XML sadržaj svih dijelova se
uspješno parsira; struktura, ID-jevi, reference, redosljed elemenata, makroi,
slike i ostali sadržaj su sačuvani. Promijenjeni su samo tekstovi elemenata
`Name`, `NameRef`, `HelpString`, `TooltipTitle` i `CLICommand` (prikazani naziv
komande), te atributi `Text` ribbon dugmadi i panela. Stvarni izvršni tekstovi
u `Command` elementima nijesu mijenjani.

Ova provjera ne zamjenjuje učitavanje i vizuelnu provjeru u AutoCAD-u.

## Praktična provjera u AutoCAD-u

1. Preuzeti dva izmijenjena CUIX fajla sa grane `srpski-prevod` u zaseban
   folder za probu, uz postojeću instalaciju TVS-a.
2. Komandom `CUIUNLOAD` isključiti postojeće djelimične menije `TVS_VENT`
   i `TVS_TOOLS`, ako su učitani. Glavni AutoCAD meni ostaviti učitan.
3. Komandom `CUILOAD` učitati preuzete CUIX fajlove.
4. Otvoriti toolbarove i ribbon; provjeriti naslove, hintove, čitljivost
   slova `č ć š ž đ` i rad dugmadi.
5. Izvršavanje dugmadi i dalje koristi postojeće TVS dodatke i njihove
   putanje podrške; CUIX prevod ne mijenja dodatke niti prevodi njihove poruke.

Za toolbar prevod ne treba kompajliranje. Za raniji prevod dijaloga i
Properties palete i dalje je potrebno napraviti nove ARX/DBX fajlove.
Ako se pri probi zadržao stari prikaz, ponovo pokrenuti AutoCAD i koristiti
probni folder bez starih MNR keš fajlova. Postojeći `.mnr` resursi u repou
nijesu ručno mijenjani. Arhivske verzije i instalacioni EXE fajlovi ostaju
u originalnom obliku.

## Rječnik prevedenih tekstova

Ponavljanja istog prevoda u različitim elementima spojena su u jedan red.
Tehnički nazivi `TVS_*` u ovoj tabeli predstavljaju samo prevedene prikazane
natpise. Izvršni nazivi komandi i makroi i dalje su originalni.

| Izvorni natpis / hint | Srpski latinicom |
| --- | --- |
| TVS_PIPE | Crtanje kanala |
| Воздуховод | Kanal |
| TVS_СUT | Presijecanje kanala |
| Разрезает воздуховод на две части | Presijeca kanal na dva dijela. |
| TVS_CHANGE | Izmjena svojstava |
| Вызывает окно свойств для выбранных объектов TVS_VENT | Otvara dijalog svojstava izabranih TVS_VENT objekata. |
| TVS_SETTINGS | Podešavanje crtanja |
| Настройки черчения воздуховодов | Podešavanje parametara crtanja kanala. |
| TVS_CONNECTT | Spajanje koljenom |
| Соединяет два воздуховода через отвод | Spaja dva kanala koljenom. |
| TVS_CONNECTW | Spajanje T-komadom |
| Соединяет два воздуховода c помощью тройника | Spaja dva kanala pomoću T-komada. |
| TVS_LEAD | Oznaka elementa |
| Создает мультивыноску для объекта TVS_VENT | Kreira višestruku oznaku za TVS_VENT objekat. |
| TVS_ADDATRIB | Dodavanje atributa |
| Добавляет Список Атрибутов к блоку | Dodaje skup atributa bloku. |
| TVS_SPEC | Specifikacija |
| Создает спецификацию выбранных объектов TVS_VENT | Kreira specifikaciju izabranih TVS_VENT objekata. |
| TVS_EXCELSPEC | Izvoz specifikacije u Excel |
| Экспорт спецификации в EXCEL | Izvozi specifikaciju u Excel. |
| TVS_ERASE | Razlaganje TVS objekata |
| Разбивает объекты TVS_VENT из выбора | Razlaže izabrane TVS_VENT objekte na osnovne elemente. |
| TVS_Options | Podešavanja programa |
| Настройка программы | Podešavanje programa |
| TVS_Help | Pomoć |
| Помощь | Pomoć |
| TVS_BLCKMNGR | Upravljač blokovima |
| Открывает менеджер блоков TVS | Otvara upravljač TVS blokovima. |
| TVS_Activation | Aktivacija |
| Активация | Aktivacija |
| TVS_About | O programu |
| О программе | O programu |
| Команда1 | Komanda 1 |
| Команда2 | Komanda 2 |
| TVS_LEAD_RECALCULATE | Ažuriranje oznaka |
| Пересчитывает содержимое выбранных выносок TVS_VENT | Ažurira sadržaj izabranih TVS_VENT oznaka. |
| TVS_ADDPROPERTY | Dodavanje svojstava |
| Добавляет Список свойств к объекту | Dodaje skup TVS svojstava objektu. |
| TVS_SP | Specifikacija |
| TVS_AttributesToProps | Atributi u svojstva |
| Конвертация атрибутов в свойства | Pretvara atribute u TVS svojstva. |
| TVS_EXCELSP | Izvoz specifikacije u Excel |
| TVS_PROPERTYMANAGER | Upravljač svojstvima |
| Открывает менеджер свойств обьектов TVS | Otvara upravljač svojstvima TVS objekata. |
| Выноска | Oznaka elementa |
| Опции | Podešavanja programa |
| Черчение воздуховодов | Crtanje kanala |
| Соеденить отвод | Spajanje koljenom |
| Соеденить тройник | Spajanje T-komadom |
| Разрезать воздуховод | Presijecanje kanala |
| Изменить | Izmjena svojstava |
| Спецификация | Specifikacija |
| Экспорт спецификации в эксель | Izvoz specifikacije u Excel |
| Атрибуты | Atributi |
| Добавить атрибут | Dodavanje atributa |
| Менеджер блоков | Upravljač blokovima |
| Настройки | Podešavanja |
| Настройки черчения | Podešavanje crtanja |
| Разбить обьекты TVS | Razlaganje TVS objekata |
| TVS_ATTRIBUTESTOPROPS | Atributi u svojstva |
| Ducts | Kanali |
| Spec | Specifikacija |
| Leaders | Oznake |
| Attribute | Atributi |
| Settings | Podešavanja |
| Экспорт В Excel | Izvoz u Excel |
| Выноски | Oznake |
| Пересчет выносок | Ažuriranje oznaka |
| Добавить Свойства | Dodavanje svojstava |
| Менеджер свойств | Upravljač svojstvima |
| Атрибуты в свойства | Atributi u svojstva |
| Разбить воздуховоды | Razlaganje kanala |
| AX | Aksonometrija |
| Строит аксонометрическую проекцию | Kreira aksonometrijsku projekciju. |
| BX | Aksonometrijski pogled |
| Строит проекцию вида | Kreira projekciju pogleda. |
| "Объемная" проекция | Prostorna projekcija |
| Окружность с осью | Kružnica sa osom |
| Рисует окружность с осью по точке центра и радиусу | Crta kružnicu sa osom na osnovu centra i poluprečnika. |
| Ось окружности | Ose kružnica |
| Рисует оси окружностей и дуг | Crta ose kružnica i lukova. |
| Прямоугольник(BxH) | Pravougaonik (B × H) |
| Свойства по объекту | Preuzimanje svojstava objekta |
| Установка текущих свойств по объекту | Postavlja tekuća svojstva prema izabranom objektu. |
| Сумма Длин | Zbir dužina |
| Суммирует длины выбраных объектов в метрах (из расчета 1 м=1000 ед чертежа) | Sabira dužine izabranih objekata u metrima (1 m = 1000 jedinica crteža). |
| Сумма Площадей | Zbir površina |
| Суммирует площади выбраных объектов в метрах2 (из расчета 1 м2=1000000 ед2 чертежа) | Sabira površine izabranih objekata u m² (1 m² = 1 000 000 kvadratnih jedinica crteža). |
| Площадь помещения | Površina prostorije |
| Определяет площади замкнутых полилиний и указывает их в центре в метрах2 (из расчета 1 м2=1000000 ед2 чертежа) | Računa površine zatvorenih polilinija i upisuje ih u centru, u m² (1 m² = 1 000 000 kvadratnih jedinica crteža). |
| Cвободный масштаб | Slobodno skaliranje |
| Свободный масштаб по базовой точке, по точке обьекта и по точке с которой нужно совместить выбранную точку | Skalira objekat pomoću bazne tačke, tačke na objektu i ciljne tačke s kojom je treba poklopiti. |
| Текстовая спецификация | Tekstualna specifikacija |
| Создает спецификацию текста на основе введенного ключевого слова | Kreira specifikaciju tekstova na osnovu unijete ključne riječi. |
| Аксонометрия | Aksonometrija |
| Аксонометрический вид | Aksonometrijski pogled |
| Рисование | Crtanje |
| Калькулятор | Kalkulator |
| Аксонометрия и рисование | Aksonometrija i crtanje |
| Свойства по обьекту | Preuzimanje svojstava objekta |
| Aksonometry | Aksonometrija |
| Draw | Crtanje |
| Calc | Kalkulator |
| Вид | Pogled |
