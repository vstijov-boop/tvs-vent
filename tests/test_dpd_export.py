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
 out={}
 for name in sys.argv[1:]:
  s=parse_system_file(name)
  assert len(s)==1,len(s)
  s=s[0];g=izracunaj_geometriju(s.dionice)
  r={'naziv':s.naziv,'dionice':[[d.id,d.cvor_od,d.cvor_do,d.tip,d.sistem,d.Q,d.W,d.H,d.D,d.L,list(d.smjer),d.materijal] for d in s.dionice],
     'greske':g.greske,'potpuna':g.potpuna,'koordinate':{k:list(v) for k,v in g.koordinate.items()}}
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
  out[name]=r
 print(json.dumps(out))
''')
class DpdExport(unittest.TestCase):
 def test_ascii(self):(ROOT/'geometry/DpdExport.h').read_bytes().decode('ascii')
 def test_cpp_and_round_trip(self):
  with tempfile.TemporaryDirectory() as d:
   d=pathlib.Path(d);exe=d/'t';su=d/'dovod.json';ex=d/'odvod.json'
   subprocess.run(FLAGS+[str(ROOT/'tests/test_dpd_export.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe),str(su),str(ex)],check=True,capture_output=True,text=True).stdout)
   for p in (su,ex):
    raw=p.read_bytes();self.assertFalse(raw.startswith(b'\xef\xbb\xbf'));json.loads(raw.decode('utf-8'))   # UTF-8 without BOM, valid JSON
   app=dpd_app()
   if app is None:
    print('test_dpd_export: DuctPressDrop (hvac_app) nije dostupan - round trip preskocen (TVS_DPD_APP=...)');return
   env=dict(os.environ,PYTHONPATH=str(app),QT_QPA_PLATFORM='offscreen')
   (d/'check.py').write_text(CHECK,encoding='utf-8')
   r=subprocess.run([sys.executable,str(d/'check.py'),str(su),str(ex)],cwd=str(d),env=env,capture_output=True,text=True)
   self.assertEqual(r.returncode,0,r.stderr[-3000:])
   out=json.loads(r.stdout.strip().splitlines()[-1])
   s=out[str(su)];x=out[str(ex)]
   self.assertEqual(s['naziv'],'Dovod ž')
   self.assertEqual(s['dionice'][0],['D-01','J','KO1','P','supply',3600,600,300,0,5.6,[5.6,0,0],'Pocinkovani čelični lim'])
   self.assertEqual(s['dionice'][3][:10],['D-04','T1','P2','P','supply',1200,300,300,0,2.8])
   self.assertEqual(s['dionice'][5][3:10],['K','supply',1200,0,0,250,1.5]);self.assertEqual(s['dionice'][5][11],'Fleksibilni izolovani')
   self.assertEqual(s['greske'],[]);self.assertTrue(s['potpuna'])
   self.assertEqual([round(c,6) for c in s['koordinate']['T1']],[5.6,5.0,0.0])
   self.assertEqual([y[4] for y in x['dionice']],['extract']*6);self.assertEqual(x['dionice'][0][0],'O-01')
   if s['service']!='ok':
    print('test_dpd_export: '+s['service']+' - izvedeni fitinzi nisu provjereni');return
   # DuctPressDrop derives the fittings from the geometry: elbow at KO1, tee legs at T1, transitions at P1 / P2
   self.assertEqual(s['izvedeni'],[['D-02','KO1','koljeno'],['D-03','P1','tranzicija'],['D-03','T1','spoj'],['D-04','P2','tranzicija'],['D-04','T1','spoj']])
   self.assertIn(['O-01','KO1','koljeno'],x['izvedeni']);self.assertIn(['O-04','T1','spoj'],x['izvedeni'])
   self.assertTrue(all(v>0 for v in s['dp']+x['dp']))
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
