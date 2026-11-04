# DEV — kontrolisana integracija promjene kote

Ovo je razvojni paket, ne produkciono izdanje. Integracija je uključena samo sa
TVSOffsetIntegration=true (TVS_OFFSET_INTEGRATION) u ARX i DBX. Bez flaga važi stara
putanja. Dokazne komande TVS_TEST_OFFSET/CHECK/FAIL ostaju dostupne za nove prazne probe.
Njihovi neoznačeni nagibni segmenti i dalje nisu novi fizički sklopovi; za dokaz
integracije treba koristiti Z kroz redovno crtanje, ne zaključivati iz stare probe.

## Ranija potvrda

Korisnički je potvrđen samo raniji geometrijski dokaz u AutoCAD 2020/mm/Kontura:
12 slučajeva, 24/24 TAP, 216/216 objekata, stabilni REGEN, Undo/Redo, FAIL rollback,
konture i OSNAP. To nije potvrda nove integracije, podataka ili obračuna.

## Prvi test u kopiji/praznom probnom DWG-u

1. Čist AutoCAD proces, bez učitanog produkcionog TVS-a. DEV DBX pa ARX iste godine
   iz zasebnog direktorijuma; ne prepisivati produkcioni bundle.
2. TVS_SETTINGS: mm, 400x200, kota ose -400; za referentni Ri=100 izabrati k=0.5
   pravougaonog koljena. Postaviti ostale korisničke osobine prije crtanja.
3. TVS_PIPE: nacrtati pravac duži od 1500 mm. Opcija Z, Na kotu 0, Ugao 45.
4. Provjeriti novu geometriju i nastavak trase. P=282.842712 mm, riseStraight=
   282.842712 mm, fizička dužina=400 mm, količina=0.4 m, površina=0.48 m²,
   debljina=0.6 mm, masa=2.2608 kg bez rezerve. Samo srednji označeni komad ima
   ovaj obračun; ulazni/izlazni kanali su obični susjedi.
5. Ponoviti spuštanje sa -400 na -800, zatim 30/60° i nekardinalnu trasu.
6. Cancel/Esc/zatvaranje dijaloga i jednake kote: bez promjene aktivnog kraja,
   globalne kote i geometrije. Nastaviti trasu običnom sljedećom tačkom.

Razvojna podešavanja Lmin=100 mm i minimumProjection=0.1 mm nisu zahtjev norme niti
konačno pravilo. Strogi solve nije izmijenjen. Na numeričkoj granici može odbiti
slučaj umjesto da promijeni završnu kotu. Tačno 90°, nepodržan presjek/tip,
nepozitivan Ri, premala visina i nedovoljno dug ulazni kanal odbijaju se bez fallback-a.

## Novi podaci i granice

Svaki od tri člana nosi verzionisan TVS_OFFSET_MEMBER sa GUID-om, ulogom, tri
recipročne soft-pointer reference i rise poljem (nenulto samo kod međukomada).
To nije migracija starih objekata. Fizička dužina izvodi se iz vlastite XY projekcije
i rise, ne čuva se nezavisan L. Otvaranje/REGEN samo čita i ne popravlja DWG.
Nepotpuno članstvo i veza sa susjednim kanalima su odvojeni pojmovi.

Obračun običnih objekata bez zapisa ostaje raniji. Novi nagibni komad ima lokalnu
projekciju spojnih presjeka sa vidljivim i skrivenim linijama; obični horizontalni
kanali nisu globalno promijenjeni. Tačnost punog izgleda mora se potvrditi u CAD-u.

## Obavezna praktična matrica — ovo NIJE izvještaj da su testovi prošli

| Slučaj | Očekivano | Izmjereno | Odstupanje/prolaz |
|---|---|---|---|
| Interaktivni Z +400 / 45° | P=282.842712; rise=282.842712; L=400 mm | | |
| Količina i masa srednjeg komada | 0.4 m; 0.48 m²; 2.2608 kg | | |
| Z -400 / 30° / 60° | fizička i projektovana dužina prema solve; nastavak trase | | |
| Cancel / NoChange | bez pomjeranja kraja i promjene kote | | |
| Premala visina / ulaz / 90° | potpuno odbijanje | | |
| Undo/Redo pa nastavak | saglasna geometrija, podaci i aktivna kota | | |
| Snimiti/zatvoriti/otvoriti | isti validni podaci, količine i konture | | |
| COPY cijele trojke | novi GUID i međusobne reference kopija, original isti | | |
| COPY dijela | Incomplete, bez operativne veze ka originalu | | |
| Brisanje člana / Undo brisanja | izvedeni Incomplete pa Valid | | |
| Nevažeći/nepoznat zapis | prijavljena greška, bez horizontalnog fallback-a | | |
| Parcijalni grip/MOVE/SCALE/Properties | član se ne mijenja; dozvoljeno COPY pozicioniranje odvojeno | | |
| AutoCAD/Excel tabele, COM, KGTEZ, Properties | isti fizički obračun | | |
| mm/cm/m i rotiran UCS | isti fizički rezultat | | |
| Stari objekti | raniji prikaz i obračun, bez automatske migracije | | |

Čuvati izvještaj sa AutoCAD verzijom, jedinicama, režimom i BUILD commit-om.
Ograničenja i status izvršenih testova biće navedeni u završnom DEV izvještaju.

## Dijagnostika podataka i razvojnih postavki

- TVS_OFFSET_STATUS: izabrati bilo koji član. INTERNAL je valjanost trojke;
  NEIGHBORS posebno broji odgovarajuće ulazne/izlazne susjede (0/1/više).
  Srednji komad prikazuje P, L, količinu, površinu i masu. Nema popravke/upisa.
- TVS_OFFSET_POLICY: sesijska komandnolinijska podešavanja Lmin>=0 i
  minimumProjection>0 u mm. Podrazumijevano 100/0.1, bez upisa u DWG ili novog
  dijaloga. Nulti ravanski segment nije podržan integracijom.
- TVS_OFFSET_TEST_BAD: samo na NOVOM označenom članu namjerno upisuje verziju 999.
  Koristiti na kopiji probnog DWG-a; STATUS/COM/tabele moraju prijaviti grešku,
  pa UNDO vraća prethodne podatke. Nije komanda migracije starih objekata.

COPY cijele trojke treba dati novi GUID i recipročna ID-jeva kopija. Djelimična
kopija nulira veze ka nekopiranim originalima i daje Incomplete. Nepotpuna kopija
može imati vlastitu fizičku količinu ako je njen rise validan; ne postaje operativni
povezani sklop. Originalni članovi su zaštićeni setter/grip/transform putanjama;
novim kopijama dozvoljena je samo translacija pozicioniranja tokom clone komande.
SCALE/ROTATE/MIRROR originalnih članova nisu podržani parametarski editor.
Obični susjedi nisu članovi: njihovo nezavisno pomjeranje može prekinuti vezu sa
unutrašnje validnim sklopom, što STATUS pokazuje odvojeno.

Ove tvrdnje opisuju implementiranu namjeru/putanje; stvarni COPY/Undo/Redo i
konture ovog integracionog paketa još zahtijevaju praktičnu AutoCAD provjeru.

## Dodatne granice i proba rollback-a

- TVS_OFFSET_FAIL_NEXT uključuje jednokratni namjerni kvar nakon upisa sva tri
  zapisa, a prije skraćivanja dolaznog kanala. Sljedeći inače izvodljiv Z mora
  biti potpuno poništen: bez novih objekata/zapisa, pomjeranja kraja i promjene
  kote. Nastaviti običnim crtanjem. Ako Z bude otkazan/odbijen ranije, kvar ostaje
  pripremljen za sljedeći izvodljivi pokušaj.
- COPY između crteža sa različitim faktorima jedinica nije podržan. Kopija dobija
  rezervisanu nepodržanu verziju -1 i blokiran obračun; nema tihog skaliranja rise.
  Za ovaj paket testirati COPY unutar istog DWG-a. WBLOCK/INSERT i vanjski API clone
  tokovi nisu potvrđeni praktičnim testovima.
- Prva verzija ne dopušta MOVE/ROTATE/SCALE/MIRROR originalne trojke ni pojedinih
  članova. COPY pozicioniranje je dozvoljeno samo translacijom novih kopija u
  njihovom clone komandnom ciklusu. Susjede po potrebi ponovo nacrtati.
- Zaštite pokrivaju postojeće TVS settere, gripove i transform API; nisu garancija
  protiv proizvoljnog dodatka koji direktno prepisuje javna C++ polja ili Xrecord.
  Naknadna validacija odbija geometrijski nesaglasne podatke.
- Globalna kota nakon Undo/Redo usklađuje se u tekućoj sesiji. Z uvijek ponovo čita
  stvarni aktivni kanal. Višedokumentni tok, zamjena aktivnog DWG-a i nastavak nakon
  ponovnog učitavanja dodatka zahtijevaju posebnu praktičnu provjeru.
- Provjera integriteta pretražuje vlasnički Model prostor radi kolizije GUID-a;
  performanse velikih mreža još nisu mjerene. Postojeće tabele treba ponovo generisati.
