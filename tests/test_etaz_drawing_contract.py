"""Plan 4: kota promijenjena u dijalogu dimenzija tokom crtanja nudi etaz (etazAfterSize)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class EtazWhileDrawing(unittest.TestCase):
 def test_wiring(self):
  e=(ROOT/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_text(encoding='utf-8-sig')
  self.assertEqual(e.count('etazAfterSize(oldsizeA, oldsizeB, sized, A1, A2, pipi, pt1);'),2,'TVS_DRAW and TVS_PIPE')
  # etaz first, then the transition to the new size
  i=e.index('etazAfterSize(oldsizeA');self.assertLess(i,e.index('addtrans(oldsizeA, oldsizeB, globSizeA, globSizeB',i))
  self.assertIn('Umetnuti etaz? [Da/Ne] <Da>',e)
  self.assertIn('tvsTransAlign::endOffset(tvsTransAlignCmd::defaultAlign(), oldA, oldB, globSizeA, globSizeB)',e)
  o=(ROOT/'TVS_Ventilation_ARX/OffsetIntegrationCommand.h').read_text(encoding='utf-8-sig')
  self.assertIn('inline Outcome executeTo(TVS_Pipe*& current,double elevation,double angle',o)
  self.assertIn('tvsOffsetService::write(snapshot,request,plan,output)',o)
if __name__=='__main__':unittest.main()
