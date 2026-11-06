"""TVS_KOTE labels and the elevation tooltip (source contracts)."""
import pathlib,unittest
from entry_source import entry_source
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig',errors='replace')
class LevelsContract(unittest.TestCase):
 def test_contracts(self):
  l=text('TVS_Ventilation_DBX/LevelLabels.h')
  self.assertIn('mode->context()->isPlotGeneration()',l,'labels are not plotted')
  self.assertIn('traits.setColor(8);',l);self.assertIn('inline double textMm(){return 25;}',l)
  for f,call in (('TVS_Pipe','tvsLevelLabels::draw(mode, database(), FirstPoint + (LastPoint - FirstPoint) / 2'),('TVS_WYE','tvsLevelLabels::draw(mode, database(), Basepoint'),('TVS_TRANS','tvsLevelLabels::draw(mode, database(), FirstPoint + VectTr * 0.8, VectTr, Elev + tvsTransAlign::endOffset')):
   s=text('TVS_Ventilation_DBX/%s.cpp'%f);self.assertIn(call,s)
   self.assertLess(s.index('#include "StdAfx.h"'),s.index('#include "LevelLabels.h"'),'after the precompiled header')
  e=entry_source()
  self.assertIn('Ventilation_ARX, TVS_KOTE, TVS_KOTE, ACRX_CMD_MODAL',e);self.assertIn('tvsLevelTooltip::start();',e);self.assertIn('tvsLevelTooltip::stop();',e)
  self.assertIn('tvsLevelTooltip::setEnabled(dg.LevelTooltip == TRUE);',e)
  self.assertIn('"Oblačić sa kotama",1043',text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc'))
  self.assertIn('DDX_Check(pDX, IDC_LevelTooltip, LevelTooltip);',text('TVS_Ventilation_ARX/dgPipeSize.cpp'))
  for f in ('TVS_Ventilation_ARX/LevelTooltip.h','TVS_Ventilation_DBX/LevelLabels.h','TVS_Ventilation_ARX/SectionCommand.h'):
   for line in text(f).splitlines():self.assertFalse(line.rstrip().endswith('L"'),f)
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_bytes().startswith(b'\xef\xbb\xbf'))
if __name__=='__main__':unittest.main()
