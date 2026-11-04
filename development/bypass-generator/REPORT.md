# Izvršena Windows/Linux provjera i generator — razvojni izvještaj

Datum CI izvršavanja: 2026-10-31 UTC. Testirani izvor: `05252ad0bbb347b6866f4b6fbb6daf2393b54a9a`; naredni dokumentacioni commit ne mijenja izvršni kod.

Grana: razvoj-bypass-candidate. [Draft PR #1](https://github.com/vstijov-boop/tvs-vent/pull/1) ostaje otvoren prema razvoj-offset-arx; nije spojen u produkciju.

## Stvarno izvršeno

| Okruženje | Izvršenje | Stvarni rezultat | Izlaz |
|---|---|---|---|
| Ubuntu 24.04 / GCC 13.3.0 | [Run 41253216240](https://github.com/vstijov-boop/tvs-vent/actions/runs/41253216240) | 20 testova, 13.973 s, OK | [linux-gcc.txt](results/linux-gcc.txt) |
| Windows Server 2022 x64 / v141 | [Run 41253216430, v141](https://github.com/vstijov-boop/tvs-vent/actions/runs/41253216430/job/124690859741) | 4 testa, 21.347 s, OK | [windows-v141.txt](results/windows-v141.txt) |
| Windows Server 2022 x64 / v142 | [Run 41253216430, v142](https://github.com/vstijov-boop/tvs-vent/actions/runs/41253216430/job/124690859827) | 4 testa, 24.406 s, OK | [windows-v142.txt](results/windows-v142.txt) |

Kompajliranje, linkovanje i izvršavanje su završeni uspješno na sva tri okruženja. Vremena u tabeli obuhvataju kompiliranje testova i njihovo izvršavanje, bez instalacije toolchain-a.

Sačuvani CI artefakti: [Linux](https://github.com/vstijov-boop/tvs-vent/actions/runs/41253216240/artifacts/13061489996), [v141](https://github.com/vstijov-boop/tvs-vent/actions/runs/41253216430/artifacts/13062301744), [v142](https://github.com/vstijov-boop/tvs-vent/actions/runs/41253216430/artifacts/13062476641). Sadrže stvarni tests.txt, okruženje gdje je zapisano i references.json. To su testni artefakti, bez izmjene ili zamjene produkcionih ARX/DBX paketa.

Na svakom od tri okruženja izvršeni su:
- 452 nezavisna exact-Fraction intervalna slučaja i negativna kompilaciona provjera fast-math režima;
- postojeći 70-check provjeravač cijelih tijela i razmaka;
- 30 provjera floating-point okruženja: najbliže zaokruživanje, tri nepodržana režima, exception flags, normalan/pogrešan/graničan povratak i izuzetak; FTZ, DAZ i oba zajedno; kontrolni registri i zastavice poslije vraćanja;
- 274 provjere generatora; 17 prihvaćenih rješenja je svaki put nezavisno ponovo predato Candidate.check() i vraća Feasible;
- Linux dodatno pokreće cijeli postojeći regresioni skup: ukupno 20 Python test-metoda. Windows pokreće četiri izolovane numeričke test-metode; stare Python regresije koje neposredno očekuju g++ nijesu predstavljene kao MSVC testovi.

Rezultat nije dokaz AutoCAD runtime-a. Nijesu pokrenuti AutoCAD 2020/2022, CAD komande, preview ili DWG upis. Raniji korisnički proof testovi i nova, još praktično neprovjerena Z integracija nijesu korišćeni kao zamjena za takvu provjeru.

## Toolchain i komande

Windows: postojeći razvojni x64 toolchain-i, odvojeni Actions posao za svaki. V141 VS2017 Build Tools, VC 14.16.27023 / cl 19.16.27054, Windows SDK 10.0.17763.0. V142 VC 14.29.30133 / cl 19.29.30159 u VS2022 Enterprise, Windows SDK 10.0.26100.0 (izabran postojećim vcvars okruženjem; nije mijenjan ciljni SDK ARX/DBX projekata). Nema ObjectARX SDK zavisnosti ni ARX/DBX builda u ovom koraku.

```text
# V141: prije Python testa
call "<VS2017>/VC/Auxiliary/Build/vcvars64.bat" 10.0.17763.0 -vcvars_ver=14.16
# V142:
call "<VS2022>/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.29

# CI bira TVS_BYPASS_TOOLSET=v141 ili v142:
python tests/run_bypass_msvc.py
# pokretač postavlja TVS_BYPASS_CXX=msvc i stvarno izvršava:
python -m unittest discover -s tests -p "test_bypass*.py" -v
# svaki izolovani C++ program:
cl /nologo /EHsc /std:c++14 /O2 /fp:strict /W4 /WX /Fe<exe> /Fo<obj> <source.cpp>
```

MSVC nema /std:c++11 selektor; /std:c++14 je najstariji dostupan eksplicitni režim u oba toolchain-a. C++11 izvor potvrđen je istim testovima pod GCC -std=c++11, bez MSVC-specifičnih jezičkih dodataka u generatoru. To nije tvrdnja da MSVC ima zaseban C++11 mod. MSVC 2017/2019 su stvarno kompajlirali, linkovali i izvršili numeričke programe.

Linux: Ubuntu 24.04 x64, GCC 13.3.0, Python 3.12.3:

```sh
TVS_BYPASS_REFERENCE_OUT=bypass-test-results/references.json \
  python3 -m unittest discover -s tests -p 'test_*.py' -v
g++ -std=c++11 -O2 -fno-fast-math -ffp-contract=off \
  -Wall -Wextra -Werror -pedantic <source.cpp> -o <exe>
```

Fast-math negativni test namjerno pokušava /fp:fast ili -ffast-math i očekuje odbijanje u neizmijenjenom OutwardInterval.h: "strict IEEE binary64 evaluation without fast-math". Taj očekivani C1189 nije greška uspješnog posla. FLT_EVAL_METHOD je 0 u podržanim izvođenjima, IEEE binary64, FE_TONEAREST, postupno podlivanje. Eksplicitni std::fma ostaje dio intervalnog jezgra; implicitna kontrakcija/fast-math nije dopuštena.

Nijesu mijenjani globalni floating-point flagovi niti .vcxproj ARX/DBX projekata. Testni helper ima sopstvene opcije. Ne pokušava se tiho normalizovanje nepodržanog host okruženja. RAII feholdexcept/fesetenv je stvarno provjeren na ova tri okruženja; SSE MXCSR i MSVC controlfp se u testu porede prije/poslije. Testovi ne dokazuju sve procesore, biblioteke, konfiguracije ili buduće AutoCAD thread-ove.

Prvi Windows pokušaj je stao prije kompajliranja: WinError 2 pri nalaženju cl.exe. Popravljen je isključivo testni pokretač, tako da roditeljski proces naslijedi vcvars PATH prije pokretanja kompajlera. Nije mijenjan compiler/SDK da bi test prošao.

## Mjerenja i budžet

| Slučaj | Pozivi / ćelije | GCC ms | v141 ms | v142 ms |
|---|---:|---:|---:|---:|
| below45 | 1 / 10 | 18.957 | 51.756 | 47.608 |
| asymmetric | 1 / 286 | 42.878 | 115.002 | 105.663 |
| numerical-boundary | 15 / 8978 | 938.055 | 2518.350 | 2399.695 |
| bounded-improvement | 12 / 7144 | 494.791 | 1320.138 | 1342.365 |

Vrijeme je pojedinačno mjerenje na različitim dijeljenim CI mašinama, a ne kontrolisani benchmark. Instalacija toolchain-a i kompajliranje nijesu uračunati u elapsedMs generatora. Standardni prvi kandidat koristi jedan checker poziv i 10 distance ćelija. Asimetričan položaj koristi 286 ćelija; opcioni nastavak do 12 poziva troši 7144 ćelije i čuva kraće potvrđeno rješenje. Granični primjer ne dobija kandidat i zadržava Unresolved.

Budžeti/prekidi i ograničenje mjerenih ćelija opisani su u [INTERFACE.md](INTERFACE.md). Vremenski rok je kooperativan između poziva, ne prekida aktivan checker. Zero-budget, one-cell i zadržavanje najboljeg kandidata nakon call-limit prekida stvarno su testirani.

## Razlike između platformi

Svih 27 zapisanih generator slučajeva imaju iste statuse, D/C/s0/W/kotu, broj poziva, ćelija i poboljšanja na GCC-u, v141 i v142. Najveća zabilježena razlika krajeva ograde zazora prema Linux-u je 5.684341886080802e-14 mm, bez promjene prihvatanja. Primjer standardne donje ograde: GCC 64.999999999996561, MSVC 64.999999999996589 mm. Vremena su različita; nema zaključka da je razlika isključivo posljedica kompajlera jer hardver/opterećenje nijesu kontrolisani.

## Referentna rješenja

[reference-solutions.json](reference-solutions.json) sadrži potpune fizičke ulaze i D/C/s0, W, kotu prolaza, ogradu zazora i izmjerene brojače iz završnog Linux izvođenja za osam budućih preview primjera. Sve dimenzije su fizički mm, z0 je stvarna kota ose; nema novih DWG podataka. Dodatno svaki CI artefakt ima references.json za konkretni compiler.

Osnova za tabelu: novi kanal 400×200, Ri=100, z0=-400, prepreka širine duž trase 250 i visine 250, ose -400, s=0, bez izolacije osim posebno navedenog slučaja. Clearance=50, eksplicitna rezerva=5, minimum slope/bridge=100, minimumProjection=.1. Ugao je ulaz, nije optimizovan.

| Slučaj | Ugao | D | C | s0 | W | Kota prolaza |
|---|---:|---:|---:|---:|---:|---:|
| Ispod |45°|290|360|-635.685425|1271.370850|-690|
| Iznad |45°|290|360|-635.685425|1271.370850|-110|
| Ispod |30°|290|360|-789.474411|1578.948822|-690|
| Ispod |60°|290|360|-578.371686|1156.743371|-690|
| Izolacije oba kanala 25 |45°|340|460|-735.685425|1471.370850|-740|
| Prozor [-600,600] |45°|290|250|-580.685425|1161.370850|-690|
| Prozor [-580,820] |45°|290|360|-579|1271.370850|-690|

Za centrirani standardni slučaj nezavisno očekivani zazor je 65 mm; ovo nije snižavanje traženog razmaka. Konačni generator može vratiti prostranije rješenje. Opciono poboljšavanje pronalazi W=1161.370850, ali se minimum ni tada ne potvrđuje.

## Nezavisni nemogući i granični primjeri

Za neizolovani primjer, c=50 traži D>=275. Na 45° i R=200, X=D+400(sqrt(2)-1), pa uz C>=100 slijedi W>=800sqrt(2)-150=981.370849898476 mm. Prozor širine 900 mm je zato sigurno preuzak, nezavisno od neuspjeha pretrage. Ovo ne dokazuje da svaki pretraženi prozor bez rezultata nema rješenje. Tokom pregleda ispravljen je ranije pogrešno napisan testni primjer sa prozorom 1000 mm; taj raniji prag nije bio dokazan navedenom nejednakošću.

Za prolaz ispod: osa mosta mora biti <=-675, njegovo dno <=-775; floor=-650 je nemoguć. Ogledano iznad: osa >=-125 i vrh >=-25, ceiling=-150 je nemoguć. Generator i za ove primjere vraća samo NoCandidateFoundWithinBudget ili Unresolved, bez opšteg Infeasible zaključka.

D=275, floor=-775, c=50 i rezerva=0 dovode do same granice. Intervalna ograda/seam guard ne potvrđuju Feasible; rezultat je Unresolved, bez pomjeranja kote ili umanjenja clearance-a.

## Commit-i i datoteke

- 69010b1 — A: MSVC/Linux testni izbor, Windows workflow, FP regresije.
- cc046e5 — A: ispravka nalaženja cl kroz vcvars PATH.
- fa803ea — B: generator, regresije, JSON rezultati u DEV testnim artefaktima.
- 9e0a733 — B: potpuno čuvanje nepodržanog FP okruženja generatora; ogledani slučajevi i visinske granice.
- 05252ad — B: ispravljen nezavisni dokaz preuskog prozora.
- naredni dokumentacioni commit: ovaj izvještaj, interfejs, reference i tačni CI izlazi.

Dodato: geometry/bypass/Generator.h; tests/bypass_compiler.py; tests/run_bypass_msvc.py; tests/test_bypass_fp_environment.cpp/.py; tests/test_bypass_generator.cpp/.py; .github/workflows/dev-bypass-windows.yml i dokumentacija development/bypass-generator/.

Izmijenjeno: tests/test_bypass_candidate.py i test_bypass_bounds.py (izbor kompajlera); .github/workflows/dev-bypass-candidate.yml (testni path filter i JSON izlaz).

OffsetGeometry.h, OffsetAdapter, OutwardInterval.h, Candidate.h, Z integracija, ARX/DBX projekti, COM, specifikacija, KGTEZ, produkcioni workflow-i i paketi ostaju neizmijenjeni ovim korakom.

## Preostale granice

Konačni skup prijedloga nije potpun: može propustiti izvodljiv kandidat između nivoa ili izvan budžeta. Primarni cilj je potvrđena izvodljivost. Ne postoji globalni minimum, potvrda optimalnosti D, CAD preview ili DWG integracija. Tanak budući referentni adapter ostaje prethodno dokumentovan ugovor; nije proširivan.

Sljedeći korak zahtijeva pregled ovog rezultata. Rad se zaustavlja prije CAD preview-a, novih trajnih podataka ili povezivanja sa dijalogom.
