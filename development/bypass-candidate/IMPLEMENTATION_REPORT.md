# Izolovani provjeravač kandidata — izvještaj izvršavanja

Grana: razvoj-bypass-candidate. Izvor koji je stvarno testiran:
76d3766e427006eb65b921ca823fbf9756e4c3da.
Draft PR: https://github.com/vstijov-boop/tvs-vent/pull/1
Base je razvoj-offset-arx, ne produkciona grana. PR nije spojen.

## Stvarni rezultat

GitHub Actions, Ubuntu 24.04, Python 3.12.3, g++ 13.3.0.
Run: https://github.com/vstijov-boop/tvs-vent/actions/runs/41249736194
Job candidate-regressions: success.

    python3 -m unittest discover -s tests -p 'test_*.py' -v
    Ran 18 tests in 10.158s
    OK

Novi C++ test:
    g++ -std=c++11 -O2 -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror -pedantic tests/test_bypass_candidate.cpp -o <temporary-binary>
    ALL PASSED 70 checks. Candidate validation only, no optimizer/CAD.

Nezavisna numerička provjera:
    PASS 452 independent exact-Fraction enclosure cases; fast-math rejected

Izvršeni su i raniji SDK-free regresioni testovi. To nije praktična AutoCAD provjera.
Kompletan stvarni izlaz je u TEST_RESULTS.txt; izvor su dekodirani job logovi ovog
run-a, nisu ručno prepravljeni rezultati. CI artefakt s environment/tests tekstovima:
https://github.com/vstijov-boop/tvs-vent/actions/runs/41249736194/artifacts/13060043338

Lokalno izvršno okruženje postalo je nedostupno prije implementacije. Zbog toga su
novi fajlovi sačuvani u izdvojenoj GitHub grani, a kompajliranje i testovi zaista
izvršeni na Actions runner-u. Nisu predstavljeni kao lokalno ili CAD izvršavanje.

## Primjeri iz stvarnog izlaza

Sve mjere mm; enum status 0=Feasible, 1=Infeasible, 2=NumericalBoundary.

| Slučaj | Zahtjev c | Ograda najmanjeg rastojanja | Status |
|---|---:|---|---|
| Referenca ispod, zaseban test s marginom | 49.9 | [49.9990018646667,50.0000000000001] | Feasible |
| Ogledano iznad | 49.9 | [49.9990018646668,50.0000000000001] | Feasible |
| Originalna referenca, c nije izmijenjen | 50 | [49.9990018646667,50.0000000000001] | NumericalBoundary |
| Izolacije 25/25 | 49.9 | [49.9990052439556,50] | Feasible |
| Zadati širi neoptimalni kandidat, D=400 C=400 | 50 | [175,175] | Feasible, minimum W nije potvrđen |
| Pomjeren početak +10, kosi dio preblizu | 50 | [42.9279365573021,42.9289321969146] | Infeasible |
| Kandidat c=0, H dodiruje prepreku | 0 | [0,0], relation Touching | NumericalBoundary zbog spojne ograde |
| 30° zadati kandidat | 49.9 | [49.9990049308865,50.0000000000001] | Feasible |
| 60° zadati kandidat | 49.9 | [49.9990004890367,50.0000000000001] | Feasible |

Rezultati 49.9 su jasno različiti ulazi za testiranje unutrašnjosti izvodljivog
domena, ne pravilo snižavanja c=50. Checker nikad ne mijenja zadati razmak ili
geometriju. Zadati širi kandidat pokazuje prvu korisnu mogućnost: potvrđena
izvodljivost sa pravim c=50 bez zahtjeva za optimalnim zahvatom.

## Nezavisni oracles — ne samo brojevi mockup-a

- Unutrašnjost kružnog luka: puni sektor Ri=100, Ro=300, 60°, centar (0,0);
  prepreka [29,30]×[-40,-39] u unutrašnjoj šupljini. Najdalji njen ugao je (30,-40),
  udaljen 50 od centra (3-4-5). Zazor do unutrašnjeg luka zato je 100-50=50.
  Kritična tačka luka je (60,-80), ugao oko 36.87°, strogo između krajeva sektora.
  Stvarno: [49.9990057228653,50.0000000007342], u=0.614501953125, v=0.
- Kosi pravi komad na z=s, poluvisina 5, prepreka [50,60]×[70,80]:
  najbliži ugao (60,70), zazor 10/sqrt(2)-5 = 2.0710678118654752.
  Pozitivan razmak, ali nedovoljan za c=3. To nije preklapanje.
- H na koti -800, B=200, prepreka centar -400 visine 250:
  gornji lim -700, donji lim prepreke -525 -> gap 175.
- Puna kutija sadržana u kanalu, kanal sadržan u prepreci i ukršteni pravougaonici:
  distance=0 / Overlapping. Pozitivna udaljenost granica (npr. 9 mm) se ne vraća
  kao razmak punih tijela.
- Tačan dodir dvije ravne kutije ima razdvojene unutrašnjosti i distance=0/Touching.
  Pomjeranje sa pozitivnim preklopom daje Overlapping, i pri c=0.
- 452 exact-Fraction slučaja: tačne racionalne vrijednosti +,-,*,/ i kvadrati
  ograda sqrt; sin/cos prema nezavisnim racionalnim ogradama 40 članova sa ostatkom.
  Tu su i poništavanje, podlivanje i širok opseg eksponenata. Ovo provjerava
  mehanizam ograda; nije formalna verifikacija svih mogućih IEEE ulaza.

## Pokrivenost i ograničenja

Izvršeno:
- ispod/iznad, izolacija, fiksnih 30/45/60°, neoptimalni kandidat;
- prozor, sva tri minimuma, asimetričan položaj i translacija cijelog ulaza/prozora;
- aktivna/neaktivna gornja/donja granica, uključujući NaN u neaktivnom polju;
- kosi dio uz ugao, minimum unutar luka, kritični svjedoci;
- sadržavanje, presijecanje, dodir i preklop pri c=0, ne-susjedni parovi;
- nevažeći/nepodržani ulazi, FTZ/DAZ i režim zaokruživanja;
- čuvanje floating-point flags;
- nedovoljan radni budžet i poznata stroga Lmin granica vraćaju neodlučnost;
- ranije regresije ostaju uspješne.

Neizvršeno:
- MSVC i Windows numerički runtime; nema ObjectARX/DBX builda ovog modula;
- AutoCAD učitavanje, komanda, DWG/COPY/Undo/Redo ili praktična geometrija;
- adapter referentnog vođenja: isporučen je nalaz i ugovor, ne kod adaptera;
- novi optimizer: ne postoji. Nema tvrdnje o globalnom minimumu.

Numeričke garancije i njihov uslovni IEEE ugovor opisani su u
geometry/bypass/README.md. Nije proglašena formalno sertifikovana biblioteka, nije
pretvorena stara solve implementacija u intervalnu, niti je numerička ograda
predstavljena kao praktični CAD dokaz.

## Commit-i ovog koraka

- eeac59e: prvi izolovani model i test/CI.
- 95b64f2: nezavisna Fraction provjera, dodatni slučajevi i ispravna razlika
  između dodira i pozitivnog rastojanja kod parova.
- 0ca98a1: ispravka test-podataka translacije (pomjeriti i raspoloživi prozor);
  dodata provjera pogrešnog režima zaokruživanja.
- 21fea78: minimumi fizičkih komada i cijeli gabarit prozora sa ogradama;
  stroga granica solve-a se ne popravlja.
- 76d3766: minimumi su obavezni konstrukcioni parametri; floating environment i
  FTZ/DAZ provjera; završni izvor za navedeni uspješan run.
- Završni dokumentacioni commit dodaje README, nalaz vođenja i ovaj stvarni izvještaj.

Jedan međukorak (run 41249063731) imao je grešku u testu translacije: kandidat i
prepreka su pomjereni, a prozor je ostao na starom mjestu. Checker ga je pravilno
odbio; ispravljeni fixture pomjera i prozor. Završni run sa svim testovima prolazi.

## Spisak datoteka

Dodato:
- geometry/bypass/OutwardInterval.h
- geometry/bypass/Candidate.h
- geometry/bypass/README.md
- tests/test_bypass_bounds.py
- tests/test_bypass_candidate.cpp
- tests/test_bypass_candidate.py
- .github/workflows/dev-bypass-candidate.yml
- development/bypass-candidate/GUIDE_REFERENCE_AUDIT.md
- development/bypass-candidate/TEST_RESULTS.txt
- development/bypass-candidate/IMPLEMENTATION_REPORT.md

Nijedan postojeći ARX/DBX izvor, postojeći geometrijski modul, DWG filer, COM
potrošač ili produkcioni workflow nije izmijenjen. Produkcioni paket nije izgrađen
ili objavljen. Draft PR ostaje za pregled; nema merge-a. Ovdje se staje prije
optimizacije i CAD integracije.
