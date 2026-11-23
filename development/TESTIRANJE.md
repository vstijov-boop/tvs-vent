# TVS_VENT – period testiranja (objedinjeno na master)

## TVS_OPREMA – terminali po baznoj tački, koljeno dolje, rešetka na dnu kanala (novo, 23.11.) – testirati
Opis: [OPREMA.md](OPREMA.md) (Pravila za terminale). Crtež sa primjerima korisnika (rešetka na kraju, bočna, na dnu kanala, difuzor na flexu, ventil ispod koljena dolje).
- Kanal 250 → prelaz 250→450 → kanal 60 mm → blok rešetke (rot 90) sa baznom tačkom na kraju nastavka → `TVS_OPREMA` → povezan terminal, nastavak NIJE skraćen, priključak na kraju nastavka (Svojstva: 1 priključak, 450×…). Ranije: „kanal je cijeli ispod bloka“.
- Bočna rešetka: T komad sa granom na stranu → nastavak ~63 mm → rešetka (bazna tačka na kraju) → isto kao gore.
- Difuzor sa plenumom na kraju flexa → radi kao ranije (bez promjene).
- Kanal → koljeno DOLJE na kraju → blok ventila (LVS-100) sa baznom tačkom u tački vertikalnog izlaza → povezan; kota priključka = kota izlaza (ispod ose kanala, TVS_PROVJERA / TVS_PAD da se vidi da je na mreži). Bazna tačka pomjerena 5 mm → odbijeno kao ranije.
- Rešetka (blok bez zapisa) preko kanala, centrirana na osi → pitanje „Blok … preko kanala: [Dno/Linija] <Linija>“: Dno → kanal NIJE presječen, ispis „rešetki na dnu kanala: 1“ i poruka o priključku iz gabarita (npr. 489×190 mm); drugi blok istog naziva → bez pitanja. Linija → kanal presječen (kao ranije), sljedeći blok tog naziva bez pitanja. ESC na pitanju → prekid, ništa za taj blok.
- Rešetka vrste „Rešetka“ (Primijeni na sve blokove ovog naziva) preko kanala → odmah na dno, priključak iz podrazumijevanog zapisa ako ga ima.
- Blokovi bez podešene vrste → na kraju ispis naziva i „Podesiti vrstu sada? [Da/Ne] <Da>“ → Enter otvara Svojstva opreme za te blokove; Ne → poruka o TVS_OPREMA_SVOJSTVA.
- Ponovno povezivanje (Da) rešetke na dnu → ostaje na dnu, priključak koji je imala ostaje.

## Oprema na mreži u proračunima (TVS_PAD, TVS_PROTOK, TVS_DPD_IZVOZ, TVS_DIM_PRATI, TVS_PROVJERA) – testirati
Opis: [OPREMA.md](OPREMA.md). Priprema: mreža (dovod) sa povezanim blokovima (TVS_OPREMA): regulator protoka (model „katalog“ Δp 30 Pa pri 1800 m³/h), prigušivač buke (fiksni Δp 25 Pa), na kraju kanala / fleksa difuzor (ζ 2,5), još jedna grana sa rešetkom bez protoka.
- **TVS_PROTOK**: protok difuzora iz opreme; rešetka bez protoka → zoom, „Rešetka – protok [m³/h]“ → upisuje se u blok (svojstva opreme), ne u kanal. Kanali iza regulatora / prigušivača dobijaju zbir kao do sada; inline oprema nema svoj protok. Kanal na kome je terminal ranije imao protok, a oprema nema → protok kanala prelazi u opremu. Oba upisana i različita → poruka „protok u opremi … na kanalu … uzet protok opreme“.
- **TVS_PAD**: regulator i prigušivač su u istoj dionici sa kanalima (ne prave novu dionicu, dužina bez njih). Fitinzi: redovi sa Izvor „Oprema“, naziv vrste, oznaka (RP-01 …) i model u opisu; ζ = Δp / pd dionice. Difuzor: red bez ζ, Δp u koloni Δp; u Dionice „Δp kraj. elem.“ te dionice = ta ćelija (nije žuta) – promjena Δp u Fitinzi mijenja Dionice. Šema: kraj dionice sa difuzorom nosi njegovu oznaku (D-01). „Za ventilator“: rezerva se ne računa na Δp krajnjih elemenata (provjera: rezerva 10 %, Δp kraj. elem. 20 Pa → za 20 Pa manje uvećanje nego ranije). Oprema bez modela gubitka: red žut, kraj. elem. žut kao ranije.
- **TVS_DPD_IZVOZ**: u DuctPressDrop-u (Uvezi sistem) oprema je na svojoj dionici kao oprema iz palete (simbol, oznaka RP-01 / PB-01 / D-01): regulator i prigušivač na svom mjestu duž dionice, difuzor na slobodnom kraju; ΔP: fiksni = isti, ζ = isti, „katalog“ = Δp pri izvezenom Q (poruka: u DuctPressDrop-u fiksan).
- **TVS_DIM_PRATI**: promijeniti dimenziju kanala ispred regulatora → dionica staje na regulatoru, oznaka „oprema“ u pregledu; kanal iza regulatora se ne mijenja.
- **TVS_PROVJERA**: pomjeriti kanal 3 mm od regulatora / promijeniti mu dimenziju / kotu / provući kanal ispod bloka → „oprema: priključak ne naliježe na kanal“ (sa rastojanjem ili „druge dimenzije ili kote“), zoom na priključak.
- Crtež u cm / m i blok kopiran iz crteža u mm: priključci se preračunavaju (mmPerUnit zapisa).
## Oprema na mreži: TVS_OPREMA_SVOJSTVA, specifikacija, meni (novo, 21.11.) – testirati
Opis: [OPREMA.md](OPREMA.md). Blokovi se prvo povežu sa TVS_OPREMA.
- `TVS_OPREMA_SVOJSTVA` → izabrati jedan povezan blok (ili ga izabrati prije komande) → dijalog „Svojstva opreme“: vrsta, oznaka, opis / tip, model pada pritiska, protok (samo krajnji element sa 1 priključkom), dolje priključci / dimenzija / dužina (samo za čitanje).
- Nepovezan blok → poruka „Blok nije povezan u mrežu (TVS_OPREMA).“; mješovit izbor → povezani se uređuju, za ostale poruka „preskočeno n“.
- „Nova“: sljedeća oznaka za izabranu vrstu u crtežu (PPK-01, PPK-02 postoje → PPK-03; RP, CAV, VAV, PB, R, D, KK, PZ, OP). Promijeniti vrstu pa „Nova“ → novi prefiks.
- Model: Bez / Fiksni Δp / ζ / Δp_n pri Q_n – aktivna su samo polja izabranog modela. Decimalni zarez i tačka (0,8 = 0.8). Negativno Δp ili ζ → poruka, dijalog ostaje otvoren; Δp_n pri Q_n sa Q_n = 0 → poruka. Slovo u polju broja → „Unesite broj“.
- Više blokova: polja koja se razlikuju pokazuju „Različito“ (vrsta: stavka „Različito“, model: nijedno dugme) i ostaju kakva jesu u svakom bloku dok ih ne promijenite; promijenjeno polje ide u sve izabrane.
- „Primijeni na sve blokove ovog naziva“: svi povezani blokovi tog naziva u crtežu dobijaju vrstu, opis, model i vrijednosti (oznaka i protok ostaju svoji); novi blok tog naziva povezan sa TVS_OPREMA treba da dobije ista svojstva. Nepovezani blokovi tog naziva se ne mijenjaju.
- Zaključan sloj jednog bloka → poruka, ništa se ne mijenja. Jedan U vraća sve izmjene blokova.
- TVS_SPEC / TVS_EXCELSPEC: povezan blok je u tabeli blokova kao oprema – Pozicija = oznaka, Naziv = opis / tip (ili vrsta kad opisa nema), Tip i dimenzije = 400x250 ili Ø250, kom; isti naziv + dimenzija + oznaka se sabiraju. Šifra / proizvođač / masa / napomena iz atributa bloka ako ih ima. Nepovezani blokovi sa atributima – kao do sada. TVS_BLCKMNGR: bez promjene (atributi).
- Meni i ribbon „Crtanje kanala“: dugmad „Oprema na mreži“, „Odveži opremu“, „Svojstva opreme“ (nove ikone) – pokreću TVS_OPREMA, TVS_OPREMA_ODVEZI, TVS_OPREMA_SVOJSTVA.

## Jedna radnja = jedan UNDO korak, sve ili ništa (prijedlog 3, 21.11.) – testirati
- Svojstva (TVS_CHANGE) na kanalu u dionici, dijalog dionice → Primijeni: **jedan U** vraća i izmjenu kanala i dionicu (ranije dva U: TVS_DIM_PRATI je bila posebna komanda). Poslije toga Enter / razmak ponavlja TVS_CHANGE, ne TVS_DIM_PRATI. Isto za TVS_CHANGE na jednom T komadu (dijalog TVS_TK_TIP, pa dionica iza kanala nove dimenzije).
- Dvoklik na kanal i Properties paleta: kao i do sada (dva koraka – izmjenu bilježi AutoCAD van TVS komande). Javiti ako to smeta.
- TVS_TK_TIP: zaključati sloj kanala (ili prelaza, ili kanala iza prelaza) na jednom priključku T komada → odmah poruka „T komad nije mijenjan: on ili element na njegovom priključku je na zaključanom sloju“, dijalog se ne otvara, ništa se ne mijenja (ranije: T komad promijenjen, kanal na priključku ne). Isto kad dijalog T komada otvori dionica (ostaje prelaz na priključku). Napomena: provjeravaju se sva tri priključka, i onaj koji se ne mijenja.
- TVS_RAZBIJ / TVS_ERASE: svaki element se razbija u jednoj transakciji – ako se neki dio ne može dodati u prostor, element ostaje cio (ranije obrisan bez tog dijela). Provjeriti: razbijanje u Model, Layout i u bloku (BEDIT / REFEDIT), zaključan sloj (preskočeno, poruka kao ranije), jedan U vraća sve razbijene elemente, oznake (TVS_OZNAKE) i dalje postaju TEXT.
- Dionica sa zaključanim elementom: poruka „Dionica nije promijenjena …“ kao i do sada (ista provjera, sada zajednička).

## Izdanje i sklopovi (12.11.) – testirati
- TVS_BYPASS_PREVIEW, TVS_OFFSET_STATUS, TVS_OFFSET_POLICY više ne postoje u izdanju (Unknown command). TVS_OFFSET i opcija Z ne ispisuju „DEV“ linije.
- TVS_RAZBIJ / TVS_ERASE: izbor koji obuhvata etaž ili obilazak – članovi sklopa se preskaču uz poruku; ostali TVS elementi se razbiju kao ranije.
- Obilazak pri promjeni dimenzije: zaključati sloj jednog člana obilaska → poruka „element sklopa se ne može mijenjati“, obilazak ostaje cio.
- Tip T komada (TVS_CONNECTW, Račva, TVS_TK_TIP): klik na opciju i kucanje TK-O / TK-G / SR5-3 rade.

## TVS_DPD_IZVOZ – izvoz mreže u DuctPressDrop (novo, 21.11.) – testirati
Format i pravila: [DPD_IZVOZ.md](DPD_IZVOZ.md).
- Mreža sa jedinicom, koljenima, TK0 / TK-O / TK-G, prelazom i fleksom; protoci upisani (TVS_PROTOK).
- `TVS_DPD_IZVOZ` → klik blizu slobodnog kraja kod jedinice (ili prije komande izabrati jedan kanal sa jednim slobodnim krajem) → `Sistem [Dovod/Odvod]` → dijalog za snimanje, predloženo `<crtež>_Dovod.json`.
- Poruka: broj TVS dionica → DuctPressDrop dionica (više, jer se dijele na koljenima), Q kod jedinice, putanja. U crtežu se ništa ne mijenja.
- DuctPressDrop: Datoteka → Uvezi sistem… (Ctrl+I) → novi sistem „Dovod <crtež>“. Provjeriti: Q, dimenzije i dužine dionica kao u crtežu (L = osa između čvorova, sa fitinzima), šema (osnova) izgleda kao crtež, koljena / račve / prelazi su se sami pojavili (šuplji simboli), nema upozorenja o zatvaranju geometrije.
- Odvod: isto, sistem „Odvod …“, dionice O-01…, smjer toka ka jedinici.
- Crtež u cm / m: dužine i smjer u DuctPressDrop-u ipak u metrima, dimenzije u mm.
- Ne prenosi se: kote (sve u jednoj ravni), rešetke / difuzori (dodati u DuctPressDrop-u), temperatura.
- Krajnji element bez protoka → poruka, dionica sa Q = 0.
- Meni / ribbon „Crtanje kanala“: dugme „Izvoz u DuctPressDrop“ (ikona kao TVS_PAD).

## TVS_OPREMA – povezivanje nacrtanih blokova opreme u mrežu (novo, 21.11.) – testirati
Opis: [OPREMA.md](OPREMA.md). Blok se nikad ne pomjera; mijenjaju se samo krajevi kanala.
- **Slučaj 1:** kanal – blok regulatora (npr. 500 mm) – kanal, krajevi kanala tačno na licima bloka → `TVS_OPREMA`, izaberi blok → „povezano 1 (u liniji 1…)“, kanali nepromijenjeni.
- **Slučaj 2:** isto, ali jedan kanal 50–100 mm prije bloka, drugi ulazi 100 mm pod blok → oba kraja dođu tačno na lica bloka („krajeva kanala skraćeno / produženo 2“). Razmak veći od 150 mm → odbijeno „razmak između bloka i kanala je veći od dozvoljenog (… mm)“; opcija **Tolerancija** mijenja 150 mm.
- **Slučaj 3:** jedan neprekinut kanal ispod bloka → kanal se presiječe za dužinu bloka; novi komad ima ista svojstva (dimenzija, kota, protok, sloj, boja, TVS svojstva) kao poslije TVS_CUT. Komad kraći od 50 mm → odbijeno.
- **Zarotiran blok** (npr. 30°, kanal pod 30°) i kanal crtan s desna na lijevo / odozgo na dolje: isto kao gore. Dužina bloka = granice definicije bloka (bez atributa) duž ose kanala.
- **Terminal:** rešetka / difuzor na slobodnom kraju kanala ili flex crijeva (dodiruje, razmak ili kraj pod blokom) → 1 priključak, kraj kanala na licu bloka. Kraj spojen sa koljenom / T komadom → „kraj kanala je spojen sa drugim elementom“.
- **Odbijanja (ništa se ne mijenja, blok ostaje izabran, ispis sa handle-om):** osa kanala 100 mm od sredine bloka, kanali različitih dimenzija / kota sa dvije strane, dva kanala ukrštena ispod bloka, kanal cio ispod bloka, flex ispod regulatora, kanal na zaključanom sloju, kanal iz etaža / obilaska, blok zarotiran u 3D.
- **Podrazumijevano po nazivu bloka** (TVS_OPREMA_TIPOVI, upisuje dijalog opreme): vrsta rešetka → blok preko kontinualnog kanala odbijen („bočna ugradnja – kasnije“); vrsta regulator → blok sa kanalom samo s jedne strane odbijen; zadata dužina → kanal se reže / skraćuje na tu dužinu (centrirano na blok). Bez zapisa: terminal = Rešetka, u liniji = Ostala oprema.
- **Sve:** `TVS_OPREMA` → `Sve` → svi blokovi trenutnog prostora koji leže na kanalu; ostali se samo prebroje („blokova van kanala“). Xref blokovi se preskaču.
- **Ponovo:** već povezan blok → pitanje „Povezati ih ponovo [Da/Ne] <Ne>“; Da → isti blok se ponovo poveže (vrsta / oznaka / model gubitka ostaju).
- **Jedan U** vraća sve što je TVS_OPREMA uradila (sve blokove, kanale i nove komade). Crtež u cm / m: tolerancije u mm rade isto.
- **TVS_OPREMA_ODVEZI:** izaberi povezane blokove → zapis obrisan, kanali i blok ostaju kakvi jesu.

## Sitne ispravke iz pregleda (15.11.) – testirati
- TVS_SPEC / TVS_EXCELSPEC: blok sa atributima na zaključanom sloju je u specifikaciji (ranije tiho izostavljen).
- Fleks koljeno: dužina u specifikaciji = luk ose (npr. Ø200, 90°, k = 0,5 → ≈ 0,31 m, ranije 0,08 m).
- Izvoz specifikacije u Excel: količine, površine i mase su brojevi (i u Excelu sa decimalnim zarezom; ranije tekst).
- TVS_OFFSET / Z: poslije dijaloga Enter primjenjuje etaž (`<Primijeni>`), poruka u mm.
- Dva crteža različitih jedinica: TVS_DIM_PRATI / TVS_FLEX_MANZETNE / TVS_IZVOZ koriste jedinice aktivnog crteža; zatvaranje crteža dok čeka TVS_ETAZ_KOTA ne blokira i ne ruši.

## Okrugla štucna: valjkasta / konusna (novo, 15.11.) – testirati
- Okrugli ogranak na pravougaonom kanalu (TVS_TK_TIP komandna linija, Račva, TVS_CONNECTW): `[Stucna/Konusna/Ravni]`. Stucna = valjkasta (ASHRAE SR5-11, L = 0,25 D, min. 75 mm), Konusna = konusna (SR5-12, L = 100 mm, baza konusa D + 2·L·tan 15° ≈ D + 54 mm). Kucano SR5-11 / SR5-12 radi isto.
- Kraći glavni dio štucne (sve štucne, i pravougaone): kanal štucne ide samo 25 mm preko njenog otiska na kanalu (pravougaona: kosi ulaz + širina ogranka; okrugla: D ili baza konusa). Nove štucne ga dobijaju odmah; postojeće: ponovo izabrati tip Stucna / Konusna (TVS_TK_TIP), susjedni kanali se produže.
- Crtež: bez kosog ulaza od 45°; valjkasta = dva paralelna zida, konusna = zidovi od baze na kanalu do ogranka. Nema pitanja „strana kosog ulaza“.
- Promjena prečnika ogranka (TVS_DIM_PRATI, TVS_PROTOK Primijeni, dijalog TVS_TK_TIP): konusna ostaje konusna (nova baza), valjkasta dobija novi L.
- Specifikacija: „Valjkasta štucna SR5-11“ / „Konusna štucna SR5-12“; TVS_PAD: SR5-11 / SR5-12 (dovod), ER5-2 (odvod).
- TVS_PAD: list Dionice ima „t vazduha, °C“ (žuto, 20 °C); ρ = p/(R·T) i ν (Sutherland) su formule, λ je formula (Colebrook po Serghidesu) – promjena temperature (npr. dim 300 °C) preračuna v, pd, Re, λ, R, Z i Δp fitinga. Q je zapreminski protok pri toj temperaturi.
- TVS_PAD: koljeno gore / dolje (TVS klasično): računa se po CR3-1 sa W = visina kanala (strana u ravni savijanja); spaja se sa kanalom i kad mu kota (z) nije ista kao kanalu.
- TVS_PAD: T komad sa ulazom kroz ogranak (vazduh ide na obje strane) = bullhead: ASHRAE SR5-15 / ER5-5 (pravougaoni), SD5-19 / ED5-4 (okrugli); C na brzinu svakog kraka.
- TVS_PAD dovod: TK0 čiji je ogranak druge visine od voda → Idelchik razdvajanje (tabela kao KanCalc 0.7 / 0.6), iste visine → SR5-5.

## TVS_PAD (novo, 14.11.) – testirati
- Prvi put: opcija „Podaci“ (ili pitanje na početku) → izaberite `Equation.csv` u folderu `ashrae_data` instalacije DuctPressDrop. Folder se pamti (registry `Software\TVS_Vent`, `AshraeData`). ASHRAE tabele nisu u TVS_VENT-u.
- Izbor kraja mreže kod jedinice (kao TVS_PROTOK; protoci se sabiraju, u crtež se ništa ne upisuje) → `Sistem [Dovod/Odvod/Podaci]`.
- Excel (otvoren, nesačuvan), list **Dionice**: dionica = isti protok, presjek i vrsta kanala (nova poslije T komada, poslije prelaza, na prelazu lim/fleks); brojevi od jedinice; Od/Do: J, T (T komad), P (promjena), K (kraj). v, pd, R, Z, Δp su formule; Re skrivena kolona; žute ćelije Δp krajnjih elemenata unosite sami → kritična grana (crveno, ✓) i „Višak Pa“ (za prigušiti na T komadu) se preračunavaju.
- List **Fitinzi**: koljena CR3-1 / CD3-2 / CD3-21 / CD3-4, prelazi SR4-1 / SR4-2 / SD4-1 / SR4-3 / SD4-2 (odvod ER4 / ED4), T komadi SR5-5 (TK0), SR5-1 (TK-O), SR5-13 (štucna), SR5-11 (okrugli ogranak), SR5-14 (TK-G), SD5-9 (okrugli); odvod ER5-1 / ER5-2 / ER5-4 / ED5-3. ζ je sveden na pd svoje dionice i može se ispraviti; žuto = nema ASHRAE komada (npr. okruglo koljeno 30°), ζ unosite sami.
- List **Šema**: jednolinijska osnova, fleks isprekidano, broj dionice, čvorovi; kritična grana pri izvozu crveno.
- Odvod: TK0 i štucna sa Ab/Ac van 0,4–0,6 → Idelchik (oštar T, As = Ac, C na Vc; tabela kao KanCalc 1.7 / 1.6), izvor „Idelchik“, napomena da je za pravougaoni spoj vjerovatno potcijenjeno; štucna 0,4–0,6 → ER5-3; TK-O → ER5-1.
- Slobodan priključak T komada / štucne na kraju mreže = zatvoren (čep): štucna bez ičega na sebi je krajnji element, vazduh izlazi kroz ogranak (rešetka), prolaz je zatvoren – računa se ASHRAE sa Qb = Q, napomena „čep“.
- Provjeriti: T komad ulazom kroz ogranak (napomena „nestandardno“), etaž / obilazak (koljena unutra se broje), fleks na kraju (k = 3 mm), mreža bez ASHRAE foldera (samo trenje).

## TVS_OZNAKE (novo, 13.11.) – testirati
- TVS_OZNAKE: [Ukljuci/Iskljuci/Polozaj/Razmjera/Pojedinacno]. Uključivanje pravi sloj i stil teksta AT-DIM_KANAL (romans.shx, faktor širine 0,7).
- Oznaka na svakom pravom kanalu dužem od teksta: `400x150/1150` (okrugli `%%c250/500`, bez protoka samo dimenzija), bez jedinica; visina 2,5 mm × razmjera (prvi put iz podešavanja TVS oznaka, npr. 1:50 → 125 mm).
- Crta je sam kanal na sloju AT-DIM_KANAL (boja ByLayer), **štampa se**; isključen / zamrznut sloj je sakriva. Mijenja se sama sa dimenzijom, protokom (TVS_PROTOK), pomjeranjem i STRETCH-om.
- Polozaj: podrazumijevano iznad / na sredini / ispod kanala (čitljivo odozdo / zdesna). Pojedinacno: izabrani kanali [Iznad/Sredina/Ispod/Podrazumijevano].
- TVS_RAZBIJ / TVS_IZVOZ: oznaka postaje TEXT na sloju AT-DIM_KANAL.
- Provjeriti štampu (PLOT / PDF) i brzinu crteža sa puno kanala.

## TVS_SPOJ_TKO (novo, 13.11.) – testirati
- Izbor: ulazni kanal, prolazni kanal (paralelan, u nastavku), ogranak (okomit). Pravougaoni, iste kote; prolaz i ogranak ne širi od ulaza. Visine mogu biti različite: T komad ima visinu ulaza na svim priključcima, a poslije „Da“ na prolazu / ogranku druge visine umeće se prelaz (isto i u TVS_SPOJ_TKG).
- TK-O se postavlja na presjek ose ulaza i ose ogranka; ulaz, prolaz i ogranak se skraćuju / produžuju do lica T komada.
- „Ivica“: prolaz u osi ulaza (prolazni kanal se pomjeri poprijeko ako nije) / prolaz poravnat uz ivicu ulaza **nasuprot ogranku** / **na strani ogranka** (jedan zid T komada ide ravno). Podrazumijevano ono što crtež već ima (kao na slici: desne ivice u liniji → nasuprot ogranku), inače osa.
- Rezultat se nacrta pa „[Da/Ivica/Ne]“; Ivica prelazi na sljedeće poravnanje, Ne vraća sve. Prolaz dalje spojen i treba ga pomjeriti → nudi se poravnanje bez pomjeranja.
- TK-O sa pomjerenim prolazom pamti pomak u zapisu (kao vrat kod TK-G); TVS_TK_TIP ga zadržava, Okreni ga uklanja. Provjeriti specifikaciju i TVS_MOVE / STRETCH na njemu.

## TVS_SPOJ_TKG (novo, 13.11.) – testirati
- Izbor: ulazni kanal, prvi ogranak, drugi ogranak (ogranci okomiti na ulaz, na suprotne strane, iste visine i kote kao ulaz, pravougaoni).
- Ako ogranci nisu u istoj osi (novo): podrazumijevano se **ništa ne pomjera** – T komad ide na osu bližu ulazu, a dalji ogranak dobija duži **vrat** (pravi zid prije luka), pa oba lica padnu na postojeće ose. „[Da/Pomjeri/Ne]“: Pomjeri = kao ranije, ogranak u osu drugog (pa [Da/Vrat/Zamijeni/Ne]). Vrat nije više ograničen: kad se vanjski lukovi ne sijeku, dalji ogranak ima cijeli luk, bliži luk staje na pravom zidu, a linija preko zatvara do zida ulaza (kao ručno crtano 13.11.).
- Vrat se pamti u T komadu (TVS_TEE_V1, 6. podatak); stariji T komadi su kao i prije. TVS_TK_TIP ga zadržava (Okreni ga uklanja); SCALE / jedinice ga skaliraju. Provjeriti specifikaciju (površina lima) i TVS_MOVE / STRETCH na takvom T komadu.
- TK-G se postavlja na presjek ose ulaza i ose ogranaka, ulaz se skraćuje (prava dužina se računa iz širina: okovratnik + veća širina ogranka), ogranci završavaju na licima T komada.
- Rezultat se nacrta pa „Ovako spojiti? [Da/Zamijeni/Ne]“: Zamijeni = pomjeri drugi ogranak; Ne = sve nazad. Jedan U vraća spajanje.
- Odbija: okrugli, različite visine / kote, preuski ogranci (TK-G nemoguć), prekratak ulaz / ogranak, kraj kod račve već spojen.

## TVS_TK_TIP: visina TK-O / TK-G otključana (13.11.) – testirati
- „Na T komadu“ B je sada otključan: to je visina svih priključaka i ulaza. Nova visina → T komad je dobija, **ulazni kanal je preuzima** (širina ostaje; dalje TVS_DIM_PRATI / prelaz na dalekom kraju); na ograncima prelaz ako kanal ima drugu visinu (ili promijenite „Kanal u nastavku“ B). Osa (Elev) ostaje.

## TVS_TK_TIP: „Kanal u nastavku“ i iza prelaza (13.11.) – testirati
- Priključak sa prelazom pa kanalom (kao na slici TK-G, Ogranak C): Oblik, A i B kanala u nastavku su sada otključani. Nova dimenzija → prelaz je dobija na drugom kraju, kanal iza njega takođe (i dalje TVS_DIM_PRATI kao kod kanala direktno na priključku). Ako je nova dimenzija ista kao na T komadu, prelaz se uklanja i kanal ide do priključka.
- „Redoslijed“ ostaje zaključan dok nije izabrano koljeno (bez koljena nema šta da se redoslijed).

## TVS_PROTOK (novo, grana razvoj-protoci) – testirati
- Klik na kanal kod jedinice, blizu njegovog slobodnog kraja (taj kraj ne smije biti spojen). Svi elementi od tog kraja dalje dobiju zbir protoka krajnjih elemenata iza sebe; krajnji elementi (ništa dalje spojeno) zadržavaju svoj protok.
- Ispis: broj elemenata mreže, krajnjih elemenata (bez protoka), ukupno m³/h, slobodni priključci na fitinzima (npr. otvoren ogranak T komada), najveća brzina u kanalu.
- Krajnji elementi bez protoka: [Unos/Nastavi] – Unos zumira na svaki i pita protok (Enter = preskoči, Kraj = prekini unos).
- Ništa se ne upisuje prije „Upisati protoke u N elemenata? [Da/Ne]“; jedno UNDO vraća sve. TVS oznake (MLeader) promijenjenih elemenata se osvježe.
- Etaž i obilazak su jedan element (svi članovi dobiju isti protok); flex crijeva nose protok kao kanali.
- Petlja u mreži ili tačka u kojoj se sastaje više od dva kraja: poruka sa zumom, ništa se ne upisuje.
- Tip sistema se ne bira (dovod i odvod isto).
- Brzine (poslije upisa, i kad su protoci već tačni ili je odgovor Ne): „najveća brzina [m/s] ili [Ne] <5.0>“ (pamti se do zatvaranja AutoCAD-a). Lista dionica (dio mreže između račvi) iznad te brzine, najbrža prva, sa prijedlogom: pravougaoni „širina: 800×200“ (visina ostaje) i „š×v: 400×400“ (š ≥ v, odnos ≤ 1:4, visina ne manja od sadašnje); okrugli – sljedeći standardni prečnik.
- Poslije liste otvara se dijalog „Dimenzije dionice“ za svaku dionicu iznad granice (crtež prethodno zumira i označi kanale dionice):
  - gore: protok, broj kanala, sadašnja dimenzija sa m/s i Pa/m (crveno kad je iznad granice);
  - „Najveća brzina, m/s“ – izmjena odmah osvježava prijedloge (pamti se);
  - „Visina ostaje (H = const)“ / „Širina : visina ≤ 4:1“ (kod okruglog: standardni prečnici);
  - lista prijedloga: dvije prebrze (sive) pa prve odgovarajuće, sa m/s i Pa/m (Darcy-Weisbach, lim k = 0,15 mm, vazduh 20 °C); klik prepiše dimenziju u „Ručni unos“;
  - „Ručni unos“: 300x150 / d200, brzina i Pa/m dok se kuca;
  - ◀ Prethodna / Sljedeća ▶ (zum na drugu dionicu, dijalog se ponovo otvara na istom mjestu), „Ukloni oznaku“, „Zatvori“, „Zadrži ručno“ (Enter; prazan unos = ručno sa sadašnjom dimenzijom).
- Provjeriti Pa/m na poznatom primjeru (npr. 400×200, 2880 m³/h: oko 4,2 Pa/m).
- Dionice se nude od najudaljenije od jedinice prema jedinici (pa po brzini).
- **Primijeni** (v1, plan po prijedlogu Fable 5.2): mijenja se samo širina / prečnik (promjena visine i oblika se odbija), dio dionice iste dimenzije kao najbrži kanal. Prije upisa ispis: šta se mijenja (kanala, koljena, T komad, prelazi, dijalog TK) i upozorenja sa koordinatama, pa „Primijeniti? [Da/Ne]“.
  - kanali i koljena dionice dobiju širinu; koljeno sa nove poluprečnikom pomjera krajeve susjednih kanala;
  - T komad na kraju dionice (dionica ulazi u njega): ulaz (glavni vod) dobija širinu, iza prolaza se umeće prelaz na staru dimenziju, kanal ogranka prati za pola razlike; ako iza prolaza nije kanal (koljeno, drugi T...) T komad ostaje i prelaz ide ispred njega;
  - TK-O / TK-G: njihov dijalog sa novom dimenzijom na ulazu (Odustani = prelaz na priključku);
  - T komad na početku dionice (ona izlazi iz njega): ostaje, prelaz na njegovom priključku;
  - postojeći prelaz preuzima dimenziju (ili se uklanja kad više nema razlike); poravnat uz stranu ostaje + novi prelaz;
  - koljeno direktno na T komadu / etaž / obilazak / flex: ostaju, prelaz u susjedni kanal;
  - poslije upisa provjera spojeva: razdvojen spoj → poruka sa zumom (U vraća).
- Probati: dionica između dva T komada, dionica do običnog T komada sa kanalom iza prolaza, do TK-O, dionica sa koljenom u sredini, kratak kanal uz T komad.
- Izgled: lista viša (6 prijedloga bez skrolovanja), dugmad „Zadrži ručno“ i „Primijeni“ šira.
- TVS_ALIGN: opcije reference sada [Izabrana/Druga/Osa] (klik radi).

## TVS_PROVJERA (novo) – testirati
- Enter = cijeli crtež, ili izbor dijela mreže. Ispis: broj problema po vrsti, pa lista (do 200) sa X/Y u mm.
- Vrste: spoj različitih dimenzija bez prelaza; ista dimenzija, druga kota; kraj promašuje priključak (0,1–5 mm); kraj kanala usred drugog kanala (bez T komada); koljeno malo van 15/22,5/30/45/60/75/90°; kanal kraći od 10 mm.
- Obilazak: zum na problem, elementi označeni (gripovi), [Sljedeci/Prethodni/Kraj].
- Probati na jučerašnjim slučajevima (kanal na 0 uz T komad na 2850; kanal 150×150 na ogranku visine 100) – oba treba da se nađu.
- Lažni nalazi (npr. kanali jedan iznad drugog sa krajevima u istoj tački) – javiti.

## Novi meni (TVS_VENT.cuix)
Učitajte novi TVS_VENT.cuix (CUILOAD / Menuload ili zamjena postojećeg fajla).
Nove komande su u meniju "Crtanje kanala", na traci TVS_VENT i na ribbonu:

| Komanda | Naziv | Ribbon panel |
|---|---|---|
| TVS_OFFSET | Promjena kote | Crtanje kanala |
| TVS_BYPASS | Obilazak prepreke | Crtanje kanala |
| TVS_TK_TIP | Tip T komada | Crtanje kanala |
| TVS_PRELAZ_KOTA | Poravnanje prelaza | Crtanje kanala |
| TVS_FLEX_MANZETNE | Flex manžetne | Crtanje kanala |
| TVS_FLEX_MANZETNA | Flex manžetna uklj/isklj | Crtanje kanala |
| TVS_MATCH | Prenos svojstava | Atributi |
| TVS_PRESJEK | Poprečni presjek | Oznake |
| TVS_KOTE | Prikaz kota | Oznake |
| TVS_BYPASS_DETALJ | Detalj obilaska | Oznake |

TVS_MOVE i TVS_STRETCH imaju nove ikone.
Regularni paket (tvs-vent-autocad-20xx-x64) i DEV paket sada imaju iste komande, uključujući TVS_OFFSET, TVS_BYPASS i TVS_BYPASS_DETALJ.

## Otvoreno – treba testirati i javiti nazad
**Prelaz – prenos dimenzija na susjedne kanale:** ispravljeno (vidi „Dimenzije na spojevima prelaz–kanal“ niže); provjeriti oba smjera.

## Pregled koda – faze 1 i 2 (REVIEW_2026-11.md) – testirati
**TVS_DIM_PRATI**
- **Dijalog cijele dionice je podrazumijevano ISKLJUČEN** (TVS_SETTINGS → „Promjena dimenzije cijele dionice (dijalog)“, čuva se u profilu). Isključeno: radi samo prenos dimenzije kanal ↔ prelaz; spoj kanala nove dimenzije sa elementom stare dimenzije daje UPOZORENJE; TVS_TK_TIP umeće prelaz na kraju kanala kao ranije. Stavke ispod važe kad je uključeno.
- Kanal sa krajem na istoj tački u osnovi kao drugi kanal/prelaz na drugoj visini (jedan iznad drugog): promjena dimenzije gornjeg ne smije dirati donji.
- Umjesto pitanja u komandnoj liniji otvara se dijalog **„Promjena dimenzije dionice“** (samo kad dionica stare dimenzije ide dalje od izabranog kanala):
  - **Obuhvat:** cijela dionica do granica / samo izabrani kanal (UPOZORENJE ako naliježe na element stare dimenzije) / prema strani 1 ili 2 (brojevi su u pregledu na krajevima kanala) / samo kanal + prelaz na spoju.
  - **T komad:** Nastavi (TK0 i štucna: glavni kanal se mijenja i dionica ide dalje; **TK-O / TK-G: poslije primjene otvara se dijalog tog T komada** sa novom dimenzijom na ulazu – tu se podešavaju ogranak, prolaz, kanali u nastavku i prelazi) / Stani / Prelaz iza T komada.
  - **Etaž, Obilazak:** preračunaj ili stani prije njih. Redovi su aktivni samo ako takav element postoji na trasi.
  - **Pregled:** osnova trase; tamno plavo = izabrani kanal, plavo = mijenja se, žuto = T komad čiji se dijalog otvara, sivo = ostaje. Mijenja se sa svakim izborom.
  - **Otkaži** = samo izabrani kanal. Otkaži u dijalogu T komada = T komad ostaje, na priključku prelaz (kao do sada).
  - Provjeriti primjer: kanal između prelaza i TK-O → poslije Primijeni otvara se dijalog TK-O, ulaz ima novu dimenziju, visine priključaka je prate.
  - Dionica koja u TK-O / TK-G dolazi sa strane ogranka ili prolaza: dijalog se otvara na toj kartici.
- **Dionica dolazi na ogranak / prolaz TK-O ili TK-G:** u dijalogu T komada taj priključak odmah ima novu širinu (prelaz se ne umeće). Visina ogranka kod TK-O/TK-G ostaje visina ulaza – ako se razlikuje, prelaz samo zbog visine.
- **Promjena oblika (pravougaoni ↔ okrugli) u Svojstvima kanala ili u TVS_TK_TIP („Kanal u nastavku“):** dijalog dionice se otvara i tada. Kanali i koljena u osnovi mijenjaju oblik i dimenziju (koljeno dobija radijus iz TVS_SETTINGS za novi oblik; krajevi kanala prate koljeno). T komad, etaž i obilazak su granica (ostaju; T komad dobija prelaz na priključku). Prelaz na kraju dionice preuzima oblik na tom kraju (postaje prelaz pravougaoni ↔ okrugli). „Kanal + prelaz na spoju“ umeće prelaz novog → starog oblika.
- **TVS_TK_TIP → kanal u nastavku nove dimenzije:** kad se u dijalogu T komada promijeni dimenzija kanala na prolazu/ogranku (isti oblik), a iza njega se nastavlja mreža stare dimenzije (koljeno, kanali, etaž…), otvara se dijalog „Promjena dimenzije dionice“ za taj dio (T komad se ne dira). Otkaži = prelaz na spoju sa prvim elementom stare dimenzije (kao do sada). Ako iza kanala nema ništa stare dimenzije – prelaz na kraju kao do sada.
- Poslije promjene cijele dionice (koljena pomjere krajeve) prelaz na kraju dionice treba da preuzme dimenziju.
- Prelaz drugog oblika na kraju izmijenjenog kanala (okrugli/pravougaoni): poruka umjesto tihog preskakanja.
- Dva otvorena crteža: izmjena u jednom ne smije se izgubiti ni prenijeti na drugi.
- Kanal na zaključanom sloju uz prelaz: poruka.

**Ostalo**
- **Komande uklonjene iz izdanja** (samo razvojni build): TVS_TEST, TVS_TRANS, TVS_WYE, TVS_CONNECTTest, TVS_DUCT, TVS_SpecificationOld, TVS_Flex, TVS_Show, TVSSomeParts, MyCommand/MyPickFirst/MySessionCmd, TVS_OFFSET_TEST_BAD, TVS_OFFSET_FAIL_NEXT, TVS_TEST_OFFSET*, TVS_BYPASS_DETAIL_TEST/CHECK. Javiti ako je neka od njih bila potrebna.
- **Komande više nisu transparentne** (ne mogu se pokrenuti sa ' usred druge komande), osim TVS_SETTINGS. Provjeriti da meni i ribbon pokreću sve kao ranije.
- TVS_AttributesToProps: radi samo nad izabranim blokovima i pita `Da/Ne` (podrazumijevano Ne).
- Svojstva okruglog koljena, „konstantan radijus“: poslije OK koljeno ostaje konstantnog radijusa.
- TVS_EXCELSPEC bez Excel-a: poruka „Excel nije dostupan“, bez pada.
- Menadžer blokova: dug tekst (>512 znakova) u ćeliji – bez pada (tekst se skraćuje).
- TVS_SCALE: prazan klik i Enter ne ruše komandu; Esc izlazi.
- TVS_1D2D: Enter = 2d.
- TVS_AddAtrib: poruke grešaka sa nazivom bloka.
- Dugi rad sa TVS_Change / TVS_1D2D / TVS_ERASE / specifikacijama: izbor objekata ne prestaje da radi.
- Podešavanja specifikacije (TVS_Sp → Podešavanja): Odustani ne mijenja kolone.

## Flex crijeva u TVS_STRETCH / TVS_MOVE / TVS_ALIGN – testirati
- **TVS_STRETCH:** kraj flex crijeva u okviru se pomjera sa okvirom (drugi kraj ostaje, crijevo se savije); cijelo crijevo u okviru se pomjeri.
- **Kraj flexa na pomjerenom kanalu / fitingu** (i van okvira): ide za tim priključkom.
- **TVS_MOVE / TVS_ALIGN kanala sa flexom na kraju:** više ne odbija („nepodržan element“), flex prati.
- Samo flex u okviru (bez kanala): pomjeri se samo flex.
- Na kraju poruka „… flex crijeva: N“.

## Spojevi koje TVS_STRETCH / TVS_MOVE odbijaju – testirati
- **Odbijen spoj:** ispis oba presjeka (dimenzije, osa u mm) i tačke spoja.
- **Ista dimenzija, druga kota** (kanal na 0, T komad na 2850): pitanje „Izjednačiti kotu kanala…? [Da/Ne] <Da>“ → kanal dobija kotu susjeda → ponoviti komandu.
- **Kanal druge dimenzije na priključku TK-O / TK-G** (npr. 150×150 na ogranku visine 100): „umetnuti prelaz? <Da>“ → prelaz na priključku → ponoviti komandu.
- **TVS_DRAW / TVS_PIPE do priključka TK-O / TK-G** kanalom druge dimenzije: na kraju komande prelaz se umeće sam (poruka „T komad, …: umetnut prelaz…“). Isto kad crtanje počne sa priključka bez konektor-gripa.
- **Prelaz poravnat gore/dolje** u mreži: TVS_STRETCH / TVS_MOVE više ne javljaju lažnu razliku kota iza njega.

## Ponovni pregled N2 – kopiranje između crteža sa različitim jedinicama – testirati
Napomena: AutoCAD pri lijepljenju preračunava po INSUNITS (Drawing Units → Insertion scale), TVS po svojim jedinicama (TVS_SETTINGS). Najbolje je da su u oba crteža iste (mm-crtež: INSUNITS = Millimeters, cm-crtež: Centimeters).
- **INSUNITS odgovaraju TVS jedinicama:** Ctrl+C iz mm crteža, Ctrl+V u cm crtež → kanali, koljena, prelazi, T komadi su iste stvarne veličine (400 mm ostaje 400 mm, u cm crtežu 40); i linije oko njih. Isto cm → mm, mm → m.
- **INSUNITS ne odgovaraju** (npr. oba „Unitless“): poslije lijepljenja poruka „TVS: umetnuti elementi su iz crteža u mm…“ → TVS_PRERACUNAJ, izbor, [mm/cm/m] izvora, bazna tačka → sve na pravu veličinu.
- **EXPLODE / INSERT bloka sa TVS elementima** umetnutog sa razmjerom: poslije razbijanja dimenzije prate razmjeru.
- **SCALE** nad TVS elementima: i dalje odbijen.
- **TK-O / TK-G** poslije kopiranja mm → cm: oblik i priključci isti (kragna, druga širina, radijus).

## Numerička polja dijaloga, jedan sloj (prijedlog 4, 21.11.) – testirati
- **Svojstva, OK bez izmjene** (koljeno 45° / 90°, kanal Ø315 u crtežu u m, kota 2,755 m, TK-O / TK-G sa drugim ogrankom, prelaz): ništa se ne mijenja – Properties pokazuje iste brojeve, oznake se ne osvježavaju zbog „izmjene“.
- **Prikaz kao do sad:** dimenzije u mm, ugao u stepenima (npr. 89,95437384 kod starog zaokruženog koljena), protok i k radijusa bez promjene broja decimala; „Različito“ kod različitih vrijednosti.
- **Unos sa zarezom:** ugao „44,5“, dimenzija „315,5“, protok „1,5“, kota „2,75“ – upisuje se tačno ta vrijednost. Ukucati isti broj drugačije („45,0“ umjesto „45“) – nije izmjena.
- **Novo – provjera opsega (S-2):** ukucati ugao 0, 180, 200 ili -10 → poruka „Ugao koljena mora biti veći od 0° i manji od 180°.“, dijalog ostaje otvoren; dimenzija A (ili A2) 0 → „Dimenzija mora biti veća od 0.“. Odustani i dalje ne mijenja ništa.
- **Više izabranih elemenata:** polje „Različito“ prepisati brojem → svi dobiju taj broj; ostaviti „Različito“ → ništa.
- **Vrh / Centar / Dno** u Svojstvima i TVS_SETTINGS preračunava kotu kao do sad; TVS_PRESJEK (kote, razmjera) i dijalog promjene kote (TVS_OFFSET / opcija Z pri crtanju) primaju zarez i tačku kao do sad.

## Ponovni pregled N1 – Svojstva ne mijenjaju ništa na „OK“ – testirati
- **Koljeno 45° / 90°** → Svojstva → OK bez izmjene → Properties: ugao ostaje tačno 45 / 90 (ranije 45,26 / 89,954).
- **Crtež u metrima:** kanal Ø315 → Svojstva → OK → ostaje Ø315 (ranije Ø320). Isto kote i dužine prelaza.
- **Više izabranih elemenata različitih vrijednosti:** polje prikazuje „Različito“ (ranije ruski tekst); ako se ne dira, ništa se ne mijenja.
- **Dvoklik → Svojstva → Odustani:** ništa se ne mijenja.
- **Promjena ugla u Svojstvima** (npr. 45 → 30) i dalje radi.
- **TVS_KOLJENA_UGAO (novo):** izbor koljena ili Enter = cijeli crtež. Koljena u osnovi sa uglom manje od 0,5° od 15 / 22,5 / 30 / 45 / 60 / 75 / 90° dobijaju tačan ugao, krajevi kanala na njima se pomjere za njima. Koljeno uz koje je prelaz / T komad / drugo koljeno se ne dira (ispis koliko). Probati na crtežu gdje je TVS_MOVE / TVS_ALIGN javljao „ose nisu tangentne“ ili je TVS_DIM_PRATI stajao na koljenu, pa ponoviti.
- **Dionica iz Properties palete (D6):** promijeni A pa B kanala u paleti – dijalog dionice se otvara tek kad izađeš iz palete (klik / kursor u crtež), jednom za obje izmjene. **Otkaži** u dijalogu vraća kanal na staru dimenziju. Provjeriti i dvoklik / Svojstva (dijalog odmah, kao do sad).
- **Dionica (D1, D3, D4, D5b, D7, D8):** koljeno sa kotom malo pomjerenom se prelazi; okruglo segmentno koljeno se pri promjeni oblika pretvara; ogranak štucne / TK0 preuzima novi oblik; element na zaključanom sloju → ništa se ne mijenja; fleks crijevo kao granica „fleksibilno crijevo“.

## Faza 6 pregleda (elementi) – testirati
- **SCALE nad TVS elementima:** odbijen, poruka jednom po komandi; ostali (ne-TVS) objekti u izboru se skaliraju normalno. MOVE, ROTATE, COPY i MIRROR rade kao ranije.
- **MIRROR:** koljeno, prelaz poravnat uz ivicu (tip 2/3) i TK-O / TK-G treba da izgledaju kao ogledalska slika i ostanu spojeni.
- **Provjeriti:** kopiranje (Ctrl+C / Ctrl+V) TVS elemenata između crteža sa različitim jedinicama (INSUNITS) – AutoCAD tada skalira, pa će TVS elementi biti odbijeni; javiti kako se ponaša.
- **Kanal nulte dužine** (npr. oba kraja u istoj tački): bez čudne geometrije / gripova.

## Faza 5 pregleda (dijalozi) – testirati
- **Decimalni zarez:** u TVS_SETTINGS (dimenzije, protok, dužine, kota, k radijusa, c spiro), u Svojstvima (protok, ugao, k radijusa, dimenzije) i u podešavanjima oznaka (razmjera) „0,5“ = „0.5“. Ranije se zarez čitao kao kraj broja (0,5 → 0) ili dijalog nije prihvatao unos. Nebroj u TVS_SETTINGS: poruka „Unesite broj“ i dijalog ostaje otvoren.
- **Svojstva, Odustani:** ništa se ne mijenja (ni ugao koljena).
- **Svojstva, elementi na zaključanom sloju:** na kraju poruka „Nije promijenjeno N elemenata (zaključan sloj)“ (ranije tiho preskočeno).
- **Menadžer blokova / Menadžer svojstava:** otvaranje i zatvaranje kao ranije.

## Faza 4 pregleda (stari kod crtanja i spajanja) – testirati
- **TVS_PIPE, TVS_DRAW:** crtanje kao ranije, sa Dimenzija / Koljeno / Račva / Tkomad / Z. Na zaključanom trenutnom sloju komanda odmah javi i ne počinje. Promjena dimenzije tokom crtanja u crtežu u cm ili m: prvi komad iza prelaza je 100 mm (ranije 100 jedinica = 1 m / 100 m).
- **Koljeno tokom crtanja (opcija K):** klik na nešto što nije kanal ponovo traži izbor (ranije se komanda „zaglavila“); izbor kanala koji se crta – poruka.
- **TVS_CONNECTW / TVS_CONNECTT:** spajanje kao ranije (T komad, koljeno, kolinearni sa prelazom). Kanal na zaključanom sloju ili zaključan trenutni sloj – poruka umjesto pada.
- **TVS_CUT:** jedan rez; novi dio ima sve iz originala (protok, kotu, flex, sloj, boju, TVS svojstva / oznake). Najmanje 50 mm do kraja. Član etaža / obilaska se ne može presjeći (poruka). Klik pored kanala – ponovo izbor; Esc izlazi.
- **TVS_ERASE (novo ime TVS_RAZBIJ, staro radi):** razbija TVS elemente u prostor u kojem su (i u Layout-u); ako razbijanje ne uspije, element ostaje (ranije bi bio obrisan). Na kraju ispis koliko je razbijeno / preskočeno.

## Novo – testirati
- **Štucna 45° (ASHRAE SR5-3):** opcija `Stucna` u TVS_CONNECTW, pri spajanju tokom crtanja, u opciji Tkomad i u TVS_TK_TIP. L = 0,25 × širina ogranka, min. 75 mm; kosi ulaz uzvodno (oKreni ga prebacuje); glavni kanal se crta neprekinut.
- **TVS_ERASE:** TK-O, TK-G, štucna i bočno poravnati prelazi se razbijaju u nacrtani oblik.
- **Etaž prati kotu kanala:** promjena kote kanala uz etaž (Properties, Svojstva, TVS_MATCH) pita „Prilagoditi etaž? [Da/Ne]“. Ugao, radijus i dimenzije ostaju; mijenja se dužina etaža u osnovi, a promijenjeni kanal prati koljeno (komanda TVS_ETAZ_KOTA se pokreće sama). Obilazak (TVS_BYPASS) se ne mijenja ovim putem.
- **Stil oznaka:** TVS_MleaderStyle iz Resources\TVS_Oznake.dwg se učitava sam.

- **TVS_STRETCH i etaž:** etaž (TVS_OFFSET) se pomjera cijeli kad ga okvir obuhvati; ako ga okvir presijeca, komanda to javi. Susjedni kanali se produžavaju/skraćuju.
- **Maskiranje:** zadana vrijednost za nove elemente je 1 mm (umjesto 50).

- **Dimenzije na spojevima prelaz–kanal:** promjena dimenzije kanala (Svojstva, dvoklik, Properties) mijenja dimenziju prelaza na tom kraju; promjena dimenzije prelaza mijenja kanal na tom kraju (i dalje do sljedećeg prelaza na drugom kraju tog kanala). Komande TVS (crtanje, TVS_MATCH ...) ovo ne pokreću. Pokreće se samo (TVS_DIM_PRATI).
- **TVS_IZVOZ (Izvoz bez TVS):** pravi kopiju crteža u kojoj su TVS elementi zamijenjeni linijama; otvoreni crtež ostaje netaknut. Elementi na zamrznutim slojevima ostaju TVS objekti (komanda javi broj).

- **Kota u dijalogu dimenzija tokom crtanja → etaž:** kad u TVS_DRAW/TVS_PIPE opcijom Dimenzija promijeniš kotu, otvara se prošireni dijalog etaža sa predloženom novom kotom (U redu = etaž, Otkaži = bez etaža); etaž je na postojećoj dimenziji; ako se mijenja i dimenzija, poslije etaža ide prelaz, a nova kota (Vrh/Osa/Dno iz dijaloga) važi za novi kanal. Ne: kao do sada.

- **Prošireni dijalog Z (etaž):** opcija Z / TVS_OFFSET: postojeći vrh, osa i dno kanala; novo stanje biraš kao Vrh/Osa/Dno i upisuješ vrijednost (ostale dvije se prikazuju); ugao 30/45/60 ili upisan; odmah razlika kota, kosi dio i dužina etaža u osnovi ili razlog zašto nije izvodljivo; skica etaža u presjeku. U redu ne prolazi dok etaž nije izvodljiv.

- **TVS_STRETCH i obilazak (TVS_BYPASS):** kad okvir presijeca kanal prije i poslije obilaska i obuhvata cijeli obilazak, obilazak se pomjera cio (7 članova) duž kanala; ulazni kanal se produži, izlazni skrati. Ako okvir presijeca obilazak, komanda to javi. Prepreka se ne pomjera.
- **TVS_MOVE i etaž/obilazak:** bočno pomjeranje kanala u liniji sa etažom ili obilaskom pomjera i etaž/obilazak cio; kanali iza koljena se produže/skrate. Kanal koji je dio etaža ili obilaska se ne bira (komanda javi).
- **Oznake (MLeader)** na članovima pomjerenog etaža/obilaska idu zajedno sa njima.

- **Štucna 45° za okrugli ogranak na pravougaonom kanalu:** TVS_CONNECTW (i spajanje tokom crtanja) pita „Tip T komada, okrugli ogranak [Stucna/Ravni]“; TVS_TK_TIP na takvom T komadu nudi Stucna/Ravni/oKreni. Ista pravila kao SR5-3: L = 0,25 × D, min. 75 mm; kosi ulaz uzvodno. U osnovi se crta kao pravougaona štucna širine D. Specifikacija: „Štucna 45°, okrugli ogranak“ A×B / ØD (L=). Osa okruglog kanala mora biti na koti ose pravougaonog: inače TVS_CONNECTW (sa štucnom) javi dijalog i ne pravi spoj, a TVS_STRETCH/TVS_MOVE javi dijalog sa obje kote.

- **Uzdužni presjek (P2):** TVS_PRESJEK → opcija **Uzduzni**: klikni prvi i zadnji element trase; program nađe put kroz mrežu (i kroz T komad), istakne trasu i razvije je u ravan (ista razmjera po dužini i visini). Crta: kanale, kose dijelove i koljena gore/dolje Z prelaza i obilaska, koljena u osnovi i T komade na svojoj koti (oznaka „koljeno“, „T komad“, „prelaz“), prelaze kao trapez, presječene kanale drugih trasa (pravougaonik sa X / krug, dimenzija, kota dna), linije etaža (pita kote, kao P1). Ispod: tabela Stacionaža [m] / Kota dna [m] / Dimenzija. Blok TVS_UZDUZNI_<oznaka>; **Osvjezi** ga ponovo generiše; u osnovi oznaka slova na oba kraja trase.

- **oKreni uz prikaz:** pri spajanju sa štucnom / TK-O / TK-G (TVS_CONNECTW, spajanje tokom crtanja) i u TVS_TK_TIP T komad je nacrtan i istaknut prije pitanja „[oKreni] <Gotovo>“; oKreni se može ponavljati dok se ne pritisne Enter.

- **Dimenzija cijele dionice do prelaza (plan 14, faza 1):** promijeni dimenziju kanala (Svojstva / Properties) u dionici sa koljenima u osnovi; program pita „Promijeniti cijelu dionicu do prelaza (koljena N, kanala M)? [Da/Ne] <Da>“. Na Da: koljena dobiju novu dimenziju i radijus, kanali iste stare dimenzije novu dimenziju, krajevi kanala prate priključke koljena; prelaz na kraju preuzme dimenziju kao do sada. Staje na obilasku, TK-G, koljenu gore/dolje, elementu druge dimenzije (ispiše gdje). Ako bi kanal postao prekratak – ništa se ne mijenja.
  - **Faza 2 – T komad:** dionica ulazi u ogranak (TK0, štucna, TK-O) → ogranak dobije novu dimenziju (štucna novu L, TK-O novi radijus), dionica staje. Ulazi u glavni kanal TK0 / štucne → pitanje „[Nastavi iza T komada/Prelaz iza T komada/Stani] <Nastavi>“ (Prelaz: iza T komada se ubaci prelaz na staru dimenziju). TK-O na glavnom: mijenja se samo ta strana (A ili C). Kanal na ogranku prati pomjereni priključak.
  - **Faza 3 – obilazak:** „Dionica prolazi kroz obilazak: preračunati ga za novu dimenziju (otvara se dijalog obilaska)? [Da/Ne] <Da>“ → dionica ide i iza obilaska; na kraju se otvara dijalog obilaska sa novom dimenzijom, prvobitnom pravom trasom, preprekom na trenutnom mjestu i postavkama postojećeg obilaska (ugao, strana, razmak, izolacija, rezerva). Prihvati: stari obilazak se uklanja, ulaz i izlaz spajaju u pravu trasu i upisuje se novi (kao TVS_BYPASS). Odustani / nije moguće: obilazak ostaje star (poruka; U vraća sve).
  - **Faza 2 – etaž:** „Dionica prolazi kroz etaž: preračunati ga za novu dimenziju? [Da/Ne] <Da>“ → etaž se ponovo napravi sa istom razlikom kota, uglom i radijusom; susjedni kanali prate nove priključke. Ako sa novom visinom ne staje – poruka, ništa se ne mijenja.

- **MIRROR fitinga:** koljena u osnovi i prelazi poravnati uz jednu stranu se ispravno preslikavaju (koljeno obrne luk, prelaz zamijeni stranu); T komadi i kanali kao i do sada. Etaž i obilazak se i dalje ne mogu preslikati (poruka) – samo pomjeriti / rotirati.

- **TVS_LEAD_Recalculate:** ako su objekti već izabrani – ažurira njih; inače „TVS oznake za ažuriranje [Sve/Izbor] <Izbor>“: **Sve** = sve MLeader oznake u crtežu, **Izbor** = običan izbor (klik, prozor, crossing; ostali objekti se preskaču). Na kraju: „Ažurirano TVS oznaka: N; nije ažurirano: M (element obrisan ili oznaka na zaključanom sloju)“.

- **Automatsko ažuriranje oznaka:** poslije izmjene TVS elementa (Properties, Svojstva, TVS_STRETCH, TVS_MOVE, MOVE, MIRROR, grip, dimenzija dionice) tekst njegovih TVS oznaka u trenutnom prostoru se sam ažurira (TVS_DIM_PRATI). **TVS_LEAD_AUTO [Uklj/Isklj]** – uključi / isključi (pamti se u AutoCAD profilu; zadano Uklj). Pod LISP-om / skriptom oznake se ažuriraju poslije sljedeće obične komande.

- **TVS_NATPIS** (nova ikona uz TVS_KOTE): uključuje / isključuje u crtežu natpis na kanalima `400x200/1500/5.21` (dimenzija / protok m³/h / brzina m/s; okrugli `Ø250/…`; bez protoka samo dimenzija). romans.shx, faktor širine 0,7, visina 30 mm (preračun za druge jedinice), boja 8, iznad sredine kanala, čita se odozdo / s desna. Nikad se ne štampa. Natpis (i kote TVS_KOTE) je uvijek tanak (0,09 mm), bez obzira na debljinu lejera. Oblačić na kanalu: prvi red kote, drugi red dimenzija (uvijek), protok i brzina (kad ima protoka).
- **TVS_SETTINGS**: nove kvačice „Automatsko ažuriranje TVS oznaka” (= TVS_LEAD_AUTO, pamti se u AutoCAD-u) i „Natpis kanala” (= TVS_NATPIS, po crtežu, REGENALL pri promjeni). Komande ostaju. Dijalog je nešto viši.
- **Spiro prelazi (SOKO)**: TVS_SETTINGS → „Spiro prelazi po katalogu SOKO”, „Gumena zaptivka (RG / RAG)”, „Ravni dio do sićke, mm” (25), „Priključak u kanalu isprekidano” – po crtežu. Okrugli prelaz Ø400→Ø300 pri crtanju: dužina 350 mm (RG: 300). Izgled: ravno 25, konus, ravno 25, dvostruka linija sićke, isprekidan priključak (55 / 30 mm u kanalu). Oblačić: „Spiro R-400-300, L = 430 mm, u crtezu 350 mm”, nestandardna kombinacija napomenuta. Promjena prečnika kanala uz prelaz (dimenzija dionice) → prelaz dobija katalošku dužinu, kanal na tom kraju se skrati / produži. Postojeći prelazi zadržavaju dužinu, mijenja se samo izgled. Flex i pravougaoni bez promjene.
- **TVS_ALIGN** (ikona „Poravnanje”): (1) slobodan kanal koso → LINE: okrene se i legne na liniju (osa / izabrana / druga ivica), kota ista; (2) odmak 100, strana kursorom lijevo / desno; Enter = trenutna strana; (3) kanal skoro suprotnog smjera – ne smije se okrenuti za 180°; (4) koljeno, T komad (glavni prolaz i ogranak), prelaz – priključak najbliži kliku; (5) povezan kanal između koljena paralelan liniji → pomjeri se, susjedi se produže / skrate kao u TVS_MOVE; neparalelan → poruka; (6) povezan T komad: osa ogranka paralelna liniji → T klizi duž kanala, kanali lijevo / desno se skrate / produže; povezano koljeno → pomjeri se kao u TVS_MOVE; neparalelno → poruka; etaž, obilazak, flex → poruka, bez izmjene; (7) segment polilinije, luk (odbija), XLINE, RAY, kosa 3D linija (odbija); (8) Esc u svakom koraku, Undo / Redo (jedan korak), zaključan sloj, oznaka prati element; (9) cm / m crtež i rotiran UCS; (10) TVS_MOVE i TVS_STRETCH rade kao prije.
- **TVS_TK_TIP (dijalog)**: (1) TK-O na pravougaonom T komadu: ogranak B uži / širi, prolaz C – T komad se mijenja, kanali ostaju svoje dimenzije, a na priključku se umeće prelaz; (2) visina ogranka različita od ulaza → postaje visina ulaza, kanal dobija prelaz; (3) „Kanal u nastavku”: promjena na okrugli Ø → prelaz pravougaoni → okrugli; okrugli T komad (štucna) → okrugli kanal drugog Ø → spiro dužina po katalogu (polje sivo); (4) TK-G: oba ogranka, visine = ulaz, gaćasti nemoguć → poruka i U redu sivo; (5) Okreni T komad (skica se ogleda), kanali ostaju na svojim mjestima; (6) Otkaži – ništa; Undo – jedan korak; (7) promjena dimenzije kanala na ogranku TK-O (Svojstva / dimenzija dionice) → T komad ostaje, umetne se prelaz; vraćanje na dimenziju priključka → prelaz se uklanja; (8) štucna i TK0 – B slobodno. (9) izgled po mockupu: lijevo Na T komadu / Odmah iza priključka / Kanal u nastavku, desno skica; (10) koljeno 30/45/60/90 desno / lijevo na slobodnom priključku i na kanalu sa slobodnim krajem (kanal ide iza koljena), redoslijed koljeno → prelaz / prelaz → koljeno; gore / dolje samo na slobodnom priključku; (11) oblik ogranka (štucna / TK0) pravougaoni ↔ okrugli; (12) dvoklik i TVS_CHANGE na jedan T komad otvaraju ovaj dijalog.
- **Konektori na fitinzima**: izaberi koljeno / T komad / prelaz – na svakom SLOBODNOM priključku crveni grip malo izvan njega (na spojenim nema). Klik na grip → „Kraj prvog kanala”: kanal ide u pravcu priključka (tačka sa strane se projektuje na pravac), dimenzija = priključak (TK-O ogranak: širina ogranka; prelaz: dimenzija tog kraja, okrugli kraj → okrugli kanal), kota fitinga; dalje crtanje kao obično (koljena se dodaju sama). Opcije prije prvog kanala: [Dimenzija] – druga dimenzija / oblik → prelaz odmah na priključku (spiro po katalogu), pa kanal te dimenzije; [Koljeno] – 30/45/60/90 lijevo / desno na priključku, kanal izlazi iz koljena; mogu se kombinovati (npr. koljeno pa dimenzija). Obični gripovi fitinga (pomjeranje) rade kao prije. Grip kanala i obični TVS_Draw rade kao prije.
- **TVS_PRESJEK (dijalog)**: dvije tačke + klik na stranu gledanja (dubina se NE vuče mišem – vrijednost „Dubina pogleda” iz dijaloga, pamti se, početno 0) → dijalog: pregled pokazuje presječene kanale između poda i gornje etaže; kvačica „Spušteni plafon, od vrha” 0,60 → isprekidana linija u pregledu i u presjeku sa kotom „SP +2.600”; Razmjera mijenja visinu teksta (napomena u kartici); Dubina 0 → zaklonjeni sivo; postojeća oznaka → crvena poruka, Umetni sivo; „Osvježi postojeći” → izbor presjeka; tab Uzdužni → izbor prvog / zadnjeg elementa trase pa isti dijalog (bez dubine); Osvjezi starog presjeka bez SP radi kao prije.
- **KGTEZ:** u dijalogu polje „Razmjera crteza 1:“ (zadano 50 ili zadnja razmjera u crtežu) – razmjera se više ne pita u komandnoj liniji.

## Odloženo
- Koljeno direktno na priključku grane T komada.
- Detalji koljena u presjeku (kao TVS koljena gore/dolje).
- Finese nastavka crtanja poslije umetanja T komada.
