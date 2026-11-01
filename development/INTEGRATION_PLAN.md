# Zajednička integracija offset / bypass — predlog

Datum: 01.11.2026. Status: **samo plan; implementacija i nove DWG šeme nisu započete**.
Pregledani HEAD: `bd01b2e208a5ef58e2905ce9854d8a63dac75382`, grana
`razvoj-bypass-candidate`. Izvršni r3 izvor: `ee85e72cce0e805991d172a2c34cdc4306530cb3`.
Dokumentacioni commit koji dodaje ovaj plan nasljeđuje taj HEAD; ne mijenja izvršni kod.
PR #1 ostaje draft, produkcija netaknuta.

Ovaj plan je aktuelni redosljed narednog rada. Raniji `geometry/INTEGRATION_PLAN.md`
ostaje istorijski predlog prve offset integracije: neke njegove stavke su sada
implementirane. Stvarno današnje stanje je u [CODE_STATE.md](CODE_STATE.md), a
otvorene provjere u [INTEGRATION_BACKLOG.md](INTEGRATION_BACKLOG.md).

## 1. Granice i redosljed

Undo/Redo i kompatibilnost startup dodataka ostaju otvorena završna stabilizacija.
Ne blokiraju naredno razvojno planiranje. U ovoj isporuci nema novih Redo proba,
pregleda tog problema, izmjena reaktora, novih funkcija niti upisa novih šema.
Postojeće transakcije, validacija i rollback ostaju obavezni.

Razlikovati dva zahtjeva:

- **Offset / promjena kote:** dva koljena i kosi međukomad; izlaz ostaje na novoj
  fizičkoj koti, sa horizontalnim ulazom/izlazom.
- **Bypass / obilazak:** četiri koljena, dva kosa i srednji horizontalni komad;
  izlaz se vraća na početnu fizičku kotu. Koristi postojeći checked kandidat.

Nisu dva naziva za istu operaciju. `TVS_OFFSET` i `TVS_BYPASS` su predloženi nazivi,
ne danas registrovane komande. `Z` ostaje postojeći ulaz tokom crtanja;
predložena opcija `Obilazak` poziva isti bypass servis kao samostalna komanda.

Prvo razdvojiti postojeći offset servis od ulaznog toka i utvrditi ugovor granica
izmjene (faza 0). Zatim trajni pravougaoni bypass samostalno (1), isti servis u
crtanju (2), oznaka/detalj (3), kružne kombinacije (4), završna stabilizacija (5).
Ova mala prethodna faza je potrebna zato što današnji `execute(TVS_Pipe*&)`
pretpostavlja aktivnu trasu i pravi novi izlazni nastavak; nije bezbjedan opšti
editor bilo kog već povezanog kanala.

## 2. Jedan servis, dva načina pokretanja

Predloženi novi moduli ispod su projektovana mjesta odgovornosti, ne postojeći API.
Ne uvoditi paralelnu geometriju za samostalnu komandu i crtanje.

1. **Snapshot / reference / jedinice.** Predloženi
   `TVS_Ventilation_ARX/RoutingSnapshot.h`: aktivni dokument i owner, izabrani
   objekti i stvarne veličine, fizička kota, presjek, izvedba, WCS plan-ravan,
   jedinice iz `DrawingUnits.h`, orijentacija aktivnog kraja, susjedi/konektori,
   atributi, dozvoljeni prozor, eksplicitne izolacije i numerička politika.
   Snapshot nosi otisak pročitanih vrijednosti, ne samo handle. Razdvojiti trajnu
   reviziju izvora od sesijskog broja revizije UI rezultata.
2. **Proračun / provjera.** `geometry/OffsetGeometry.h`, `OffsetAdapter.h`,
   `geometry/bypass/Candidate.h`, `Generator.h` i `BypassNumericBridge.cpp`
   ostaju zajedničko jezgro. Predloženi `RoutingPlan` nosi potvrđene spojne tačke,
   sve članove, fizičke dužine i provjereni kolizioni rezultat. Adapter za DBX ne
   računa drugi oblik. Ne pretraživati ponovo pri upisu ili izradi detalja.
   Strict FP most i ograničenja budžeta ostaju; `NoCandidateFoundWithinBudget`
   nije dokaz nemogućnosti, a `Found` nije dokaz globalnog minimuma.
3. **Dijalog / preview.** Izdvojiti ponovo upotrebljiv kontroler iz
   `BypassPreviewCommand.cpp`; offset koristi `dgChangeZ` i isti životni ciklus.
   Promjena fizičkog ulaza poništava rezultat. Kontrole prikaza ne računaju novu
   geometriju. Samo transient prikaz prije potvrde, bez privremenih DWG entiteta.
4. **Transakcioni upis.** Predloženi `RoutingWriteService.h` i
   `BypassIntegrationCommand.h`: ponovo pročitati i porediti snapshot, jedinice,
   vlasnika, zaključanost i granice; zatim jednom transakcijom izmijeniti samo
   odobreni obuhvat. Zadržati postojeći offset `BuildTransaction` i detail
   rollback ugovor. Novi zahtjev ne opravdava slabljenje postojećih zaštita.
   Ishodi: Applied / NoChange / Cancelled / Failed; nijedan neuspjeh ne mijenja
   aktivnu trasu niti ostavlja stilove, zapise ili članove.
5. **Fizički obračun.** Zajednički čitač podataka člana, pa
   `TVS_Pipe::get_Length/getCount`, `TVS_Entity::getSheetMetal`, `ComPipe`,
   `SPEC.cpp`, Properties, MLeader i `KGTEZ.lsp`. Za svaki kosi komad čuvati njegov
   vlastiti rise: L = hypot(P, rise), ne ukupnu promjenu kote. Koljena obračunavati
   po stvarnoj izvedbi, presjeku i radijusu. Plan-prikaz koristi P, količine L.
   Dokumentacioni blokovi i oznake ne postaju dodatne fizičke TVS količine.
6. **Oznake / detalji.** Predloženi `RoutingDetailService.h` koristi isti committed
   snapshot i postojeće `geometry/bypass/SideDetail.h` / `BypassSideDetail.h`.
   Bez računanja geometrije iz tekstualne oznake ili približne polilinije.

Samostalna komanda samo bira postojeći kanal, kraj/prozor i prepreku. Adapter
crtanja daje aktivni kanal i kontekst; ne simulira drugu komandnu petlju niti
kopira solver ili writer. Oba vraćaju isti rezultat sa identitetima upisanih
članova. Samo adapter crtanja poslije uspješnog commita postavlja aktivni izlaz,
A1/A2/pt1 i fizičku kotu; pri otkazu vraća prethodni kontekst. Na ponovnom ulasku
čita stvarni objekt, ne vjeruje zastarjelim globalnim vrijednostima.

## 3. Očuvanje spoljašnjih priključaka i podataka

### Samostalni offset

Prva verzija: samo nedvosmislen **slobodan izlazni kraj** običnog krutog kanala.
Ulazni spoljašnji priključak ostaje na istoj WCS tački, fizičkoj koti, smjeru,
presjeku i identitetu veze. Offset završava na zadatoj novoj koti i daje izlazni
nastavak. Postojeći priključeni izlaz na staroj koti ne može se očuvati i ujedno
završiti na novoj koti bez dodatne izmjene: takav slučaj odbiti, bez pomjeranja
nizvodne mreže. Podrška za njega traži zasebno definisan i odobren obuhvat.
Omogućiti izbor Prvi/Posljednji kraja; obrnuti smjer je dio istog adaptera.

### Samostalni bypass

Izabrati ograničeni pravolinijski interval unutar jednog kanala. Ulazni i izlazni
spoljašnji port ostaju na prvobitnim mjestima i početnoj fizičkoj koti, sa istim
pravcem/presjekom i recipročnim vezama. Zadržati originalni objekt kao jedan
spoljašnji ostatak gdje je moguće; drugi ostatak dobija nov identitet. Izlaznog
susjeda prevezati samo sa zamijenjenog kraja na novi ostatak u istoj transakciji,
bez njegove geometrijske izmjene. To je jedina dozvoljena izmjena van zamjenjivanog
kanala u prvom obimu: tačno evidentirana recipročna konektorska referenca.
Prepreka je uvijek read-only.

Prije upisa utvrditi `TVS_Connector` point, connectionID, cTypeCurrent/cTypeAnother
na oba kraja i recipročne podatke susjeda. Sačuvati orijentaciju i tok; poslije
upisa provjeriti kontinuitet položaja, fizičke kote, tangentnog smjera i presjeka
na svakom novom spoju. Članstvo u sklopu nije zamjena za konektorsku topologiju.
Račvu unutar prozora, dvosmislene ili pokvarene veze, postojeće članove sklopa,
zaključane susjede, XREF/blok, nedovoljnu dužinu ostataka ili konflikt oznaka odbiti
prije izmjene. Nema pretrage i pomjeranja cijele nizvodne mreže.

Za oba moda napraviti eksplicitnu politiku prenosa: layer/boja/linetype/lineweight,
Flow, Grani, materijal i korisnička svojstva. Današnji `appearance()` kopira samo
ograničeni skup; nije dokaz očuvanja svih atributa. Originalnom ostatku sačuvati
postojeći identitet i povezane anotacije; nove fizičke članove stvarati s novim
identitetima. Prenos dijeljenih inženjerskih svojstava dozvoljen, kopiranje
jedinstvenih oznaka/identiteta i proizvoljnih XData nije. Atribut koji ne može
jednoznačno preživjeti podjelu kanala prijaviti i odbiti dok politika nije utvrđena.
MLeader veze prema originalu ne prebacivati tiho na proizvoljni novi član.

## 4. Trajni podaci — koncept za zasebnu implementacionu reviziju

Postojeći `TVS_OFFSET_MEMBER` v1 ostaje nepromijenjen: tri člana, GUID, uloga,
recipročne reference i rise. Nije sedmočlani bypass ugovor. Ne upisivati dva
nezavisna offset sklopa i predstavljati ih kao jedan kompletno validiran obilazak.

Predlog: poseban verzionisan bypass ugovor (tačan ključ i DXF raspored tek pri
implementacionom pregledu), sa sljedećim semantičkim podacima:

- AssemblyId, tip operacije, revizija sklopa; sedam stabilnih uloga i reference
  svih članova; odvojene reference spoljašnjih ostataka i konektora.
- Tip presjeka i stvarne dimenzije, izvedba koljena, Ri/ugao, jedinice i frame,
  početna/izlazna fizička kota; potpisani rise svakog kosog komada.
- Potvrđeni parametri i geometrija kandidata, politika provjere, izolacije,
  clearance/rezerva, prozor; read-only referenca/otisak prepreke u trenutku provjere.
- Statusi Valid / Incomplete / Corrupt / UnsupportedVersion / GeometryMismatch;
  promjena prepreke može zastarjeti provjeru razmaka iako je sklop interno validan.

DBX čitač fizičke dužine i zaštita editovanja razrješavaju legacy, offset v1 i novi
bypass zapis preko zajedničkog interfejsa. Nikad tihi horizontalni fallback za
oštećen ili nepoznat novi zapis. Filer redoslijed starih entiteta i stari crteži
ostaju bez automatske migracije. Nepodržana stara verzija TVS-a ne smije se
predstavljati kao tačan obračun nove šeme; DEV paketi ostaju upareni.

COPY/WBLOCK/INSERT traže eksplicitnu politiku: puna kopija novog trajnog sklopa
dobija novi AssemblyId i unutrašnje reference kopija; parcijalna kopija je
Incomplete i ne povezuje se operativno s originalom. Promjena jedinica između
baza ostaje odbijena dok poseban import adapter nije potvrđen. Ovo su budući
kriterijumi, ne tvrdnja da današnji bypass to već podržava.

## 5. Sklop → oznaka OB-xx → standardni kotirani detalj

AssemblyId i DetailId su različiti interni identiteti. Vidljiva oznaka nije ključ.
Predlog: sklop posjeduje jedinstvenu vidljivu oznaku u dokumentu; odvojeni standardni
MTEXT/MLeader prikazuje OB-xx. Povezani detalj prikazuje istu oznaku i ima vlastiti
DetailId, SourceAssemblyId i lokalnu objekt-referencu uz identitet dokumenta,
SourceRevision, otisak/snapshot i verziju writer-a. Jedna sklopna oznaka i njen
detalj predstavljaju namjerno dijeljenje oznake, ne današnji zabranjeni duplikat
dva nezavisna statička detalja. Inventory mora razlikovati ove uloge.

Prva verzija nudi **ručno osvježavanje**. Status pri eksplicitnoj provjeri ili
otvaranju dijaloga: Current, Stale, MissingSource, AmbiguousSource, StaticUnlinked
ili Unsupported. Stale se izvodi poređenjem revizije i relevantnog otiska; time
se otkriva i izmjena koja nije prošla kroz budući editor. Read-only provjera
ne upisuje oznaku statusa u DWG i ne popravlja podatke iz callback-a.

Osvježavanje ponovo čita stvarni sklop, validira članove i razmak prema važećoj
prepreci. Geometrija se ne pretražuje niti trajni sklop mijenja zbog obnove
dokumentacije. Nevažeći sklop ili promijenjena prepreka bez potvrđene provjere
onemogućavaju status Current. Novi snapshot dokumenta i zamjena geometrije/kota
čine jednu transakciju, čuvaju DetailId, tačku umetanja i izbor anotacija; greška
ostavlja prethodni detalj netaknut i zastario. Kod prvog povezivanja predložiti
jedan aktivni detalj po sklopu; više pogleda ostaviti za zaseban zahtjev.

Postojeći `TVS_BYPASS_SIDE_DETAIL_V1` i STATIC_SNAPSHOT ostaju samostalni.
Ne povezivati ih po OB-xx, bliskoj geometriji ili informativnom handle-u. Eventualna
buduća eksplicitna konverzija traži izbor izvora i zasebnu potvrdu; nije prva faza.
Nova veza zahtijeva zaseban verzionisan ugovor, a ne reinterpretaciju starih polja.
Standardni entiteti moraju ostati vidljivi i kotirani bez TVS-a. Bez TVS-a nema
provjere svježine niti automatskog osvježavanja.

Postojeći COPY duplirani identitet statičkog detalja ostaje dokumentovano
ograničenje. Prva povezana verzija ne obećava automatsko popravljanje kopiranog
DetailId: konflikt se prijavljuje i osvježavanje blokira do eksplicitnog rješenja.
Oznaka bez detalja ne smije blokirati geometrijsku operaciju: neuspjelo naknadno
pravljenje dokumentacije ne vraća već zasebno potvrđen sklop, već jasno prijavljuje
da detalj nije napravljen. Svaki pojedinačni upis ostaje atomski.

## 6. Matrica presjeka i izvedbe koljena

- **Pravougaoni kanal / pravougaona prepreka:** danas postoje checker, bounded
  generator, dijalog, preview i statički detalj. Adapter traži obične krute
  horizontalne kanale, međusobno upravne ose i pun poprečni obuhvat. Trajni upis
  sedam TVS članova, spoljna topologija i veza sa detaljem tek se planiraju.
- **Kružni kanal / pravougaona prepreka:** zahtijeva eksplicitan Circular(D),
  tijela kružnih pravih komada i glatkih koljena, checker, adapter, količine,
  prikaz i detalj. Postojeće legacy kružno crtanje nije ovaj integrisani bypass.
- **Pravougaoni kanal / kružna prepreka:** zahtijeva stvarni model konačnog
  cilindričnog tijela prepreke, njegov položaj/dužinu/izolaciju, provjeru rastojanja
  i drugačiji prikaz prepreke. Današnji pravougaoni Region nije ta podrška.
- **Kružni kanal / kružna prepreka:** zahtijeva oba proširenja i njihove međusobne
  kolizije, ne samo kombinovanje oznaka u dijalogu.

Presjek je zaseban podatak: Rectangular(A,B) ili Circular(D). A=B ostaje kvadrat,
ne kružni presjek. Glatko ili segmentno koljeno je druga osa izbora. Prvo kružno
proširenje podržava samo **glatka kružna koljena** s razriješenim radijusom osi
R=Ri+D/2 i odgovarajućom debljinom/izolacijom. Segmentna, fleksibilna, redukcije
i promjene presjeka nisu uključene. Legacy TAP ima TapRound/TapSection i različite
režime radijusa; njihovo postojanje nije dokaz ispravnog novog 3D modela.

Kružna prepreka je konačni zatvoreni cilindar: osa, krajnje tačke, spoljašnji
poluprečnik i izolaciona ovojnica. Računati razmak od stvarnog tijela, uključujući
krajeve; ne svesti automatski prečnik na kvadrat. Za upravnu horizontalnu osu
prepreke duž lokalnog t, presjek u s/z je disk. Svođenje na 2D može biti tačno
samo uz dokaz da je relevantni dio prepreke translaciono nepromjenljiv duž cijelog
potrebnog t-opsega trase i najbližih tačaka, da krajevi ne utiču i da geometrija
trase/izolacije i simetrija daju istu 3D minimalnu udaljenost. Za kružnu trasu
posebno dokazati uslov za swept kružni presjek i torusne lukove; 2D spoljašnja
silueta sama nije takav dokaz. Kose/neupravne ose, djelimičan poprečni obuhvat i
blizina kraja zahtijevaju 3D checker ili eksplicitno odbijanje.

Ako se radi ranog korisnog obima ponudi obuhvatna kutija cilindra, mora biti
označena **konzervativna aproksimacija**. Prolaz oko kutije može dati konzervativno
prihvatanje uz dokaz uključenja izolovanog tijela; odbijanje ne dokazuje da nema
prolaza oko cilindra, niti rastojanje do kutije predstavlja stvarni minimalni
razmak. Ne proglasiti tim korakom podršku za stvarnu kružnu prepreku.

## 7. Male pregledne faze i kriterijumi

### Faza 0 — zajednički ugovor i samostalni pravougaoni offset

Moduli: postojeći `OffsetIntegrationCommand.h`, `OffsetSessionState.h`,
`acrxEntryPoint.cpp`, `DrawingUnits.h`, `OffsetAssemblyData.{h,cpp}`, `TVS_Connector`,
predloženi `RoutingSnapshot.h`, `RoutingWriteService.h`; `tests/test_offset_*`.
Podaci: postojeći offset v1, snapshot portova/svojstava i rezultat adaptera;
ne mijenjati v1 da bi se samo dodao novi način pokretanja.
Zavisnost: saglasnost o slobodnom izlaznom kraju; razdvajanje UI/konteksta od write.
Testovi: isti ulazi iz standalone i Z daju iste spojeve/rise/količine; Prvi i
Posljednji, mm/cm/m, rotiran UCS, A!=B, oba smjera promjene kote, 30/45/60;
Cancelled/NoChange/odbijanje bez promjene; postojeći fail-next rollback; očuvan
ulazni konektor, atributi i pravilna izlazna kota. Za povezani izlaz potpuno
odbijanje bez pomjeranja susjeda. Referentni fizički obračun iz CODE_STATE.
Prihvatanje: jedan servis i iste numeričke vrijednosti kroz oba ulaza; nema
regresije postojećeg Z. Ograničenja: kruti pravougaoni, pozitivan Ri, bez 90°,
bez mrežnog premještanja i nepodržanih atributa. Redo stabilizacija ostaje u fazi 5.

### Faza 1 — samostalni trajni pravougaoni bypass

Moduli: `BypassPreviewCommand.cpp`, `BypassNumericBridge.cpp`, postojeći
`Candidate.h/Generator.h/PreviewModel.h`, predloženi `BypassIntegrationCommand.h`,
`RoutingWriteService.h`, `TVS_Ventilation_DBX/BypassAssemblyData.{h,cpp}`,
`TVS_Pipe.cpp`, `TVS_TAP.cpp`, `TVS_Entity.cpp`, `ComPipe.cpp`, `SPEC.cpp`.
Nova registracija ide u `acrxEntryPoint.cpp`, projekti i DEV workflow-i samo kada
implementacija bude odobrena. Solver/generator se ne prepisuju radi trajnog upisa.
Podaci: odobren novi ugovor sedam članova, konektori, lokalni rise, fizički
snapshot kandidata i provjere. Zavisnosti: faza 0 i pregled koncepta nove šeme.
Testovi: svi spojevi, četiri luka i tangente prema istom checked Result-u;
oba krajnja porta i kota ostaju isti; prepreka i nedozvoljeni susjedi netaknuti;
fizičke količine sedam članova bez dupliranja; read-only save/reopen validacija;
fail-injection poslije kreiranja, metapodataka i prevezivanja portova vrati sve
članove i susjede. Negativni testovi: stale snapshot, zaključan sloj/susjed,
račva u prozoru, premali ostatak, nevažeći/strani zapis i nepotpun sklop.
Prihvatanje: jedna atomska izmjena u odobrenom prozoru, validnih sedam članova,
očuvana spoljna topologija i atributi; jasno prijavljen odbijen slučaj.
Ograničenja: samo sadašnji upravni puni pravougaoni obuhvat, bez migracije,
asocijativnog detalja, kružnih presjeka i automatskog pomjeranja mreže.

### Faza 2 — isti bypass tokom crtanja

Moduli: oba mjesta opcija u `acrxEntryPoint.cpp` (`TVS_PIPE` i `drawStep3` za
`TVS_DRAW`), predloženi `RoutingDrawingAdapter.h`, isti servisi faze 0/1.
Podaci: aktivni objekt/kraj, smjer, trenutna kota, snapshot konteksta i izlazni
objekt iz commita. Zavisnost: faza 1; nema nove DWG šeme samo zbog načina pokretanja.
Testovi: isti kandidat i količine standalone/crtanje; nakon bypass-a nastavak
na početnoj koti, nakon offset-a na novoj; kontinuitet prvog narednog segmenta;
Cancel na izboru prepreke, u dijalogu i pri potvrdi, NoChange, stale izbor i
greška upisa ne mijenjaju aktivni kraj ili globalne postavke; više dokumenata.
Prihvatanje: bez kopiranog solver-a/writer-a i bez izgubljene trase; opcija
Obilazak je vidljiva samo gdje su ispunjeni uslovi. `Gore/Nadolje` ne reinterpretirati
tiho kao novu funkciju. Ograničenja geometrije ista kao faza 1.

### Faza 3 — oznaka i povezani kotirani detalj

Moduli: `BypassSideDetail.h`, `geometry/bypass/SideDetail.h`,
`BypassPreviewCommand.cpp`, `MleaderController.{h,cpp}`, predloženi
`RoutingDetailService.h` i inventory za sklopne oznake/povezane detalje.
Podaci: odvojeni AssemblyId/DetailId, OB-xx, source reference, revizija/otisak,
snapshot, writer verzija i anotacije. Zavisnost: stabilan upis/čitanje faze 1;
može poslije faze 2 da se ne šire paralelno korisnički tokovi.
Testovi: sklop/oznaka/detalj se pravilno razrješavaju; ručno osvježavanje,
zastarjelost poslije promjene izvora/prepreke; Missing/Ambiguous; duplirani COPY
identitet blokira osvježavanje; rollback zamjene detalja; mm/cm/m i DIMLFAC;
otvaranje standardnog detalja bez TVS-a; stari STATIC_SNAPSHOT ostaje nepovezan;
specifikacija i masa ne broje dokumentaciju.
Prihvatanje: jedan aktivni povezani detalj po sklopu, status bez callback upisa,
ručno osvježavanje čuva DetailId/položaj i nikad ne mijenja sklop. Ograničenja:
bez automatskog osvježavanja ili uparivanja starih detalja; COPY ograničenje jasno.

### Faza 4 — kružne kombinacije, u tri manje isporuke

4a Circular/Rectangular i zajednički glatki kružni offset;
4b Rectangular/Circular sa provjerenim cilindrom; 4c Circular/Circular.
Redosljed prvo uvodi jednu novu vrstu tijela po isporuci, pa njihovu kombinaciju.
Moduli: `geometry/bypass/Candidate.h`, `Generator.h`, `PreviewModel.h`,
`SideDetail.h`, `OffsetGeometry.h/OffsetAdapter.h` ili njihovi zajednički
shape-aware adapteri, `BypassNumericBridge.cpp`, DBX TAP/Pipe/SheetMetal i
AssemblyData čitači. Ne kopirati pravougaoni solver radi novog ulaznog toka.
Podaci: označen tip presjeka, D, glatka izvedba, Ri/R, cilindrična osa/krajevi,
izolacije; verzionisanje novih varijanti tek nakon pregleda.
Zavisnosti: faze 1–3, dokaz geometrijskog modela i razdvojena obračunska politika.
Testovi: nezavisni analitički cilindar/torus orakuli, tangenta/dodir/preklop,
krajnje kape, izolacije, odbijanje neupravnog/krajnjeg slučaja izvan 2D domena,
rotacije/jedinice, poređenje sa nezavisnim 3D referentnim proračunom, svi fizički
spojevi/količine, statički i povezani detalj. A=B i Circular(D) moraju dati
različita tijela/površine. Regresija pravougaone matrice obavezna.
Prihvatanje: posebno objavljena podrška svake od četiri kombinacije, bez tvrdnje
da konzervativna kutija predstavlja stvarni cilindar. Ograničenja: bez segmentnih,
fleksibilnih, redukcija i proizvoljnih prostornih obilazaka.

### Faza 5 — završna stabilizacija prije produkcionog prihvatanja

Moduli: prethodni writer-i, podaci, potrošači količina, testovi i razvojni
izvještaji; reaktore dirati samo ako tada izolovan uzrok opravda ciljanu ispravku.
Podaci: tačan build commit i inventar modula za svaki runtime izvještaj.
Zavisnost: odabrani funkcionalni obim prethodnih faza. Testovi: preostala checklist-a
prethodne isporuke, odloženi kontrolisani Undo/Redo/startup slučajevi, snimanje i
ponovno otvaranje, veze/copy/atributi, veliki crteži i svi fizički potrošači.
AutoCAD 2020 i 2022 runtime potvrđivati zasebno; raniji proof nije nova potvrda.
Prihvatanje: zatvorena obavezna checklist-a, dokumentovana kompatibilnost i
ograničenja, zaseban korisnički pregled i izričito produkciono odobrenje.
Nema hot unload/reload; svaka nova binarna verzija poslije restartovanja.

## 8. Odluke za potvrdu prije implementacije

1. **Obuhvat prve samostalne izmjene:** offset samo na slobodnom izlaznom kraju;
   bypass unutar jednog običnog kanala, uz očuvanje oba spoljašnja porta i samo
   nužno prevezivanje recipročne konektorske reference izlaznog susjeda.
   Povezani offset izlaz, račve u prozoru i nejasni atributi se odbijaju.
2. **Prva veza dokumentacije:** jedan aktivni povezani detalj po sklopu, ručno
   osvježavanje i status; postojeći statički detalji ostaju samostalni.

Nazivi `TVS_OFFSET` / `TVS_BYPASS` su radni predlog; ne traže odvojenu odluku sada.
Glatka kružna koljena, odloženi Redo i netaknuta produkcija već su zadati obim.
Tačan raspored nove DWG šeme biće konkretna zasebna revizija unutar odobrene faze,
ne neizrečena odluka u ovom planu. Završetak ove isporuke nije početak implementacije.
