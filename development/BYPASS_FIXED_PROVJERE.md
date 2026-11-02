# razvoj-bypass-fixed — integritet bypass-a i praktične provjere

Osnova: `razvoj-bypass-candidate` @ a352072. Bez novog UI ponašanja, bez promjene
geometrijskog algoritma, bez spajanja u produkciju. Ranija UX dorada (pamćenje unosa,
Ri predlog, DPI, treptanje, Prihvati iz pregleda) parkirana je na grani
`razvoj-bypass-ux-deferred` i nije dio ove isporuke.

## 1. Konektori — stanja

Činjenica iz koda: `TVS_Pipe::dwgOutFields/dwgInFields` ne snimaju konektore
(`writeConnectors/readConnectors` su zakomentarisani). Undo filer i DWG/DXF filer
zato nikada ne vraćaju `connectors`. Jedini legacy proizvođač veze je `TVS_CONNECTTest`;
crtanje (TVS_PIPE/TVS_DRAW) pravi par nepovezanih konektora, a ponovo otvoren kanal
ima prazan vektor konektora.

| Stanje | Prije (a352072) | Sada |
|---|---|---|
| Prije bypass-a | povezan spoljašnji kraj moguć samo poslije TVS_CONNECTTest/ranijeg DEV bypass-a | isto |
| Poslije upisa | susjed → novi ostatak; izvor očišćen, tačka = rez | nijedan konektor se ne mijenja; povezan spoljašnji kraj se **odbija** `[konektori]` |
| Poslije korisnikovog Undo | susjed ostaje na obrisanom ostatku; izvor ostaje očišćen (izvedeno iz filing koda) | nema razlike u odnosu na stanje prije, jer ništa nefiled nije mijenjano |
| Poslije save/reopen | svi konektori prazni (legacy) | isto; roditelj (filed) čuva inlet/outlet/članove/granice |

`ConnectorRollback` je uklonjen: pokrivao je samo abort transakcije, ne Undo.
Posljedica ograničenja: kanal čiji je izabrani kraj eksplicitno povezan mora se
prvo razvezati/ponovo nacrtati; nepovezan kraj radi kao i prije. Fiksni kraj i
njegova postojeća veza ostaju netaknuti.

## 2. Unutrašnja povezanost — potrošači

- **Konektori (samo sesija):** `TVS_Pipe` centralni grip → `correctAnother` pomjera
  povezanog susjeda; offset/bypass `topology()` provjerava recipročnost. Članovi i
  novi ostatak imaju nepovezane konektore, pa centralni grip ne prenosi pomjeranje kroz
  sklop. Konektori nisu autoritativni ni za šta trajno.
- **Geometrija:** TVS_MOVE mreža (`tvsMoveNetwork`) i `topology()` nalaze kontakte po
  krajevima/osama; portovi članova se geometrijski poklapaju (`validate()`).
- **Sklop (filed, autoritativan):** roditelj + dvije V1 trojke. Koriste ga `pinned()`,
  `validate()`, COPY/WBLOCK zabrana i `tvsOffsetData::marked` (uključuje bypass), pa
  TVS_MOVE `editable()` odbija članove.
- **Obračuni:** TVS_SPEC, Excel specifikacija i Lead recalculate rade nad izborom
  (`acedSSGet`) i ne čitaju konektore; KGTEZ je LISP nad izborom. Pojedinačne formule
  nisu ponovo pregledane ovim zadatkom.
- Nije uvedeno: automatsko popravljanje konektora iz Undo/Redo callback-a.

## 3. Dijagnostika writer-a

Svaki odbijeni upis: `TVS BYPASS [faza] razlog (ErrorStatus N: tekst) Nema izmjena.`
Faze: `snapshot` (posebno: granica postojećeg sklopa / promijenjen kanal / promijenjena
prepreka), `duzina`, `konektori`, `atributi`, `append`, `metapodaci`, `commit`, `DEV`.
ErrorStatus se navodi gdje ga SDK vraća (otvaranje, append, V1/roditelj, commit).

## 4. Ciljane izmjene

- Parent writer provjerava `acdbGetAdsName` za postojećeg susjeda i backlink roditelja.
  Nepostojeći susjed ostaje 330 sa nultim imenom. Kako se sada nijedan spoljašnji kraj
  ne prevezuje, **gotovo svaki bypass upisuje bar jednu nultu 330** — praktična proba
  ispod je zato obavezna.
- `Umetni detalj…` u TVS_BYPASS i Obilazak: dijalog se vraća sa istim kandidatom;
  detalj ostaje nepovezan, Prihvati ponovo provjerava snapshot.
- `inject()` postoji samo uz `TVS_BYPASS_FAULT_INJECTION` (samo TVSBypassPreview paket).
- Nastavak crtanja čita krajeve novog ostatka kratkim otvaranjem po ObjectId.

## Razdvajanje nalaza

**Potvrđeno iz koda (i regresijom gdje je moguće):** nefiled konektori i prevezivanje
koje Undo ne vraća; neprovjeren `acdbGetAdsName`; detalj prekida prihvatanje; generička
poruka odbijanja.

**Rizici iz pregleda, nisu mijenjani:** legacy grip izmjena pojedinog člana nije
blokirana (postojeći V1 rizik); centralni grip ostatka ne vuče geometrijskog susjeda
(nije ni ranije bez eksplicitne veze).

**Native testovi koji NISU izvršeni:** AutoCAD runtime, korisnički Undo/Redo, save/reopen,
nulta 330 u stvarnom Xrecord-u. Regresije koriste SDK zamjene; CI samo kompajlira.

## Praktična provjera (svježa AutoCAD sesija, DEV paket 2020 ili 2022)

1. Nacrtati kanal kroz prepreku (bez TVS_CONNECTTest). `TVS_BYPASS` → Pronađi →
   Prihvati. Očekivano: „BYPASS upisan“.
2. `U` (Undo). Očekivano: kanal vraćen, sedam članova nestaje. Zatim TVS_OFFSET ili
   TVS_BYPASS na istom kanalu mora raditi bez poruke o vezi.
3. Ponoviti 1, `SAVE`, zatvoriti, otvoriti. Na članu `LIST`/Properties bez greške;
   novi `TVS_BYPASS` na nepovezanom kanalu prolazi; na granici sklopa poruka `[snapshot]`.
4. Kanal bez ijednog susjeda: 1 + save/reopen (nulta 330 u oba polja susjeda).
5. TVS_CONNECTTest između dva kanala, pa TVS_BYPASS na povezanom kraju. Očekivano:
   `[konektori]`, bez izmjena; isti kraj nepovezan (drugi smjer) prolazi.
6. Obilazak tokom TVS_PIPE: Umetni detalj → dijalog se vraća → Prihvati → sljedeća tačka
   nastavlja sa spoljašnjeg kraja.
7. Rollback proba: `TVS_BYPASS_FAIL=1/2/3` prije pokretanja AutoCAD-a; očekivano
   `[DEV] Proba prekida N`, bez novih objekata. Ukloniti varijablu i restartovati.

Za svaku stavku zapisati verziju AutoCAD-a i tačan tekst poruke.

## UX dorada dijaloga (na fixed osnovi)

Selektivno prenesen UI dio sa `razvoj-bypass-ux-deferred`; writer i servis ostaju sa
fixed grane.

- **Pamćenje:** posljednja potvrđena projektna podešavanja (izolacije, razmak, rezerva,
  minimalne dužine, ugao, strana, napredni budžeti) čuvaju se u `CDocData` — samo za
  taj crtež i samo dok je otvoren; ništa se ne upisuje u DWG. Ri, prozor s i granice Z
  se ne pamte; dimenzije, kote, smjer i jedinice uvijek dolaze iz novog izbora.
  Zapamćene izolacije su u zaglavlju označene kao prijedlog.
- **Ri:** prijedlog po aktivnom TVS režimu radijusa pravougaonog koljena (konstantni,
  k*B, specifični); inače Rc = 1,0 × B, Ri = B/2. Ručno upisan Ri se ne prepisuje
  (ni poslije promjene jedinica). Izvor prijedloga piše u zaglavlju.
- **Čitljivost/treptanje:** veličina prati DPI monitora; presjek se crta van ekrana i
  osvježava samo on; kursor čekanja tokom računanja.
- **Pregled:** `[Prihvati/Dijalog/Odustani]` — Prihvati koristi isti writer i iste
  snapshot provjere; preview-only komanda nema Prihvati.

Praktične UX provjere (nastavak liste iznad):

8. Dva otvorena crteža: računati u A, otvoriti dijalog u B — B nema prijedloge iz A.
9. U istom crtežu drugi kanal druge dimenzije: izolacije predložene, Ri/prozor/kote nove.
10. TVS_SETTINGS: promijeniti režim radijusa (konstantni / k*B) → novi Ri prijedlog;
    ručno upisan Ri ostaje i poslije promjene jedinica.
11. Pregled → Enter → dijalog sa istim rezultatom; Pregled → Prihvati → „BYPASS upisan“;
    Pregled → Odustani / Esc → bez upisa i bez zaostale privremene grafike.
12. Između računanja i Prihvati pomjeriti kanal/prepreku (u drugom pokušaju) →
    `[snapshot]` odbijanje, bez izmjena.
13. 125 % i 150 % skaliranje: dijalog čitljiv, bez treptanja pri kucanju.

## Native nalaz AutoCAD 2022 (paket d444b5d)

Korisnička proba: `Pregled → Prihvati` →
`TVS BYPASS [metapodaci] Upis roditelja/backlink-ova nije uspio. (ErrorStatus 31: eInvalidAdsName)`.
Uzrok: `AcDbXrecord::setFromRbChain` odbija 330 sa nultim ads imenom — upravo slučaj
kanala bez susjeda (sumnja iz pregleda je potvrđena). Ispravka bez novih podataka sklopa:
nepostojeći susjed u istom V1 slotu čuva inlet kao rezervisanu referencu sa portom 0;
`read()` port 0 tumači kao „nema susjeda“. Raniji DEV roditelji sa nultom 330 ne postoje,
jer takav upis nikad nije uspio. Regresija sada modeluje ovo SDK ponašanje.
