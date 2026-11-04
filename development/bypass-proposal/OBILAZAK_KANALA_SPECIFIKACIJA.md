# Obilazak kanala — prijedlog MVP-a i geometrijska specifikacija

**31.10.2026. — SAMO PLAN I MOCKUP.** Osnova pregleda: razvojni izvor `da3e2c2`,
dokumentacija `e8586c1`, grana `razvoj-offset-arx`. Ovim dokumentom nisu odobreni
novi solver, komanda, DWG zapisi ili produkciono izdanje.

Mockup: `obilazak-kanala-mockup.html`. Samostalan, bez mreže i CAD veze. Prikazuje
četiri unaprijed zadata primjera: ispod, iznad, neizvodljivo i sa izolacijom.
Polja su namjerno samo za čitanje; izbor primjera mijenja prikaz, ne rješava nove ulaze.

## 1. Šta već postoji i šta još nije potvrđeno

| Dio | Konkretan postojeći kod | Ponovna upotreba / granica dokaza |
|---|---|---|
| Lokalno etažiranje | `geometry/OffsetGeometry.h:37`, `tvsOffset::solve`, Input/Result | Dva poziva u memoriji za suprotne deltaZ; ose, centri, spojevi, L/P. Nema provjere prepreke, izolacije ni minimuma cijelog obilaska. |
| Pozicioniranje i TVS parametri | `geometry/OffsetAdapter.h:36`, `tvsOffsetAdapter::adapt` | Dva DTO-a iz ISTIH Result-a, WCS pravac, mm/model faktor; fizička kota odvojena od crtačkog Z. Nije nova komanda changeZ. |
| Fizička dužina | `geometry/OffsetRecordRules.h:12`; `TVS_Pipe.cpp:1273`; `ComPipe.cpp:429` | hypot(P,rise), getCount i COM. Za oba nova nagibna dijela rise se izvodi iz njihovih axis[2].z-axis[1].z, a ne ukupne visine etažiranja. |
| Projekcija spojnih presjeka | Result.corners; `geometry/SlopeProjection.h:7`; `TVS_Pipe.cpp:298` | Uglovi spojnih ravni za osnovu. To NISU kompletne krive ovojnice koljena; za sudar trebaju i kružni lukovi između spojeva. |
| Članstvo i validacija | `OffsetAssemblyData.h`, `.cpp:24/93/142/149` | V1 je TROJKA. Jedan TVS_OFFSET_MEMBER po članu sadrži i rise; nema posebnog TVS_SLOPE zapisa u stvarnoj integraciji. Validacija, kopiranje i zaštite su implementirani u DEV-u. |
| Preview i transakcija | `OffsetIntegrationCommand.h:26`, `TVSTestOffset.h:31`, TVS_MOVE/STRETCH pregled | Iskustvo i pomoćni obrasci; za obilazak treba pregled cijelog rezultata i jedna transakcija, ne dva upisa postojećeg Z. |
| Testovi | `tests/test_offset_geometry*`, `test_offset_adapter*`, `test_offset_integration*`, `test_offset_membership_guard.py`, `test_offset_transform_guard.py` | Numerički oracles i zamjenski SDK modeli mogu se koristiti kao donja regresiona osnova. Nisu dokaz nove funkcije obilaska. |

Postojeći solver: fizički mm/radijani; podrazumijevani minimum ugla 0,1°, bez 90°;
Ri=0 matematički dopušten, ali nije podržan ovim MVP-om. Lmin=0 jeste podržan u
čistom etažiranju. Strogo poređenje Lmin i numerička granica ostaju nepromijenjeni.

**Dokazi:** prethodni izvještaj bilježi 16 automatizovanih testova i uspješne DEV
buildove za 2020/2022. U ovom koraku nisu ponavljani bez razloga. Korisnički proof
12 slučajeva / 24 TAP / 216 objekata potvrđen je samo za raniji geometrijski alat,
AutoCAD 2020 / mm / Kontura. **Nova interaktivna Z integracija, novi zapisi, COPY,
Undo/Redo i obračun još nisu praktično potvrđeni. Obilazak nije implementiran.**
Stari `OFFSET_ASSEMBLY_B_PROPOSAL.md` je istorijski predlog, ne opis sadašnjih zapisa.

## 2. Precizan obim MVP-a

- Novi kanal: kruti pravougaoni A×B, stalni presjek; ravan savijanja s–z.
- Jedna prepreka: ravni horizontalni pravougaoni kanal, upravan u osnovi.
- Horizontalni ulaz i izlaz na istoj fizičkoj koti ose z0; prolaz ispod ili iznad.
- Četiri jednaka glatka kružna koljena, isti Ri i ugao theta, bez ravnih nastavaka.
- Sedam članova: E1 / S1 / E2 / H / E3 / S2 / E4. Ulazni i izlazni nastavak su susjedi.
- Eksplicitni ulazi: Ri, obe izolacije, čisti razmak c, LminSlope, LminBridge,
  minimumProjection i rezervisane minimalne ulazne/izlazne dužine.
- Ugao bira korisnik; predlozi 30/45/60°, validacija prema podržanom domenu bez 90°.
- Prvi model izolacije: konstantna debljina; kod novog kanala ne zatvara unutrašnju
  krivinu: Ri > iNew. Ako Ri <= iNew, MVP odbija slučaj jasnom porukom.
- Oba izvora moraju biti horizontalna u WCS-u. Rotacija/pomjeranje UCS-a u horizontalnoj
  ravni mijenja unos i pozicioniranje, ne fizički model. Nagnute trase nisu MVP.

Prepreka mora pokrivati punu poprečnu širinu novog izolovanog kanala, uključujući
potrebnu rezervu do kraja prepreke: novi poprečni interval u t proširen za c mora biti
unutar pravog dijela prepreke. U tom opsegu svaka kritična tačka ima odgovarajuću
prepreku na istom t i s–z provjera daje stvarno minimalno rastojanje u 3D.
Ako je kraj prepreke blizu, ukrštanje nepotpuno ili uslov nije poznat, status je
UnsupportedGeometry/MissingInput. Ne pretvarati prolaz oko kraja prepreke u ovaj MVP.

Nema drugih prepreka u certifikatu: poruka mora reći „provjerena izabrana prepreka“.
Sudari sa ostalom mrežom, nosači, prirubnice i montažni pristup nisu ovdje implicitno
obuhvaćeni; njihove rezerve korisnik mora zadati u eksplicitnim razmacima.

## 3. Koordinate, dimenzije i granice

Sve računanje u fizičkim mm. Lokalni sistem je ortonormalan:

- s: smjer novog kanala u osnovi; pozitivan u smjeru crtanja;
- z: fizička vertikala, pozitivno nagore;
- t: poprečno na novi kanal u horizontalnoj ravni;
- s=0: projekcija presjeka osa novog kanala i prepreke; t=0: osa novog kanala;
- z u formulama može biti relativan prema z0; apsolutni datum čuva se odvojeno.

**A** novog kanala je širina duž t; **B** je visina u ravni savijanja. Ne zamjenjivati
A i B. Prepreka se pruža duž t: njena širina poprečno njenoj osi, Ao, jeste dimenzija
**duž nove trase s**; Bo je njena visina. Dužina prepreke nije Ao.

| Veličina | Definicija |
|---|---|
| Goli novi kanal, horizontalno | vrh zAxis+B/2; dno zAxis-B/2 |
| Izolovan novi kanal, horizontalno | vrh zAxis+B/2+iNew; dno zAxis-B/2-iNew; širina A+2iNew |
| Prepreka, goli lim | s u [-Ao/2,Ao/2], z u [zo-Bo/2,zo+Bo/2] |
| Spoljna prepreka O | s u [-Ao/2-iObs,Ao/2+iObs], z u [zo-Bo/2-iObs,zo+Bo/2+iObs] |
| Spoljni gabarit novog kanala | stvarni sektori/pravougaonici lima, prošireni normalno za iNew; kontinuirana izolacija preko spojeva |
| Početak s0 / kraj s7 | osa horizontalnog ulaznog presjeka E1 / osa horizontalnog izlaznog presjeka E4 |
| Ukupni zahvat W | s7-s0, između ta dva presjeka; nije zbir fizičkih dužina |

Izolacija prepreke modeluje se kao njena spoljna pravougaona kutija. Izolacija novog
kontinuiranog tijela u presjeku je paralelni spoljni offset (Minkowski disk iNew):
poluprečnici konturnih lukova su Ri-iNew i Ri+B+iNew. Ovaj geometrijski model nije
potvrda svakog fabričkog detalja izolovanja. Rastojanje se ne mjeri između osa.
Ravni nastavci fitinga, ako se naknadno traže, mijenjaju model i testove.

Dostupnost nije automatski otkrivena iz cijelog crteža. Eksplicitni efektivni prozor
[sLeft,sRight] ograničava s0/s7 NAKON rezervacije LminIn/LminOut za susjedne prave
kanale. Ulazna dužina se provjerava prema stvarnom početku aktivnog kanala; nakon
preview-a opet se provjerava snapshot. Izlazni prozor bez podataka je „nije provjeren“,
a ne proizvoljna potvrda prostora. Cilj W ne uključuje rezervisane susjedne nastavke.

## 4. Dva različita značenja zazora — prijedlog B

**A — ortogonalni zazori:** zabrana ulaska spoljnog tijela u pravougaonik O proširen
za cS duž s i cZ duž z. To je jasno definisana konzervativna kutija, odnosno
kombinacija odvojenih projekcionih uslova. „Bočno“ ovdje znači od ruba prepreke
duž s u uzdužnom presjeku; bočni zazor duž t je drugi uslov i nije isto polje.
Nije euklidska izodistanca.

**B — najkraće geometrijsko rastojanje:** min dist(Soutside,O) >= c, za sva tijela
novog obilaska. U presjeku je izodistanca oko O pravougaonik sa zaobljenim uglovima,
radijusa c. U MVP punom poprečnom preklopu ova provjera važi i za 3D površine.

Primjer: tačka 40 mm bočno i 40 mm ispod ugla ima rastojanje 56,569 mm. B/c=50
je prihvata; A/cS=cZ=50 je odbija. Obrnuto, samo mjerenje vertikalnog zazora srednjeg
segmenta ne provjerava nijedan kriterijum za kosi dio i koljena.

**Preporuka za MVP: B.** Polje se zove „Čisti razmak — najkraće rastojanje [mm]“.
Dijalog posebno prikazuje ostvareni minimum, par kritičnih tačaka i vertikalni zazor
iznad/ispod srednjeg dijela. Za kasnije odvojene servisne zazore treba novo imenovano
ograničenje; ne mijenjati naknadno značenje ovog polja.

Provjera jednakosti ne smije progutati koliziju tolerancijom. Predlog za budući
račun: interval rastojanja [dLow,dHigh], tolerancija numerike 0,001 mm i optimalnosti
W 0,01 mm kao eksplicitna politika. Feasible samo ako se d>=c može potvrditi;
ako interval presijeca granicu, refine, pa NumericalBoundary/Unresolved ako nije
razriješeno. Ne pomjerati ulaze niti tvrditi „minimum“ iz gustine uzorkovanja.
Ove tolerancije su prijedlog, nisu implementirane niti standard.

## 5. Kompozicija geometrije i kriterijum minimuma

Za zadate theta i Ri: R=Ri+B/2. Za visinski otklon D>0 jedan postojeći solve daje:

- Dturn=2R(1-cos(theta));
- Ls=(D-Dturn)/sin(theta), fizička dužina svakog kosog međukomada;
- P=Ls cos(theta), njegova horizontalna projekcija;
- X=2R sin(theta)+P, zahvat jedne promjene kote.

Ovo je opis postojećeg solvera, **ne zahtjev za njegovu novu implementaciju**.
Novi planerski sloj za svaki kandidat zove solve(-D,...) / solve(+D,...), odnosno
obrnuto za iznad, pa koristi oba stvarna Result-a i oba adaptera. Z0 i drugi apsolutni
datum unosi se samo pri pozicioniranju. Primjena changeZ() se ne poziva.

Srednji horizontalni dio ima fizičku i projektovanu dužinu C. Njegove kote:

- ispod: zPass=z0-D; potreban uslov zPass+B/2+iNew <= O.zMin-c;
- iznad: zPass=z0+D; potreban uslov zPass-B/2-iNew >= O.zMax+c.

Uslov je potreban, nije dovoljan. D mora zadovoljiti i Ls>=LminSlope,
P>=minimumProjection; C>=LminBridge. Prvo se provjerava da li već obični horizontalni
pravac ima c: tada ponuditi AlreadyClear/bez obilaska, bez novih elemenata.

Lokalna osa cijelog rješenja ima osam spojeva q0..q7:
prvi Result.axis[0..3] pozicionira q0..q3, H povezuje q3/q4, drugi Result.axis[0..3]
pozicionira q4..q7. Zato W=2X(D)+C. Tri varijable su D, C i horizontalni položaj a=s0.
Ne nametati C>=širina prepreke: pravilno provjerena koljena mogu djelimično prolaziti
ispod/iznad rubova i skratiti C.

Cilj je leksikografski:

1. najmanji W u cijelom izvodljivom domenu;
2. unutar iste minimalne vrijednosti W, najmanji |zPass-z0|;
3. ako položaj ostane neodređen, sredina zahvata najbliža s=0 uz stvarne granice ulaza.

### Cijela ovojnica

Napraviti neperzistentnu geometriju sedam zatvorenih tijela: četiri prstenasta kružna
sektora, dva rotirana pravougaonika i H. Izolaciju dodati kontinuiranom vanjskom
offsetu; unutrašnje spojne plohe nisu izložene površine. Za svaki član provjeriti
sudar i minimalno rastojanje prema O, uključujući krajnje tačke, ravne ivice i lukove
u njihovom stvarnom ugaonom domenu. Provjeriti i preostale ulazni/izlazni pravac,
međusobnu saglasnost spojeva i preklapanje ne-susjednih dijelova. Zajedničke spojne
plohe susjednih dijelova su dozvoljene. Negativan Ri-iNew nije prećutno drugi model.

Provjera samo Result.corners nije dovoljna: minimum može biti unutar luka. Kružne
lukove ne svesti na poliliniju za certifikovanje. Polilinija je dopuštena za prikaz;
za konzervativnu provjeru mora imati dokazanu ogradu greške i dodatno razrješenje
na granici. Osnova je projekcija istog rezultata; nikada ručno pomjerena grafika.

### Predloženi izolovani algoritam — nije implementiran

1. Validirati snapshot, jedinice, upravnost, puni t preklop, izolacije, ugao,
   prozor, minimum svake vrste prave dionice. MissingInput nije GeometricFailure.
2. Od potrebnog Dclear i uslova etažiranja formirati donju granicu D. Ne mijenjati
   strogi solve; ako numerički ne potvrdi granični kandidat, izvesti to kao zaseban status.
3. Dobiti konzervativan izvodljiv početni kandidat: oba etažiranja potpuno sa strana
   proširene prepreke i dovoljno dugačak H; zatim provjeriti sva tijela. Ako ga prozor
   ne prima, to samo po sebi NE dokazuje neizvodljivost kompaktnijeg kandidata.
4. Za fiksni D oba Result-a su poznata. Iz računanja distancije linija/lukova prema
   O izvesti dozvoljene INTERVALE horizontalnog položaja lijevog i desnog etažiranja.
   Ne pretpostavljati jedan monotoni interval ako se to nije dokazalo. Spojiti intervale
   uslovima C>=LminBridge, a>=sLeft i a+2X+C<=sRight; H i susjedni pravci se posebno
   provjeravaju. Naći najmanji C i dopušteno pozicioniranje za taj D.
5. Optimizovati D globalno: kontaktne grane i njihovi stacionarni/obodni kandidati,
   sa intervalnim branch-and-bound kao kontrolom nepokrivenog domena. Donja ograda
   W>=2X(D)+LminBridge služi za odbacivanje intervala. Konačan dostupni Wmax daje
   Dupper<=Dturn+(Wmax-LminBridge-4R sin(theta))/(2 cot(theta)). Bez prozora koristiti
   W najboljeg provjerenog kandidata za konačnu ogradu pretrage. Ako nema ni prozora
   ni provjerenog kandidata, tražiti granice; ne izmišljati certifikat minimuma.
6. Porediti sve kritične grane: ugao prepreke–luk, ugao–kosa ivica, ivica–luk,
   tangencije, krajnje tačke, aktivni minimumi i granice raspoloživog prostora.
   Uzorkovanje može dati početnog kandidata, ne dokaz globalnog minimuma.
7. Vratiti immutable Solution: oba Result-a, osam spojeva/uglova, četiri luka,
   dužine, kote, ovojnicu, najbliže tačke i njihove uloge, raspoloživost, status i
   numeričku ogradu optimuma. Unresolved nije Infeasible niti Optimal.

Statusi prijedloga: MissingInput / UnsupportedGeometry / AlreadyClear /
InfeasibleWithinLimits / NumericalBoundary / FeasibleCandidate / OptimalWithinTolerance.
Bez dokazanih ograda prikazati „izvodljiv kandidat — minimum nije potvrđen“.

## 6. Poznati analitički primjeri za buduće testove i mockup

Zajednički ulazi: A×B=400×200, Ao×Bo=250×250, z0=zo=-400, Ri=100, theta=45°,
iNew=iObs=0, c=50; LminSlope=LminBridge=100, minimumProjection=0,1 mm.
Efektivni prozor za izvodljive primjere [-700,+700] mm (rezervisani nastavci izvan njega).
Prepreka dovoljno duga duž t. Sve mjere mm, bez montažnih dodataka.

| Veličina | Ispod | Iznad |
|---|---:|---:|
| R ose | 200 | 200 |
| D iz uslova samo vertikalnog zazora | 275 | 275 |
| Izabrani D pri najmanjem W pa najmanjoj dubini/visini | 289,644661 | 289,644661 |
| Apsolutna kota ose prolaza | -689,644661 | -110,355339 |
| Fizička Ls, svaki od 2 komada | 243,933983 | 243,933983 |
| P i apsolutni rise svakog kosog komada | 172,487373 | 172,487373 |
| C, srednji horizontalni komad | 179,289322 | 179,289322 |
| X jedne promjene kote | 455,330086 | 455,330086 |
| W ukupnog obilaska | 1089,949494 | 1089,949494 |
| Najkraće rastojanje | 50 | 50 |
| Vertikalni zazor H–prepreka | 64,644661 | 64,644661 |
| L ose jednog koljena, informativno R*theta | 157,079633 | 157,079633 |

Dodatni oracles za iste ulaze bez izolacije i neaktivne minimume na prikazanom
kontaktnom optimumu (Ri=100, c=50, oba minimalna prava komada 100):

| Ugao | D | Ls | P | C | W |
|---|---:|---:|---:|---:|---:|
| 30° | 281,698730 | 456,217783 | 395,096189 | 200,000000 | 1390,192379 |
| 45° | 289,644661 | 243,933983 | 172,487373 | 179,289322 | 1089,949494 |
| 60° | 300,000000 | 115,470054 | 57,735027 | 163,397460 | 971,687836 |

Za ovu kontaktnu granu: D=325-50 cos(theta), C=250-100 sin(theta),
Ls=(-75+350 cos(theta))/sin(theta). To nije opšta formula MVP-a: prestaje biti
važeći optimum kada se aktiviraju drugi minimumi ili kontaktne grane. Na 30° prozor
1400 mm ima svega oko 9,81 mm ukupne rezerve, što je koristan test dostupnosti.

### Zašto ovo nije biranje minimalne kote

Za D=275 dovoljna širina H u B kriterijumu je C=250; tada W=1131,370850.
To jeste izvodljivo, ali nije najmanji W. Za referentnu simetričnu granu neka je
u=D-275. Dok 0<=u<=50-25√2, kritični kontakt je na unutrašnjem luku:
Cmin=250-2√(100u-u²), X=D+400√2-400.
W opada do u*=50-25√2. Zatim je kontakt na kosoj ivici i W ostaje konstantno
na **100+700√2=1089,949494** sve do D=400-50√2=329,289322, kada C dostiže 100.
Poslije toga W raste uz aktivni Cmin. LminSlope=100 nije aktivan na tom intervalu.
Tie-break zato daje D*=325-25√2, C*=250-50√2, Ls*=350-75√2.
To je analitički oracle ovog zadatog modela, a ne rezultat implementiranog optimizer-a.

Kritični par tačaka za ispod je ugao prepreke (s=-125; z=-525) i gornja izolovana
kontura spoja S1/E2 (s=-160,355339; z=-560,355339), t=0. Razlike su 35,355339 po s i z,
rastojanje 50. Drugi par je ogledan po s. Za iznad ogledati z oko z0, uz odgovarajuće
mapiranje gornje/donje konture. Razmak nije vertikalnih 50 na tim kritičnim spojevima.

Osam tačaka ose za ispod (t=0), korisno za budući nezavisan oracle:

| Spoj | s | Apsolutno z | Uloga |
|---|---:|---:|---|
| q0 | -544,974747 | -400 | početak E1 |
| q1 | -403,553391 | -458,578644 | E1/S1 |
| q2 | -231,066017 | -631,066017 | S1/E2 |
| q3 | -89,644661 | -689,644661 | E2/H |
| q4 | 89,644661 | -689,644661 | H/E3 |
| q5 | 231,066017 | -631,066017 | E3/S2 |
| q6 | 403,553391 | -458,578644 | S2/E4 |
| q7 | 544,974747 | -400 | kraj E4 |

**Neizvodljiv primjer mockup-a:** isti ulazi, ali efektivni prozor [-500,+500].
Potreban W=1089,949494 > 1000; nedostaje ukupno 89,949494, simetrično 44,974747 po
strani. Prikaz je duh prethodnog geometrijskog kandidata, nije prihvaćeno rješenje.

**Izolacija:** iNew=iObs=25, ostalo isto. Spoljna prepreka 300×300; novi kanal
spolja 450×250, Ri-iNew=75. Oracle za isti tip kontakta: D=350-12,5√2=332,322330;
C=300-25√2=264,644661; Ls=375-50√2=304,289322; P=187,5√2-50=215,165043;
W=200+750√2=1260,660172. Ispod: zPass=-732,322330. Čisti minimum 50, vertikalno
57,322330. Lim ostaje 400×200; izolacija ne mijenja čelični presjek u obračunu mase.

### Dodatni granični testovi za narednu fazu — nisu sada izvršeni

| Slučaj | Očekivanje |
|---|---|
| Ispod / iznad uz iste simetrične ulaze | ogledanje, iste fizičke dužine; ispravno mapiranje uglova spojeva |
| 30/45/60°, A!=B | rezultati iz dva postojeća solve-a; svi lukovi i spojne ravni u istom modelu; nezavisni dodatni oracles prije testa |
| Cmin=1000 | C ograničenje aktivno, ne upisati 179,289 mm; ponovo računati dostupnost |
| LminSlope=300 | referentni minimum 243,934 više ne važi; D i/ili W veći, ili InfeasibleWithinLimits |
| Samo centralni zazor dobar, ugao koljena preblizu | odbijanje / povećanje W; mora se prikazati kritični član |
| Minimum unutar luka, ne na spoju | analitička arc–corner provjera, ne samo vertices |
| Oba kriterijuma sa dx=dz=40,c=50 | B prihvata tačku; A je odbija |
| c=0, tangentni dodir; ili c vrlo mali | razdvojiti dozvoljeni dodir po zadatom uslovu od preklapanja i numeričke nerazriješenosti |
| Ri<=0; Ri<=iNew | eksplicitno nepodržan fizički model MVP-a |
| 0°, 90°, NaN, negativna izolacija/razmak/dužina | InvalidInput/UnsupportedGeometry, bez fallback-a |
| LminSlope=0 u podproblemu | solve ostaje neizmijenjen; zasebno odbiti nulti P u budućem CAD adapteru |
| Granica Lmin / prozora ±0,001 | bez tihog zaokruživanja ili povećavanja ulazne kote; NumericalBoundary ako nije razriješeno |
| Asimetričan prozor | testirati pozicioniranje; ne pretpostaviti da je s=0 centar uvijek dopušten |
| Novi pravac već slobodan po c | AlreadyClear, ništa se ne dodaje |
| Prepreka nije upravna, kružna, kosa ili blizu njenog kraja | UnsupportedGeometry, ne pogađati dimenzije |
| Nedostaje izolacija ili kota prepreke | MissingInput, ne zamijeniti nulom bez potvrđenog ručnog unosa |
| cm/m, veliki koordinatni pomak, rotiran UCS | iste fizičke veličine i odnos s/t/z; ne izgubiti poprečnu komponentu |
| Stari DWG objekti bez novih podataka | raniji obračun; bez migracije |
| Snimanje/COPY/brisanje/Undo/Redo | posebna buduća AutoCAD matrica za roditelja i oba podsklopa |

## 7. Korisnički tok i dijalog

1. Tokom redovnog crtanja predložiti opciju „Obilazak“; naziv/registracija komande
   ostaju za kasniju fazu. Ništa se sada ne povezuje sa crtanjem.
2. Izbor postojeće prepreke i isticanje. Iz native TVS kanala pročitati SizeA/SizeB,
   stvarnu Elev, pravac i krajeve; pretvoriti u mm. Kod linije/bloka/proxy-ja ručni
   unos, jasno „ručno“. Ne izvoditi fizičku kotu iz nacrtanog Z ili samo iz ekrana.
3. Izolacija, projektni razmak i minimumi nisu pouzdani postojeći parametri ovih
   osnovnih klasa: tražiti eksplicitan unos; prazno nije 0. U budućem CAD-u oznake
   „iz objekta / ručno / nedostaje“ uz svako relevantno polje. Za promjenu pročitanih
   vrijednosti tražiti jasno uključenu ručnu zamjenu, ne neprimjetno odvajanje od izvora.
4. Dijalog: novi kanal / prepreka; iznad/ispod; ugao, Ri; obe izolacije; razmak;
   posebno LminSlope i LminBridge; proširivo LminIn/Out i dostupni prozor. Presjek u
   istoj skali s i z, automatski fit; bez vertikalnog preuveličanja.
5. Buduća promjena polja invalidira prethodni rezultat; pokazati „računanje/nedostaje“.
   Prikazati W, zPass, cActual i kritični član, vertikalni zazor, sve dužine i P/L.
   Kod neizvodljivosti razlog i nedostajuća dužina; bez lažno validnog zelenog preview-a.
6. Budući preview: neperzistentni transient crtež cijelog Solution-a; obstacle i
   aktivni kanal ostaju bez izmjene. „Pregled u crtežu“ privremeno skriva dijalog,
   povratak/Esc uklanja samo preview. Otkaži/zatvaranje uvijek bez DWG upisa i
   promjene stanja trase. Mockup ne pokreće ovu radnju.
7. Buduće „U redu“ tek nakon ponovne validacije snapshota, istog Result-a, dostupnosti,
   izvora jedinica i članova: jedna atomska operacija svih 7 članova, roditelja i
   dozvoljenog prilagođavanja susjeda. Greška poništava sve. Nastavak trase i globals
   tek nakon uspješnog commita. Jedan Undo/Redo vraća cijelu operaciju i podatke.

## 8. Poređenje budućih modela podataka

Postojeći TVS_OFFSET_MEMBER/version=1 ostaje tačno tročlan. Ne smije se dodati sedam
uloga u isti zapis niti mu promijeniti rise značenje pod istom verzijom.

| Model | Prednost | Nedostatak |
|---|---|---|
| Dvije samostalne V1 trojke + H | odmah koristi postojeće količine i validaciju | nema identiteta kompletnog obilaska; nema zajedničkog presjeka, potpunog COPY statusa ili zaštite H |
| Poseban ravni sklop od sedam članova | direktna zajednička validacija i identitet | novi membership i clone sistem za svih 7; postojeći fizički consumer čita V1 trojku pa traži novi adapter; nepotrebno dupliranje rise |
| **Tanak roditelj nad dvije neizmijenjene V1 trojke + H** | ponovna upotreba fizičkog obračuna, root i male zasebne veze ka roditelju | lifecycle roditelja i njegovih veza mora se posebno provjeriti |

**Preporuka: treći model, zasebni ključ/ugovor npr. TVS_BYPASS_ROOT/version=1**, samo
u budućem odobrenom DEV koraku. Root u extension dictionary srednjeg H, koji je
stabilni nosilac identiteta; ne globalni NOD registar koji se ne kopira s izborom.
Root zapis: novi bypassGuid, 2 soft-pointer reference ka predstavnicima V1 trojki,
soft-pointer/self identitet H i verzija. Uloge sedam članova izvode se iz redoslijeda
dvije V1 trojke i H; child GUID se čita iz stvarnih zapisa, ne čuva kao druga
neusklađena kopija. Identitet detalja vezuje se za bypassGuid i root ObjectId.

Na ostalih šest članova predložiti mali **zasebni** TVS_BYPASS_LINK/version=1:
root ObjectId + bypassGuid + verzija. Uloge se i dalje izvode iz V1 trojki/root-a;
postojeći TVS_OFFSET_MEMBER ostaje neizmijenjen. Root-only sa sesijskim indeksom
bio bi manji u bajtovima, ali nije dovoljan: nakon brisanja H i ponovnog otvaranja,
dvije preostale valjane trojke izgubile bi dokaz da su bile dio obilaska. Mali backlink
omogućava izvesti ParentMissing/Incomplete i tada. To je najmanji preporučeni model
nove semantike koji pokriva ovu granicu, ne tvrdnja o najmanjem broju bajtova.

Da bi model bio funkcionalan i poslije otvaranja, predlog minimalnog root-a ima i
verzionisani projektni kontekst: mode, iNew/iObs, c i imenovani kriterijum B,
LminSlope/LminBridge/LminIn/LminOut/minimumProjection, kao i snapshot provjerene
prepreke (Ao/Bo, odnos njene fizičke kote prema ulazu i njen s/t interval u lokalnom
sistemu sklopa). Opciona soft-pointer referenca označava izvor, nije autoritativno
članstvo. A/B/Ri/ugao i fizičke dužine čitaju se iz stvarnih članova, ne dupliraju se.
Prozor raspoloživosti može ostati izvještaj kreiranja: nije obećanje da prostor u
mreži trajno ostaje slobodan. Snapshot omogućava istorijski presjek; tek ponovno
čitanje žive prepreke potvrđuje današnji zazor. Izmijenjena/nestala prepreka daje
ContextChanged/Unknown, ne automatsku izmjenu kanala. Ova predložena polja nisu upisana;
tačan DWG raspored odobrava se prije DEV upisa.

Zajednička validacija: recipročni root/link podaci, oba child statusa + isti A/B/Ri/ugao, suprotni deltaZ, jednake
početna/završna kota, H spaja prava dva porta na istoj koti, svih 7 različitih članova,
iste baze, tipovi/identitet i geometrija. Referenca ka prepreci je kontekst, ne članstvo
ili konektor: njeno brisanje ne smije obrisati obilazak. Interni status, status kontakta
sa mrežom i status provjere zazora prema prepreci moraju biti odvojeni. Bez validnog projektnog konteksta ili ponovne provjere žive prepreke, status
današnjeg zazora poslije otvaranja je Unknown, ne Valid.

Obračun: jedinstven skup ObjectId-a od 7 fizičkih entiteta; root ne nosi količinu,
izračunate mase/dužine se ne dupliraju. Dva rise ostaju u postojećim slope zapisima.
Presjek se rekonstruiše iz istih geometrijskih parametara i sačuvanog izolacionog
konteksta; nedostajući/nepoznati kontekst se ne zamjenjuje nulom.

Budući COPY: kompletna sedmorka dobija novi root GUID, dva nova child GUID-a i sve
root/link reference na kopije. Kontekst prepreke kopije postaje Unverified dok se nova
prepreka ne izabere/provjeri; istorijski snapshot nije potvrda na novoj poziciji.
Planarno COPY pozicioniranje samo po sebi ne mijenja fizičke kote iz Elev. Samo H:
root Incomplete, reference na nekopiranu djecu null. Kopija bez H: novi fragment GUID,
root reference null, nikad operativne veze sa originalom. Cijela kopirana V1 trojka
može ostati interno Valid kao etažiranje i imati vlastite količine, ali njen
bypass status je Fragment/Incomplete, ne cjelovit obilazak. Ne uklanjati taj podatak
tiho; budući eksplicitni „odvoji od obilaska“ može biti zasebna odluka.

Sesijski indeks samo ubrzava recipročne provjere; DWG linkovi/root ostaju izvor
istine. Dva roditelja istog člana daju Conflict, ne dvije mase. Brisanje člana ne briše
ostatak automatski. Brisanje H daje ParentMissing na djeci i nakon otvaranja;
Undo treba vratiti isti root/child identitet i reference. Poslije trajnog purge-a
nedostajući roditelj ostaje izvedeni status; nema njegovog automatskog rekreiranja.
Snimanje/REGEN ne popravljaju zapise. Ako clone/transaction hookovi ne mogu bezbjedno
obraditi root + linkove, taj DEV korak je blokiran; ne primjenjivati nepotpunu zaštitu.

Prva buduća verzija: bez slobodnih gripova/MOVE/SCALE/MIRROR; kontrolisano cijelo
ponovno računanje ili brisanje; potpuni COPY pozicionira samo kopije. Nestali član
izvodi Incomplete; ne popravljati pri otvaranju/REGEN-u. Generisani detalj je izveden
prikaz s referencom na root, ne drugi količinski entitet. Obnova kompletnog sklopa iz
proizvoljno pomjerenih dijelova nije dio prijedloga.

## 9. Faze i kriterijumi prelaska

| Faza | Obim poslije posebnog odobrenja | Uslov prije sljedeće faze |
|---|---|---|
| Sada — specifikacija/mockup | samo dokument i fiksni prikaz 4 stanja | pregled definicije zazora, cilja i modela |
| Izolovani solver | immutable Solution bez SDK-a, ovojnice, minimum i numeričke ograde; bez DWG | nezavisni oracles, svi kritični kontakti i granice; Unresolved odvojen od infeasible |
| Preview | razvojni read-only izbor, dijalog, transient iz ISTOG Solution-a | praktično slaganje spojeva/kontura/clearance, Esc bez promjene baze |
| DEV upis | tek odobrena nova root šema i projektni uslovi; jedna transakcija, lifecycle | prethodna Z integracija praktično provjerena; build 2020/2022 i fail-injection testovi |
| Praktični test | matrica 2020/2022, mm/cm/m, UCS, konture/maska/OSNAP, COPY/Undo/Redo/reopen/obračun | korisnički zapis stvarnog okruženja; potom zasebna odluka o produkciji |

Buduće faze: kosa ukrštanja (orijentisani 3D gabariti), više prepreka, različite
ulazne/izlazne kote, automatski izbor ugla uz novu ciljnu funkciju, ograničenje visine
plafonom/podom, trajne oznake detalja i generisanje presjeka. Ne dodavati ih ovom MVP-u.

## 10. Odluke za pregled

1. Prihvatiti B: čisti razmak kao najmanje euklidsko rastojanje; pored njega prikazati
   i vertikalni zazor. Alternativa A mora imati druga imena polja i druge rezultate.
2. Prihvatiti cilj najmanji W, pa najmanji visinski otklon u slučaju istog W.
   Time se svjesno dopušta prolaz dublje/više od minimalne vertikalno potrebne kote.
3. Prihvatiti smjer budućeg modela: tanak roditelj nad dvije postojeće V1 trojke + H,
   uz male zasebne root veze na članovima radi brisanja/COPY/reopen-a.
   Tačna trajna projektna polja i lifecycle odobravaju se zasebno prije DEV upisa.

Vrijednosti 100/100 mm minimuma i 0,1 mm projekcije u primjerima su demonstracione,
ne norma i ne konačno korisničko pravilo. U ovom koraku nije potrebna potvrda
implementacije: sljedeći solver/CAD upis ne počinje prije pregleda ovog prijedloga.
