# B — minimalni podaci sklopa etažiranja (PREDLOG, NIJE IMPLEMENTIRANO)

## Identitet, članovi i mjesto čuvanja

Tri člana: FirstElbow, SlopedStraight, SecondElbow. Ulazni i izlazni horizontalni
kanal su samo susjedi, ne članovi sklopa. Njihove reference u prvoj šemi se ne čuvaju;
provjera povezivanja je geometrijska, bez novog univerzalnog konektorskog sistema.

Predlog: XRecord TVS_OFFSET_MEMBER u extension dictionary SVAKOG od tri člana:

- schemaVersion=1;
- assemblyGuid: jedinstven za jedan sklop u bazi;
- role: FirstElbow / SlopedStraight / SecondElbow;
- tri referentna ObjectId-a u fiksnom redoslijedu uloga, kao soft-pointer podaci
  koji moraju proći ID remapping pri kloniranju, ne stringovani handle-i.

SlopedStraight dodatno nosi XRecord TVS_SLOPE (verzija 1, riseStraightMm).
Fizička dužina se izvodi iz XY projekcije i riseStraight, ne čuva se duplikat L.
Referentne kote horizontalnih priključaka ostaju postojeće Elev na dva koljena.
Sklop ne dobija posebnog DBX entiteta niti globalni NOD registar kao izvor istine.
Sva tri member zapisa moraju imati saglasne reference i GUID. Nije dovoljno naći
samo tri ista GUID stringa: obavezna je provjera recipročnih ObjectId referenci.

To ne mijenja postojeći filer raspored, ali jeste nova semantika DWG podataka.
Ne uvodi se dok korisnik zasebno ne odobri šemu i migracionu/kompatibilnosnu politiku.

## COPY i kloniranje

- Kopija sva tri člana: poslije završenog clone ID mapping-a prepoznati kompletnu
  novu trojku; dodijeliti NOV assemblyGuid i reference ka KOPIJAMA. riseStraight
  ostaje fizička visinska razlika u mm. Original se ne mijenja.
- Kopija samo dijela: kopija NE smije zadržati operativne veze ka originalnim
  članovima. Kopiranim zapisima dati novi identitet i nulirati reference ka
  nekopiranim članovima; označavati izvedenim statusom Incomplete. Nagibni komad
  čuva riseStraight i fizičku dužinu uz upozorenje da sklop nije kompletan.
- Završni clone adapter mora raditi u clone kontekstu, ne pogađati šta je kopirano
  tokom pojedinačnog callback-a. Mehanizam ObjectARX mapiranja treba potvrditi prije
  izbora konačnih hookova. COPY/clipboard/WBLOCK/INSERT imaju zasebne testove.
- Kopiranje između različitih jedinica ne obećavati prije provjere konverzije
  geometrije i referenci. riseStraightMm je fizički mm, ne model-jedinica.
- Ako clone završna obrada nije moguća, ne proglasiti kopiju validnim sklopom.
  Provjera mora otkriti reference na članove čiji povratni zapisi ne odgovaraju.

## Brisanje i nedostajanje člana

Brisanje sva tri člana u jednoj komandi dozvoliti kao atomsku operaciju.
Brisanje jednog člana ne briše ostatak automatski: preostali sklop postaje
Incomplete, odbija sklopno editovanje i prikazuje upozorenje. Undo brisanja treba
da vrati integritet bez regenerisanja GUID-a. Dok je referenca na erased objekat
poznata, ne prepisivati je u null samo zbog privremenog brisanja; to omogućava Undo.
Trajno nedostajući ili pogrešnog tipa član je Broken. Nema automatskog spajanja,
dodavanja članova ili popravke njihove geometrije.

Pojedinačni preostali objekti mogu se obračunati uz izvještaj o nepotpunom sklopu
ako su njihovi vlastiti podaci validni; ne koristiti pogrešan rise ili zbir cijelog
sklopa za njih. Neispravan TVS_SLOPE mora blokirati fizički obračun tog komada.

## Provjera nakon otvaranja

Read-only provjera verzije, GUID-a, role i tipa entiteta, tri različita člana,
iste baze, recipročnih referenci i TVS_SLOPE zapisa. Provjeriti usklađenost presjeka,
Ri i uglova, forme para, kota i odgovarajućih projektovanih spojeva prema prihvaćenom
adapteru i geometrijskom rezultatu. Tolerancije moraju biti eksplicitne.
Izvesti status Valid / Incomplete / Broken / UnsupportedVersion / GeometryMismatch.
Ne mijenjati podatke ili grafiku samo zbog otvaranja ili REGEN-a.
Duplirani GUID dvije disjunktne trojke je identitetski konflikt za izvještaj,
ne razlog da se članovi pomiješaju. Za veliku bazu koristiti sesijski indeks koji
se može odbaciti, nikada kao zamjenu za podatke DWG-a.

## Granice editovanja prve verzije

Prvo odobriti samo kreiranje, čitanje/obračun, kontrolisani COPY i brisanje.
Nema slobodnih gripova, STRETCH-a, promjene presjeka, Ri, ugla, kota ili krajeva
pojedinačnog člana. Dok se ne odobri transakcioni editor cijelog sklopa, te operacije
odbijati na označenim članovima. Ne praviti par ispravne nove slike i starog rise.

Ni MOVE/ROTATE cijelog sklopa nisu unaprijed obećani: sačuvati ili provjeriti veze
ka susjedima prije podrške. Za početnu zaštićenu verziju blokirati geometrijske
transformacije članova; COPY treba testirati da završi sa ispravnim novim podacima.
Uniformni SCALE, MIRROR, neuniformne i prostorne transformacije ostaju odbijene.
Ove zabrane su plan buduće integracije, ne ponašanje već postojećeg TVS-a.

## Potrebne datoteke u eventualnoj odobrenoj integraciji

- Novi DBX helper OffsetAssemblyData.h/.cpp: zapis, čitanje, validacija referenci
  i integriteta. Novi SlopeData.h/.cpp ili zajednički jasno izdvojen helper za rise.
- TVS_Entity.h/.cpp: zajedničke zaštite za označene članove i odgovarajući clone/
  transaction hookovi nakon provjere stvarnog SDK ponašanja.
- TVS_Pipe.h/.cpp i TVS_TAP.h/.cpp: zaštita settera/gripova/transformacija i podrška
  kontrolisanom internom upisu; razdvajanje fizičke i projektovane dužine kanala.
- acrxEntryPoint.cpp: tek odobrena integracija changeZ i provjera Cancel/NoChange,
  atomsko kreiranje tri člana i povezivanje, stanje nastavka crtanja poslije commit-a.
- DBX/ARX projektne datoteke: uključivanje novih helpera.
- ComPipe.cpp, TVSPropertyController.cpp, SPEC.cpp i količinski/maseni potrošači:
  tek zasebna odobrena semantika fizičke dužine; audit prema INTEGRATION_PLAN.md.
- Novi AutoCAD integracioni testovi za clone, partial clone, erased/reopened član,
  zabrane editovanja i fizički obračun. Trenutni izolovani testovi nisu taj dokaz.

Ne mijenjati postojeće objekte bez eksplicitne komande migracije. Nema migracije u
ovom predlogu. Soft-pointer reference, clone završna obrada i guardovi moraju biti
provjereni u 2020/2022 prije tvrdnje da je sklop zaštićen u stvarnom AutoCAD-u.
