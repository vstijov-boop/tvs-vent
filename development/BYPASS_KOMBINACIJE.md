# Obilazak — kombinacije presjeka i ukrštanja (vertikalno, iznad/ispod)

Obim: vertikalni obilazak iznad i ispod prepreke, i to:
- pravougaona i kružna trasa;
- pravougaona i kružna prepreka;
- upravno i podržano koso ukrštanje;
- samostalna komanda `TVS_BYPASS` i opcija `Obilazak/O` tokom `TVS_PIPE`/`TVS_DRAW`;
- trajni upis i kotirani bočni detalj (`TVS_BYPASS_DETALJ`).

Bočni obilazak u osnovi: **van obima po odluci korisnika — ručno crtanje.**

## Matrica

| Trasa \ Prepreka | pravougaona 90° | kružna 90° | pravougaona koso | kružna koso |
|---|---|---|---|---|
| pravougaona | upis ✔ (praktično potvrđeno) | upis ✔ (F2a) | upis ✔ (F2a) | upis ✔ (F2a) |
| kružna | **upis ✔ (F2b)** | **upis ✔ (F2b)** | **upis ✔ (F2b)** | **upis ✔ (F2b)** |

„Upis“ znači da writer trajno upisuje stvarne TVS članove. Za kružnu trasu su to kružne
cijevi Ø D (`ThisRound`, `SizeB = 0`) i kružna koljena gore/dolje Ø D, konstantnog
radijusa, sa `TypeRoundTap` iz TVS podešavanja. Uz članove idu V1 trojke i roditelj, uz
iste provjere snapshota, rollback i ograničenja konektora.

### Režimi radijusa kružnih koljena (TVS_SETTINGS)

| Režim | Status | Napomena |
|---|---|---|
| konstantni | podržan | Ri = konstanta TVS-a; glatko koljeno |
| k·D (varijabilni) | podržan | Ri = k × D; glatko koljeno |
| specifični | **odbijen** porukom prije dijaloga | vidi „45 mm“ ispod; nije tiho zamijenjen |
| `TypeRoundTap` (sekcijsko/glatko) | upisuje se kako je podešeno | u konstantnom i k·D režimu TVS ga ne koristi u geometriji (provjereno u `subWorldDraw`) |

Pravougaona koljena se ponašaju kao i ranije: Ri po TVS režimu, upisuju se kao konstantni.

## Geometrija kružnog koljena gore/dolje (iz `TVS_TAP::subWorldDraw`)

- TVS za kružna koljena gore/dolje koristi drugu parametrizaciju od pravougaonih:
  `CenterPoint = M − rc/cos(θ/2)·dir(sa+θ/2)`, `MA = C + rc·dir(sa)`,
  `MC = MA + u·rc·sinθ`, gdje je rc = Ri + D/2.
- Pravougaono koljeno ima `MC = M` i `MA = M − u·rc·sinθ`.
- Oba daju iste dvije spojne tačke u osnovi. `geometry/RoundElbow.h` računa MiddlePoint
  kružnog koljena iz tačaka koje daje zajednički adapter.
- Regresija `test_bypass_round_elbow` provjerava dvije stvari:
  - da su formule i dalje tekst iz DBX izvora;
  - da se tačke poklapaju na 648 slučajeva (30/45/60°, razni Ri, D i smjerovi).
- **Nije provjereno u AutoCAD-u:**
  - izgled kružnih koljena gore/dolje pod 30/45/60°;
  - da je `Elev` kružnog koljena kota MA, kao kod pravougaonog (pretpostavka iz zajedničkog
    `describe()`).

  Zato je to prvi praktični test.

## Parametar 45 mm (specifični režim kružnih koljena)

Šta pokazuje kod `TVS_TAP::subWorldDraw`, grana `RadiusTypeRound_RadiusSpecific`:
- `flanez = 45 mm` važi samo za Ø < 630, inače je 0.
- Spojne tačke su pomjerene tangentno za flanez na oba kraja:
  - MA je pomjerena za −flanez duž početne tangente;
  - kraj je na `MidRad·sin(θ+midRl) = rc·sinθ + flanez·cosθ`, što je projekcija ravnog
    nastavka dužine flanez duž krajnje tangente.
- Nastavak je dio samog fitinga, jer su MA i MC tačke na krajevima nastavaka. Susjedni
  pravi komadi bi zato morali biti kraći za flanez na svakom kraju, i to samo jednom.
- U osnovi spoljni zid crta kao poligon segmenata, sa uglovima na
  (Ri+D)/cos(α), α = θ/4 za θ > 45,6°, inače θ/2. Iz toga slijedi dodatak ovojnici
  (Ri+D)(1/cos α − 1).
- Ri je D za Ø < 355, inače D/2.

Za ovu isporuku je režim **odbijen**. Za upis bi trebalo:
- skraćenje pravih komada i kota krajeva nastavaka na kosim komadima;
- minimumi između stvarnih spojeva;
- validacija spojnih tačaka sa nastavkom;
- isti obračun i prikaz.

To nije urađeno ni praktično provjereno, pa se režim ne zamjenjuje tiho drugim. Checker
već prima dodatak ovojnici (`elbowExcessMm`), pa je to sljedeći korak ako ga zatražiš.

## Provjera i garancije

- Prihvatanje se zasniva na konzervativnoj matematičkoj provjeri u ravni trase, sa
  intervalnom aritmetikom.
- Gusto 3D uzorkovanje u testovima je **regresiona kontrola**, a ne dokaz za sve tačke
  neprekidnog tijela.
- Razlikuju se tri stvari:
  1. **Stvarni presjek prepreke u ravni trase:** ono što dijalog i detalj crtaju. To je
     krug ili pravougaonik, a za koso ukrštanje elipsa ili pravougaonik izdužen 1/sinβ.
  2. **Konzervativni provjerni gabarit:** taj presjek razvučen za poluširinu trase
     (koso), a za kružnu trasu ploča širine D oko cijevi.
  3. **Garantovana donja granica prostornog razmaka:** sinβ × razmak do gabarita.
     - Upravno ukrštanje (β = 90°): razmak u presjeku jeste prostorni razmak i prikazuje
       se kao najmanji razmak.
     - Koso ukrštanje: prikazuje se samo kao „garantovana donja granica“, nikad kao
       izmjereni minimum.
- Detalj kružne trase je presjek kroz osu: traka širine D, uz stvarni prečnik u napomeni.
- Količine idu preko postojećeg V1 obračuna fizičke dužine kosih komada, nezavisno od
  presjeka. Kružni članovi se specificiraju po dužini i prečniku. KGTEZ kružne preskače
  kao i do sada.

## Praktični test (redom)

1. Kružna trasa i pravougaona prepreka, 45°, konstantni radijus. Proći redom:
   - `TVS_BYPASS` → Pronađi → Prihvati;
   - nastavak crtanja (`Obilazak/O` u `TVS_PIPE`);
   - save/reopen;
   - `TVS_BYPASS_DETALJ`.

   Provjeriti da su koljena kružna i da se dodiruju sa kosim komadima i H u osnovi, i da
   specifikacija prikazuje kružne članove.
2. Isto sa k·D radijusom, 30° i 60°, ispod i iznad.
3. Kružna prepreka (90°), pa koso ukrštanje (60°, 45°), za kružnu i pravougaonu trasu.
4. Specifični režim kružnih koljena: očekuje se poruka odbijanja bez izmjena.
