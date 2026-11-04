# Zadati kandidat obilaska — izolovani provjeravač

Obim: SDK-nezavisan C++11 modul, bez optimizacije, CAD komande, DWG zapisa,
COM/specifikacije i izmjene postojeće Z integracije. Prihvaćeni OffsetGeometry.h
nije izmijenjen. Čisti razmak je ograničenje, ugao je ulaz; W se ovdje ne minimizuje.

## Interfejs

- geometry/bypass/Candidate.h: tvsBypass::Input, check(Input), Result.
- geometry/bypass/OutwardInterval.h: ograničeno intervalno jezgro.
- Input konstruktor ZAHTIJEVA minimumSlope, minimumBridge, minimumProjection.
  Nema skrivenog pravila od 100 mm. Ostali početni brojevi su javni primjerni ulazi;
  budući CAD adapter mora ih popuniti stvarnim ili eksplicitno ručnim podacima.
- Sve dužine/kote u fizičkim mm; lokalni s duž trase, z fizički vertikalno, t poprečno.
  Ulaz z0 je kota OSE. Crtački Z i UCS nisu ulazi matematičkog provjeravača.
- angleDegrees je 30, 45 ili 60. Nema traženja/zaokruživanja ugla.
- above, D>0, C>0 i s0 su ZADATI; nijedan se ne popravlja niti optimizuje.
- windowLeft/windowRight: efektivni prozor poslije rezervacije susjednih nastavaka.
- floor.active / ceiling.active podrazumijevano false; njihova vrijednost se tada
  uopšte ne koristi, čak ni ako je NaN. Aktivne granice važe za SPOLJNI gabarit,
  ne samo osu.
- Obstacle: obstacleS/Z, obstacleWidth/Height, obstacleInsulation i T interval.
  Mora biti jedna horizontalna pravougaona prepreka, upravna i dovoljno duga da
  pokrije A+2iNew i c do oba kraja. Nepotpuni preklop se odbija kao Unsupported.
- Insulation/clearance moraju biti eksplicitne nenegativne vrijednosti.
  Ri > insulation; popunjavanje unutrašnje krivine izolacijom nije podržano.
- Numerički domen ulaznih skalara: konačni |x|<=1e9 mm. Ovo je granica ovog
  provjeravača, ne norme ni postojećeg TVS-a.
- Policy: distanceToleranceMm=.001, jointToleranceMm=.000001, maxCells=30000,
  maxDepth=40, supportSlices=24. To su eksplicitni numerički/radni limiti.
  Dostizanje limita nije dokaz neizvodljivosti.

Primjer zadatog izvodljivog, neoptimalnog kandidata:

    #include "geometry/bypass/Candidate.h"
    tvsBypass::Input p(100, 100, .1);
    p.D=400; p.C=400; p.s0=-765.685424949238;
    p.windowLeft=-1000; p.windowRight=1000;
    p.clearance=50; // ostaje 50, ne snižava se zbog numeričke granice
    tvsBypass::Result r=tvsBypass::check(p);

Result:
- status: Feasible / Infeasible / NumericalBoundary / InvalidInput / Unsupported;
- reason: razlog, bez prećutnog fallback-a;
- geometryReady, offsets[2] i axis[8] iz dva postojeća solve poziva;
- members[7], u redoslijedu FirstElbow, FirstSlope, SecondElbow, Bridge, ThirdElbow,
  SecondSlope, FourthElbow;
- run, passElevation, slopeLength, bridgeLength, physicalRise kao intervali;
- obstacle.mm=[donja,gornja ograda], relation, converged, cells;
- obstacle.onBody/onObstacle: intervali koordinata konkretnog svjedoka gornje ograde;
- witnessMember i lowerMember mogu biti različiti. Svjedok gornje ograde nije
  garancija jedinstvene tačne tačke minimuma; njen kvalitet se čita uz ogradu rastojanja;
- selfFirst/selfSecond identifikuju problematičan par kada razdvajanje nije dokazano;
- jointDeviationUpperMm: opažena konzervativna ograda slaganja spojeva;
- minimumConfirmed je UVIJEK false. Čak ni Feasible ne potvrđuje najmanji W.

## Geometrija

Postojeći solve se poziva za +D/-D (ili obrnuto), bez promjene njegove stroge politike.
Iz ISTIH Result-a uzimaju se ose, centri, radijus i pozicioniranje. Ne postoje drugi
generator etažiranja, poziv changeZ ili ručna popravka krajnjih tačaka.

Svako koljeno je CIJELI prstenasti sektor. Ravni dijelovi su zatvoreni puni
paralelogrami presjeka, sa spojnim ravnima prema istom uglu. Sektori obuhvataju sve
radijuse između Ri-iNew i Ri+B+iNew, ne samo dvije konturne krive. Izolovana prepreka
je puna spoljna pravougaona kutija. Ovo je lokalni s-z model pune poprečne
pokrivenosti; nije univerzalni 3D collision engine.

Slaganje šest spojeva provjerava oba kraja spojnih presjeka i potporne poluravni.
Preklop susjednih komada ograničen na numeričku spojnu nesaglasnost mjeri se i
uključuje u guard; ništa se ne pomjera radi njegovog uklanjanja. Veća/nepotvrđena
nesaglasnost daje NumericalBoundary. Isti guard dodatno proširuje ograde rastojanja
i granica, da se ne bi proglasila izvodljivost oslanjanjem na mikroskopsku pukotinu
u spoju. Podrazumijevani prag slaganja nije dozvola da se c umanji za taj prag:
koristi se stvarno izračunata ograda, a ne prećutno oduzimanje 1e-6 od zahtjeva.

Ne-susjedni članovi provjeravaju se intervalnim potpornim poluravnima. Ako postoji
dokaz preklapanja punih tijela, status je Infeasible; neuspjeh nalaženja razdvajanja
nije dokaz kolizije i daje NumericalBoundary. Susjedstvo nije dozvola za proizvoljno
preklapanje: spojne ravni i njihov numerički preklop zasebno se provjeravaju.

Uz sedam članova za zazor se provjeravaju i preostali horizontalni pravci do granica
prozora. To su dijagnostički IncomingNeighbor/OutgoingNeighbor, nisu dodatni članovi,
ne mijenjaju W i ne nose novu količinu.

Minimumi fizičkih kosih komada i projekcije provjeravaju se i intervalnim nužnim
nejednakostima. One samo provjeravaju ograničenje; ne generišu zamjenski Result.
Strogi solve ostaje jedini izvor etažiranja. Ako odbije numeričku granicu koja se
ne može pouzdano klasifikovati, vraća se NumericalBoundary, bez zaokruživanja D.
Fizička dužina izvodi se i iz dviju stvarnih krajnjih tačaka ose, ne samo sirovog
double polja straightLength.

## Rastojanje i preklapanje

Koljeno se parametrizuje uglom i radijusom, a pravi komad sa dvije koordinate.
Intervalni AABB svake parametarske podoblasti obuhvata SVE njene tačke. Donja
ograda rastojanja dobija se iz tih spoljnih kutija. Konkretne tačke tijela i sigurne
tačke prepreke daju gornju ogradu. Dijele se podoblasti koje mogu dati minimum.

To nije dokaz zasnovan na uzorkovanju: uzorci popravljaju samo gornju ogradu.
Donja ograda obuhvata cijeli neprekidni domen, uključujući unutrašnjost luka.
Radni limit ili nedovoljno tijesna ograda ostaju vidljivi u rezultatu.

Relation:
- PositiveDistance: donja ograda >0;
- Touching: dokazano razdvojene unutrašnjosti i tačan svjedok rastojanja 0;
- Overlapping: strogo unutrašnji svjedok ili ugao prepreke strogo unutar punog
  tijela. Posljednji slučaj dokazuje pozitivni preklop jer prepreka ima nenultu
  unutrašnjost u okolini svog ugla;
- Unresolved: ostalo. To se ne predstavlja kao siguran dodir ili sigurna kolizija.

Kod sadržavanja jednog tijela u drugom rezultat je 0/Overlapping, ne pozitivno
rastojanje dvije granice. c=0 ne dopušta pozitivan preklop. Za c=0 dokazani tačan
dodir na jednostavnom ravnom slučaju razlikuje se od preklopa. Na sastavljenom
kandidatu numerička spojna ograda može i takav dodir ostaviti kao NumericalBoundary.

Feasible zahtijeva dokaz svih ograničenja, sa donjom ogradom zazora poslije guard-a
najmanje c. Infeasible ima svjedok kršenja ili dokazano nemoguć minimum/prozor.
NumericalBoundary znači da nema takvog konačnog dokaza. InvalidInput i Unsupported
nisu geometrijska neizvodljivost.

## Šta numeričke garancije ZAISTA pokrivaju

Jezgro ima stvarni mehanizam intervalnih ograda:
- osnovne operacije šire se prema spolja pomoću nextafter;
- TwoSum i fma rezidual prepoznaju tačne ili jednostrano zaokružene operacije;
  sumnjivo podlivanje ne proglašava se tačnom nulom;
- sqrt je omeđen rezidualom uz pretpostavku IEEE pravilno zaokruženog sqrt;
- sin/cos nisu pozivi običnom libm-u: intervalni Taylorov polinom i analitička
  ograda ostatka, plus dokazana monotonost na korišćenom domenu [0,1.1];
- zahtijevaju se strict binary64, FE_TONEAREST, FLT_EVAL_METHOD=0, bez fast-math
  i bez FTZ/DAZ. Kompajliranje sa fast-math se odbija, kao i nepodržan runtime;
- pozivi check/measure čuvaju i vraćaju floating-point environment/flags.

Ovo nisu samo double brojevi nazvani intervalima. Ipak, ovo NIJE formalno
sertifikovana/verifikovana biblioteka niti certifikat kompletnog ObjectARX lanca.
Garancije zavise od navedenog IEEE/C++ ugovora (uključujući ispravno fma/sqrt/
nextafter i odsustvo nedozvoljenog preuređivanja izraza). Izvršeni su Linux/GCC
testovi sa -fno-fast-math -ffp-contract=off. MSVC/Windows i drugi kompajleri još
nisu provjereni; nikakva promjena njihovog SDK-a/toolset-a nije izvršena.

Postojeći solve ostaje običan C++98 proračun. Checker provjerava njegov model i
spojna odstupanja; ne tvrdi da je sam solve postao intervalno sertifikovan.
Ograde geometrijskog jezgra odnose se na eksplicitno sastavljen Result-based model
sa uglom u binary64 radijanima i navedenom spojnom ogradom, ne na crtež, proizvoljan
proizvodni fitting ili nenavedene prepreke. Potrebna je posebna provjera prije CAD
integracije. Niskonivojski Body/Region/pairRelation pomoćnici nisu ulaz za nepodržano floating\nokruženje: direktni pozivalac mora obezbijediti isti strict/nearest, netrapirajući\naritmetički ugovor. Za korisnički ulaz koristiti check. Pomoćnici zahtijevaju valjane pune oblike,
pozitivne dimenzije i podržani ugaoni domen; javni check validira korisnički Input.

## Granice i korisnička poruka

Referentni D=289.64466094067262, C=179.28932188134524 uz c=50 daje ogradu oko 50,
pa NumericalBoundary ostaje ispravan rezultat. Ne postoji prećutna promjena c na
49.9. Test sa c=49.9 je ZASEBAN ulaz koji provjerava potvrđenu unutrašnju oblast
izvodljivosti, a ne proizvodna politika zaobilaženja granice. Zadati širi kandidat
D=400,C=400 prolazi sa stvarnim c=50 i ne potvrđuje minimum W.

Planirano korisničko označavanje budućeg jednostavnog traženja:
„Izvodljiv kandidat; minimalni zahvat nije potvrđen.“ Traženje nije dio ovog koraka.
Za budući optimizer posebno definisati pouzdanost i tolerancije cilja W i cilja D.
Tolerancija zazora nije tolerancija optimalnosti W, a W-tie nije D-tie. Gusta pretraga
bez globalnih ograda ne potvrđuje globalni minimum. Prvi cilj ostaje pouzdano
izvodljiv i predvidljiv kandidat, pa tek poboljšavanje zahvata.

Nema dodatih DWG podataka ili promjene V1. Roditelj nad dvije V1 trojke ostaje samo
prihvaćen smjer; tačan format/lifecycle i vođenje trase zahtijevaju zasebno odobrenje.
