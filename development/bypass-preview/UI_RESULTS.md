# DEV UI v2 — izvršene provjere i isporuka

## Korisnički test

1. novembra 2026. korisnik je prijavio praktičnu probu nove promjene Z i BYPASS preview-a: osnovno ponašanje djeluje ispravno. To je prvi korisnički funkcionalni test. Snimak pokazuje konkretan primjer u mm, ne kompletnu AutoCAD matricu. Verzija AutoCAD-a i ostali režimi nijesu potvrđeni ovim izvještajem.

Nova v2 reorganizacija/kotiranje nije praktično testirana u AutoCAD-u od strane agenta. Nema novih stvarnih AutoCAD snimaka. Prethodni korisnički prolaz ne prenosi se automatski na ovu reviziju.

## Promjene

- Osnovni panel čuva projektantske ulaze; Napredno / dijagnostika otvara sve ranije parametre, validaciju i detaljan ispis. Prikazivanje/skrivanje grupe, Kote/Ose i režim osnove ne mijenjaju numeričke ulaze.
- Presjek u osnovnom panelu ima 710 umjesto 590 projektnih piksela širine. Kotni tekstovi imaju odvojene pojaseve; W/C/D kotne linije i početna/završna osa su direktno uz presjek, ostale mjere u trokolonskoj legendi. Aktivne granice su ljubičaste, kritični geometrijski razmak zelen sa krajnjim kružićima. Ista razmjera s/z je sačuvana.
- Sažetak ima 1–2 decimale; interval razmaka se proširuje za prikaz, a u dijagnostici se čuva 17 značajnih cifara njegovih krajeva. Zaokruženje ne utiče na status ili prihvatanje.
- Dijalog se smješta u radnu površinu monitora AutoCAD-a i skalira kontrole/tekst. Stvarni DPI, fontovi, mali ekrani i višemonitorski prelazi ostaju za praktičnu provjeru.

## Režim vidljivosti — jasno ograničen

Isporučena je odobrena jednostavnija varijanta: **Konture / Sve ivice — dijagnostika, bez zaklanjanja**. Nije implementiran tačan hidden-line algoritam, ni A. samozaklanjanje, ni B. zaklanjanje preprekom. Nema testa/dokaza dijeljenja vidljivih/skrivenih odsječaka pri ulasku/izlasku iz zone zaklanjanja. Fizičke kote nijesu zamijenjene crtačkim Z, a nije uvedena heuristika da se cijeli niži kanal crta isprekidano.

Pune linije: projektovane konture. Cyan crtice: simbolički spojevi. Sive tačkaste: pomoćni gabariti pojedinih dijelova, samo u Sve ivice. Dugo–kratko: ose. Iscrtavanje fizičkih kontura je posljednje i uklanja tačno poklopljene duplikate pomoćnih segmenata. Taj test fizička/pomoćna linija nije dokaz hidden-line prikaza. Unutrašnje ćelije teselacije se ne prikazuju kao fizičke ivice.

Originalna trasa ostaje neizmijenjena i vidljiva ispod preview-a. Nema pisanja/brisanja DWG entiteta radi prikaza, trajnog umetanja, nove šeme, izmjene changeZ(), solvera ili generatora.

## Rezultati

Konačni izvršni izvor: `6054343af0b808ec7bd639984be4e19c304a239d`. CI checkout merge `622643f4d7f222d546679eaf16907f585c81641e` ima identično stablo (poređenje: 0 promijenjenih datoteka); PR nije spojen.

| Provjera | Stvarni rezultat |
|---|---|
| Lokalni GCC, cijele regresije | 21 test, 23.567 s, OK |
| [CI GCC 13.3 / C++11](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636508) | 21 test, 17.956 s, OK |
| [CI v141](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636698/job/124824770440) | 5 test-metoda, 32.413 s, OK |
| [CI v142](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636698/job/124824770381) | 5 test-metoda, 34.184 s, OK |
| [DEV AutoCAD 2020 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636551) | Build/link succeeded, 0 errors, 35.75 s |
| [DEV AutoCAD 2022 x64](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636513) | Build/link succeeded, 0 errors, 52.84 s |

Broj 11692 provjera u novoj metodi prolazi na sva tri kompajlera. Buildovi imaju prethodna SDK LNK4099 upozorenja o rxapi.pdb; nijesu nova greška. [Sačuvani CI izlazi](results-ui-v2/) uključuju stvarne compile opcije, rezultate i ograničenja.

Automatizovana test-metoda sadrži 11692 provjere: 604 adapter/presentation/bridge provjere i 11088 provjera pravougaonih slotova kotnog rasporeda. Ovi brojevi nijesu broj različitih geometrijskih kandidata niti potvrda stvarnih Windows fontova. Layout matrica pokriva 72 kombinacije skala .65/.8/1/1.25/1.5/2 po X/Y i osnovni/napredni panel. Provjerava da tekstualni slotovi ne ulaze u geometriju, jedni u druge ili van panela. Kote=off nema stare slotove.

Prikazna matrica: iznad/ispod × 30/45/60° × smjer0/37° × izolacija0/25mm; kritični par, ose, fizičke i projektovane dužine iz checked Result-a. Referentni fiksni primjer sa snimka: 400×250/250×250, obje ose0, Ri100,45°, izolacije0, razmak50,rezerva5 daje D315,C360,W1362.792206,Zprolaza−315 i zazor oko65. Rezultati nijesu hardkodirani u dijalog.

Postojeće 452 Fraction,70 checker,30 FENV,274 generator provjere i stare regresije ostaju uključene. Strogi FP uslovi nijesu ublaženi. MSVC /W4 /WX je otkrio zasjenjivanje lokalne promjenljive p u novoj pripremi projekcije; ispravljeno je preimenovanjem vertex, bez gašenja upozorenja ili promjene proračuna.

## Build i paketi

- [AutoCAD 2020 — DEV UI v2](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636551/artifacts/13080127926): 533175 B, SHA-256 `ad492576e170281f5c4e40f6acd229a88042bd50f97fbbeed976c1b61da425f3`.
- [AutoCAD 2022 — DEV UI v2](https://github.com/vstijov-boop/tvs-vent/actions/runs/41298636513/artifacts/13081005021): 525140 B, SHA-256 `86e266b03b732bc6f52b31f127277fed10b73564683cf7a1baf3456ca8519ee5`.

ZIP-ovi su preuzeti; hash-evi su provjereni prema CI metapodacima. BUILD.txt potvrđuje checkout i cilj; ARX/DBX su stvarno provjereni kao PE x64. Binarne veličine 2020: ARX921088/DBX427008 B; 2022: ARX877056/DBX449024 B. U ZIP-u su GUIDE.md i UI_READABILITY.md. Prethodni korisnički DEV ZIP-ovi nijesu prepisani.

Koriste se isti DEV workflow-i, toolchain-i v141/ObjectARX2020/Windows SDK10.0.17763 i v142/ObjectARX2022/Windows SDK10.0 (runner10.0.26100), Release x64. Tačne komande ostaju:

```powershell
& $env:TVS_MSBUILD 'TVS_Ventilation 2020.sln' /m /t:Build /p:TVSBypassPreview=true /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0.17763.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
& msbuild 'TVS_Ventilation 2022.sln' /m /t:Build /p:TVSBypassPreview=true /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
```

Lokalne regresije: `python3 -m unittest discover -s tests -p 'test_*.py' -v`; Windows: tests/run_bypass_msvc.py, TVS_BYPASS_TOOLSET=v141/v142. FP ostaje /fp:strict samo u numeričkom bridge-u; UI/ostatak projekta zadržavaju ranije postavke. PCH/UTF-8 konfiguracija iz prethodne DEV faze je sačuvana.

## Commit-i

- [d222c73](https://github.com/vstijov-boop/tvs-vent/commit/d222c73dc08e21cc0443c020ca6d4ef5cda8432d) — UI A: checked podaci i klase linija; korisnički regresioni primjer
- [d65d7a9](https://github.com/vstijov-boop/tvs-vent/commit/d65d7a94672614dd8b4bc0f9f05d8112098526b9) — UI B: osnovni/napredni panel, kotiranje i dijagnostička osnova
- [df1bc63](https://github.com/vstijov-boop/tvs-vent/commit/df1bc6336af8a20a122232366e84d836977998e0) — UI C: veći presjek i testirani slotovi kotnih pojaseva
- [d736599](https://github.com/vstijov-boop/tvs-vent/commit/d736599445b8871bad119b6b68c9ac285d1434b8) — UI D: puni intervali i radna površina AutoCAD monitora
- [6054343](https://github.com/vstijov-boop/tvs-vent/commit/6054343af0b808ec7bd639984be4e19c304a239d) — UI E: MSVC zasjenjivanje lokalne promjenljive; konačni izvršni izvor
- Sljedeći dokumentacioni commit sadrži samo završni izvještaj i CI izlaze.

Izmjene su ograničene na PreviewModel/novi PreviewPresentation, izvoz checked podataka iz numeričkog bridge-a, dijalog/transient prikaz, testove i DEV pakovanje dokumentacije. Postojeći OffsetGeometry, OffsetAdapter, Candidate, Generator, DBX, specifikacija, COM i produkcioni workflow-i nijesu mijenjani. Draft PR #1 ostaje otvoren, bez merge-a u produkciju.

## Korisnički praktični pregled

U testnoj sesiji učitati odgovarajući DEV DBX pa ARX iz istog ZIP-a, bez drugog TVS para; pokrenuti TVS_BYPASS_PREVIEW. Koristiti [UI_READABILITY.md](UI_READABILITY.md) i [GUIDE.md](GUIDE.md): ponoviti primjer sa snimka, oba smjera/strane, tri ugla, izolacije i aktivne granice; Kote/Ose/Napredno; promjenu ulaza; Enter/Esc/Cancel; ponavljanje i REGEN; DPI i manje prozore; broj entiteta, DWGMOD, podatke referentnih objekata i stanje trase prije/poslije. Ove AutoCAD provjere nijesu izvršene u CI-u. Za zaklanjanje provjeriti samo da je režim jasno DIJAGNOSTIČKI — puna vidljivost nije implementirana niti označena kao prolaz.
