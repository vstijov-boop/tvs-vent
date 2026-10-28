# Razmjera tabela i masa u Properties paleti

Grana: `unapredjenja-masa`.

## Razmjera pri umetanju

Sve TVS komande koje umeću CAD specifikaciju i KGTEZ traže:

`Razmjera tabele 1:<50> (50 za 1:50, 1 za 1:1):`

Unosi se pozitivan imenilac razmjere. Enter prihvata prethodni unos.
Osnovna visina teksta je 2,5 mm na papiru. Računanje u modelu:

`visina u jedinicama crteža = 2,5 × imenilac razmjere / mm_po_jedinici`

| Razmjera | mm | cm | m |
|---|---:|---:|---:|
| 1:20 | 50 | 5 | 0,05 |
| 1:50 | 125 | 12,5 | 0,125 |
| 1:100 | 250 | 25 | 0,25 |

Model koristi jedinice iz TVS_SETTINGS, kao geometrija kanala. Razmjera i ispravno
podešavanje štampe modela moraju opisivati isti fizički odnos papira i geometrije.
U metričkom Paper Space-u osnovna razmjera je 1:1, tekst 2,5 jedinice papira;
faktor modela cm/m se tamo ne primjenjuje. Rad unutar aktivnog viewporta tretira
se kao Model Space. Papir u inčima nije predmet ovog metričkog obračuna.

Širine kolona, visine redova i zaglavlja, margine i tekst koriste isti faktor.
Sačuvani šabloni kolona ostaju neizmijenjeni: dimenzije su šablonske vrijednosti
sa osnovnom visinom teksta 250 i pri umetanju se skaliraju proporcionalno.
Excel ne traži razmjeru jer nije geometrija u crtežu.

Za zajedničko pamćenje TVS/KGTEZ koriste se Xrecord zapisi u NOD-u DWG-a:
`TVS_TABLE_SCALE_MODEL` i `TVS_TABLE_SCALE_PAPER`, DXF grupa 40.
Početna vrijednost je 50 u modelu i 1 na papiru. Prije upisa razmjere nema
promjene geometrije. Postojeće nacrtane tabele se ne prepravljaju automatski.

U KGTEZ dijalogu visina slova sada je izražena u milimetrima na papiru, sa
početnom vrijednošću 2,5. Razmjera se traži samo ako je izabrana CAD tabela.
KGTEZ zadržava sopstveni raspored kolona i redova, proporcionalan visini teksta.

## Properties

Automation getteri `SheetArea`, `SheetThickness`, `SheetMass` ostaju kompatibilni
sa prethodnim VARIANT potpisima. Njihove implicitne OPM stavke sakrivene su da ne
prave duplikate. Paleta dobija tri zasebna numerička dinamička svojstva tipa
VT_R8, direktno registrovana za kanal, koljeno, prelaz i račvu:

- Površina lima, m²
- Debljina lima, mm
- Masa elementa, kg

Sva tri su izvedena iz geometrije i samo za čitanje. Za fleksibilne/nepodržane
objekte se ne omogućavaju. Za miješani izbor sa različitim vrijednostima ova
pojedinačna polja normalno mogu pokazati različite vrijednosti.

Posebno polje `Masa TVS izbora, kg` prikazuje zbir mase izabranih krutih TVS
elemenata. Pri izboru jednog elementa jednako je njegovoj masi. Pri izboru
različitih tipova i dužina sabira stvarne mase, ne kg/m. Tekst, oznake, drugi
objekti i fleksibilni kanali se ne dodaju; ponovljen ID ne broji se dvaput.

Kada je dostupan, koristi se izbor koji daje Property Inspector kroz
IDynamicPropertyNotify2. Prije povezivanja tog interfejsa čita se postojeći
implied selection, bez mijenjanja izbora ili prikazivanja novog upita.

Stari PropertyAdmin je pravio svojstva pa ih odmah uklanjao. Nova registracija
ostaje aktivna do unload-a, sa pravilnim QueryInterface za IPropertyManager2.
Stara eksperimentalna uređiva dinamička svojstva ostaju neaktivna kao ranije.
Pojedinačna i zbirna polja imaju različite, stabilne GUID-ove. Registracija je
izričita za sva četiri TVS tipa; zajedničko polje vezano je i za AcDbEntity.

Zbir se kešira prema izboru i reviziji geometrije, da paleta ne otvara ponovo
cijeli sistem za svaki element. Otvaranje/zatvaranje za pisanje, poništavanje
izmjene i čitanje geometrije invalidiraju reviziju. TVS_Pipe sada prosljeđuje
svoje subOpen/subClose/subCancel baznoj TVS_Entity klasi umjesto da je preskače.
Ovo je privremeno procesno stanje; DWG format i sačuvana masa se ne mijenjaju.

## Provjera

`tests/test_table_and_selection.cpp`: razmjere 1/20/50/100/200 u mm/cm/m,
konstantne dimenzije na papiru, neispravni ulazi, zbir različitih masa,
ponovljeni ID, preskakanje nepodržanih objekata i poništavanje keša.
`tests/test_table_scale_kgtez.py`: poređenje 15 C++ primjera sa stvarnom LISP
funkcijom za visinu, odvojeni Model/Paper zapisi i prethodno sačuvana razmjera.
Ostali testovi mase i poređenja sa KGTEZ ostaju aktivni.

Za Windows pakete zamijeniti ARX i DBX zajedno, uz zatvoren AutoCAD i backup.
Zamijeniti i KGTEZ.lsp u TVS_TOOLS/Resources. CUIX se ne mijenja.

Provjeriti u AutoCAD-u:

1. U mm/cm/m umetnuti obje TVS vrste CAD tabele i KGTEZ sa 20, 50 i 100.
   U razmjeri 50 izmjeriti visinu teksta 125 / 12,5 / 0,125 i odgovarajuće
   proporcije redova/kolona. Ponoviti u metričkom Layout-u sa 1:1.
2. Ponoviti unos sa Enter; TVS i KGTEZ treba da dijele posljednju razmjeru.
   Provjeriti drugi DWG i povratak u prvi, Escape i ponovno otvaranje DWG-a.
3. Izabrati kanal 400×200 L=3 m: masa 16,956 kg u oba polja mase.
   Dodati kanal L=1 m i koljeno Ri=150 mm, 90°: zbir 25,7153492937 kg.
4. Promijeniti izbor, ukloniti element, promijeniti dužinu/radijus/ugao, Undo/Redo.
   Zbir mora pratiti geometriju. Provjeriti i Properties filter po tipu objekta.
5. Izabrati sistem sa tekstovima i oznakama, zatim samo tekst ili fleksibilni
   kanal. Provjeriti da oni ne dodaju masu i da nema duplih polja.
6. Upravljač svojstava i CAD/Excel specifikacije treba da zadrže isti obračun mase.

Kompilacija i numerički testovi ne zamjenjuju ovu provjeru rada OPM-a u AutoCAD-u.
