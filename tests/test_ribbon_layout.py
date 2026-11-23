"""Ribbon TVS_Vent po panelima (tools/ribbon_layout.py, odluka korisnika 23.11.): velika glavna komanda,
ostale male ikone bez teksta u najvise tri reda; toolbar i meni ostaju."""
import importlib.util,pathlib,unittest,zipfile
import xml.etree.ElementTree as E
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ribbon_layout',ROOT/'tools'/'ribbon_layout.py');rl=importlib.util.module_from_spec(spec);spec.loader.exec_module(rl)
class Ribbon(unittest.TestCase):
 def setUp(self):
  z=zipfile.ZipFile(ROOT/'Installer/TVS_VENT/TVS_VENT.cuix')
  self.r=E.fromstring(z.read('RibbonRoot.cui'));self.m=E.fromstring(z.read('MenuGroup.cui'));self.t=E.fromstring(z.read('ToolbarRoot.cui'))
 def test_panels(self):
  panels=[p.get('Text') for p in self.r.iter('RibbonPanelSource')]
  self.assertEqual(panels,[p[0] for p in rl.LAYOUT])
  tab=[t for t in self.r.iter('RibbonTabSource') if t.get('Text')=='TVS_Vent'][0]
  self.assertEqual(len(tab.findall('RibbonPanelSourceReference')),len(rl.LAYOUT))
  for p in self.r.iter('RibbonPanelSource'):
   for rp in p.iter('RibbonRowPanel'):self.assertLessEqual(len(rp.findall('RibbonRow')),3)
   for b in p.iter('RibbonCommandButton'):self.assertIn(b.get('ButtonStyle'),('LargeWithText','SmallWithoutText'))
 def test_every_command_once_and_toolbar_kept(self):
  ids=[b.get('MenuMacroID') for b in self.r.iter('RibbonCommandButton')]
  self.assertEqual(len(ids),len(set(ids)))
  macros={m.get('UID') for m in self.m.iter('MenuMacro')}
  self.assertTrue(set(ids)<=macros)
  tb={b.get('MenuMacroID') for b in self.t.iter() if b.get('MenuMacroID')}
  for c in ('MMU_TVS_OPREMA','MMU_TVS_RESETKA','MMU_TVS_PAD','MMU_TVS_MOVE'):self.assertIn(c,tb);self.assertIn(c,ids)
 def test_idempotent(self):
  if rl.E is None:self.skipTest('lxml nije instaliran')
  z=zipfile.ZipFile(ROOT/'Installer/TVS_VENT/TVS_VENT.cuix')
  r=rl.E.fromstring(z.read('RibbonRoot.cui'));before=rl.E.tostring(r);rl.build(r,rl.E.fromstring(z.read('MenuGroup.cui')))
  self.assertEqual(rl.E.tostring(r),before)
if __name__=='__main__':unittest.main()
