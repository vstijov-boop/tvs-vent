# Ispravka radijusa pravougaonih koljena gore/dolje

29.10.2026 — grana `unapredjenja-masa`.

Kod pravougaonog kanala A je širina u osnovi, a B visina. Promjena Form na
Gore/Nadolje ne zamjenjuje sačuvane A/B. Ranije je setRadius uvijek koristio A,
a obračun lima uvijek srednji radijus Ri+A/2. Za vertikalno koljeno oba treba
računati prema B: Ri=k*B i Rc=Ri+B/2.

Sada `get_radius()` računa radijus prema trenutnoj formi i podešavanjima, bez
zavisnosti od prethodnog redraw-a. `setRadius()` koristi isti proračun, a put_Form
odmah osvježava radijus. Konstantni radijus ostaje konstantan; specifični režim
koristi dimenziju u ravni savijanja i postojeći prag 355 mm. Okrugli/fleksibilni
režimi zadržavaju prethodnu dimenziju prečnika. Ne mijenjaju se dimenzije presjeka,
IDL/COM potpisi ni format DWG-a.

Native površina i masa koriste isti izračunati Ri i orijentisani presjek B×A za
vertikalna pravougaona koljena. Debljina i dalje zavisi od najveće stranice.
Sve native tabele i Properties koriste zajednički getSheetMetal. Usklađen je i
međukorak stare specifikacije. Rezerva nije dodata native masi.

KGTEZ za vertikalno koljeno ne tumači projekciju kao razvijeni luk. Koristi native
SheetArea kada je dostupna, što obuhvata i fiksni radijus. Za stariji dodatak bez
te osobine fallback primjenjuje postojeći koeficijent na B. KGTEZ rezerva i
opcija debljine 2 mm ostaju kao ranije; okrugli elementi se i dalje preskaču.

## Provjera

- `test_elbow_radius_plane.py` kompajlira stvarne get_radius/setRadius/put_Form i
  stvarni dio geometrije koji računa priključke pravougaonog koljena gore/dolje.
  Provjerava 400×200, 200×600, 600×200 i 250×250, oba vertikalna smjera, uglove
  45°/90°, četiri rotacije, mm/cm/m, prelazak nazad u osnovu, konstantni i specifični
  radijus i okrugle presjeke.
- Za 400×200, k=0,5: osnova Ri=200/Rc=400 mm; gore/dolje Ri=100/Rc=200 mm.
  U postojećoj projekciji 90° zahvat do priključka postaje 200 mm. UpDown komanda
  koristi taj priključak za skraćivanje novog kanala.
- Native adapter test koristi stvarni getter i getSheetMetal: vertikalno 400×200,
  k=0,5, 90° daje površinu 0,12π m²; fiksni Ri=150 daje 0,15π m².
- KGTEZ test pokriva vertikalni fallback i prioritet native površine.
- Svih 11 Python testova prolazi. Rad u AutoCAD-u još treba potvrditi.

## Postojeći crteži

Ispravka važi i pri ponovnom crtanju postojećih vertikalnih koljena. Njihov zahvat
može se promijeniti. Ranije pogrešno skraćeni susjedni kanali nisu automatski
prepravljeni: svjetsko crtanje ne smije mijenjati druge objekte baze. Postojeće
spojeve provjeriti na kopiji crteža; zahvaćena koljena po potrebi ponovo formirati.
Nova UpDown crtanja skraćuju kanal prema novom priključku. Ova dorada ne mijenja
postojeću konstrukciju promjene kote sa dva koljena niti njen tretman ugla 89,4°.

Pri instalaciji zamijeniti ARX i DBX zajedno, uz zatvoren AutoCAD, i ažurirati
KGTEZ.lsp iz istog paketa. Sačuvati prethodne fajlove prije zamjene.


## Potvrđeni buildovi

Kod: `4b157b48af8819316ebffa00522af95eabefbb62`, 29.10.2026.
Oba Windows builda su uspješna: 0 grešaka i 2 postojeća LNK4099 upozorenja
za SDK rxapi.pdb. Svih 11 lokalnih Python testova prolazi.

- AutoCAD 2020 x64: [paket](https://github.com/vstijov-boop/tvs-vent/actions/runs/40934777858/artifacts/12923394815).
  SHA256: `a4af059759570dc644517425503ef238bdf8890bce3fb04587ef4502a93eadc5`.
- AutoCAD 2022 x64: [paket](https://github.com/vstijov-boop/tvs-vent/actions/runs/40934777777/artifacts/12923698573).
  SHA256: `88b6f6ab38dc39356c56ffe341f176797cd608e7090bbad9350e85c1b65d9e91`.

Provjereni su ZIP CRC, x64 PE format, zavisnosti acdb23/acdb24, BUILD.txt,
KGTEZ i oba CUIX fajla prema izvoru. Potvrda ponašanja unutar AutoCAD-a još
je potrebna. CUIX fajlovi se ovom ispravkom nisu mijenjali.
