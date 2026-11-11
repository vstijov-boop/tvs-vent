"""Runs every contract / geometry test (python + g++), as the CI does before the AutoCAD builds:
   python3 tests/run_all.py
Skipped: helpers (entry_source, bypass_compiler) and the MSVC runner (run_bypass_msvc)."""
import pathlib,subprocess,sys,time
HERE=pathlib.Path(__file__).resolve().parent
SKIP=('entry_source.py','bypass_compiler.py','run_bypass_msvc.py','run_all.py')
failed=[];start=time.time()
tests=sorted(p for p in HERE.glob('*.py') if p.name not in SKIP)
for p in tests:
    r=subprocess.run([sys.executable,str(p)],cwd=str(HERE.parent),capture_output=True,text=True)
    if r.returncode==0:print('ok    '+p.name)
    else:
        failed.append(p.name);print('FAIL  '+p.name);print((r.stdout+r.stderr)[-4000:])
print('\n%d testova, palo %d, %.0f s'%(len(tests),len(failed),time.time()-start))
if failed:print('Palo: '+', '.join(failed));sys.exit(1)
