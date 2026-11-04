"""Static contract behind the bypass connector rule (no AutoCAD runtime).

Legacy TVS connectors are not filed: TVS_Pipe::dwgOutFields/dwgInFields skip them, so a
user UNDO (undo filer) and save/reopen never restore a changed link. The bypass writer
therefore must not modify any connector. If connector filing is ever re-enabled, this
test fails on purpose: the bypass relinking rule has to be revisited then.
"""
import pathlib, re, unittest
from test_offset_service import function
ROOT=pathlib.Path(__file__).resolve().parents[1]

def code_only(text):
    text=re.sub(r'/\*.*?\*/','',text,flags=re.S)
    return re.sub(r'//[^\n]*','',text)

class ConnectorContract(unittest.TestCase):
    def test_connectors_are_not_filed(self):
        pipe=(ROOT/'TVS_Ventilation_DBX/TVS_Pipe.cpp').read_text(encoding='utf-8-sig',errors='replace')
        out=code_only(function(pipe,'Acad::ErrorStatus TVS_Pipe::dwgOutFields'))
        inn=code_only(function(pipe,'Acad::ErrorStatus TVS_Pipe::dwgInFields'))
        self.assertNotIn('writeConnectors',out)
        self.assertNotIn('readConnectors',inn)
        self.assertNotIn('connectors',out+inn)
        for name in ['TVS_Entity.cpp','TVS_TAP.cpp','TVS_TRANS.cpp','TVS_WYE.cpp']:
            path=ROOT/'TVS_Ventilation_DBX'/name
            if not path.exists():continue
            text=code_only(path.read_text(encoding='utf-8-sig',errors='replace'))
            for sig in re.findall(r'Acad::ErrorStatus\s+\w+::dwg(?:Out|In)Fields\s*\(',text):
                body=function(text,sig)
                self.assertNotIn('Connectors(',body,name)

    def test_bypass_writer_never_modifies_connectors(self):
        bypass=(ROOT/'TVS_Ventilation_ARX/BypassService.h').read_text(encoding='utf-8-sig')
        body=code_only(function(bypass,'inline bool write('))
        self.assertIsNone(re.search(r'connectors\s*\[[^\]]*\]\s*(?:\.\w+\s*)?=(?!=)',body))
        self.assertIsNone(re.search(r'connectionID\s*=(?!=)',body))
        self.assertNotIn('connectors.swap',body)
        self.assertNotIn('connectors.push_back',body)
        self.assertIn('connectionID.isNull()',body)  # linked outer end is refused

if __name__=='__main__':
    unittest.main()
