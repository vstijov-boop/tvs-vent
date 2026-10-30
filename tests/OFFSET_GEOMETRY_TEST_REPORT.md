# Stvarni izvještaj izvršavanja — izolovana geometrija promjene kote

Datum: 30.10.2026. Okruženje: Linux x86_64, Ubuntu g++ 13.3.0, Python 3.12.14.
C++98; -Wall -Wextra -Werror -pedantic. Nije korišćen ObjectARX SDK.

## Izolovani test

Komanda: `PYTHONDONTWRITEBYTECODE=1 python tests/test_offset_geometry.py`

Stvarni objedinjeni stdout/stderr:

```text
test_cpp98_geometry (__main__.OffsetGeometryTests.test_cpp98_geometry) ... PASS fixed numerical oracles (30/45/60 deg, joints, contours, A != B)
PASS explicit Lmin zero/positive/boundary, negative/zero Ri, projection policy
PASS rising/falling, shared projections, radii and tangent constraints (15..75 deg)
PASS placement: mm/cm/m, four headings, absolute elevation separated
PASS NoChange, exact 90 rejection, invalid/NaN/infinite/overflow, failure has no geometry
ALL PASSED: 314 checks
ok

----------------------------------------------------------------------
Ran 1 test in 0.321s

OK
```

## Postojeći regresioni testovi i novi test zajedno

Komanda: `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests -p 'test_*.py' -v`

Stvarni objedinjeni stdout/stderr:

```text
test_actual_radius_and_projection (test_elbow_radius_plane.ElbowRadiusPlane.test_actual_radius_and_projection) ... ok
test_kgtez_vertical_fallback_and_native_area (test_elbow_radius_plane.ElbowRadiusPlane.test_kgtez_vertical_fallback_and_native_area) ... ok
test_actual_elbow_geometry_conversion_expression (test_kgtez_units.KgtezUnits.test_actual_elbow_geometry_conversion_expression) ... ok
test_channel_and_fittings_mass_invariant (test_kgtez_units.KgtezUnits.test_channel_and_fittings_mass_invariant) ... ok
test_record_and_legacy_default (test_kgtez_units.KgtezUnits.test_record_and_legacy_default) ... ok
test_actual_handler_and_calculator (test_mleader_pressure.PressureLabel.test_actual_handler_and_calculator) ... ok
test_enter_default_and_options (test_move_reference_input.MoveReferenceInput.test_enter_default_and_options) ... ok
test_actual_helpers (test_move_topology.MoveTopology.test_actual_helpers) ... ok
test_cpp98_geometry (test_offset_geometry.OffsetGeometryTests.test_cpp98_geometry) ... PASS fixed numerical oracles (30/45/60 deg, joints, contours, A != B)
PASS explicit Lmin zero/positive/boundary, negative/zero Ri, projection policy
PASS rising/falling, shared projections, radii and tangent constraints (15..75 deg)
PASS placement: mm/cm/m, four headings, absolute elevation separated
PASS NoChange, exact 90 rejection, invalid/NaN/infinite/overflow, failure has no geometry
ALL PASSED: 314 checks
ok
test_real_adapter_fields_units_and_exclusions (test_sheet_metal_adapter.SheetMetalAdapter.test_real_adapter_fields_units_and_exclusions) ... ok
test_cpp_matches_kgtez (test_sheet_metal_kgtez.SheetMetalParity.test_cpp_matches_kgtez) ... Sheet metal calculations passed
ok
test_cpp_lisp_height_and_defaults (test_table_scale_kgtez.TableSizing.test_cpp_lisp_height_and_defaults) ... Table scaling and selection totals passed
ok

----------------------------------------------------------------------
Ran 12 tests in 1.481s

OK
```

314 provjera pripada jednom C++ test-programu koji Python prikazuje kao jedan test.
Ukupno 12 Python testova uključuje novi test i prethodnih 11; to nisu 314 zasebnih
Python testova. Prolazak nije potvrda rada unutar AutoCAD-a.

## Dopuna testova — nezavisne granice, ogledanje uglova i poprečno pozicioniranje

30.10.2026, isto okruženje i iste komande. Geometrijski modul i stroga politika
nisu mijenjani. Brojevi ispod su novo stvarno izvršavanje, prethodni rezultat 314
ostaje istorijski zapis.

Analitički ulazi visine fiksirani su prije poziva solve, za R=200 mm:

| Ugao | H za Lmin=100 mm | H za Lmin=0 |
|---|---|---|
| 30° | 450 - 200√3 | 400 - 200√3 |
| 45° | 400 - 150√2 | 400 - 200√2 |
| 60° | 200 + 50√3 | 200 |

Za svaki je provjereno H−0.000001 mm (TooShort), zaokružena analitička granica
(Ok ili numerički TooShort) i H+0.000001 mm (Ok). Na samoj granici namjerno se
ne propisuje smjer zaokruživanja biblioteke sin/cos. Ako je rezultat Ok, dužina
mora odgovarati nezavisnom Lmin do 1e-10 mm. solve sa Lmin=0 koristi se dodatno
samo za štampu dijagnostičke dužine, nikad za definisanje ulaza/pragova.

Na ovom okruženju 45°/100 mm daje 99.999999999999986 mm i TooShort. Ostale
navedene analitičke granice prihvaćene su; nulti međukomad može imati pozitivan
ostatak reda 1e-14 mm. Tolerancija NoChange ne ublažava strogu Lmin provjeru.

Ogledanje z mijenja znak vertikalne lokalne strane presjeka, ali čuva poprečnu
stranu: svih 16 uglova se poredi sa indeksom k XOR 2, za pet uglova trase.
Adapter place koristi i nezavisni 3-4-5 smjer: s=120,t=35,z=-40 →
x=44,y=117,z=-40 prije dodavanja početne tačke, za mm/cm/m.

### Izolovani test — stvarni izlaz

```text
test_cpp98_geometry (__main__.OffsetGeometryTests.test_cpp98_geometry) ... PASS fixed numerical oracles (30/45/60 deg, joints, contours, A != B)
PASS explicit Lmin zero/positive/boundary, negative/zero Ri, projection policy
BOUNDARY 30deg Lmin=100 H=103.58983848622454 below=TooShort exact-decimal=Ok above=Ok Lcomputed=100.00000000000003
BOUNDARY 30deg Lmin=0 H=53.589838486224544 below=TooShort exact-decimal=Ok above=Ok Lcomputed=2.8421709430404014e-14
BOUNDARY 45deg Lmin=100 H=187.86796564403573 below=TooShort exact-decimal=TooShort above=Ok Lcomputed=99.999999999999986
BOUNDARY 45deg Lmin=0 H=117.15728752538099 below=TooShort exact-decimal=Ok above=Ok Lcomputed=0
BOUNDARY 60deg Lmin=100 H=286.60254037844385 below=TooShort exact-decimal=Ok above=Ok Lcomputed=100.00000000000006
BOUNDARY 60deg Lmin=0 H=200 below=TooShort exact-decimal=Ok above=Ok Lcomputed=6.5637126361892324e-14
PASS independent analytic Lmin boundaries (30/45/60 deg, Lmin=0/100)
PASS rising/falling, ALL joint corners mapped k XOR 2, shared projections and tangency (15..75 deg)
PASS placement: mm/cm/m, four headings + nonzero s/t/z arbitrary-heading oracle, absolute elevation separated
PASS NoChange, exact 90 rejection, invalid/NaN/infinite/overflow, failure has no geometry
ALL PASSED: 589 checks
ok

----------------------------------------------------------------------
Ran 1 test in 0.226s

OK
```

### Zajednički regresioni testovi — stvarni izlaz

```text
test_actual_radius_and_projection (test_elbow_radius_plane.ElbowRadiusPlane.test_actual_radius_and_projection) ... ok
test_kgtez_vertical_fallback_and_native_area (test_elbow_radius_plane.ElbowRadiusPlane.test_kgtez_vertical_fallback_and_native_area) ... ok
test_actual_elbow_geometry_conversion_expression (test_kgtez_units.KgtezUnits.test_actual_elbow_geometry_conversion_expression) ... ok
test_channel_and_fittings_mass_invariant (test_kgtez_units.KgtezUnits.test_channel_and_fittings_mass_invariant) ... ok
test_record_and_legacy_default (test_kgtez_units.KgtezUnits.test_record_and_legacy_default) ... ok
test_actual_handler_and_calculator (test_mleader_pressure.PressureLabel.test_actual_handler_and_calculator) ... ok
test_enter_default_and_options (test_move_reference_input.MoveReferenceInput.test_enter_default_and_options) ... ok
test_actual_helpers (test_move_topology.MoveTopology.test_actual_helpers) ... ok
test_cpp98_geometry (test_offset_geometry.OffsetGeometryTests.test_cpp98_geometry) ... PASS fixed numerical oracles (30/45/60 deg, joints, contours, A != B)
PASS explicit Lmin zero/positive/boundary, negative/zero Ri, projection policy
BOUNDARY 30deg Lmin=100 H=103.58983848622454 below=TooShort exact-decimal=Ok above=Ok Lcomputed=100.00000000000003
BOUNDARY 30deg Lmin=0 H=53.589838486224544 below=TooShort exact-decimal=Ok above=Ok Lcomputed=2.8421709430404014e-14
BOUNDARY 45deg Lmin=100 H=187.86796564403573 below=TooShort exact-decimal=TooShort above=Ok Lcomputed=99.999999999999986
BOUNDARY 45deg Lmin=0 H=117.15728752538099 below=TooShort exact-decimal=Ok above=Ok Lcomputed=0
BOUNDARY 60deg Lmin=100 H=286.60254037844385 below=TooShort exact-decimal=Ok above=Ok Lcomputed=100.00000000000006
BOUNDARY 60deg Lmin=0 H=200 below=TooShort exact-decimal=Ok above=Ok Lcomputed=6.5637126361892324e-14
PASS independent analytic Lmin boundaries (30/45/60 deg, Lmin=0/100)
PASS rising/falling, ALL joint corners mapped k XOR 2, shared projections and tangency (15..75 deg)
PASS placement: mm/cm/m, four headings + nonzero s/t/z arbitrary-heading oracle, absolute elevation separated
PASS NoChange, exact 90 rejection, invalid/NaN/infinite/overflow, failure has no geometry
ALL PASSED: 589 checks
ok
test_real_adapter_fields_units_and_exclusions (test_sheet_metal_adapter.SheetMetalAdapter.test_real_adapter_fields_units_and_exclusions) ... ok
test_cpp_matches_kgtez (test_sheet_metal_kgtez.SheetMetalParity.test_cpp_matches_kgtez) ... Sheet metal calculations passed
ok
test_cpp_lisp_height_and_defaults (test_table_scale_kgtez.TableSizing.test_cpp_lisp_height_and_defaults) ... Table scaling and selection totals passed
ok

----------------------------------------------------------------------
Ran 12 tests in 1.443s

OK
```

589 je broj C++ provjera unutar jednog Python testa; svih 12 Python testova
uključuje prethodnih 11 i taj runner. AutoCAD integracioni testovi nisu izvršeni.
Plan integracije je u geometry/INTEGRATION_PLAN.md i nije implementiran.
