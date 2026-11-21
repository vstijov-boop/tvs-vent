"""Oprema na mrezi u citacima mreze: geometry/EquipmentFlow.h (cisto) i ARX strana
(EquipmentNodes.h, TVS_PROTOK / TVS_PAD / TVS_DPD_IZVOZ / TVS_DIM_PRATI / TVS_PROVJERA)."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class EquipmentFlow(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_equipment_flow.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_ascii(self):
  for h in ('EquipmentFlow.h','PressureDrop.h','PressureSheet.h','DpdExport.h','NetworkCheck.h'):(ROOT/'geometry'/h).read_bytes().decode('ascii')
  (ROOT/'TVS_Ventilation_ARX/EquipmentNodes.h').read_bytes().decode('ascii')
 def test_nodes(self):
  n=text('TVS_Ventilation_ARX/EquipmentNodes.h')
  self.assertIn('tvsEquipmentStore::read(b,r)',n);self.assertIn('tvsEquipmentFlow::unitScale(r.mmPerUnit,f)',n)
  self.assertIn('tvsEquipmentFlow::pickLevel(',n);self.assertIn('db->currentSpaceId()',n)
 def test_flow(self):
  f=text('TVS_Ventilation_ARX/FlowCommand.h')
  self.assertIn('tvsEquipmentNodes::append(db,net.nodes,&recs,tol)',f)
  self.assertIn('tvsEquipmentFlow::terminalFlow(',f)
  self.assertIn('tvsEquipmentStore::write(b,rec)',f)                 # TVS_PROTOK: the flow into the record
  self.assertIn('if(net.linked[i]&&!net.records[i].terminal())continue;',f)
  for c,cmd in (('PressureCommand.h','TVS_PAD'),('DpdExportCommand.h','TVS_DPD_IZVOZ')):
   self.assertIn('tvsFlow::equipmentFlows(net,el,r,root,port,tol,L"%s");'%cmd,text('TVS_Ventilation_ARX/'+c))
  self.assertIn('t.kind=tvsPressure::Equipment;t.equipment=rec;',text('TVS_Ventilation_ARX/PressureCommand.h'))
 def test_follow_and_check(self):
  s=text('TVS_Ventilation_ARX/SizeFollow.h')
  self.assertIn('return L"oprema";',s);self.assertEqual(s.count('tvsEquipmentNodes::append(db,nodes,NULL,0)'),2)
  c=text('TVS_Ventilation_ARX/CheckCommand.h')
  self.assertIn('oprema: priključak ne naliježe na kanal',c);self.assertIn('tvsEquipmentNodes::append(db,nodes,NULL,0)',c);self.assertIn('int count[8]',c)
 def test_encoding(self):
  for p in ('FlowCommand.h','PressureCommand.h','DpdExportCommand.h','SizeFollow.h','CheckCommand.h'):
   b=(ROOT/'TVS_Ventilation_ARX'/p).read_bytes();self.assertTrue(b.startswith(b'\xef\xbb\xbf'),p);self.assertNotIn(b'\r\n',b,p)
if __name__=='__main__':unittest.main()
