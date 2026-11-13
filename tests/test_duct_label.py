"""TVS_OZNAKE: oznake kanala dimenzija/protok (DuctLabel.h) i komanda."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Label(unittest.TestCase):
 def test_module(self):
  d=text('TVS_Ventilation_DBX/DuctLabel.h')
  self.assertIn('L"AT-DIM_KANAL"',d);self.assertIn('style.setFileName(L"romans.shx");style.setTextSize(pl.h);style.setXScale(0.7)',d)
  self.assertIn('h=paperMm()*s.scale/f',d);self.assertIn('inline double paperMm(){return 2.5;}',d)
  self.assertIn('if(ext.x+h>length)return false;',d)                 # samo gdje tekst stane
  self.assertIn('swprintf_s(out,n,L"%ls/%.0f",size,flow)',d)          # bez jedinica, kosa crta
  self.assertNotIn('isPlotGeneration',d)                               # stampa se
  self.assertIn('traits.setLayer(lay);traits.setColor(256);',d)
 def test_duct_draws_and_explodes(self):
  p=text('TVS_Ventilation_DBX/TVS_Pipe.cpp')
  self.assertIn('tvsDuctLabel::draw(mode, this, FirstPoint, LastPoint, SizeA, SizeB, ThisRound, Flow);',p)
  self.assertIn('tvsDuctLabel::entity(this, FirstPoint, LastPoint, SizeA, SizeB, ThisRound, Flow)',p)
 def test_command(self):
  c=text('TVS_Ventilation_ARX/LabelCommand.h')
  self.assertIn('[Iskljuci/Polozaj/Razmjera/Pojedinacno]',c);self.assertIn('[Iznad/Sredina/Ispod/Podrazumijevano]',c)
  self.assertIn('acutBuildList(RTDXF0,L"TVS_PIPE",RTNONE)',c)
  self.assertIn('TVS_OZNAKE, TVS_OZNAKE',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('tvsLabelCmd::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('OZNAKE',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_OZNAKE_%d.png'%s)).exists())
if __name__=='__main__':unittest.main()
