# Plan prevoda dijaloga na srpski latinicom

Datum: 26.10.2026.
Repozitorijum: https://github.com/vstijov-boop/tvs-vent
Radna grana: `srpski-prevod`
Polazna grana: `master`; polazni commit: `5aa1ac5` (`Update Radme`).

## Cilj i granice

Prevesti korisnički tekst dijaloga na srpski jezik, latinicom i ijekavicom,
uz stručnu terminologiju za ventilaciju. Ova faza ne mijenja proračune,
geometriju, komande, format DWG podataka ili ponašanje programa.
Plan je izrađen; prvi prevod izvornog koda je unesen. Detaljan status i
preostale provjere nalaze se u `PREVOD_STATUS.md`.

Obuhvat: naslovi prozora, dugmad, natpisi polja, grupe, opcije, zaglavlja
tabela, objašnjenja i poruke koje pripadaju dijalozima. Prevesti i postojeća
engleska dugmad radi ujednačenog interfejsa.

Obuhvat je naknadno proširen na TVS svojstva u AutoCAD Properties paleti
(`Ctrl+1`), na izričit zahtjev korisnika. Prevedeni su nazivi, kategorije
i opisi za prikaz, uz očuvanje internih ključeva. Vidi `PREVOD_STATUS.md`.

Obuhvat je potom proširen i na nazive i hintove toolbarova, ribbon panela
i povezane menije u dva aktivna CUIX paketa. Izvršni makroi ostaju isti;
vidi `PREVOD_TOOLBAROVA.md`.

Izvan obuhvata: komandna linija AutoCAD-a, instalater, izvršni nazivi
komandi, prevod crteža,
korisničkih vrijednosti i sadržaja generisanih specifikacija. Njih treba
evidentirati ako se pojave, ali ne mijenjati u ovoj fazi.

## Nalazi pregleda

Ovo je C++/MFC/ObjectARX projekat, a ne AutoLISP. Glavni resursi dijaloga su
u `TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc`.

Pronađeno je osam definicija dijaloga. Postojanje definicije ne znači da je
svaka dostupna iz aktivne komande; vezu s pozivima treba provjeriti prije prevoda.

| ID resursa | Postojeći naslov | Predloženi naslov | Napomena |
| --- | --- | --- | --- |
| `IDD_PIPESIZEDIALLOG` | Размер | Dimenzije kanala | Dimenzije, protok, brzina i prikaz elemenata |
| `IDD_DGALLEDIT` | Свойства | Svojstva | Aktivna veza u `dgAllEdit.h` |
| `IDD_DGALLEDIT1` | Свойства | Svojstva | Provjeriti upotrebu; zaseban stariji obrazac |
| `IDD_HANGEZDG` | Z | Promjena visinske kote | Početna i nova kota, ugao |
| `IDD_BLCKMNGRDLG` | Менеджер блоков TVS | Upravljač blokovima TVS | Zaglavlja tabele nastaju i u C++ kodu |
| `IDD_MLEADERPROP` | Настройка мультивыноски | Podešavanje višestruke oznake | Termin uskladiti s funkcijom MLeader |
| `IDD_SPECPROPS` | Настройка спецификации | Podešavanje specifikacije | Objašnjenja sadrže funkcionalne oznake šablona |
| `IDD_TVSPROPERTYMANAGER` | `???????? ???????` | Upravljač svojstvima TVS | Naslov je već oštećen u izvoru; provjeriti funkciju prozora |

Dodatni izvori teksta:

- `dgBlockManager.cpp`: ruska zaglavlja kolona.
- `dgTVSPropertyManager.cpp`: zaglavlja preko `CTVSProperty_*` i tekst „Выбрано“.
- `MLeaderSettings.h`: objašnjenja oznaka prikazana u dijalogu MLeader.
- `TVSSpecTable.h` i `TVSSpecTable.cpp`: objašnjenja i popunjavanje dijaloga specifikacije.
- Ostali `dg*.cpp` i `dg*.h`: provjeriti dinamičke natpise i poruke.

Nisu pronađene definicije dijaloga u `TVS_Ventilation_DBX.rc`.

## Redosljed rada

### 1. Napraviti konačan popis tekstova

Za svaku kontrolu zabilježiti resurs/ID, ruski tekst, srpski prevod, mjesto
u kodu i status provjere. Obuhvatiti i skrivene kontrole da se ruski tekst
ne pojavi pri promjeni stanja dijaloga. Utvrditi koji se dijalozi zaista otvaraju.

### 2. Ujednačiti terminologiju

| Ruski tekst | Predloženi prevod |
| --- | --- |
| Размер | Dimenzije |
| Свойства | Svojstva |
| Основные параметры | Osnovni parametri |
| Ширина / Высота | Širina / Visina |
| Расход | Protok |
| Скорость | Brzina |
| Отвод | Koljeno |
| Переход | Prelaz |
| Тройник | T-komad |
| Прямоугольный / Круглый | Pravougaoni / Okrugli |
| Отметка | Visinska kota |
| Режим отображения | Način prikaza |
| Производитель | Proizvođač |
| Количество / Масса / Примечание | Količina / Masa / Napomena |
| Выбрано | Izabrano |
| OK / ОК / Ok | U redu |
| Cancel / Отмена | Otkaži |

Jedinice prikazivati kao `m³/h`, `m/s`, `Pa`, `Pa/m` i `mm`, bez promjene
jedinica proračuna. Značenje oznaka `Dпр`, `Dотв`, `Hпр` i `Hотв` provjeriti
prema kodu prije izbora njihovih latiničnih oznaka. Referencu `ВСН` sačuvati
kao tehničku referencu; ne zamjenjivati je drugim standardom.

### 3. Provjeriti kodiranje

Glavni `.rc` fajl koristi Windows-1251 u ruskom dijelu i Windows-1252 u
engleskom dijelu. Pregledani C++ fajlovi koriste UTF-8, dok projekti za
AutoCAD 2017 i 2022 imaju podešen Unicode. Postoje i različite postavke fontova.

Prije masovnog prevoda odabrati kodiranje resursa koje podržava `č ć š ž đ`
i koje prihvata odgovarajući Windows Resource Compiler; kandidat je UTF-16LE
sa BOM-om uz usklađene direktive. Napraviti probno kompajliranje i provjeriti
učitavanje resursa. Ne ostaviti srpske znakove pod neodgovarajućom kodnom stranom.
Provjeriti jezičke oznake resursa i izbor resursa u radu programa.

### 4. Prevesti statičke dijaloge

Prvo prevesti Dimenzije i Svojstva, zatim promjenu kote, upravljače blokovima
i svojstvima, pa podešavanja oznaka i specifikacije. Prevesti sve pripadajuće
natpise i dugmad. Sačuvati ID-jeve kontrola, stilove koji određuju ponašanje
i veze s obradom događaja.

### 5. Prevesti tekst koji se formira u kodu

Prevesti zaglavlja i objašnjenja koja dijalozi dobijaju tokom rada.
Posebno razdvojiti tekst za prikaz od tekstova koje program tumači kao ključeve.

`TVSProperty.h` definiše nazive `CTVSProperty_*` koji se koriste i izvan
dijaloga, uključujući prepoznavanje svojstava u `TVSPropertyController.cpp`.
Zato za zaglavlja dijaloga koristiti zasebne prevedene natpise, bez globalnog
preimenovanja tih konstanti.

Očuvati postojeće funkcionalne oznake šablona, npr. `[L]`, `[V]`, `[D]`,
`[Тип]` i `[Размер]`. Prevesti njihova objašnjenja, ali ne i stvarne ključeve,
podrazumijevane funkcionalne šablone, nazive atributa ili korisničke podatke.

### 6. Prilagoditi prostor prevedenom tekstu

Po potrebi proširiti natpise, dugmad i grupe ili pomjeriti susjedna polja.
Sačuvati logičan raspored i tab-redosljed. Provjeriti tabele i ponašanje pri
promjeni veličine prozora. Ne raditi redizajn niti dodavati nove funkcije.

### 7. Provjeriti rezultat u Windows-u i AutoCAD-u

- Kompajlirati odgovarajuću konfiguraciju uz Visual Studio/MFC i ObjectARX SDK.
- Dogovoriti ciljnu verziju AutoCAD-a prije prve izgradnje; repo ima posebna
  rješenja za 2017 i 2022, koja su relevantna za korisnikovo okruženje.
- Otvoriti svaki dostupan dijalog; za neaktivne resurse zabilježiti status.
- Provjeriti slova `č ć š ž đ`, naslove, sve opcije i dinamička zaglavlja.
- Provjeriti čitljivost na skaliranju ekrana 100%, 125% i 150%, gdje je dostupno.
- Provjeriti dugmad U redu/Otkaži, unos vrijednosti i čuvanje podešavanja.
- Na kopiji postojećeg DWG-a potvrditi da dimenzije, protok i proračuni daju
  iste rezultate i da oznake/šabloni i postojeći atributi i dalje rade.
- Potražiti preostalu ćirilicu u tekstovima dijaloga; dokumentovati opravdane
  izuzetke kao što su tehničke reference i oznake šablona.

Izmjenu izvora treba ponovo kompajlirati da bi se dobio prevedeni dodatak.
Postojeći instalacioni EXE fajlovi neće se sami promijeniti.
U ovoj sesiji nije obavljeno Windows/ObjectARX kompajliranje niti test u AutoCAD-u.

## Evidencija i završetak

Predloženi zasebni commit-i: popis i rječnik; statički dijalozi i kodiranje;
dinamički tekstovi; raspored i rezultati provjere. Svaki commit slati na
`srpski-prevod`, bez spajanja u `master`.

Prevod je završen kada su svi dostupni dijalozi pregledani, sav obuhvaćeni
tekst preveden, znakovi i raspored ispravni i osnovne funkcije provjerene u
ciljnoj verziji AutoCAD-a. Neprovjereni slučajevi moraju biti jasno evidentirani.

Trenutni status: grana `srpski-prevod` objavljena je na GitHub-u, zajedno s
planom. Unesen je prvi prevod osam definicija dijaloga, zaglavlja tabela i
objašnjenja šablona. Resursi su prebačeni na UTF-8 sa BOM-om i direktivama
`#pragma code_page(65001)`; potvrda Windows Resource Compiler-om je preostala.
Obavljene su statičke provjere; Windows/ObjectARX izgradnja i test u AutoCAD-u
još nisu obavljeni. Glavna grana `master` ostaje nepromijenjena.
