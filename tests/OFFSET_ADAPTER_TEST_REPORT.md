# Stvarni izvještaj adaptera Result → postojeći geometrijski parametri

Datum: 30.10.2026. Linux x86_64, g++ 13.3.0, Python 3.12.14.
C++98, -Wall -Wextra -Werror -pedantic, bez ObjectARX SDK-a.

## Izolovani adapter test

Komanda: `PYTHONDONTWRITEBYTECODE=1 python tests/test_offset_adapter.py`
Python runner kompajlira privremeni C++ program sa -I korijenom repozitorijuma,
a zatim ga stvarno izvršava. Program sadrži izdvojene stvarne izraze iz TAP izvora
i test slučajeve iz offset_adapter_cases.inc. Precizna ograničenja zamjenskog
modela opisana su u geometry/OFFSET_ADAPTER.md.

```text
test_extracted_tap_branch (__main__.OffsetAdapterTests.test_extracted_tap_branch) ... PASS 540 mappings through extracted real TAP radius/form/projection expressions (30/45/60, rise/fall, 5 headings, mm/cm/m, A!=B, 2 radius modes)
PASS fixed placement oracle and independent drawing Z / physical reference elevation
PASS invalid/absent geometry, mismatched pair, lost coordinate precision, Lmin=0
STUB LIMITATION: no ObjectARX runtime, DB, drawing pipeline or real AcGe rotation executed
ALL PASSED: 28641 checks
ok

----------------------------------------------------------------------
Ran 1 test in 0.207s

OK
```

## Svi Python regresioni testovi

Komanda: `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests -p 'test_*.py' -v`

```text
test_actual_radius_and_projection (test_elbow_radius_plane.ElbowRadiusPlane.test_actual_radius_and_projection) ... ok
test_kgtez_vertical_fallback_and_native_area (test_elbow_radius_plane.ElbowRadiusPlane.test_kgtez_vertical_fallback_and_native_area) ... ok
test_actual_elbow_geometry_conversion_expression (test_kgtez_units.KgtezUnits.test_actual_elbow_geometry_conversion_expression) ... ok
test_channel_and_fittings_mass_invariant (test_kgtez_units.KgtezUnits.test_channel_and_fittings_mass_invariant) ... ok
test_record_and_legacy_default (test_kgtez_units.KgtezUnits.test_record_and_legacy_default) ... ok
test_actual_handler_and_calculator (test_mleader_pressure.PressureLabel.test_actual_handler_and_calculator) ... ok
test_enter_default_and_options (test_move_reference_input.MoveReferenceInput.test_enter_default_and_options) ... ok
test_actual_helpers (test_move_topology.MoveTopology.test_actual_helpers) ... ok
test_extracted_tap_branch (test_offset_adapter.OffsetAdapterTests.test_extracted_tap_branch) ... PASS 540 mappings through extracted real TAP radius/form/projection expressions (30/45/60, rise/fall, 5 headings, mm/cm/m, A!=B, 2 radius modes)
PASS fixed placement oracle and independent drawing Z / physical reference elevation
PASS invalid/absent geometry, mismatched pair, lost coordinate precision, Lmin=0
STUB LIMITATION: no ObjectARX runtime, DB, drawing pipeline or real AcGe rotation executed
ALL PASSED: 28641 checks
ok
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
Ran 13 tests in 1.682s

OK
```

28641 su provjere u jednom test-programu, ne broj zasebnih Python testova.
Ukupno 13 Python testova uključuje novi adapter test i prethodnih 12.
Nema stvarnog ObjectARX izvršavanja, GUI-ja, podataka B niti Windows builda.
Obavezne praktične provjere navedene su u geometry/OFFSET_ADAPTER.md.

## Granice promjene

Dodati: geometry/OffsetAdapter.h, geometry/OFFSET_ADAPTER.md,
geometry/OFFSET_ASSEMBLY_B_PROPOSAL.md, tests/test_offset_adapter.py,
tests/offset_adapter_cases.inc, tests/OFFSET_ADAPTER_TEST_REPORT.md.
Nijedna ranija datoteka nije izmijenjena. Naročito OffsetGeometry.h, njegova
stroga politika i testovi iz 30106e8 ostaju identični.
