"""Poravnanje prelaza po visini: record, end offset, settings, Properties, dialog, drawing (contracts + math)."""
import pathlib,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig',errors='replace')
class TransAlign(unittest.TestCase):
 def test_end_offset(self):
  # the record helper is header-only; check the offset rule in isolation
  src=text('TVS_Ventilation_DBX/TransAlignData.h')
  i=src.index('inline double height');j=src.index('inline int read')
  cpp='#include <cassert>\n#include <cmath>\nnamespace t{enum Align {Axis=0,Top=1,Bottom=2};\n'+src[i:j]+'}\nint main(){using namespace t;\n'\
      ' assert(std::fabs(endOffset(Top,400,300,400,200)-50)<1e-12);    /* top flush: 300 -> 200, axis +50 */\n'\
      ' assert(std::fabs(endOffset(Bottom,400,300,400,200)+50)<1e-12); /* bottom flush: axis -50 */\n'\
      ' assert(endOffset(Axis,400,300,400,200)==0);\n'\
      ' assert(std::fabs(endOffset(Top,250,0,400,200)-25)<1e-12);       /* round D250 -> 400x200 */\n}\n'
  with tempfile.TemporaryDirectory() as f:
   p=pathlib.Path(f)/'a.cpp';p.write_text(cpp);t=pathlib.Path(f)/'a';compile_cpp(p,t);subprocess.run([str(executable(t))],check=True)
 def test_contracts(self):
  d=text('TVS_Ventilation_DBX/TransAlignData.h');self.assertIn('L"TVS_TRANS_VERT_V1"',d)
  c=text('TVS_Ventilation_ARX/TransAlignCommand.h')
  self.assertIn('L"\\nPrelaz: kota ose na kraju se mijenja za %+.0f mm. Koja strana ostaje [Pocetak/Kraj] <Pocetak>: "',c)
  self.assertIn('if(!tvsElevationShift::side(db,work[i].id,keepEnd?0:1,tol,ids,loop))continue;',c)
  self.assertIn('acDocManager->sendStringToExecute(doc,L"_TVS_PRELAZ_KOTA\\n",false,true,false);',c)
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('Ventilation_ARX, TVS_PRELAZ_KOTA, TVS_PRELAZ_KOTA, ACRX_CMD_MODAL',e)
  self.assertIn('globalElevMid = axisBefore + tvsTransAlign::endOffset(align, pSizeAp1, pSizeBp1, pSizeAp2, pSizeBp2);',e,'drawing continues at the shifted axis')
  self.assertIn('if (pTransType == 2) pFirstPoint += right * s;',e,'next duct starts at the side-shifted outlet')
  self.assertIn('tvsTransAlignCmd::setDefault(dg.TransVert);',e)
  self.assertIn('if (ok && types[i]==TVS_TRANS::desc()) ok=add(item->manager,item->align);',text('TVS_Ventilation_ARX/PropertyAdmin.cpp'))
  self.assertIn('COM_INTERFACE_ENTRY(IDynamicEnumProperty)',text('TVS_Ventilation_ARX/TVSPropertyTransAlignDyn.h'))
  f=text('TVS_Ventilation_ARX/Func.cpp');self.assertEqual(f.count('tvsTransAlignCmd::afterEdit(Transie, transBefore, dg.TransVert);'),2)
  r=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc');self.assertIn('COMBOBOX        1044,300,122',r);self.assertIn('COMBOBOX        1046,300,104',r)
  self.assertIn('ftTransType = dg.TransPlan; tvsTransAlignCmd::storePlan(ftTransType);',e);self.assertIn('ftTransType = tvsTransAlignCmd::loadPlan();',e);self.assertIn('COMBOBOX        1045,300,160',r)
  for fn in ('TVS_Ventilation_ARX/TransAlignCommand.h','TVS_Ventilation_ARX/ElevationShift.h','TVS_Ventilation_ARX/TVSPropertyTransAlignDyn.h','TVS_Ventilation_DBX/TransAlignData.h'):
   for line in text(fn).splitlines():self.assertFalse(line.rstrip().endswith('L"'),fn)
if __name__=='__main__':unittest.main()
