"""Bypass dialog state rules (BypassDialogState.h) and their wiring in the dialog source.
Pure C++ for the rules; static checks for the Win32/ObjectARX wiring. No AutoCAD runtime.
"""
import pathlib, re, subprocess, tempfile, unittest
from test_offset_service import function
from bypass_compiler import compile_cpp, executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
STATE=ROOT/'TVS_Ventilation_ARX/BypassDialogState.h'
DIALOG=ROOT/'TVS_Ventilation_ARX/BypassPreviewCommand.cpp'

class DialogState(unittest.TestCase):
    def test_rules(self):
        code='#include "'+STATE.as_posix()+'"\n'+r'''
#include <cassert>
#include <iostream>
using namespace tvsBypassUx;
int main(){
 // Memory: design settings only; Ri, window s and absolute Z limits never carried.
 std::wstring a[FieldCount],b[FieldCount];
 for(int j=0;j<FieldCount;++j)a[j]=L"a"+std::to_wstring(j);
 Memory docA,docB;store(docA,a,2,1);
 for(int j=0;j<FieldCount;++j)b[j]=L"new"+std::to_wstring(j);
 int angle=0,side=0;assert(apply(docA,b,angle,side)&&angle==2&&side==1);
 for(int j:{0,8,9,10,11})assert(b[j]==L"new"+std::to_wstring(j));
 for(int j:{1,2,3,4,5,6,7,12,25})assert(b[j]==a[j]);
 // Another document starts empty: nothing is transferred.
 for(int j=0;j<FieldCount;++j)b[j]=L"x";
 angle=0;side=0;
 assert(!apply(docB,b,angle,side)&&angle==0&&side==0);for(auto& v:b)assert(v==L"x");
 // Stored memory never contains the geometry-bound fields.
 for(int j:{0,8,9,10,11})assert(docA.values[j].empty());
 // Radius: TVS mode first, else Rc = 1.0 x B (Ri = B/2); mm from drawing units.
 RadiusSetting s;s.mode=RadiusVariable;s.k=.5;auto r=radius(s,400,10);assert(r.ok&&r.ri==200);
 s.mode=RadiusConstant;s.constantDrawing=15;r=radius(s,400,10);assert(r.ok&&r.ri==150);
 s.mode=RadiusSpecific;assert(radius(s,300,1).ri==300&&radius(s,400,1).ri==200);
 s.mode=0;r=radius(s,400,1);assert(r.ok&&r.ri==200&&std::wstring(r.source).find(L"1,0 x B")!=std::wstring::npos);
 assert(!radius(s,400,0).ok&&!radius(s,0,1).ok);
 // Invalidation: numeric/side/angle/units/confirm do; display controls never.
 for(int id=100;id<126;++id){assert(invalidatesResult(id,kEnChange));assert(!invalidatesResult(id,0x0100));}
 for(int id:{500,501,502})assert(invalidatesResult(id,kCbnSelChange));
 assert(invalidatesResult(503,kBnClicked));
 for(int id:{504,505,506})assert(!invalidatesResult(id,kBnClicked));
 assert(!invalidatesResult(507,kCbnSelChange)&&!invalidatesResult(700,kBnClicked));
 // Preview answers: Enter -> dialog, Esc -> cancel, Prihvati only when accepting.
 assert(previewChoice(false,true,L"",true)==BackToDialog);
 assert(previewChoice(false,false,L"",true)==Cancel);
 assert(previewChoice(true,false,L"Prihvati",true)==Accept);
 assert(previewChoice(true,false,L"Prihvati",false)==BackToDialog);
 assert(previewChoice(true,false,L"Dijalog",true)==BackToDialog);
 assert(previewChoice(true,false,L"Odustani",true)==Cancel);
 std::cout<<"PASS DIALOG STATE: per-document memory without geometry/units/ids, TVS radius proposal, invalidation table, preview answers\n";
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/'state.cpp';cpp.write_text(code,encoding='utf-8')
            target=pathlib.Path(folder)/'state';compile_cpp(cpp,target)
            subprocess.run([str(executable(target))],check=True)

    def test_memory_holds_no_object_state(self):
        text=STATE.read_text(encoding='utf-8')
        memory=function(text,'struct Memory')
        for forbidden in ['AcDbObjectId','Snapshot','View','Frame','factor','units','double']:
            self.assertNotIn(forbidden,memory)

    def test_dialog_wiring(self):
        src=DIALOG.read_text(encoding='utf-8-sig')
        # Memory is per document (CDocData), never a process-wide static.
        self.assertIn('DocVars.docData().bypassMemory',src)
        self.assertIsNone(re.search(r'static\s+(?:tvsBypassUx::)?Memory\b',src))
        # The dialog proc uses the shared invalidation table.
        self.assertIn('tvsBypassUx::invalidatesResult(',src)
        run=function(src,'static bool runBypass(')
        # Exactly one writer call: Prihvati from the dialog and from the preview share it.
        self.assertEqual(run.count('tvsBypassService::write('),1)
        # Detail -> dialog keeps the same candidate in accepting flows.
        self.assertRegex(run,r"(?s)if\(action==703\)\{.*?if\(permanent\)continue;")

if __name__=='__main__':
    unittest.main()
