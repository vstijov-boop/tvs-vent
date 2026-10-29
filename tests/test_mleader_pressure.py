"""Compile the actual [P] handler and existing TVS calculator with minimal CAD stubs.
This checks numeric behavior and units, not the AutoCAD multileader UI.
"""
import subprocess
import tempfile
import unittest
from pathlib import Path

class PressureLabel(unittest.TestCase):
    def test_actual_handler_and_calculator(self):
        root=Path(__file__).parents[1]
        controller=(root/'TVS_Ventilation_ARX/MleaderController.cpp').read_text(encoding='utf-8-sig')
        start=controller.index('{',controller.index('else if (stringTemplate == L"P")'))
        end=start+1
        depth=1
        while depth:
            if controller[end]=='{':depth+=1
            if controller[end]=='}':depth-=1
            end+=1
        handler=controller[start:end]
        start_v=controller.index('{',controller.index('else if (stringTemplate == L"V")'))
        end_v=controller.index('}',start_v)+1
        speed_handler=controller[start_v:end_v]
        calculator=(root/'TVS_Ventilation_ARX/Calculate.cpp').read_text(encoding='utf-8-sig')
        calculator='\n'.join(line for line in calculator.splitlines() if not line.startswith('#include'))
        source=r'''
#include <cmath>
#include <cstdlib>
#include <cfloat>
#include <cwchar>
#include <cassert>
#include <limits>
#include <string>
using std::abs;
#include "Calculate.h"
struct TVS_Pipe {
    bool pipe,round;
    double a,b,flow,units;
    static const TVS_Pipe *cast(const TVS_Pipe *p) { return p->pipe ? p : NULL; }
    double get_SizeA() const { return a; }
    double get_SizeB() const { return b; }
    double get_Flow() const { return flow; }
    bool get_ThisRound() const { return round; }
    double get_Speed() const { return flow; } // Inject known m/s to test label formatting.
};
struct Ptr {
    const TVS_Pipe *object;
    explicit Ptr(const TVS_Pipe &p):object(&p) {}
    const TVS_Pipe *operator->() const { return object; }
};
namespace tvsUnits { double mm(double value,const TVS_Pipe *p) { return value*p->units; } }
struct Text {
    std::wstring value;
    void Format(const wchar_t *format,double number) {
        wchar_t buffer[128]; std::swprintf(buffer,128,format,number); value=buffer;
    }
};
static bool label(const TVS_Pipe &object,Text &stringText) {
    Ptr pEnt(object);
HANDLER
}
static bool speedLabel(const TVS_Pipe &object,Text &stringText) {
    Ptr pEnt(object);
SPEED_HANDLER
}
static double reference(double a,double b,double flow) {
    const double area=b==0 ? a*a*M_PI/4e6 : a*b/1e6;
    const double diameter=b==0 ? a/1000 : 2*a*b/(a+b)/1000;
    const double speed=std::fabs(flow)/3600/area;
    const double reynolds=speed*diameter/.000014;
    return .6*speed*speed*.11*std::pow(.0001/diameter+68/reynolds,.25)/diameter;
}
int main() {
    const double units[]={1,10,1000};
    for (int i=0;i<3;++i) for (int circular=0;circular<2;++circular) {
        TVS_Pipe p={true,circular!=0,400/units[i],200/units[i],720.5,units[i]};
        const double a=p.a,b=p.b,flow=p.flow;
        Text text;
        assert(label(p,text));
        const double wanted=reference(400,circular ? 0 : 200,720.5);
        const double actual=std::wcstod(text.value.c_str(),NULL);
        assert(std::fabs(actual-wanted)<=.005000001);
        assert(p.a==a && p.b==b && p.flow==flow);
        const std::wstring positive=text.value;
        assert(positive.find(L'.')+3==positive.size());
        p.flow=2.5; assert(speedLabel(p,text)); assert(text.value==L"2.50");
        p.flow=2.546; assert(speedLabel(p,text)); assert(text.value==L"2.55");
        p.flow=0; assert(speedLabel(p,text)); assert(text.value==L"0.00");
        p.flow=-720.5; assert(label(p,text)); assert(text.value==positive);
        assert(p.flow==-720.5);
        p.flow=0; assert(label(p,text)); assert(text.value==L"0.00");
        p.flow=std::numeric_limits<double>::quiet_NaN(); assert(!label(p,text));
        p.flow=std::numeric_limits<double>::infinity(); assert(!label(p,text));
        p.flow=720.5; p.a=0; assert(!label(p,text));
        p.a=-1; assert(!label(p,text));
        p.a=400/units[i]; p.b=-1;
        if (!circular) assert(!label(p,text));
        p.b=0; if (!circular) assert(!label(p,text));
        p.pipe=false; assert(!label(p,text));
    }
}
'''
        source=source.replace('SPEED_HANDLER',speed_handler).replace('HANDLER',handler)+'\n'+calculator
        with tempfile.TemporaryDirectory() as tmp:
            cpp=Path(tmp)/'pressure.cpp'
            cpp.write_text(source)
            exe=str(Path(tmp)/'pressure')
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-pedantic',
                '-I',str(root/'TVS_Ventilation_ARX'),str(cpp),'-o',exe],check=True)
            subprocess.run([exe],check=True)

if __name__=='__main__':unittest.main()
