# Plan integracije promjene kote — bez implementacije

30.10.2026. Osnova: prihvaćeni izolovani modul iz commita 249e212 i naknadna
dopuna njegovih testova. Ovaj dokument ništa ne povezuje sa aplikacijom.
XRecord, novi dijalog, promjena entiteta i postojećih DWG crteža nisu uvedeni.
Svaki korak ispod predstavlja predlog za zasebno odobrenje.

## 1. Podržana geometrija prve integracije

Samo kruti pravougaoni kanali i dva jednaka glatka kružna koljena: isti A/B,
Ri i ugao, ista vertikalna ravan, horizontalni ulaz i izlaz, bez ravnih nastavaka.
Podržati konstantni Ri i promjenljivi k*B. Oba koljena moraju dobiti iste parametre
nakon provjere; ne preuzimati aktivnu globalnu formu bez eksplicitne postavke.
Predlog: specifični/automatski režim odložiti dok se ne provjeri da daje istu
izvedbu i Ri za oba koljena; prihvaćeni čisti modul već prima razriješeni Ri.

Odložiti okrugla koljena (naročito segmentna/ВСН), fleksibilna, miterska,
asimetrična i koljena sa ravnim nastavcima. D×D u postojećem modulu ne daje
kružnu konturu. Ne prikazivati takav presjek kao podržanu kružnu izvedbu.
Ri=0 ostaje dozvoljen u matematičkom modulu; u komandi predlažemo odbijanje
Ri<=0 dok se ne potvrde proizvodna izvedba i grafika tog graničnog slučaja.

Tačno 90° i ugaoni pojas Unsupported90 odbiti prije kreiranja: vertikalni
međukomad sa nultom XY projekcijom nije podržan. Nema zamjene sa 89,4°.
Ostali uglovi u domenu modula, uključujući 30/45/60°, moraju proći testove adaptera.

## 2. Koordinate i referenca početka

Uzeti A/B i početnu kotu sa stvarnog trenutnog kanala, ne samo iz globalnih
promjenljivih. Globalnu kotu uskladiti tek nakon uspješne transakcije.
Fizičku visinu voditi odvojeno od Z koordinate crtačke ravni.

U = jedinični XY pravac trenutnog kanala; T = (-Uy,Ux). F = mm po model-jedinici.
Neka Q bude postojeći LastPoint na kome korisnik zadaje opciju Z. Da zadržimo
sadašnje značenje te tačke kao MiddlePoint prvog koljena, odrediti:

```text
a = Result.axis[1].s = R sin(theta) [mm]
Oxy = Qxy - U*a/F
Pplan(s,t) = Oxy + (U*s + T*t)/F
Pplan.z = Z crtačke ravni, ne fizička kota ose
Zphysical = početnaKotaMm + Result.point.z
```

Postojeći place() vraća fizičko prostorno pozicioniranje; ne prosljeđivati njegovu
fizičku Z direktno ravanskim DBX entitetima. Zaseban tanak adapter za plan mora
čuvati Z ravni i koristiti s,t iz istog rezultata. UCS služi unosu, računanje i
upis geometrije ostaju u WCS kao u postojećoj ispravci UCS-a.

Ako a/F prelazi raspoloživu dužinu dolaznog kanala minus njegov dozvoljeni minimum,
odbiti potez bez dodavanja ili skraćivanja entiteta. Provjeriti nenulti pravac i
jedinstvenost aktivnog kraja prije svih promjena.

## 3. Mapiranje Result-a na MA/MC/MiddlePoint

Mapiranje važi za pravougaonu granu Up/Down, gdje izvor računa
MA = MiddlePoint minus usmjereni zahvat, MC = MiddlePoint.
MiddlePoint ovdje NIJE fizički centar kružnog luka ni presjek tangenti u profilu.
Ne upisivati Result.centers u njega. MA/MC su izvedeni podaci crtanja, ne novi
nezavisni autoritativni ulazi.

| Dio | Željena tačka u osnovi | Postojeće polje/uloga |
|---|---|---|
| Prvi horizontalni spoj | Pplan(axis[0]) | MA prvog koljena, novi LastPoint ulaznog kanala |
| Prvi kosi spoj | Pplan(axis[1]) | MC prvog koljena; MiddlePoint prvog koljena |
| Drugi kosi spoj | Pplan(axis[2]) | MC drugog koljena; MiddlePoint drugog koljena |
| Završni horizontalni spoj | Pplan(axis[3]) | MA drugog koljena, FirstPoint izlaznog kanala |
| Nagibni međukomad | axis[1] -> axis[2] projektovani u plan | FirstPoint -> LastPoint nagibnog TVS_Pipe |

Prvo Startvector=(Uy,-Ux,0), drugo Startvector=(-Uy,Ux,0), Normvector=(0,0,1).
Podizanje: prvo Form_Up, drugo Form_Down. Spuštanje: prvo Form_Down, drugo Form_Up.
A/B se ne zamjenjuju u sačuvanim poljima. Swectangle oba koljena je isti stvarni
ugao theta; Ri se razrješava iz B i odabranog režima.

Referentna Elev prvog koljena je početna horizontalna kota, a drugog završna
horizontalna kota, u oba smjera. To nije potpuna visinska ovojnica koljena.
Izlazni horizontalni kanal dobija završnu kotu.

Nakon kreiranja probni adapter/test mora izvesti MA/MC iz iste stvarne matematičke
grane subWorldDraw i dokazati gornje jednakosti. Ne oslanjati se na keš MA/MC iz
prethodnog redraw-a. Geometriju isplanirati prije pisanja, ne popravljati crtež
ručnim pomjeranjem nakon crtanja. Postojeće pune/isprekidane linije mogu ostati
simboličke, ali fizički spojevi i kotirane karakteristične tačke moraju odgovarati
zajedničkom Result-u. Potpun prikaz konture kosog spoja provjeriti posebno: ne
obećavati da današnji ravanski Pipe već crta svaku projekciju kosog završnog presjeka.

## 4. Tok komande, NoChange i otkazivanje

1. Pročitati kanal i snimiti stanje globalnih promjenljivih/lokalne trase.
2. Prikazati postojeći dijalog; rezultat različit od IDOK znači Cancel.
3. Validirati jedinice, presjek, režim, pravac, kote i eksplicitnu politiku.
4. Pozvati izolovani solve() bez otvaranja transakcije pisanja.
5. NoChange: poruka da kote ne zahtijevaju promjenu; bez entiteta, pomjeranja
   A1/A2/pt1 i izmjene globalnih vrijednosti. Pozivalac mora razlikovati taj status.
6. Ostale greške: jasna poruka sa razlogom; bez promjene crteža/globalnog stanja.
7. Ok: provjeriti da je pročitani kanal i dalje isti, pa jednom transakcijom
   skratiti dolazni kanal i dodati dva koljena, međukomad i izlazni kanal.
8. Tek poslije commit-a promijeniti aktivni pPipe, A1/A2/pt1 i globalnu kotu/ugao.

Ne koristiti bool za sve ishode bez jasnog razlikovanja Applied/NoChange/Cancelled/
Failed. Ne miješati AcDb objekatski Undo sa globalnim C++ promjenljivim: poslije
Undo/Redo naredno crtanje mora ponovo pročitati kotu sa stvarnog aktivnog objekta,
a ne vjerovati zastarjeloj globalnoj vrijednosti.

## 5. Politika Lmin i projekcije u komandi — predlog, nije odobrena

Čisti modul ostaje nepromijenjen. Lmin=0 nastavlja da bude podržan u njemu.
Za prvu komandu predlog je imenovani IntegrationPolicy, sa eksplicitnim Lmin
100 mm i minimumProjection 0.1 mm. Te vrijednosti traže odobrenje; nisu EN/DIN
pravilo, nisu sada uvedene i neće biti sakrivene u geometrijskoj funkciji.
Postojeći dijalog se ne proširuje u prvom koraku; usvojena politika mora biti
vidljiva u dokumentaciji i poruci odbijanja, uključujući potreban minimum visine.

Strict poređenje ostaje: nema dodavanja epsilon-a fizičkoj dužini da bi se
kratak komad prihvatio. Na analitičkoj granici 45°/100 mm zabilježeno je odbijanje
zbog rezultata 99.999999999999986 mm. Predlog za sada: prijaviti da je slučaj na
numeričkoj granici i tražiti neznatno veću kotnu razliku; ne prilagođavati kotu
ili grafiku tiho. Ako se kasnije želi tolerancijski pojas, prvo odobriti odvojenu
politiku sa prikazom reziduala i testovima, ne promijeniti solve usput.

## 6. Fizička dužina i njeni potrošači

Nagibni komad mora čuvati vlastitu visinsku razliku između njegovih spojeva:

```text
riseStraightMm = Result.axis[2].z - Result.axis[1].z
Pmm = XY dužina komada * faktor jedinica njegove baze
Lmm = hypot(Pmm, riseStraightMm)
```

Ne čuvati ukupni deltaZ etažiranja kao rise međukomada: dio visinske razlike
već pripada koljenima. Za 400x200, Ri=100, deltaZ=400, 45°:
riseStraight=282.842712474619 mm, P=282.842712474619 mm, L=400 mm.
Ovo mora biti eksplicitni integracioni test.

Predlog API-ja: poseban getProjectedLength za grafiku i getPhysicalLength za
količine. Sve stare upotrebe get_Length auditovati; ne mijenjati semantiku slijepo.

| Potrošač | Potrebno ponašanje |
|---|---|
| TVS_Pipe::getCount | fizička dužina u metrima |
| TVS_Entity::getSheetMetal | površina/massa iz fizičke dužine; kg/m po fizičkom metru |
| SPEC.cpp, AutoCAD/Excel specifikacije, TVSPropertySpec i TVSSpecTable | količine i ukupne mase iz fizičke dužine, grupisanje provjeriti |
| ComPipe::get_Length | isti COM potpis, fizička dužina u mm za označene nove nagibne komade |
| KGTEZ.lsp | kroz COM Length dobija fizičku dužinu; potvrditi površinu i rezervu odvojeno |
| Properties i menadžer svojstava | jasno razlikovati fizičku i projektovanu dužinu; ukupna masa izbora fizička |
| MLeader tekst i preračun oznaka | audit getStringMessage/TVSPropertyController: količinski tokeni fizički; položaj strelice ostaje geometrijski |
| TVS_Pipe::subWorldDraw, gripovi, wipeout, hit-test, MOVE/STRETCH | XY dužina ostaje projektovana; ne koristiti fizičku za podjelu vektora ili grafičke odsječke |
| Keširani Length i COM/svojstva | ne koristiti keš iz posljednjeg redraw-a kao fizičku dužinu |

Čitanje količina ne smije mijenjati DWG. Statične tabele ponovo generisati nakon
izmjene; fizička dužina se izvodi, ne duplira kao ručno promjenjivi autoritativni broj.

## 7. Potrebne izmjene podataka — tek nakon zasebnog odobrenja

Predloženi najmanji podatak za fizičku dužinu je verzionisani TVS_SLOPE XRecord u
extension dictionary novog nagibnog TVS_Pipe: verzija i potpisani riseStraightMm.
Nije dodat u ovom zadatku. Ne mijenja redoslijed postojećih dwgIn/dwgOut polja,
ali mijenja semantiku DWG sadržaja. Stariji TVS dodatak ga ne tumači i može prikazati
pogrešnu XY količinu. Nepoznat/neispravan zapis mora biti prijavljena greška obračuna,
ne tihi povratak na horizontalnu dužinu. Stari objekti bez zapisa ostaju postojeći
režim i ne migriraju se automatski.

Samo rise nije dovoljan da se trajno prepozna cio sklop i zabrani parcijalna izmjena
njegovih koljena. Prije integracije izabrati i odobriti jedno od sljedećeg:

- A: samo podatak rise + zabrana editovanja označenog međukomada, uz jasno ograničenje
  da nezavisno editovanje koljena može razdvojiti sklop; nije potpuno rješenje za
  obećanu zaštitu cijelog etažiranja.
- B (preporuka ako se zahtijeva zaštita cjeline): dodatna verzionisana evidencija
  pripadnosti dva koljena i međukomada sa ulogama i referencama objekata. Potrebno je
  definisati deep-clone mapiranje, djelimični COPY i nedostajuće/obrisane članove.
  To je mali sklop etažiranja, ali jeste dodatni obim podataka, ne postojeća funkcija.

Ne uvoditi to prećutno kao dio matematičke ispravke. Odluka A/B je otvorena i
mora prethoditi obećanju pune podrške gripovima/MOVE/STRETCH. Za početnu zaštićenu
varijantu potreban je zajednički adapter koji ažurira cijeli sklop ili odbija
parcijalnu promjenu; obična kopija rise ne održava tangentnost koljena.
Uniformni SCALE mora skalirati i rise; neuniformni/3D transform odbiti.
COPY i Undo/Redo moraju obuhvatiti geometriju i podatke u istoj transakciji.
Kopiranje između baza različitih jedinica zahtijeva posebnu konverziju i testove.

## 8. Konkretni AutoCAD integracioni testovi — još nisu izvršeni

Matrica za AutoCAD 2020 i 2022, sa DWG kopijama:

1. Referentni 400x200, Ri=100, ±400, 45°: izmjeriti sva četiri spoja, P=282.842712,
   L=400, ukupni zahvat=565.685425. Uporediti sa nezavisnim profilom istog presjeka.
2. 30/45/60°, spuštanje i podizanje sa različitih apsolutnih kota, A!=B i k*B /
   konstantni radijus; nakon REGEN MA/MC i granice konture ostaju na istim tačkama.
3. mm/cm/m, trase 0/37/90/180/270°, pomjeren i rotiran UCS: isti fizički rezultat.
4. Premala visina, prekratak dolazni kanal, nevažeći ugao, 90°, Ri=0 prema usvojenoj
   politici, nepodržan tip: broj objekata i prethodna geometrija nepromijenjeni.
5. Jednak unos, Cancel, Esc i zatvaranje dijaloga: bez promjene objekata, kote,
   reference aktivnog kraja i sljedećeg običnog segmenta.
6. Granice Lmin i minimumProjection: ispod/na/iznad; posebno dokumentovana stroga
   granica 45°/100 mm, bez skrivenog pomjeranja rezultata.
7. Svi potrošači iz tabele: fizička dužina 400 mm / količina 0.4 m; površina 0.48 m²
   i masa 2.2608 kg za 400x200 i debljinu 0.6 mm, bez rezerve. Provjeriti KGTEZ
   rezervu zasebno. Svaka oznaka mora prikazati usvojenu semantiku dužine.
8. COPY kompletnog sklopa i samog međukomada: izolovani podaci, bez veze ka originalu;
   ponašanje djelimičnog sklopa mora biti unaprijed definisano prema A/B odluci.
9. MOVE, ROTATE, MIRROR, SCALE, grip i TVS_MOVE/STRETCH: uspješno ažuriranje svih
   potrebnih podataka ili potpuno odbijanje; nema geometrijski ispravne slike uz
   zastarjelu fizičku dužinu/ugao.
10. Jedan Undo vraća cijelu operaciju; Redo vraća geometriju i podatke. Provjeriti
    globalnu kotu pri nastavku crtanja nakon Undo/Redo.
11. Snimiti, zatvoriti, otvoriti; provjeriti iste koordinate, dužine, mase i oznake.
    WBLOCK/INSERT i drugi DWG istih/različitih jedinica posebno testirati.
12. Zaključan sloj, nedostupan objekat ili greška upisa podatka: bez djelimičnog sklopa.
13. Stari DWG bez novih zapisa: identičan raniji prikaz/količine; ništa se ne konvertuje
    samo otvaranjem, REGEN-om ili generisanjem tabele.

Ovi testovi predstavljaju plan, ne izvještaj izvršavanja. Prihvatanje dopunjenih
izolovanih testova nije automatsko odobrenje integracije ili izmjena podataka.
