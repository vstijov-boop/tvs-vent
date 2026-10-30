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
