# INT-00 — razvojna implementacija i rezultati

01.11.2026. Samo odobrena faza 0. Produkcija netaknuta; PR #1 draft.
Osnova prihvaćenog plana: `80c53c6c6fbca13808cf173145dac168828892a9`.
Izvršni izvor ove isporuke: **`0b8130b131ec21b608b93f033cd179d0d28d2f8d`**.
Praktično AutoCAD prihvatanje nije izvršeno ovim izvještajem.

## Isporučene izmjene

- `geometry/OffsetOperation.h`: zajednički fizički plan koristi postojeći solve
  i OffsetAdapter, bira Prvi/Posljednji i čuva originalni endpoint redosljed.
  Nema kopije solver-a i nema novih trajnih podataka.
- `TVS_Ventilation_ARX/OffsetService.h`: read-only snapshot izvora, jedinica,
  svojstava, topologije i referenci; provjera slobodnog izabranog kraja i
  suprotnog priključka, stale provjera i jedan zajednički transakcioni writer.
  Izvorni objekt ostaje odgovarajući ostatak; susjedi se ne otvaraju za upis.
- `OffsetIntegrationCommand.h`: pribavljanje ulaza i postojeći dgChangeZ,
  transient pregled projektovane ose/spojeva sa apsolutnim fizičkim kotama,
  eksplicitna potvrda i odvojeni standalone/drawing adapteri.
- `acrxEntryPoint.cpp`: DEV registracija **TVS_OFFSET** uz TVS_OFFSET_INTEGRATION;
  oba postojeća Z pozivaoca ostaju i dalje zaštićena uslovom Applied. Samo
  drawing adapter ažurira aktivni izlaz, globalne kote/ugao i A1/A2/pt1.
- ARX 2020/2022 projekti i postojeći DEV workflow-i uključuju nove headere,
  proširene regresije i `OFFSET_INT00_GUIDE.md`. Artefakti su jasno imenovani
  DEV-OFFSET-INT00. Postojeći bypass preview/statički detalj ostaju u tim DEV
  buildovima; nema novog trajnog bypass-a.

`TVS_OFFSET_MEMBER` v1, DBX filer-i, solver/generator i fizički čitači količina
nisu izmijenjeni. Kosi komad i dalje čuva svoj lokalni rise u istoj trojci.
Postojeći transakcioni BuildTransaction/abort ostaje; fail-next se sada može
provjeriti i poslije skraćivanja izvornog objekta, prije commita.

Izričita politika prenosa obuhvata osnovna prikazna svojstva, lineweight,
materijal, providnost, plot stil, Flow, Grani, Wipeout, WipeoutLength i This1D.
Pozicija/identitet ostaje samo na originalu; podržani zajednički proizvođač i
napomena prenose se. Nepoznati ili dvosmisleni korisnički podaci odbijaju operaciju.
AcDbEntity `setPropertiesFrom` sam prenosi samo osnovna svojstva; ostala su
namjerno prenesena posebnim setter-ima, prema
[SDK ugovoru](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AcDbEntity__setPropertiesFrom_AcDbEntity__Adesk__Boolean.html).

## Commit-i

- `4a6dc92771eee8a008cfe19e4bd4e76a5061e957`: zajednički servis/plan, komanda,
  preview, početni testovi i uputstvo.
- `c96dfacf134ec1e9688364d986bef6f83ad9cb41`: SDK potpisi i izvršne provjere oba
  stvarna Z pozivaoca i politike atributa.
- `8a953b729de0d22aa583310b8bef2e540173a0e3`: eksplicitan prenos materijala i
  prikaznih svojstava; provjera SDK void setElevation i cleanup preview-a.
- `0b8130b131ec21b608b93f033cd179d0d28d2f8d`: odbijanje neispravnih numeričkih
  podataka konektora/susjeda i početne kote; konačna proširena matrica.

Raniji neuspješni CI pokušaji nisu konačna potvrda. Rezultati ispod važe za
posljednji navedeni izvršni izvor. Dokumentaciona završna dopuna ne mijenja kod.

## Automatizovane provjere — nisu AutoCAD runtime

- [GCC puna regresija](https://github.com/vstijov-boop/tvs-vent/actions/runs/41350649964):
  **27 metoda prošlo**, 36.764 s; strogi warning/error i FP uslovi zadržani.
- Windows v141: 11 metoda prošlo (5 postojećih bypass, 1 zajednički offset plan,
  5 offset servis/UI funkcija), [CI matrica](https://github.com/vstijov-boop/tvs-vent/actions/runs/41350649919).
- Windows v142: istih **11 metoda prošlo** u istoj CI matrici; oba matrix job-a
  završila su uspješno na konačnom izvršnom izvoru.
- Lokalni Windows v143: istih 11 metoda prošlo; zapis van repozitorijuma:
  `DEV-offset-int00/local-v143-final.txt`. Prvi lokalni pokušaj s nedostupnim
  privremenim folderom nije bio prolaz; konačni test je koristio izdvojeni
  workspace temp folder i završio s exit 0.

Nove provjere izvršavaju stvarni zajednički plan i izdvojene stvarne C++ funkcije:

- **144 fizička slučaja / 4614 provjera:** Prvi/Posljednji, podizanje/spuštanje,
  30/45/60, A!=B, mm/cm/m, četiri WCS pravca, nezavisan crtački Z; ekvivalentni
  ulazi oba kraja daju iste spojeve, rise i dužine. Premala visina/ulaz, NoChange,
  90°, nepodržane jedinice i nehorizontalnost su odbijeni bez izmjene snapshot-a.
- **48 scenarija stvarnog topology/current/write koda sa SDK zamjenama:**
  eksplicitne recipročne i implicitne veze, povezan izlaz, dvosmislen/pokvaren
  priključak, neispravne tačke, očuvanje suprotnog konektora i endpoint redosljeda;
  stale atributi/mreža/jedinice, zaključan izvor; prekidi append-a, metapodataka,
  poslije izmjene izvora i na commitu. Testna baza vraća izvor i nove objekte.
- **15 slučajeva stvarne politike atributa**, uključujući odbijanje nepoznatih
  aplikacija, ručnih override-a, duplikata i extension podataka; oslobađanje bufera.
- **7 slučajeva stvarnog prenosa svojstava:** materijal/prikaz/protok/zajednički
  podaci, bez kopiranja jedinstvene pozicije, povrat greške svakog setter-a.
- **8 slučajeva stvarnog Preview cleanup-a:** običan završetak, greška svakog
  transient add poziva, izuzetak i nedostupan manager; nema preostalih testnih objekata.
- **16 izvršnih slučajeva oba stvarna Z pozivaoca:** Applied/NoChange/Cancelled/
  Failed i kvar sesijske evidencije poslije commita. Neuspjeh čuva aktivni kraj,
  A1/A2/pt1 i globalne vrijednosti; Applied postavlja stvarni izlaz/nove vrijednosti.

Ovi brojevi nisu nezavisni AutoCAD DWG slučajevi. SDK zamjene nisu dokaz stvarnog
ObjectARX transakcionog filing-a, MFC dijaloga, UCS transformacije ili runtime
rollback-a. WCS rotacije u numeričkoj matrici nisu potvrda stvarno rotiranog UCS-a.
Fizički orakul 400x200/Ri100/+400/45 daje P=rise=282.842712474619 mm, L=400 mm,
0.4 m, 0.48 m² i 2.2608 kg pri 0.6 mm bez rezerve; nije izmjeren runtime COM/Excel.

## DEV buildovi i preuzeti paketi

Oba SDK builda/linka su **prošla**. Ovo nisu AutoCAD runtime testovi.
CI checkout je PR test-merge `489baccede12fdc204e6d1b6bfe7ef111ab732a2`;
[poređenje s izvršnim HEAD-om](https://github.com/vstijov-boop/tvs-vent/compare/0b8130b131ec21b608b93f033cd179d0d28d2f8d...489baccede12fdc204e6d1b6bfe7ef111ab732a2)
vraća nula promijenjenih datoteka. To je checkout koji navodi BUILD.txt u oba paketa.

- **AutoCAD 2020 x64:** [build](https://github.com/vstijov-boop/tvs-vent/actions/runs/41350649903),
  [DEV ZIP artefakt](https://github.com/vstijov-boop/tvs-vent/actions/runs/41350649903/artifacts/13100988256).
  ZIP 590371 bajt; SHA-256 `c4892ec3b8dc8d6ff002393100eb434695a5a779f5ca908efed8844c52163bc9`.
  ARX 1035264 / DBX 427008 bajta; PE x64 provjeren.
- **AutoCAD 2022 x64:** [build](https://github.com/vstijov-boop/tvs-vent/actions/runs/41350649932),
  [DEV ZIP artefakt](https://github.com/vstijov-boop/tvs-vent/actions/runs/41350649932/artifacts/13101118350).
  ZIP 579125 bajtova; SHA-256 `61481f31fc65442b35b3c5b1fb15d6da7a3efedc27525663d1f91fd631e9670e`.
  ARX 980992 / DBX 449024 bajta; PE x64 provjeren.

Oba ZIP-a su preuzeta u izdvojeni lokalni folder `DEV-offset-int00` izvan
repozitorijuma; SHA-256 svakog preuzetog ZIP-a odgovara GitHub digest-u. Raspakovani
parovi su u `DEV-offset-int00/2020` i `DEV-offset-int00/2022`. Svaki paket sadrži
BUILD.txt i OFFSET_INT00_GUIDE.md. Ne miješati godine ili stare/nove parove.
Nijedna binarna datoteka nije instalirana u produkcioni bundle niti učitana u
postojeću AutoCAD sesiju za potrebe INT-00.

## Praktične provjere — otvorene

Tokom INT-00 nisu kontrolisane AutoCAD sesije niti izvršene nove interaktivne
probe ovog paketa. Nijedna nova Undo/Redo proba ili istraga nije pokrenuta.
STAB-01 ostaje odložen. Posebna runtime checklist-a je u
[backlog-u](INTEGRATION_BACKLOG.md), a kratki postupak u
[OFFSET_INT00_GUIDE.md](OFFSET_INT00_GUIDE.md).

Posebno ostaju: stvarni izbor i preview oba kraja, rotiran UCS, originalni
priključci/atributi/MLeader, fail-next bez ostataka, oba Z nastavka i svi fizički
potrošači, save/reopen, zasebno u AutoCAD-u 2020 i 2022. Ranije matrice detalja ili
proof-a nisu zamjena za taj pregled.

## Poznata ograničenja i granica isporuke

Samo običan direktan horizontalni kruti pravougaoni TVS_Pipe u Model-u, mm/cm/m,
slobodan izabrani kraj, Ri>0 i podržani ugao bez 90°. DEV minimumi 100/0.1 mm,
ostatak ulaza >=1 mm, novi izlaz 300 mm. Direct!=0, poseban Form i negativan protok
su odbijeni. Nepoznata TVS geometrija u Model-u konzervativno odbija operaciju.
Eksplicitna suprotna veza prve verzije podržava provjereni recipročni pipe port;
poznati geometrijski fitting kontakti mogu ostati implicitni.

Legacy Pipe/TAP filer ne snima konektorski vektor. Originalni vektor ostaje
netaknut i ne oslanja se na novi DB undo zapis; INT-00 ne uvodi novu šemu radi
njegovog snimanja. Null-ID konektorski point cache nije autoritet; portove servis
izvodi iz stvarne geometrije. Ovo ograničenje nije predstavljeno kao nova trajna
topološka podrška. Originalni MLeader ostaje na originalnom ostatku; ne prenosi
se proizvoljnom novom članu i tekst se ovdje automatski ne preračunava.

Preview prikazuje projektovanu osu/spojeve i fizičke kote, ne pune limene konture
niti provjeru kolizije cijele okoline. Nema pomjeranja nizvodne mreže ili susjeda.
Nema nove bypass/parent šeme, povezanih detalja ili kružnih presjeka. Postojeći
statički detalji i COPY duplirani identitet ostaju nepromijenjeni.

Prije buduće faze 1 obavezno kratko uporediti ranije prihvaćeni roditelj nad dvije
v1 trojke i H sa predlogom posebnog sedmočlanog ugovora. Nijedan smjer nije sada
implementiran, a promjena ranijeg smjera nije odobrena ovim INT-00 zahtjevom.
Poslije isporuke INT-00 stati radi pregleda; nema merge-a ili produkcionog izdanja.
