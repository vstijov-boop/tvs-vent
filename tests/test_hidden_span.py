"""GFX-01 variant 1: bypass-only hidden edges. Plan clipping is compiled and run; the DBX
record and draw wiring are checked as source contracts (no AutoCAD runtime here)."""
import pathlib,re,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
CPP=r'''
#include "%s"
#include <cassert>
#include <cstdio>
using namespace tvsHidden;
static double hiddenLength(P a,P b,P f,P l,double w){P q[4];if(!footprint(f,l,w,q))return 0;
 double t0,t1;
 if(!clip(a,b,q,t0,t1))return 0;
 return (t1-t0)*std::hypot(b.x-a.x,b.y-a.y);}
static bool near(double a,double b){return std::fabs(a-b)<1e-9;}
int main(){
 // Perpendicular: lower edge along y, upper duct (width 400) along x -> hidden length = 400.
 assert(near(hiddenLength(P(125,-1500),P(125,1500),P(-460,0),P(460,0),400),400));
 // Reversed edge direction and reversed upper axis give the same interval.
 assert(near(hiddenLength(P(125,1500),P(125,-1500),P(460,0),P(-460,0),400),400));
 // Oblique 60 deg crossing: hidden length = width / sin(60).
 const double s=std::sin(3.141592653589793/3),c=std::cos(3.141592653589793/3);
 assert(near(hiddenLength(P(-1500*c,-1500*s),P(1500*c,1500*s),P(-700,0),P(700,0),400),400/s));
 // Edge outside the upper's length (beyond its end): nothing hidden.
 assert(near(hiddenLength(P(900,-1500),P(900,1500),P(-460,0),P(460,0),400),0));
 // Parallel edge outside / inside the band.
 assert(near(hiddenLength(P(-100,300),P(100,300),P(-460,0),P(460,0),400),0));
 assert(near(hiddenLength(P(-100,100),P(100,100),P(-460,0),P(460,0),400),200));
 // Edge ending inside the footprint: partial interval.
 assert(near(hiddenLength(P(0,-1000),P(0,100),P(-460,0),P(460,0),400),300));
 // Degenerate upper (zero length or width) hides nothing.
 assert(near(hiddenLength(P(0,-1000),P(0,1000),P(0,0),P(0,0),400),0));
 assert(near(hiddenLength(P(0,-1000),P(0,1000),P(-460,0),P(460,0),0),0));
 std::puts("PASS HIDDEN SPAN plan clipping");
}
'''
class HiddenSpan(unittest.TestCase):
 def test_plan_clipping(self):
  with tempfile.TemporaryDirectory() as folder:
   cpp=pathlib.Path(folder)/'hidden.cpp';cpp.write_text(CPP%(ROOT/'geometry/HiddenSpan.h').as_posix())
   target=pathlib.Path(folder)/'hidden';compile_cpp(cpp,target);subprocess.run([str(executable(target))],check=True)
 def test_record_and_draw_contract(self):
  data=(ROOT/'TVS_Ventilation_DBX/HiddenSpanData.h').read_text()
  self.assertIn('L"TVS_HIDDEN_BY_V1"',data)
  self.assertIn('const short types[]={330,10,11,40,41};',data)
  self.assertIn('numActiveTransactions()>0',data,'writes go through the active writer transaction')
  pipe=(ROOT/'TVS_Ventilation_DBX/TVS_Pipe.cpp').read_text(encoding='utf-8',errors='replace')
  body=pipe[pipe.index('// Bypass crossing (GFX-01 variant 1)'):]
  body=body[:body.index('if (DuctType==DuctTypeFlex)')]
  for needle in ('e.upper.isErased()','acdbOpenObject(live, e.upper, AcDb::kForRead)','if (!(Elev < e.elevation)) continue;','setHideProperty(new AcDbLine','setMainProperty(edges[j]); continue;'):
   self.assertIn(needle,body,needle)
  service=(ROOT/'TVS_Ventilation_ARX/BypassService.h').read_text()
  self.assertRegex(service,r'(?s)members\[3\]=h;\s*if\(!markHidden\(s\.obstacle,options\.above,h\)\)return false;\s*if\(inject\(1\)\)')
  self.assertIn('TVS_Pipe* lower=above?obstacle:h;TVS_Pipe* upper=above?h:obstacle;',service)
if __name__=='__main__':unittest.main()
