"""TVS_DPD_IZVOZ: mreza kao sistem za DuctPressDrop (geometry/DpdExport.h) i komanda.
Round trip: uzorak JSON-a se ucitava importerom samog DuctPressDrop-a (services/system_import.py),
ako je njegov folder hvac_app dostupan (TVS_DPD_APP ili /home/claude/hvac-proracun/hvac_app);
inace se taj dio preskace uz poruku. Bez PySide6 se preskace samo dio sa ProjectService."""
import json,os,pathlib,subprocess,sys,tempfile,textwrap,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
FLAGS=['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic','-Wno-misleading-indentation']
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
def dpd_app():
 for p in (os.environ.get('TVS_DPD_APP',''),'/home/claude/hvac-proracun/hvac_app'):
  if p and (pathlib.Path(p)/'services'/'system_import.py').exists():return pathlib.Path(p)
 return None
CHECK=textwrap.dedent('''
 import json,sys,math
 from services.system_import import parse_system_file
 from hvac_geometrija import izracunaj_geometriju
 from hvac_data import FITING_META
 out={}
 for name in sys.argv[1:]:
  s=parse_system_file(name)
  assert len(s)==1,len(s)
  s=s[0];g=izracunaj_geometriju(s.dionice)
  r={'naziv':s.naziv,'dionice':[[d.id,d.cvor_od,d.cvor_do,d.tip,d.sistem,d.Q,d.W,d.H,d.D,d.L,list(d.smjer),d.materijal] for d in s.dionice],
     'greske':g.greske,'potpuna':g.potpuna,'koordinate':{k:list(v) for k,v in g.koordinate.items()},
     'oprema':[[d.id,f.oprema['oznaka'],f.oprema['tip'],f.fn,f.naziv in FITING_META,round(f.dP(d.Pd),4),round(d.Pd,4),f.je_terminal,f.oprema['pos']] for d in s.dionice for f in d.fitinzi if f.oprema]}
  try:
   import os;os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
   from services.project import ProjectService
   from hvac_calc import vrsta_fitinga
  except ImportError as ex:
   r['service']='skip: '+str(ex)
  else:
   ps=ProjectService();ps.dodaj_uvezeni_sistem(s.naziv,s.dionice)
   r['service']='ok'
   r['izvedeni']=sorted([d.id,f.veza['cvor'],vrsta_fitinga(f)] for d in ps.active_system.dionice for f in d.fitinzi if f.veza)
   r['dp']=[d.dP_ukupno for d in ps.active_system.dionice]
   r['oprema_servis']=[[d.id,f.oprema['oznaka'],round(f.dP_s_re(d.Pd,d.Re,q_m3h=d.Q),4)] for d in ps.active_system.dionice for f in d.fitinzi if f.oprema]
   r['dp_oprema']=[round(d.dP_oprema,4) for d in ps.active_system.dionice]
  out[name]=r
 print(json.dumps(out))
''')
class DpdExport(unittest.TestCase):
 def test_ascii(self):(ROOT/'geometry/DpdExport.h').read_bytes().decode('ascii')
 def test_cpp_and_round_trip(self):
  with tempfile.TemporaryDirectory() as d:
   d=pathlib.Path(d);exe=d/'t';su=d/'dovod.json';ex=d/'odvod.json';op=d/'oprema.json';bt=d/'dno.json';bc=d/'dno_nastavak.json'
   subprocess.run(FLAGS+[str(ROOT/'tests/test_dpd_export.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe),str(su),str(ex),str(op),str(bt),str(bc)],check=True,capture_output=True,text=True).stdout)
   for p in (su,ex,op,bt,bc):
    raw=p.read_bytes();self.assertFalse(raw.startswith(b'\xef\xbb\xbf'));json.loads(raw.decode('utf-8'))   # UTF-8 without BOM, valid JSON
   app=dpd_app()
   if app is None:
    print('test_dpd_export: DuctPressDrop (hvac_app) nije dostupan - round trip preskocen (TVS_DPD_APP=...)');return
   env=dict(os.environ,PYTHONPATH=str(app),QT_QPA_PLATFORM='offscreen')
   (d/'check.py').write_text(CHECK,encoding='utf-8')
   r=subprocess.run([sys.executable,str(d/'check.py'),str(su),str(ex),str(op),str(bt),str(bc)],cwd=str(d),env=env,capture_output=True,text=True)
   self.assertEqual(r.returncode,0,r.stderr[-3000:])
   out=json.loads(r.stdout.strip().splitlines()[-1])
   s=out[str(su)];x=out[str(ex)];o=out[str(op)];b=out[str(bt)];c=out[str(bc)]
   # oprema: read back as DuctPressDrop equipment (Fiting.oprema) with the right dp and tag
   self.assertEqual(s['oprema'],[]);self.assertEqual(o['greske'],[])
   pd=0.6*(900/3600/0.08)**2;self.assertAlmostEqual(o['oprema'][0][6],pd,3)
   self.assertEqual([y[:5] for y in o['oprema']],[['D-01','RP-01','zaluzina','dp_fix',True],['D-02','PB-01','prigusivac','dp_fix',True],['D-02','D-01','difuzor','konst',True]])
   self.assertEqual([y[5] for y in o['oprema']],[7.5,25.0,round(2.5*pd,4)])            # Nominal at Q, FixedDp, zeta * pd
   self.assertEqual([y[7] for y in o['oprema']],[False,False,True]);self.assertEqual(o['oprema'][2][8],1.0)   # terminal at the free end
   self.assertEqual(s['naziv'],'Dovod ž')
   self.assertEqual(s['dionice'][0],['D-01','J','KO1','P','supply',3600,600,300,0,5.6,[5.6,0,0],'Pocinkovani čelični lim'])
   self.assertEqual(s['dionice'][3][:10],['D-04','T1','P2','P','supply',1200,300,300,0,2.8])
   self.assertEqual(s['dionice'][5][3:10],['K','supply',1200,0,0,250,1.5]);self.assertEqual(s['dionice'][5][11],'Fleksibilni izolovani')
   self.assertEqual(s['greske'],[]);self.assertTrue(s['potpuna'])
   self.assertEqual([round(c,6) for c in s['koordinate']['T1']],[5.6,5.0,0.0])
   self.assertEqual([y[4] for y in x['dionice']],['extract']*6);self.assertEqual(x['dionice'][0][0],'O-01')
   # resetka na dnu: vertical branch (dz = -0.10 m, neck 300x100) with the grille at its free end
   for y in (b,c):
    self.assertEqual(y['greske'],[]);self.assertTrue(y['potpuna'])
    br=[q for q in y['dionice'] if q[10]==[0,0,-0.1]];self.assertEqual(len(br),1);self.assertEqual(br[0][1:3],['T1','K1']);self.assertEqual(br[0][5:8],[400,300,100])
    self.assertEqual([round(v,6) for v in y['koordinate']['K1']],[2.92,0.0,-0.1])
    self.assertEqual([q[1:6] for q in y['oprema']][0],['R-01','resetka','dp_fix',True,15.0])
   self.assertEqual([q[5] for q in b['dionice']],[400,0,400]);self.assertEqual(b['dionice'][1][2],'Z1')   # closed end: Q = 0, as drawn
   if s['service']!='ok':
    print('test_dpd_export: '+s['service']+' - izvedeni fitinzi nisu provjereni');return
   # DuctPressDrop derives the fittings from the geometry: elbow at KO1, tee legs at T1, transitions at P1 / P2
   self.assertEqual(s['izvedeni'],[['D-02','KO1','koljeno'],['D-03','P1','tranzicija'],['D-03','T1','spoj'],['D-04','P2','tranzicija'],['D-04','T1','spoj']])
   self.assertIn(['O-01','KO1','koljeno'],x['izvedeni']);self.assertIn(['O-04','T1','spoj'],x['izvedeni'])
   self.assertTrue(all(v>0 for v in s['dp']+x['dp']))
   self.assertEqual(o['oprema_servis'],[['D-01','RP-01',7.5],['D-02','PB-01',25.0],['D-02','D-01',round(2.5*pd,4)]])   # kept by ProjectService
   self.assertEqual(o['dp_oprema'],[7.5,round(25+2.5*pd,4)])
   # DuctPressDrop derives a T komad at the tap node (closed end / duct on), not an elbow
   for y in (b,c):
    legs=[q for q in y['izvedeni'] if q[1]=='T1' and q[2]=='spoj'];self.assertEqual(len(legs),2,y['izvedeni'])
    self.assertNotIn('koljeno',[q[2] for q in y['izvedeni']])
    self.assertEqual(y['dp_oprema'][-1 if y is c else 2],10.0 if y is c else 15.0)
 def test_command(self):
  c=(ROOT/'TVS_Ventilation_ARX/DpdExportCommand.h').read_bytes();self.assertTrue(c.startswith(b'\xef\xbb\xbf'))
  c=text('TVS_Ventilation_ARX/DpdExportCommand.h')
  self.assertIn('tvsFlowNetwork::solve(el,root,port,tol)',c);self.assertIn('tvsDpd::segments(',c);self.assertIn('tvsDpd::json(',c)
  self.assertIn('[Dovod/Odvod]',c);self.assertIn('acedInitGet(0,L"Dovod Odvod")',c)
  self.assertIn('acedGetFileD(',c);self.assertIn(',L"json",1,',c);self.assertIn('L"wb"',c)
  self.assertNotIn('put_',c)                                    # nothing written into the drawing
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('#include "DpdExportCommand.h"',e)
  self.assertIn('TVS_DPD_IZVOZ, TVS_DPD_IZVOZ, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)',e)
  self.assertIn('tvsDpdExport::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('DPD_IZVOZ',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_DPD_IZVOZ_%d.png'%s)).exists())
if __name__=='__main__':unittest.main()
