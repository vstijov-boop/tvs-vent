# Razvojni dijalog i neperzistentni CAD preview

## Obim i granica potvrde

Posebna komanda **TVS_BYPASS_PREVIEW** postoji samo uz MSBuild /p:TVSBypassPreview=true i TVS_BYPASS_PREVIEW. Nema trajnog upisa obilaska, nove DWG šeme, upis-pa-brisanje, promjene specifikacije/COM-a ili povezivanja sa changeZ(). Grana razvoj-bypass-candidate i Draft PR #1 ostaju za pregled prema razvoj-offset-arx, bez merge-a u produkciju.

Testirani izvor: `1c60f0f2d436d61d26fc48cda08fac331a67a816`, 2026-11-01 UTC. Oba CI builda checkout-uju PR merge `ef88a892bf62992d523bc7a7b7f51aa62ad8b096`; poređenje potvrđuje isti sadržaj stabla (0 promijenjenih datoteka). To nije spajanje PR-a u ciljnu granu.

| Cilj | Toolchain / SDK / konfiguracija | Stvarni rezultat |
|---|---|---|
| AutoCAD 2020 x64 | v141 14.16.27023 / VS2017; ObjectARX 2020; Windows SDK 10.0.17763.0; Release x64 | [Run 41255987745](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987745): Build succeeded, 0 errors, 33.11 s |
| AutoCAD 2022 x64 | v142 14.29.30133 u VS2022; ObjectARX 2022; Windows SDK 10.0 / runner 10.0.26100.0; Release x64 | [Run 41255987781](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987781): Build succeeded, 0 errors, 67.43 s |

Oba builda imaju tri Autodesk LNK4099 upozorenja zbog nedostajućeg rxapi.pdb. ARX/DBX su kompajlirani, linkovani i zapakovani. [2020 izlaz](results/build-2020.txt), [2022 izlaz](results/build-2022.txt).

Učitavanje ARX/DBX, stvarni izgled Win32 dijaloga u AutoCAD-u, transient lifecycle i interaktivne probe **nijesu izvršeni** u ovom okruženju. Uspješan build i izolovani testovi nijesu AutoCAD runtime potvrda. Raniji proof testovi korisnika i neprovjerena DEV Z integracija nijesu korišćeni kao dokaz rada novog preview-a. Nema stvarno snimljenih AutoCAD screenshot-ova.

## Implementacija i izvori podataka

- **PreviewModel.h**: SDK-nezavisan snapshot/adaptiranje iz stvarnih osnih FirstPoint/LastPoint, A/B, get_Elevation, smjera, jedinica i crtačkog Z. Adapter ne dodaje A/2 ili B/2 osi. Fizička kota i crtačka ravan su odvojene.
- **BypassPreviewCommand.cpp**: read-only izbor dva različita direktna TVS_Pipe objekta u Model prostoru; kruti pravougaoni obični kanali, bez XREF/blokova i označenih etažiranja. Izričit izbor Prvi/Posljednji određuje smjer +s. UCS se ne primjenjuje drugi put na već WCS osne tačke.
- **BypassNumericBridge.cpp**: SDK/MFC-nezavisna compilation jedinica sa prihvaćenim neizmijenjenim Generator/solve/checker modulima. UI ne uključuje intervalne/checker inline definicije.
- Podaci iz objekta: ose, dimenzije, fizičke kote, smjer i crtačka ravan. Jedinice iz valjanog TVS zapisa se nude za potvrdu. Ako zapis nedostaje/ne može biti potvrđen, izbor jedinica ostaje nepopunjen. Ručni izbor je samo lokalni testni ulaz, bez pisanja TVS jedinica.
- Izolacije obaju kanala su početno prazne: potrebno ih je unijeti; nepoznato nije 0. Ri, clearance, dodatna rezerva, svi minimumi i razvojni budžeti imaju vidljiva polja i traže potvrdu. Vrijednosti 100/.1 mm su razvojni predlozi, ne standard ili novo proizvodno pravilo.
- Prazna opcionalna gornja/donja granica izričito znači neaktivna. Kada je uključena unosom broja, to je apsolutna fizička kota spoljnih gabarita, ne kote crtačke ravni.

**Lokalni prozor**: s=0 je presjek stvarnih osa u osnovi; +s ide od suprotnog ka izabranom aktivnom kraju trase. Početni predlog lijeve/desne granice dolazi sa krajeva izabrane trase, u fizičkim mm od presjeka, i može se ručno izmijeniti/potvrditi. Asimetričan prozor se ne centrira prinudno. Izabrana trasa ostaje vidljiva, ne skraćuje se i ne mijenja se. Presjek osa se računa u WCS XY; fizičke kote se čitaju odvojeno.

Adapter odbija kose/nevažeće ose: horizontalnost crtačke ose unutar .001 fizičkih mm, upravnost sa tolerancijom normalizovanog dot proizvoda 1e-10. To su razvojne numeričke tolerancije, ne formalni sertifikat mjerenja iz DWG-a. Puni poprečni preklop/end-margin provjerava prihvaćeni checker sa stvarnim krajevima prepreke, A i izolacijom trase te clearance-om. Širina prepreke duž s je A prepreke pri upravnom ukrštanju; B ostaje fizička visina.

Za stare nagibne međukomade bez autoritativnih visinskih zapisa, dvije ravanske tačke i jedna kota ne mogu dokazati fizičku horizontalnost. Koristiti obične horizontalne dionice; nova označena etažiranja se odbijaju. Ne migriraju se niti popravljaju stari objekti. Ogromne WCS koordinate i granična ortogonalnost nijesu praktično potvrđeni; numerički dokaz checkera se odnosi na lokalni model, ne sve CAD transformacije.

## Dijalog i rezultat

Dijalog ima iznad/ispod, 30/45/60°, Ri, izolacije, clearance, zasebnu projektnu rezervu, minimum nagiba/mosta/projekcije, prozor, opcione visinske granice i budžete. Jedinice i porijeklo podataka su označeni; geometrija iz objekata je read-only, ostalo ručno/predlog/nedostaje. UI se skalira prema radnoj površini; vrlo mali ekran i visoki DPI ostaju praktične provjere.

**Pronađi prolaz**: stopAtFirstFeasible=true. **Pokušaj kraći prolaz**: ograničeno poboljšavanje; prethodni potvrđeni bolji kandidat za iste ulaze se zadržava i ako nova ograničena pretraga ne donese bolji. Nema globalnog optimuma.

Promjena bilo kog kontrolisanog ulaza odmah briše važenje presjeka/rezultata i onemogućava pregled. Engine dodatno poredi cijeli numerički ključ: snapshot oba objekta, jedinice, smjer i sve opcije/budžete. Ni direktni poziv bridge-a sa novim podacima ne može operativno zadržati stari rezultat.

Found je prikazan kao **„Izvodljiv kandidat — minimalni zahvat nije potvrđen.“** NoCandidateFoundWithinBudget je **„Kandidat nije pronađen u zadatom budžetu.“**, bez zaključka da je obilazak nemoguć. NumericalBoundary/Unresolved se ne prikazuju kao prolaz. Ispisuju se kota ose prolaza, D/C/s0/W, fizička i projektovana dužina oba nagiba, dužina luka i mosta, ograda ostvarenog zazora, kritični član i par kritičnih tačaka, brojači, vrijeme i razlog završetka pretrage.

Dijagnostičke šifre članova iz postojećeg checkera: 0 Prvo koljeno; 1 Prvi nagib; 2 Drugo koljeno; 3 Most; 4 Treće koljeno; 5 Drugi nagib; 6 Četvrto koljeno; 7 Ulazni susjed; 8 Izlazni susjed. To nijesu brojevi trajnih DWG entiteta.

## Presjek i transient grafika

Oba prikaza dolaze iz istog **checked Result-a**, ne iz nezavisnog proračuna etažiranja. Vertikalni presjek u dijalogu koristi jednaku skalu s/z, stvarne kružne sektore i prave komade, lim i eksplicitnu izolaciju, prepreku, osu i segment kritičnog zazora. Kote, zahvat i dužine su u rezultatu uz presjek. Lukovi su samo za grafiku teselirani sa hordinom greškom do .05 mm; ne koristi se grafičko uzorkovanje za prihvatanje kandidata.

Osnova prikazuje projektovane gabarite sedam tijela i odgovarajuće spojne presjeke iz istih Body/port podataka. Projektovani dijelovi mogu se preklapati u osnovi; to je očekivano za različite fizičke kote i ne predstavlja dokaz kolizije. Nema skrivenolinijskog renderovanja, stvarnih TVS fiting entiteta ili oslanjanja na redraw cache.

**Pregled u crtežu** završava modalni dijalog, zatim dodaje isključivo AcGi transient drawables. Polyline objekti ostaju na heap-u bez dodavanja u block table, transakciju ili DWG. Vlasnik prvo rezerviše memoriju za sve pokazivače, briše transient registracije prije uništavanja i čisti djelimičan pregled pri grešci. Enter uklanja grafiku i vraća isti dijalog; Esc završava. Otkaži/zatvaranje nema trajne posljedice. Umetni je uvijek onemogućen: **„Razvojni pregled — upis nije implementiran.“**

## Floating-point i odziv

Numerika: samo BypassNumericBridge.cpp /fp:strict, bez /GL i bez precompiled header-a, /std:c++14 zbog MSVC selektora; izvor je C++11. Nova UI datoteka ima /utf-8 i svoj normalni include StdAfx.h bez korišćenja starog zajedničkog PCH-a. Numeric bridge takođe ima /utf-8. Globalni ARX/DBX floating-point flagovi i postojeći PCH ostaju isti; ostale datoteke i dalje koriste ranije /fp:precise i /GL gdje je konfiguracija tako određivala.

Provjereno numeričko okruženje se ne normalizuje. Bridge/adapter koriste postojeći RAII za čuvanje i vraćanje FENV-a; zaokruživanje različito od FE_TONEAREST i FTZ/DAZ se odbijaju. Ograde i prihvatanje ostaju iz postojećeg checkera.

Računanje je **sinhrono samo nakon tastera**; nema CAD API poziva iz radne niti jer nema radne niti. UI tokom računanja čeka i to izričito prikazuje. Rok je kooperativan između checker poziva, pa jedan aktivni poziv može prekoračiti rok. Ne obećava se trenutno otkazivanje usred njega. Javni UI limiti: do 10000 ms, 128 checker poziva, 1000000 distance ćelija; nivoi D/C/položaja, seed/proposal limiti, geometrijski koraci, max D, inset i odvojeni W/D pragovi su vidljivi. Postojeći checker dodatno koristi maxCells=30000 po pozivu, maxDepth=40, supportSlices=24, distanceTolerance=.001 mm i jointTolerance=.000001 mm. Globalni preostali ćelijski budžet dodatno ograničava svaki poziv.

## Izvršene provjere

| Izvršenje istog izvora | Stvarni rezultat |
|---|---|
| Lokalni Linux/GCC, cijele regresije | 21 test, 22.884 s, OK |
| [CI Linux/GCC 13.3, C++11](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987685) | 21 test, 27.879 s, OK |
| [CI Windows v141](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987651/job/124699137997) | 5 numeričkih testova, 39.464 s, OK |
| [CI Windows v142](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987651/job/124699138170) | 5 numeričkih testova, 38.591 s, OK |

Na GCC/v141/v142 izvršeno je po **433 novih adapter/revision/bridge provjera**, uz postojeće 452 nezavisne Fraction intervalne, 70 checker, 30 FENV i 274 generator provjere. Svih 17 prihvaćenih generator rješenja se nezavisno ponovo provjerava. Linux dodatno izvršava stare offset/masa/jedinice/oznake/tabele/edit regresije. GUI petlja i AutoCAD API nijesu zamijenjeni stubs modelom niti predstavljeni kao runtime testovi.

Sačuvani izlazi: [Linux](results/numeric-linux.txt), [v141](results/numeric-v141.txt), [v142](results/numeric-v142.txt).

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 -m unittest discover -s tests -p 'test_bypass_preview.py' -v
```

Windows koristi tests/run_bypass_msvc.py i TVS_BYPASS_TOOLSET=v141/v142; stvarno pokreće pet test-metoda test_bypass*.py. Nova test-metoda kompajlira originalni BypassNumericBridge.cpp, ne kopiju proračuna.

Testovi adaptera obuhvataju mm/cm/m; 0/37/90/217° i oba smjera; pomjereno WCS ishodište; A!=B; fizičku kotu -400 mm i nezavisan crtački Z=37 mm; lokalni prozor i poprečni pomak; nepodržane/nevažeće objekte; reviziju; stvarni numerički bridge i njegova oba prikaza; ograničeno poboljšavanje; izmjenu budžeta i ulaza; nedostajuću potvrdu; nepotpuni poprečni preklop; čuvanje FP zastavica i odbijanje drugog režima zaokruživanja. To nije ObjectARX simulacija i ne dokazuje stvarni grafički runtime.

## Komande builda i isporuka

Tačne build komande (v141 MSBuild iz postojećeg DEV install koraka; za 2022 setup-msbuild):

```powershell
& $env:TVS_MSBUILD 'TVS_Ventilation 2020.sln' /m /t:Build /p:TVSBypassPreview=true /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0.17763.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
& msbuild 'TVS_Ventilation 2022.sln' /m /t:Build /p:TVSBypassPreview=true /p:TVSOffsetProof=true /p:TVSOffsetIntegration=true /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0 /bl:build-logs/build.binlog /fl '/flp:logfile=build-logs/build.log;verbosity=normal'
```

Izlazi: x64/Release/2020 odnosno /2022, TVSTVS_Ventilation_ARX.arx i TVSTVS_Ventilation_DBX.dbx; kopiraju se u odvojeni dist-bypass-preview.

- [DEV AutoCAD 2020 ZIP](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987745/artifacts/13063835398), 512121 B, SHA-256 `5b48c55b7439dfb714064beb6d91f489a26c83edbef88bdc182e9253aa0b4aa3`.
- [DEV AutoCAD 2022 ZIP](https://github.com/vstijov-boop/tvs-vent/actions/runs/41255987781/artifacts/13063137732), 503323 B, SHA-256 `416be7bb2a93e860530b1f3b4994f0e1b5240ce3e8dd7d495d363ceb6c01fe44`.

ZIP-ovi su stvarno preuzeti i provjereni po SHA-256, sadržaju BUILD.txt i PE x64 tipu. 2020 ARX/DBX: 879616/427008 B; 2022: 835584/449024 B. BUILD.txt navodi checkout ef88a892... čije stablo odgovara izvoru 1c60f0f.... U ZIP-u su GUIDE.md i prethodna OFFSET uputstva. Nema prepisivanja produkcionog bundle-a. Prethodne proof/Z komande zadržavaju ranije ponašanje; odsustvo upisa ovdje važi za novu **TVS_BYPASS_PREVIEW** komandu.

Za probu pokrenuti AutoCAD bez drugog već učitanog TVS para, učitati DEV DBX pa ARX iz istog ZIP-a i pokrenuti **TVS_BYPASS_PREVIEW** u probnom Model DWG-u. Ne prepisivati produkcionu instalaciju; ako je TVS već učitan, koristiti novu testnu sesiju/profil. [GUIDE.md](GUIDE.md) sadrži detaljnu matricu.

Prve novonastale greške su otklonjene: LNK2001 za __pArxCmdMap_Ventilation_ARX1 pri makroregistraciji nove komande (zamijenjena eksplicitnim guarded addCommand/removeGroup); zatim C2855 charset/PCH nekompatibilnost nakon /utf-8 (nova UI datoteka je izdvojena iz zajedničkog PCH-a). Nije bilo širokih nepovezanih SDK/toolset popravki. Poznato prethodno LNK4099 upozorenje zbog Autodesk rxapi.pdb nije numerička greška niti nova neuspješna kompilacija.

## Commit-i i datoteke

- `7623955` — snapshot adapter, revizije, strict bridge i testovi.
- `faaed97` — DEV dijalog, read-only izbor, transient, dva zasebna workflow-a.
- `80ce2fb` — registracija komande i dodatne FP provjere.
- `0141876` — UTF-8 samo za nove datoteke/test.
- `1c60f0f` — novi dijalog bez nekompatibilnog zajedničkog PCH-a; **konačni izvršni izvor oba paketa**.
- Naredni dokumentacioni commit dodaje samo ovaj izvještaj i sačuvane CI izlaze.

Dodato: geometry/bypass/PreviewModel.h; TVS_Ventilation_ARX/BypassNumericBridge.cpp, BypassPreviewCommand.cpp/.h; tests/test_bypass_preview.cpp/.py; .github/workflows/dev-bypass-preview-2020.yml i -2022.yml; ova dokumentacija i GUIDE.md.

Izmijenjeno: ARX2020/2022.vcxproj samo za uslovne nove datoteke/per-file opcije; acrxEntryPoint.cpp samo include i guarded DEV registracija/uklanjanje komande. Nema DBX promjena, nove trajne šeme ili izmjene changeZ i njegovih pozivalaca. Prihvaćeni solve(), OffsetAdapter, Candidate i Generator nijesu mijenjani.

## Preostalo za korisnički AutoCAD pregled

[GUIDE.md](GUIDE.md) sadrži redosljed učitavanja i matricu: izbor, oba smjera, tri ugla, iznad/ispod, izolacija, granice, promjena ulaza, preview/Enter/Esc/Cancel, ponavljanje, UCS i mm/cm/m, provjera jednakog broja DWG entiteta i nepromijenjenih objekata/stanja trase. Takođe provjeriti aktivne kontrolne fokus/Tab putanje, DPI, konture i ponašanje transient-a poslije REGEN-a.

Brojanje, DWGMOD, čišćenje grafike i odsustvo trajnih izmjena trenutno su implementacioni ugovor i pregled koda, ne označeni AutoCAD prolazi. Ovaj korak se završava isporukom DEV paketa i staje prije trajnog upisa ili nove DWG šeme.
