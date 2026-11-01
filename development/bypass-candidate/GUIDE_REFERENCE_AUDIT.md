# Nalaz postojećeg vođenja i ugovor budućeg adaptera

Izvor pregleda: razvojni commit c97f308 (stvarni ARX/DBX izvori). Ovaj dokument ne
mijenja vođenje, dijalog, Z integraciju ili DWG podatke. Adapter nije implementiran.

## 1. Šta je potvrđeno iz koda

| Opcija/podatak | Datoteka/metoda | Potvrđeno značenje |
|---|---|---|
| IDC_ElevUp / Vrh | dgPipeSize.h:66, dgPipeSize.cpp:52/95/139/303 | Checkbox visinske reference; handler uključuje Vrh i isključuje Centar/Dno. |
| IDC_ElevMid / Centar | dgPipeSize.h:67, dgPipeSize.cpp:53/96/140/316 | Checkbox centralne visinske reference. |
| IDC_ElevDown / Dno | dgPipeSize.h:68, dgPipeSize.cpp:54/97/141/326 | Checkbox donje visinske reference. |
| Elev i unitsFactor | dgPipeSize.cpp:123 | Unos fizičkih mm konvertuje se u model-jedinice. |
| Emode 0/1/2 | Func.cpp:1360, 1853 i getElev:2743 | Lokalna semantika u Change(sset)/Change(entity): 0 osa, 1 vrh, 2 dno. |
| Elev / ElevUp / ElevDown | TVS_Entity.cpp:706/712/872/900 | Fizičke kote ose i golog lima; ne „gore/dolje na ekranu“. |
| FirstPoint / LastPoint | TVS_Pipe.cpp:1021, Gimme4PipePoints | Kontura u osnovi nastaje simetričnim pomakom ±SizeA/2 oko ovih tačaka. One su osna linija, ne odabrana ivica. |
| Direct | TVS_Entity.cpp:690/696; TVS_Pipe.cpp:97/158/1070 | Polje se čuva/čita i inicijalizuje, ali pregledana grana Gimme4PipePoints ga ne koristi za trostruko XY vođenje. Nije dokaz za „gornja/lijeva ivica“. |
| globalElevMid | Settings.h:61, acrxEntryPoint.cpp:935/1067/1097 | Stvarna globalna kota ose u pregledanim crtajućim putanjama. |
| globalElevUp/mid/down | Settings.h:6–8 | Makroi 1/2/3. NISU isti numerički domen kao Emode 0/1/2. |
| globalElev | Settings.h:62, acrxEntryPoint.cpp:1098 | double inicijalizovan brojem 3, zatim prepisan kotom dg.Elev; ne može se pouzdano čitati kao sačuvani izbor vođenja. |
| UCS unos | CoordinateInput.h:15, getPoint/selectEntity | Tačke se prenose UCS→WCS; ključne riječi i otkazivanje se prosljeđuju. Nema promjene globalnog UCS-a. |

Ključni dokaz fizičkog značenja:

    // Func.cpp:2755–2775, aktivni getElev
    b = SizeB == 0 ? SizeA : SizeB;
    Emode == 0 -> Elev;
    Emode == 1 -> Elev - b/2;
    ostalo     -> Elev + b/2;

    // TVS_Entity.cpp:886–888, pravougaoni put_ElevUp
    ElevUp = newVal;
    Elev = newVal - SizeB/2;
    ElevDown = newVal - SizeB;

    // TVS_Entity.cpp:914–916, pravougaoni put_ElevDown
    ElevUp = newVal + SizeB;
    Elev = newVal + SizeB/2;
    ElevDown = newVal;

Aktivni potrošači nisu samo deklaracije: func::Change(ads_name&) počinje na Func.cpp:977,
određuje Emode na 1360–1369, a na 1416 poziva put_Elevation(getElev(...)).
Drugi overload func::Change(AcDbEntity*) počinje na 1496 i poziva isto na 1903.
COM put_ElevUp/Down u ComPipe.cpp:784/825 konvertuje mm i poziva odgovarajuće native
settere; to je dodatni dokaz fizičke visinske semantike.

## 2. Važna nedosljednost postojećeg crtajućeg toka

acrxEntryPoint.cpp, changesize:1066 uvijek postavlja dg.ElevMid=true pri otvaranju.
Nakon IDOK, 1097–1098 kopira dg.Elev u globalElevMid/globalElev, bez getElev i bez
čuvanja izbora tri checkbox-a. dgPipeSize::OnBnClickedOk:190–236 razrješava jedinice
i režime radijusa, ali ne pretvara Vrh/Dno u osu.

Dakle, pregledom se ne može potvrditi da tri opcije u TVS_SETTINGS trajno vode
novu trasu odabranom visinskom referencom. Njihovo aktivno fizičko značenje potvrđeno
je u Change/setter putanjama. To su dvije različite putanje. Ovaj korak to samo
evidentira; ne popravlja postojeći tok i ne proglašava UI ponašanje praktično testiranim.

U pregledanim aktivnim TVS_PIPE/drawStep3/nextpipe/add_new/Gimme4PipePoints putanjama
nije potvrđen poseban izbor tri lokalne XY referentne linije. Postojeće tačke su osa.
Ako korisnički workflow sadrži dodatni način vođenja ivicom, njegov tačan ulaz/callsite
mora biti identifikovan prije integracije. Ne izjednačiti checkbox „Vrh“ sa lokalnom
lijevom stranom i ne pretpostaviti da nedokumentovani Direct kodira taj režim.
Ovo nije tvrdnja da korisnikov opaženi način rada ne postoji; nalaz je ograničen
na konkretno pregledane izvorne putanje i može ukazivati na drugi UI/alat/putanju.

## 3. Lokalna strana konture u osnovi — dokaz iz crtačke grane

TVS_Pipe.cpp:1023–1032:

    dx = LastPoint.x-FirstPoint.x;
    dy = LastPoint.y-FirstPoint.y;
    X2 = dy*SizeA/Length/2;
    Y2 = dx*SizeA/Length/2;
    A = FirstPoint + (-X2,+Y2,0);
    B = LastPoint  + (-X2,+Y2,0);
    C = LastPoint  + (+X2,-Y2,0);
    D = FirstPoint + (+X2,-Y2,0);

Za pravac u=(ux,uy), lokalna lijeva normala n=(-uy,ux). A/B su Caxis+Awidth/2*n;
C/D su suprotna strana. To je lokalno lijevo/desno, ne globalno gore/dolje. Pri
obrtanju pravca n mijenja znak. Ovaj dokaz opisuje GDJE SU ivice; ne dokazuje da
postojeća komanda omogućava unos svake od njih kao referentne linije.

A je širina u osnovi. B je visina za fizičku kotu. Primjer A!=B=400x200 znači:
200 mm poprečnog XY pomaka do ivice, 100 mm visinskog pomaka do vrha/dna.
Izolacija nije uključena u postojeći put_ElevUp/Down, pa adapter tih režima koristi
goli lim. Spoljni gabarit sa izolacijom je zaseban ulaz provjere zazora.

## 4. Predlog tipiziranog ugovora — bez implementacije

GuideSnapshot, samo buduće stanje adaptera:
- coordinateMeaning: EntityAxis / UserReferenceLine / Unknown;
- planReference: Axis / LocalLeft / LocalRight / UnresolvedExistingMode;
- heightReference: Axis / MetalTop / MetalBottom / UnresolvedExistingMode;
- originalUiState (bez prepisivanja sirovih kodova);
- directed heading uWcs, stvarni axisSegmentWcs, izvorne referentne tačke;
- actualAxisElevationMm, po potrebi enteredHeightReferenceMm;
- mmPerModelUnit, drawingPlaneZ, provenance i snapshot identiteta/izmjena izvora.

Ne koristiti gornji/donji ekran kao planReference. Dok stvarno mapiranje nije
poznato, UnresolvedExistingMode se odbija u integraciji, bez resetovanja na Axis.

### ResolveToAxis(snapshot)

Ako ulaz dolazi sa stvarnog native kanala: FirstPoint/LastPoint su VEĆ osa.
Ne primjenjivati još jednom A/2. Stvarna kota get_Elevation je VEĆ kota ose.
Ne oduzimati ponovo B/2 samo zato što je raniji unos bio po vrhu.

Ako stvarni UI ulaz jeste referentna linija E, prethodno dokazano mapirana u lokalnu
stranu, neka je r = +A/2 za LocalLeft, 0 za Axis, -A/2 za LocalRight:

    Caxis = E - r*n,     n=(-uy,ux)

Dimenzije su fizički mm; za WCS/model tačke pomak je (r/mmPerModelUnit)*n.
Odabrani ugao koljena theta nije heading trase psi. Adapter ne mijenja nijedan.

Za dokazanu visinsku referencu novog korisničkog unosa:

    Axis:        zAxis = zRef
    MetalTop:    zAxis = zRef - B/2
    MetalBottom: zAxis = zRef + B/2

Ovo je referenca golog lima. Buduća referenca spoljne izolacije morala bi biti
posebno imenovana; ne mijenjati značenje postojećeg Vrh/Dno.

### ObstacleFrame(axis, obstacle)

Izračunati presjek STVARNIH osa, ne korisničke referentne ivice. U osnovi:

    Caxis(lambda)=C0+lambda*u
    Obstacle(mu)=P+mu*v
    lambda = cross(P-C0,v)/cross(u,v)
    Xcross = C0+lambda*u

Za MVP provjeriti u·v≈0 i nenulti imenilac. s=dot(X-Xcross,u), t=dot(X-Xcross,n),
u fizičkim mm. Krajeve prepreke projektovati na n i sortirati min/max t,\nuključujući obrnuti smjer, pa provjeriti puni preklop.
Širina prepreke duž s uzima se iz njenog poprečnog SizeA, visina iz SizeB.

Kod potpuno upravnog ukrštanja pomak paralelne referentne linije može ostaviti
istu s-stanicu, ali pomjeriti presjek duž prepreke (t); i tada je pogrešno ignorisati
pretvaranje u osu jer se mijenja provjera krajeva prepreke. Za buduća kosa ukrštanja
mijenja se i stanica. Kose slučajeve sada ne podržavati.

### ResumeReference(solutionExitAxis, snapshot)

Rezultat ostaje osni. Tek DTO nastavka vraća istu korisničku referencu:

    Eexit = Cexit + r*n

MVP izlazi istim pravcem i fizičkom kotom kao ulaz. HeightReference i
PlanReference ostaju isti; iz nove zAxis se izvodi odgovarajući zRef ±B/2.
drawingPlaneZ se čuva odvojeno. Ne prebacivati nastavak na osu i ne prepisivati
mode kroz globalElev koji nije pouzdano sačuvan režim.

Resolve/Resume su čiste transformacije koje pripremaju novi DTO. Snapshot se ne
mijenja. Cancel/NoChange/Failed ne primjenjuju DTO, ne pomjeraju originalni kraj,
ne mijenjaju raw UI stanje i ne prepisuju globals. Tek uspješan budući atomski CAD
commit može dati Applied i usvojiti nastavak. Feasible matematičkog provjeravača
nije isto što i Applied.

Sadašnja Z integracija se u ovom koraku ne mijenja. Prije buduće integracije
treba posebno riješiti čuvanje režima u postojećim crtajućim pozivaocima i dokazati
njegovu stvarnu UI semantiku; dodavanje prikrivenog novog režima nije odobreno.

## 5. Plan testova adaptera — nisu sada izvršeni

Za istu osu C=(1000,2000), A=400, B=200, zAxis=-400 mm:

| Heading | Lokalna lijeva referenca E | Osna referenca | Lokalna desna referenca E |
|---|---|---|---|
| 0° | (1000,2200) | (1000,2000) | (1000,1800) |
| 37° | (879.636995,2159.727102) | (1000,2000) | (1120.363005,1840.272898) |
| 90° | (800,2000) | (1000,2000) | (1200,2000) |
| 180° | (1000,1800) | (1000,2000) | (1000,2200) |
| 217° | (1120.363005,1840.272898) | (1000,2000) | (879.636995,2159.727102) |
| 270° | (1200,2000) | (1000,2000) | (800,2000) |

Sve tri odgovarajuće reference moraju kroz Resolve dati ISTU fizičku osu, isti
Xcross i isti kandidat. Pri obrtanju pravca ista fizička ivica mijenja oznaku
LocalLeft↔LocalRight; ne „popravljati“ je globalnom Y koordinatom.

Potvrđene tri visinske opcije testirati za sva navedena usmjerenja:
zRef=-300 (vrh), -400 (osa), -500 (dno) uvijek moraju dati zAxis=-400. Pravac trase
ne mijenja to pravilo. Dodatno: nepoznati/miješani checkbox statusi ne daju tihi Axis.

Matrica budućeg adaptera:
- 3 visinske reference × heading 0/37/90 i obrnuti pravci;
- ako/tek kad se potvrdi stvarni XY input režim: 3 plan reference × isti heading;
- A!=B, mm/cm/m, pomjeren i rotiran UCS;
- EntityAxis ulaz bez dvostrukog A/2/B/2 pomaka;
- ista fizička trasa kroz različite reference → isti sedmočlani Solution;
- nastavak poslije obilaska ostaje istom lokalnom stranom i visinskom referencom;
- Cancel/NoChange/Failed: identičan snapshot režima, tačaka i globals;
- promjena izvora/prepreke za vrijeme preview-a invalidira DTO, bez upisa;
- izolacija utiče na zazor, ne na postojeću referencu golog lima.

Nijedna od tih CAD/adapter provjera nije predstavljena kao sada izvršena.
