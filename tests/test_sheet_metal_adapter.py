"""Exercise the real DBX field/unit adapter with small CAD-independent type stubs.

This verifies field mapping and exclusions, not ObjectARX/COM runtime behavior.
"""
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

class SheetMetalAdapter(unittest.TestCase):
    def test_real_adapter_fields_units_and_exclusions(self):
        root = Path(__file__).parents[1]
        source = (root/'TVS_Ventilation_DBX/TVS_Entity.cpp').read_text(encoding='utf-8-sig')
        method = re.search(r'bool TVS_Entity::getSheetMetal\([\s\S]*?\n}\n',source).group()
        stub = r'''
#include "SheetMetal.h"
#include <cassert>
#include <cmath>
#define DuctTypeFlex 1
struct TVS_Entity {
    virtual ~TVS_Entity() {}
    int DuctType;
    double units;
    TVS_Entity():DuctType(0),units(1) {}
    void assertReadEnabled() const {}
    bool getSheetMetal(double&,double&,double&) const;
};
#define CAST(Type) static const Type* cast(const TVS_Entity *p) { return dynamic_cast<const Type*>(p); }
struct TVS_Pipe:TVS_Entity {
    double SizeA,SizeB,length; bool ThisRound;
    double get_Length() const { return length; }
    CAST(TVS_Pipe)
};
struct TVS_TAP:TVS_Entity {
    double SizeA,SizeB,Radius,Swectangle; bool ThisRound;
    CAST(TVS_TAP)
};
struct TVS_TRANS:TVS_Entity {
    double SizeAp1,SizeBp1,SizeAp2,SizeBp2,LengthTr;
    bool ThisRoundp1,ThisRoundp2;
    CAST(TVS_TRANS)
};
struct TVS_WYE:TVS_Entity {
    double SizeApr,SizeBpr,SizeAotv,SizeBotv,LengthPl;
    bool ThisRoundpr,ThisRoundotv;
    CAST(TVS_WYE)
};
namespace tvsUnits { double factor(const TVS_Entity *p) { return p->units; } }
static void near(double a,double b) { assert(std::fabs(a-b)<1e-10*(1+std::fabs(b))); }
'''
        checks = r'''
int main() {
    double area,t,m;
    const double factors[]={1,10,1000};
    for (int i=0;i<3;++i) {
        const double f=factors[i];
        TVS_Pipe p; p.units=f; p.SizeA=400/f; p.SizeB=200/f; p.length=3000/f; p.ThisRound=false;
        assert(p.getSheetMetal(area,t,m)); near(area,3.6); near(t,.6); near(m,16.956);
        p.ThisRound=true; // A stale rectangular height must not affect circular results.
        assert(p.getSheetMetal(area,t,m)); near(area,1.2*tvsSheetMetal::pi);
        p.DuctType=DuctTypeFlex;
        assert(!p.getSheetMetal(area,t,m)); near(area,0); near(m,0);

        TVS_TAP e; e.units=f; e.SizeA=400/f; e.SizeB=200/f; e.Radius=150/f;
        e.Swectangle=tvsSheetMetal::pi/2; e.ThisRound=false;
        assert(e.getSheetMetal(area,t,m)); near(area,.21*tvsSheetMetal::pi);
        near(m,3.107349293665664);
        e.SizeA=200/f; e.SizeB=400/f; // Actual bend-plane dimensions of an up/down elbow.
        assert(e.getSheetMetal(area,t,m)); near(area,.15*tvsSheetMetal::pi);
        e.DuctType=DuctTypeFlex; assert(!e.getSheetMetal(area,t,m));

        TVS_TRANS tr; tr.units=f; tr.SizeAp1=400/f; tr.SizeBp1=200/f;
        tr.SizeAp2=300/f; tr.SizeBp2=150/f; tr.LengthTr=500/f;
        tr.ThisRoundp1=tr.ThisRoundp2=false; tr.DuctType=DuctTypeFlex;
        assert(tr.getSheetMetal(area,t,m)); near(area,.525);
        tr.ThisRoundp2=true;
        assert(tr.getSheetMetal(area,t,m));
        near(area,tvsSheetMetal::transitionArea(400,200,300,0,500));

        TVS_WYE w; w.units=f; w.SizeApr=400/f; w.SizeBpr=200/f;
        w.SizeAotv=300/f; w.SizeBotv=150/f; w.LengthPl=100/f;
        w.ThisRoundpr=w.ThisRoundotv=false; w.DuctType=DuctTypeFlex;
        assert(w.getSheetMetal(area,t,m)); near(area,.645);
        w.SizeAotv=1200/f;
        assert(w.getSheetMetal(area,t,m)); near(t,1.0); // Largest side includes branch.
        w.LengthPl=-1; assert(!w.getSheetMetal(area,t,m));
    }
    TVS_Entity unknown;
    assert(!unknown.getSheetMetal(area,t,m)); near(area,0); near(t,0); near(m,0);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            cpp = Path(tmp)/'adapter.cpp'
            cpp.write_text(stub+method+checks)
            executable = str(Path(tmp)/'adapter')
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-pedantic',
                '-I',str(root/'TVS_Ventilation_DBX'),str(cpp),'-o',executable],check=True)
            subprocess.run([executable],check=True)

if __name__ == '__main__':
    unittest.main()
