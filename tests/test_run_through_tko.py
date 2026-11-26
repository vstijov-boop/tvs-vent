"""Plan 14 (korisnik 26.11.): dionica se nastavlja kroz prolaz TK-O samo ako je prolaz imao dimenziju ulaza."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Through(unittest.TestCase):
 def test_walk(self):
  s=text('TVS_Ventilation_ARX/SizeFollow.h')
  self.assertIn('if(r.type==tvsTee::Branch&&!branch&&!reshape&&!(r.second>0&&std::fabs(r.second-w->SizeApr)>1e-6)&&same(w->SizeApr,w->SizeBpr,w->ThisRoundpr)){',s)
  self.assertIn('e.close();prev=next;at=n.ports[port==0?1:0];continue;}',s)
  # TK-O with C != A: still its dialog
  self.assertLess(s.index('TeeStep t={next,port,ch.tee==2?2:1};'),s.index('TeeStep t={next,port,3};run.dialogs.push_back(t);'))
 def test_apply(self):
  s=text('TVS_Ventilation_ARX/SizeFollow.h')
  self.assertIn('r.second=0;w->SizeApr=A;w->SizeBpr=round?0:B;if(!round&&!w->ThisRoundotv)w->SizeBotv=B;',s)
  self.assertIn('tvsTeePort::ensure(branchHeight[k],2)',s)
  self.assertIn('kroz prolaz TK-O iste dimenzije',text('TVS_Ventilation_ARX/RunDialog.h'))
if __name__=='__main__':unittest.main()
