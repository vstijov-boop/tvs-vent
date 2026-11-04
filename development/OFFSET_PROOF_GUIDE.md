# DEVELOPMENT ONLY — TVS_TEST_OFFSET

Ovo je razvojni dokaz geometrije, ne izdanje za radne crteže. Nema integracije
changeZ, specifikacije, COM-a, XRecord-a ili šeme B. solve i stroga politika su
nepromijenjeni. Nema automatskog editovanja starih objekata.

## Uključivanje i izolacija

ARX projekti 2020 i 2022 imaju uslov TVSOffsetProof=true koji dodaje preprocessor
TVS_OFFSET_PROOF. U normalnom buildu je flag odsutan i tri komande se ne registruju.
Nove DEV workflow datoteke ciljaju samo granu razvoj-offset-arx i proizvode zasebne
DEV-ONLY-offset-autocad-2020/2022-x64 artefakte. Ne zamjenjuju produkcione artefakte.
DBX je iz istog izvora, bez nove perzistentne semantike.

## Učitavanje — čist AutoCAD proces, odgovarajuća verzija

1. Sačuvati produkcionu instalaciju. Za ovaj test NE prepisivati bundle.
2. Raspakovati DEV ZIP u zaseban direktorijum van ApplicationPlugins.
3. Spriječiti automatsko učitavanje produkcionog TVS bundle-a u tom testnom procesu:
   zatvoriti CAD i privremeno skloniti bundle van ApplicationPlugins, ili koristiti
   odvojeno testno okruženje bez njega. Samo unload ARX-a ne garantuje da je DBX uklonjen.
4. Pokrenuti AutoCAD 2020 ili 2022 prema paketu, novi prazan DWG, Model tab.
5. APPLOAD prvo TVSTVS_Ventilation_DBX.dbx, zatim TVSTVS_Ventilation_ARX.arx iz ISTOG
   DEV direktorijuma. Ako loader prijavi već učitan drugi modul, zatvoriti CAD i
   riješiti automatsko učitavanje; ne miješati verzije. Ako zahtijeva autorizaciju
   dodatka, koristiti postojeći način autorizacije. Komande su ipak samo razvojne.
6. TVS_SETTINGS prije crtanja: izabrati mm/cm/m za taj DWG. Ne mijenjati njegove
   jedinice nakon kreiranja probe. Nema novog podatka jedinica u razvojnoj komandi;
   ona samo čita postojeći TVS faktor baze (podrazumijevano mm).
7. Opcionalno UCS pomjeriti/rotirati; komanda namjerno postavlja determinističke
   WCS probe i ne koristi trenutni UCS za njihove koordinate.

## Komande

TVS_TEST_OFFSET: odbija svaki neprazan model i aktivni Paper Space. Izabrati
Kontura (default), Maska (wipeout) ili Osa (1D). Svako testiranje režima u novom
praznom DWG-u. Kreira 12 slučajeva: podizanje/spuštanje × 45/30/60° × smjer 0/37°.

Svaki slučaj: A=400 B=200 Ri=100 mm, početna fizička kota -400 mm, deltaZ=+400
(završna 0) ili -400 (završna -800). Z crtačke ravni je zasebnih 37 mm, odnosno
37/F model-jedinica. Lmin=0 je samo dijagnostički parametar ovog testa, ne odluka
za produkciju. Polazni i izlazni horizontalni segment imaju referentnu dužinu 600 mm.
Kosi segment je običan ravanski TVS_Pipe, njegova fizička dužina NIJE integrisana.

Svaki P0..P3 je označen crvenim krstićem i natpisom. Očekivane tačke se štampaju
u modelskim jedinicama i fizičkim WCS mm, fizička kota se štampa odvojeno.
Dva stvarna TVS_TAP imaju eksplicitne parametre i namjerno pokvaren MA/MC keš prije
prvog crtanja. Kreiranje ne koristi taj keš. Entiteti se konstruišu postojećim
TVS_TAP/TVS_Pipe klasama i javnim parametrima, dodaju u BTR standardnim ObjectARX
API-jem i registruju u istoj transakciji. add_new helper nije korišćen jer sam
interno postuje/zatvara objekat; test mora kontrolisati vlasništvo rollback-a.

ZOOM Extents, REGEN, pa TVS_TEST_OFFSET_CHECK. CHECK čita MA/MC samo dijagnostički
nakon stvarnog crtanja. Nije produkcioni način konstrukcije/proračuna. Ispisuje
Expected/Actual MA i MC, odstupanja u fizičkim mm i PASS/FAIL sa tolerancijom
0.001 mm. Ukupno 24 TAP-a. Ponoviti REGEN i CHECK najmanje tri puta; rezultati
moraju ostati isti. F2 ili LOGFILEON čuva pun izlaz.

Ako je provjera rađena prije crtanja/REGEN-a, pokvaren keš treba dati FAIL, ne
lažni PASS. Poslije otvaranja drugog DWG-a CHECK ne mjeri prethodnu sesiju. Sesija
živi samo u memoriji, nije XRecord niti šema B. Poslije zatvaranja/otvaranja DWG-a
nema sačuvanog testnog manifesta; ponoviti test u novom praznom fajlu.

Jedan UNDO (nakon nemodifikujućih ZOOM/REGEN/CHECK) treba ukloniti CIJELU kreaciju.
CHECK tada treba prijaviti svih 24 TAP kao missing i 0 živih kreiranih objekata.
REDO treba vratiti cijelu kreaciju; REGEN/CHECK ponovo 24/24 PASS. Ako Undo grupisanje
ne odgovara, zabilježiti rezultat — nije dokazano u CI-u.

TVS_TEST_OFFSET_FAIL u drugom praznom DWG-u namjerno prekida nakon prvog dodatog
TAP-a. Abort mora ostaviti prazan model; komanda ispisuje rezultat provjere.
Takav scenario provjerava da i reference/oznake kreirane prije TAP-a budu uklonjene.
Ne tvrditi da je rollback potvrđen samo na osnovu uspješnog kompajliranja.

Obavezno upozorenje u kreiranju i provjeri:
„Razvojni geometrijski test — fizički obračun nagibnog segmenta još nije integrisan.
Rezultat nije za specifikaciju.“

## Očekivane dužine (oba smjera visine)

| Ugao | Projekcija međukomada mm | Fizička dužina mm | Ukupni zahvat mm |
|---|---:|---:|---:|
| 45° | 282.842712 | 400.000000 | 565.685425 |
| 30° | 600.000000 | 692.820323 | 800.000000 |
| 60° | 115.470054 | 230.940108 | 461.880215 |

WCS lokalni početak slučaja nije njegov Q: O = Q - U*axis[1].s/F.
Očekivane konkretne koordinate čitati iz ispisa datog slučaja, ne porediti različite
fizičke tačke zato što su njihove vidljive linije blizu. Elev prvog TAP=-400/F,
Elev drugog TAP=0/F ili -800/F, dok MA/MC crtački Z ostaje 37/F.

## Obrazac mjerenja

Zaglavlje: CAD verzija / BUILD SHA / mm-cm-m / UCS / Kontura-Maska-Osa / REGEN broj.

| Slučaj | Mjera | Očekivano | Izmjereno | Odstupanje mm | Prolaz/pad |
|---|---|---|---|---|---|
| C1 (+400,45°,0°) | E1 MA / MC | ispis Expected | | | |
| C1 | E2 MA / MC | ispis Expected | | | |
| C1 | P / L / ukupni zahvat | 282.842712 / 400 / 565.685425 mm | | | |
| C7 (-400,45°,0°) | E1 i E2 MA / MC | ispis Expected | | | |
| C7 | Elev drugog TAP | -800 mm | | | |
| C2/C8 | nekardinalni smjer 37° | ispis Expected | | | |
| svi | CHECK poslije 3 REGEN-a | 24/24 PASS, stabilno | | | |
| svi | UNDO/REDO | 0 pa svi objekti | | | |
| FAIL proba | rollback | prazan model | | | |

Posebno ručno zabilježiti konture, pune/isprekidane linije, wipeout, OSNAP na
priključcima i da li neki režim prikriva referentni krstić. Očekivani kosi rubovi
moraju se tumačiti prema odgovarajućim tačkama konture; ove probe ne integrišu
novi prikaz kosog TVS_Pipe presjeka.

## Granice

CI dokazuje samo kompajliranje/linkovanje i izolovane regresije. Učitavanje u
AutoCAD, stvarni REGEN, OSNAP i Undo/Redo moraju biti posebno izvršeni i zabilježeni.
Ne koristiti ovu geometriju za specifikaciju. Ne preuzimati MA/MC keš kao novi
produkcioni izvor istine. B ostaje dokumentovani predlog; nije riješeno COPY
pozicioniranje naspram zaštite transformacija niti unutrašnja valjanost sklopa
naspram njegove veze sa susjednom mrežom.
