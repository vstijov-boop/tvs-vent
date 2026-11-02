"""TVS round vertical elbow parameterisation (geometry/RoundElbow.h) against the formulas in
TVS_TAP::subWorldDraw. Static check that those formulas are still the ones in the DBX source,
then a numeric comparison for 30/45/60 deg, both forms of rotation, many headings. No AutoCAD."""
import pathlib, re, subprocess, tempfile, unittest
from bypass_compiler import compile_cpp, executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
def squash(s):return re.sub(r'\s+','',s)
class RoundElbow(unittest.TestCase):
    def test_tvs_source_formulas(self):
        src=squash((ROOT/'TVS_Ventilation_DBX/TVS_TAP.cpp').read_text(encoding='utf-8-sig',errors='replace'))
        # Common CenterPoint (used unchanged by round elbows; rectangular Up/Down overwrites it).
        self.assertIn(squash('CenterPoint = AcGePoint3d(MiddlePoint.x - (Radius + SizeA / 2) / cos(Swectangle / 2)*cos(startangle + Swectangle / 2),'
                             'MiddlePoint.y - (Radius + SizeA / 2) / cos(Swectangle / 2)*sin(startangle + Swectangle / 2),'),src)
        # Round constant/variable Up/Down ports (rotated about CenterPoint by startangle).
        self.assertIn(squash('MA = AcGePoint3d(Radius + SizeA / 2 + CenterPoint.x, CenterPoint.y, CenterPoint.z ).rotateBy(startangle, vectr, CenterPoint);'
                             'MC = AcGePoint3d(Radius + SizeA / 2 + CenterPoint.x, (SizeA / 2 + Radius)*sin(Swectangle) + CenterPoint.y,'),src)
        # Rectangular Up/Down: CenterPoint recomputed, points rotated about MiddlePoint.
        self.assertIn(squash('CenterPoint = AcGePoint3d(MiddlePoint.x - SizeA / 2 - Radius, MiddlePoint.y - (SizeB / 2 + Radius)*sin(Swectangle),'),src)
        # Round geometry in constant/variable modes does not depend on TypeRoundTap.
        body=src[src.index(squash('Adesk::Boolean TVS_TAP::subWorldDraw')):src.index(squash('Adesk::UInt32 TVS_TAP::subSetAttributes'))]
        self.assertNotIn('TypeRoundTap',body)
    def test_ports_match_tvs(self):
        code='#include "'+(ROOT/'geometry/RoundElbow.h').as_posix()+'"\n'+r'''
#include <cassert>
#include <cmath>
#include <iostream>
struct Q {double x,y;};
static Q rot(Q p,double a,Q c){double dx=p.x-c.x,dy=p.y-c.y;return Q{c.x+dx*std::cos(a)-dy*std::sin(a),c.y+dx*std::sin(a)+dy*std::cos(a)};}
int main(){int n=0;const double pi=3.14159265358979323846;
 for(int deg:{30,45,60})for(double R:{100.,157.5,630.})for(double A:{160.,315.,1000.})for(int h=0;h<24;++h){
  const double t=deg*pi/180,sa=h*2*pi/24+.1,rc=R+A/2;Q M{123.4,-87.6};
  // TVS round (constant/variable) Up/Down, verbatim formulas
  Q C{M.x-rc/std::cos(t/2)*std::cos(sa+t/2),M.y-rc/std::cos(t/2)*std::sin(sa+t/2)};
  Q MA=rot(Q{R+A/2+C.x,C.y},sa,C),MC=rot(Q{R+A/2+C.x,(A/2+R)*std::sin(t)+C.y},sa,C);
  tvsRoundElbow::P ma,mc;tvsRoundElbow::ports(tvsRoundElbow::P(M.x,M.y),std::cos(sa),std::sin(sa),t,rc,ma,mc);
  assert(std::hypot(ma.x-MA.x,ma.y-MA.y)<1e-9&&std::hypot(mc.x-MC.x,mc.y-MC.y)<1e-9);
  // Inverse: place by MC (the adapter's rectangular MiddlePoint) and get the same elbow back.
  tvsRoundElbow::P m=tvsRoundElbow::middleFromMc(mc,std::cos(sa),std::sin(sa),t,rc);assert(std::hypot(m.x-M.x,m.y-M.y)<1e-9);
  // Same two ports as a rectangular Up/Down elbow with MiddlePoint = MC and B = A.
  Q Mr{MC.x,MC.y};Q Cr{Mr.x-A/2-R,Mr.y-(A/2+R)*std::sin(t)};Q MAr=rot(Q{R+A/2+Cr.x,Cr.y},sa,Mr),MCr=rot(Q{R+A/2+Cr.x,(R+A/2)*std::sin(t)+Cr.y},sa,Mr);
  assert(std::hypot(MAr.x-MA.x,MAr.y-MA.y)<1e-9&&std::hypot(MCr.x-MC.x,MCr.y-MC.y)<1e-9);++n;}
 std::cout<<"PASS ROUND ELBOW "<<n<<" cases: TVS round Up/Down ports = helper = rectangular ports"<<std::endl;}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/'round.cpp';cpp.write_text(code)
            target=pathlib.Path(folder)/'round';compile_cpp(cpp,target)
            subprocess.run([str(executable(target))],check=True)
    def test_round_slope_is_drawn(self):
        # User run (23055593): round slopes were invisible because the rectangular joint
        # projection needs SizeB > 0. The round branch must come before it.
        src=squash((ROOT/'TVS_Ventilation_DBX/TVS_Pipe.cpp').read_text(encoding='utf-8-sig',errors='replace'))
        self.assertLess(src.index('}elseif(ThisRound){'),src.index('tvsSlopeProjection::joints('))
        self.assertIn('tvsSlopeProjection::roundSides(',src)
        code='#include "'+(ROOT/'geometry/SlopeProjection.h').as_posix()+'"\n'+r'''
#include <cassert>
#include <iostream>
int main(){using tvsOffset::WorldPoint;WorldPoint s[2][2],c[2][4];
 assert(!tvsSlopeProjection::joints(WorldPoint(0,0,5),WorldPoint(300,400,5),315,0,120,c)); // the old failure
 assert(tvsSlopeProjection::roundSides(WorldPoint(0,0,5),WorldPoint(300,400,5),315,s));
 for(int k=0;k<2;++k){double nx=s[k][0].x,ny=s[k][0].y;assert(std::fabs(std::hypot(nx,ny)-157.5)<1e-9&&std::fabs(nx*300+ny*400)<1e-9);
  assert(std::fabs(s[k][1].x-s[k][0].x-300)<1e-9&&std::fabs(s[k][1].y-s[k][0].y-400)<1e-9&&s[k][0].z==5);}
 assert(!tvsSlopeProjection::roundSides(WorldPoint(1,1,0),WorldPoint(1,1,0),315,s));
 std::cout<<"PASS ROUND SLOPE SIDES"<<std::endl;}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/'sides.cpp';cpp.write_text(code)
            target=pathlib.Path(folder)/'sides';compile_cpp(cpp,target)
            subprocess.run([str(executable(target))],check=True)
if __name__=='__main__':unittest.main()
