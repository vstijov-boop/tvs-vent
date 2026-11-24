"""Oprema ide sa mrezom u TVS_MOVE / TVS_STRETCH (EquipmentFollow.h, korisnik 24.11.)."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Follow(unittest.TestCase):
 def test_hooks(self):
  m=text('TVS_Ventilation_ARX/TVSMoveCommand.h')
  self.assertEqual(m.count('equipmentFollowHook()?equipmentFollowHook()('),2)   # TVS_MOVE network + TVS_STRETCH
  self.assertEqual(m.count('hideTerminals(nodes,bodies)'),2);self.assertEqual(m.count('sameTvs(nodes,current,1e-10/factor)'),2)
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp');self.assertIn('tvsEquipmentFollow::install();',e);self.assertIn('#include "EquipmentFollow.h"',e)
 def test_follow(self):
  s=text('TVS_Ventilation_ARX/EquipmentFollow.h')
  self.assertIn('tvsEquipmentStore::anchor(b,r);',s);self.assertIn('m.conflict',s)
  for w in ('near','far','small','min','max'):self.assertNotRegex(s,r'\b%s\b'%w)
  self.assertEqual(s.count('{'),s.count('}'))
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/EquipmentFollow.h').read_bytes().startswith(b'\xef\xbb\xbf'))
if __name__=='__main__':unittest.main()
