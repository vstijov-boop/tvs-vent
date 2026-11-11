"""TVS_PROTOK: sabiranje protoka (geometry/FlowNetwork.h) i komanda."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Flow(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_flow_network.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_ascii(self):(ROOT/'geometry/FlowNetwork.h').read_bytes().decode('ascii')
 def test_command(self):
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/FlowCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  c=text('TVS_Ventilation_ARX/FlowCommand.h')
  self.assertIn('tvsFlowNetwork::solve(el,root,port,tol)',c)
  self.assertIn('Upisati protoke',c);self.assertIn('recalculateMleader',c)
  self.assertIn('tvsMove::bodiesFor(',c)   # etaz / obilazak as one element
  self.assertIn('TVS_PROTOK, TVS_PROTOK',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('#include "FlowCommand.h"',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('tvsFlow::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('PROTOK',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_PROTOK_%d.png'%s)).exists())
if __name__=='__main__':unittest.main()
