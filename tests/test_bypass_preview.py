"""Actual isolated bridge compilation/execution; no ObjectARX substitutes claimed as runtime."""
import pathlib,subprocess,tempfile,unittest,os
class BypassPreview(unittest.TestCase):
 def test_adapter_revision_and_bridge(self):
  root=pathlib.Path(__file__).resolve().parents[1]
  with tempfile.TemporaryDirectory() as folder:
   target=pathlib.Path(folder)/('preview.exe' if os.name=='nt' else 'preview')
   msvc=os.environ.get('TVS_BYPASS_CXX')=='msvc'
   sources=[str(root/'tests/test_bypass_preview.cpp'),str(root/'TVS_Ventilation_ARX/BypassNumericBridge.cpp')]
   if msvc:cmd=['cl','/nologo','/EHsc','/utf-8','/std:c++14','/O2','/fp:strict','/W4','/WX','/DTVS_BYPASS_PREVIEW','/Fe'+str(target),'/Fo'+folder+'\\']+sources
   else:cmd=['g++','-std=c++11','-O2','-fno-fast-math','-ffp-contract=off','-Wall','-Wextra','-Werror','-pedantic','-DTVS_BYPASS_PREVIEW']+sources+['-o',str(target)]
   print('BRIDGE COMPILE:',subprocess.list2cmdline(cmd),flush=True)
   subprocess.run(cmd,check=True)
   subprocess.run([str(target)],check=True,timeout=180)
if __name__=='__main__':unittest.main()
