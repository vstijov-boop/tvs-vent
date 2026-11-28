"""Plan 26 (2026-11-28): krstasti komad (TK0 + drugi ogranak) i XK (kosa racva TK + drugi ogranak 45)."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Cross(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_cross.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_record(self):
  r=text('TVS_Ventilation_DBX/TeeCross.h');self.assertTrue(r.isascii())
  self.assertIn('return L"TVS_TEE_X1";',r);self.assertIn('L"TVS_TEE_X/1"',r);self.assertIn('const short types[]={1,40,40,70};',r)
 def test_ports(self):
  w=text('TVS_Ventilation_DBX/TVS_WYE.cpp')
  self.assertIn('bool TVS_WYE::crossPort(',w);self.assertIn('dir = -v; port = Basepoint + dir * (SizeApr / 2 + LengthPl);',w)
  self.assertIn('if (cross && tee.lateral > 0) tvsTee::lateralSecond(tee, xA, shape);',w)
  self.assertIn('const AcGePoint3d q[12] =',w)
  self.assertIn('Port& x=out[3];if(w->crossPort(x.p,x.dir,x.a,x.b,x.round))return 4;',text('TVS_Ventilation_DBX/PortGrips.h'))
  self.assertNotIn('Port P[3]',text('TVS_Ventilation_ARX/commands/Draw.inl'))
  self.assertIn('if(p->crossPort(xp,xd,xa,xb,xr)&&tvsMoveGeometry::unit(Point(xd.x,xd.y,0),a)){',text('TVS_Ventilation_ARX/TVSMoveCommand.h'))
 def test_users(self):
  self.assertIn('if(x.ports.size()==4&&!pants){crossJunction(e,kids,of,r,i);return;}',text('geometry/PressureDrop.h'))
  self.assertIn('if(n.ports.size()==4){',text('TVS_Ventilation_ARX/PressureCommand.h'))
  s=text('TVS_Ventilation_ARX/SPEC.cpp');self.assertIn('Krstasti komad (Soko)',s);self.assertIn('Kosa račva XK 45° (Soko), okrugla',s)
  self.assertIn('if (p->crossPort(xp, xd, xa, xb, xr)) {',text('TVS_Ventilation_DBX/TVS_Entity.cpp'))
  c=text('TVS_Ventilation_ARX/commands/Connect.inl');self.assertIn('if (tee) { tvsTeeCrossCmd::connect(id1, id2); return; }',c)
  self.assertIn('id.objectClass() == TVS_WYE::desc()) { id2 = id; *tee = true; break; }',c)
  self.assertIn('#include "CrossCommand.h"',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/CrossCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  self.assertIn('Ukloniti drugi ogranak',text('TVS_Ventilation_ARX/TeeTypeDialog.h'))
  x=text('TVS_Ventilation_ARX/CrossCommand.h')   # XK + koljeno 45 for a duct perpendicular to the main (user 28. 11.)
  self.assertIn('const bool elbow=lateral&&std::fabs(dd.dotProduct(u))<1e-3;',x);self.assertIn('start=xp+xd*T+axis*T;',x)
  self.assertIn('tvsTeeDialog::planElbow(teeId,xp,xd,turn,sec,elev,f,exit,out)',x);self.assertLess(x.index('w.close();p.close();'),x.index('tvsTeeDialog::planElbow('))
  a=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp');self.assertLess(a.index('#include "TeeTypeDialog.h"'),a.index('#include "CrossCommand.h"'))
if __name__=='__main__':unittest.main()
