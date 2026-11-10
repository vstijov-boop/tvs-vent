# TVS_VENT – period testiranja (objedinjeno na master)

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
