# Obilazak — kombinacije presjeka i ukrštanja

Matrica (prolaz ispod/iznad, ugao koljena 30/45/60, mm/cm/m važe za sve):

| Trasa \ Prepreka | pravougaona 90° | kružna 90° | pravougaona koso | kružna koso |
|---|---|---|---|---|
| pravougaona | DEV radi (praktično potvrđeno) | **F1+F2a: DEV paket** | **F1+F2a: DEV paket** | **F1+F2a: DEV paket** |
| kružna | F1 checker gotov; F2b članovi | F1; F2b | F1; F2b | F1; F2b |

Bočni obilazak u osnovi (F3) i dalje čeka definiciju prepreke.

## Model provjere (F1, `geometry/bypass/Candidate.h`)

- **Kružna prepreka:** presjek je krug (Ø + izolacija). Region je proširen pravougaonik
  (jezgro + radijus), pa ista provjera pokriva i pravougaonik i krug.
- **Koso ukrštanje pod uglom β u osnovi:** tačka trase (s,t,z) je od ose prepreke na
  σ = s·sinβ ± t·cosβ. Presjek prepreke se u ravni trase izduži 1/sinβ i razvuče za
  poluširinu trase·cotβ. Važi: stvarni 3D razmak ≥ sinβ × razmak u presjeku, pa se u
  presjeku traži razmak/sinβ. To je konzervativno; pri 45° traži oko 41 % više u presjeku.
  Ispod 14,5° (sinβ < 0,25) se odbija.
- **Kružna trasa:** provjerava se kao presjek d × d. To je tačno za prizmatičnu prepreku,
  a za koso ukrštanje cijev leži unutar korišćene ploče, pa je i to konzervativno.
- **Segmentna koljena TVS-a:** koliko poligon izlazi van glatkog luka (`elbowExcessMm`)
  dodaje se zaštitnoj zoni sklopa (razmak, granice Z, međusobno preklapanje članova).
- Podrazumijevane vrijednosti daju bit-za-bit iste rezultate kao ranije.
- Regresija `test_bypass_sections`: 72 kombinacije (trasa × prepreka × 90/60/45° ×
  ispod/iznad × 30/45/60), svaka sa potvrđenim kandidatom. Gusto uzorkovanje STVARNE 3D
  geometrije potvrđuje razmak i da je presjek donja granica.

## F2a — pravougaona trasa, kružna i kosa prepreka (u ovom paketu)

- Prepreka smije biti kružni krut TVS kanal (B = 0) ili pravougaona, pod pravim ili kosim
  uglom ≥ 14,5°. Ishodište je presjek osa u osnovi.
- Dijalog prikazuje Ø i ugao ukrštanja. Presjek u dijalogu i u detalju je stvarni presjek
  u ravni trase: krug, ili za koso izduženi pravougaonik/elipsa. Osnova prepreke je
  paralelogram duž njene ose.
- Članovi obilaska su isti pravougaoni članovi kao do sada. Writer, zapis sklopa i
  `TVS_BYPASS_DETALJ` rade bez nove šeme.

## F2b — kružna trasa (sljedeći korak, nije u ovom paketu)

- Kružna koljena po TVS podešavanju:
  - konstantni i k·D radijus su čist luk;
  - **specifični radijus** ima prirubnicu 45 mm za Ø < 630, pa se susjedni ravni članovi
    skraćuju za prirubnicu i provjeravaju minimumi;
  - segmentna koljena dobijaju dodatak zaštitnoj zoni prema TVS poligonu.
- Novi kružni članovi (TAP gore/dolje, kružne cijevi), V1/roditelj validacija za kružne,
  offset servis za kružni izvor (capture/topologija), obračun dužina, detalj.
- Rizik: crtanje kružnih koljena gore/dolje pod 30/45/60 u TVS-u nije praktično provjereno.

## F3 — bočni obilazak u osnovi

Prepreka mora biti konačna u osnovi, jer pravu prepreku koja siječe trasu nije moguće
obići bočno. Treba odlučiti šta je prepreka: stub ili zid (AutoCAD objekat), vertikalni
TVS kanal ili kraj kanala. Tek onda se definišu provjera (ista matematika u ravni osnove)
i izbor objekta.
