"""TVS_SETTINGS level reference (Vrh/Centar/Dno): remembered, value converted to/from the axis."""
import pathlib,re,unittest
from entry_source import entry_source
root=pathlib.Path(__file__).resolve().parents[1]
class ElevationReference(unittest.TestCase):
 def test_dialog_round_trip(self):
  src=entry_source()
  self.assertIn('extern int globalElevRef;',(root/'TVS_Ventilation_ARX/Settings.h').read_text(encoding='utf-8',errors='replace'))
  self.assertIn('int globalElevRef = 0;',(root/'TVS_Ventilation_ARX/Settings.cpp').read_text(encoding='utf-8',errors='replace'))
  self.assertNotIn('dg.ElevMid = true;',src,'reference no longer forced to the axis')
  self.assertNotRegex(src,r'globalElevMid = dg\.Elev;','entered value is not stored as the axis unchanged')
  self.assertRegex(src,r'(?s)dg\.ElevUp = globalElevRef == 1;.*dg\.Elev = globalElevMid \+ \(globalElevRef == 1 \? elevHalf : globalElevRef == 2 \? -elevHalf : 0\);.*DoModal')
  self.assertRegex(src,r'(?s)globRound = false;\s*// Vrh/Dno.*globalElevRef = dg\.ElevUp == 1 \? 1 : dg\.ElevDown == 1 \? 2 : 0;.*globalElevMid = dg\.Elev - \(globalElevRef == 1 \? half : globalElevRef == 2 \? -half : 0\);\s*globalElev = globalElevMid;.*if \(a == globSizeA && b == globSizeB\)')
  # Arithmetic of the same expressions: Vrh 2700 for B=150 -> axis 2625 -> reopens as Vrh 2700.
  for ref,expected in ((1,2625),(2,2775),(0,2700)):
   half=150/2;axis=2700-(half if ref==1 else -half if ref==2 else 0);self.assertEqual(axis,expected)
   self.assertEqual(axis+(half if ref==1 else -half if ref==2 else 0),2700)
if __name__=='__main__':unittest.main()
