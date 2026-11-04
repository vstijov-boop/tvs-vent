# Jedinice crteža: mm, cm i m

Razvojna grana: `unapredjenja-jedinice`. Stabilna grana prevoda ostaje `srpski-prevod`.

U `TVS_SETTINGS` dodat je izbor jedinica crteža. Presjeci kanala, radijusi, dužine fitinga i visinske kote u TVS dijalozima i Properties paleti ostaju u **mm**. Tačke trase, osnap i gripovi koriste jedinice crteža. Količine u specifikaciji ostaju u m i m², a protok i brzina u m³/h i m/s.

| Izbor | mm po jedinici | Presjek 400 × 200 mm u geometriji | Trasa dužine 3 m |
| --- | ---: | ---: | ---: |
| mm | 1 | 400 × 200 | 3000 |
| cm | 10 | 40 × 20 | 300 |
| m | 1000 | 0,4 × 0,2 | 3 |

Jedinice se čuvaju u Named Objects Dictionary svakog DWG-a, kao Xrecord `TVS_DRAWING_UNITS`, grupa 40 sa vrijednošću 1.0, 10.0 ili 1000.0. Bez tog zapisa zadržava se dosadašnje ponašanje u mm. `INSUNITS` se ne mijenja: korisnik zadržava kontrolu nad AutoCAD skaliranjem svojih XREF-ova i blokova.

## Izbor i zaštita postojećih crteža

Jedinice izabrati **prije crtanja prvog TVS elementa**. Promjena jedinica je dostupna u TVS_SETTINGS, ne u pomoćnom dijalogu tokom crtanja. Ako postoje TVS elementi, uključujući elemente u lokalnim definicijama blokova, promjena faktora se odbija i ništa se ne skalira. TVS elementi u spoljnim XREF-ovima ne blokiraju izbor jedinica domaćeg crteža.

Ova verzija ne pretvara već nacrtanu TVS instalaciju iz mm u cm/m. Takođe nije implementirana automatska konverzija TVS objekata pri COPY/PASTE ili WBLOCK prenosu izabranih objekata između DWG-ova sa različitim podešavanjem jedinica: koristiti isti izbor jedinica ili zaseban XREF sa odgovarajućom razmjerom. Crteže sa TVS instalacijom u cm/m treba otvarati sa ovim usklađenim ARX/DBX parom; stare verzije dodatka ne čitaju novi zapis jedinica.

## Obuhvat implementacije

- Zajednički pretvarač i čitanje/pisanje jedinica po bazi, bez izmjena TVS formata serijalizacije i COM IDL interfejsa.
- Unos dimenzija u TVS_SETTINGS, pomoćnom dijalogu crtanja, višestrukoj izmjeni i dijalogu promjene Z kote; vidljive vrijednosti su u mm, interni geometrijski podaci u jedinicama DWG-a.
- Zadate dimenzije komandi usklađuju se pri promjeni aktivnog DWG-a; protok, uglovi i bezdimenzioni koeficijenti se ne skaliraju.
- Geometrija kanala, koljena, prelaza, račvi, vertikalnih promjena, maskiranja i gripova ostaje u jedinicama crteža. Fizički pragovi radijusa i geometrijski dodaci/tolerancije pretvoreni su u iste jedinice.
- COM/Properties dimenzije, radijusi predstavljeni preko dijaloga, dužine i kote koriste fizičke mm. Brzina, jedinični i ukupni pad pritiska u postojećim Properties proračunima koriste fizičke dimenzije. Prazna opcija `[P]` u tekstu MLeader-a nije mijenjana ovim zadatkom.
- Oznake i novi/stari putevi specifikacije koriste mm za natpise, m za dužine i m² za površine. Geometrija tabela i tekstova prilagođena je jedinicama crteža. Postojeći MLeader stilovi ostaju pod kontrolom AutoCAD-a i korisnika.

## KGTEZ

`Installer/TVS_TOOLS/KGTEZ.lsp` potiče iz korisnikovog dostavljenog fajla. Čita isti Xrecord pri svakom pokretanju komande. Dimenzije i dužine iz novog TVS COM interfejsa već su u mm i ne množe se ponovo. Radijusi očitani iz lukova nakon eksplodiranja kopije koljena množe se faktorom mm po jedinici. Podrazumijevana visina teksta tabele prati promjenu jedinica; ručni unos visine je jasno označen u jedinicama crteža.

Metoda površine/mase, gustina, tabela debljina, rezerva i izvoz Excel/CSV nisu mijenjani. Obračun i dalje obuhvata samo pravougaone kanale i fitinge; okrugli se preskaču. Izvorna tabela debljina je označena kao primjer, a postojeće formule fitinga zadržane su bez novih tvrdnji o usklađenosti sa normama.

## Provjera prije upotrebe na projektu

Razvojno okruženje nema AutoCAD; kompilacija i numerički testovi ne zamjenjuju provjeru u AutoCAD-u.

1. Sačuvati sada ispravan par ARX/DBX i originalni KGTEZ.lsp kao backup.
2. Pri zatvorenom AutoCAD-u novi ARX/DBX par kopirati u folder odgovarajuće verzije: `TVS.bundle/Contents/TVS_VENT/2020/win64` ili `2022/win64`. Novi KGTEZ.lsp kopirati u `TVS.bundle/Contents/TVS_TOOLS/Resources`. Koristiti dugme KGTEZ iz već potvrđenog TVS_TOOLS.cuix.
3. Na kopiji osnove u cm, bez TVS objekata, pokrenuti TVS_SETTINGS i izabrati cm. Zadati presjek 400 × 200 mm i protok 720 m³/h.
4. Nacrtati ravnu trasu dužine 300 jedinica. DIST na širini treba da pokaže 40, Properties/oznaka 400 × 200 mm, Properties dužina 3000 mm, specifikacija 3 m, brzina 2,5 m/s.
5. KGTEZ za taj kanal: isključena opcija fiksnih 2 mm, lim 0,6 mm prema postojećoj tabeli, površina 3,60 m² i masa 16,956 kg bez rezerve (u tabeli zaokruženo 16,96). Rezervu provjeriti odvojeno.
6. Ponoviti u mm (dužina 3000) i m (dužina 3), uz identične rezultate količina i mase. Provjeriti i koljeno, prelaz, T-komad, promjenu dimenzije, Z, Properties i višestruku izmjenu.
7. Sačuvati/zatvoriti/ponovo otvoriti DWG i potvrditi sačuvane jedinice. Prebacivati se između otvorenih mm/cm/m DWG-ova i provjeriti unos, Properties i KGTEZ. U crtežu sa TVS elementima pokušaj promjene jedinica mora biti odbijen bez promjene instalacije.

## Automatske provjere

- `g++ -std=c++98 -Wall -Wextra -Werror -pedantic tests/test_unit_conversion.cpp`: konverzije dimenzija, dužina, površina, mase, brzine i razvijene površine koljena daju isti fizički rezultat u mm/cm/m.
- `python tests/test_kgtez_units.py`: izvršavaju se stvarne numeričke funkcije iz KGTEZ.lsp uz simulirani COM/DWG pristup. Provjeravaju se podrazumijevani i nevažeći zapisi jedinica, stvarni izraz pretvaranja geometrijskog radijusa, kanal i grupisanje mase, koljeno, prelaz, račva i preskakanje okruglih elemenata. Test nije simulator AutoCAD-a.
- Postojeći test prevoda specifikacije ostaje dio regresione provjere.

## Završni paketi i rezultati

Izvor binarnih paketa: `93a6aaf6dd8d4595549fda3e635fcf34769dc3bb`.

- [AutoCAD 2020 x64 — paket mm/cm/m](https://github.com/vstijov-boop/tvs-vent/actions/runs/40543713544/artifacts/12752810037). SHA256 ZIP-a: `49e32705a312bcf480382a0ed451f595ec420d37222efb3b92e6ee05b0d27d8c`.
- [AutoCAD 2022 x64 — paket mm/cm/m](https://github.com/vstijov-boop/tvs-vent/actions/runs/40543713441/artifacts/12750824486). SHA256 ZIP-a: `963451460aee35159e0e3e323f10c10a87d97dce594628b4396520139ac612b5`.

Obje završne kompilacije imaju 0 grešaka i po 2 LNK4099 upozorenja o nedostajućim Autodesk PDB simbolima. Preuzeti ZIP-ovi provjereni su po SHA256 i ZIP CRC. Oba para binarnih fajlova su x64; 2020 uvozi acdb23.dll, a 2022 acdb24.dll. U oba ARX-a potvrđeni su novi natpisi dijaloga i izbor mm/cm/m; u ARX-u i DBX-u potvrđen je ključ zajedničkog zapisa jedinica. KGTEZ.lsp u paketima odgovara izvoru uz Windows CRLF krajeve redova, a TVS_TOOLS.cuix odgovara izvornom fajlu bajt po bajt. Oba ZIP-a sadrže ARX, DBX, KGTEZ.lsp, dva CUIX-a i BUILD.txt sa tačnim izvornim commitom.

Sve navedene lokalne računske i regresione provjere prolaze. Završni pregled zadržao je postojeći obračun dužine u ravni crteža (XY), uz računanje iz tekućih krajnjih tačaka bez zavisnosti od prethodnog iscrtavanja. Provjera rada u AutoCAD-u, čuvanja DWG-a i svih interaktivnih komandi ostaje korisniku prema gornjem postupku.

Prvi neuspješni pokušaj `f5fc3bf` koristio je pogrešan oblik argumenta za ObjectARX rbChain; ispravljeno je prosljeđivanje adrese pokazivača u `ecdce9a`. Istorijski neuspješni CI pokušaji nisu promijenili korisnikovu instalaciju. Stabilna grana prevoda i master nisu mijenjani tokom ovog unapređenja.

## Popravka koljena u metrima (28.10.2026.)

Prijavljen je problem pri skretanju tokom crtanja i pri spajanju dva kanala. Oba puta pozivaju isti proračun smjera. Stari GiveStartvectorAndAngle konstruisao je paralelne segmente uz fiksni odmak od 10 jedinica i tražio njihov presjek. Na trasama dužine nekoliko metara u m crtežu odmak je bio 10 m; presjek nije postojao na segmentima i vektori smjera mogli su ostati bez ispravnih vrijednosti.

Novi ElbowDirection.h računa ugao i smjer luka iz normalizovanih vektora dolaznog i odlaznog kanala, bez pomoćnog odmaka. Obje putanje crtanja koriste isti ispravljeni postupak. Nulti segmenti, prav nastavak i povratak istom osom odbijaju se bez formiranja neispravnog koljena; spajanje se provjerava prije promjene krajnjih tačaka kanala.

Regresioni test tests/test_elbow_direction.cpp obuhvata 120 kombinacija jedinica mm/cm/m, rotacije trase, lijevog/desnog skretanja i uglova 15–165 stepeni. Provjerava i rekonstruisane tangentne tačke luka prema istoj geometrijskoj formuli koju koristi TVS, uz četiri degenerisana slučaja. U AutoCAD-u treba ponoviti TVS_PIPE skretanje i TVS_CONNECTT na istim 3 m i 2 m trasama u sve tri jedinice.
