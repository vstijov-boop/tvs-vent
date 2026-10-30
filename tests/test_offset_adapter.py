"""Execute extracted real TAP radius/form/projection expressions with small SDK stubs.

This is NOT an ObjectARX runtime test: rotation, DB assertions, vectors and units
are substitutes. The geometry expressions themselves come from current TVS_TAP.cpp.
"""
import pathlib
import re
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class OffsetAdapterTests(unittest.TestCase):
    def test_extracted_tap_branch(self):
        text = (ROOT / 'TVS_Ventilation_DBX/TVS_TAP.cpp').read_text(encoding='utf-8-sig')
        constants = '\n'.join(line for line in (ROOT / 'TVS_Ventilation_DBX/TVS_TAP.h').read_text().splitlines()
                              if line.startswith('#define RadiusType') or line.startswith('#define Form_'))
        methods = '\n'.join(re.search(signature + r'[\s\S]*?\n}\n', text).group() for signature in (
            r'double TVS_TAP::get_radius\(void\) const', r'void TVS_TAP::setRadius\(\)',
            r'Acad::ErrorStatus TVS_TAP::put_Form\(int newVal\)'))
        start = text.index('#pragma region Duct Type Still')
        start = text.index('#pragma region\tform Updown', start)
        start = text.index('CenterPoint =', start)
        end = text.index('t1 =', start)
        projection = text[start:end]
        angle_start = text.index("double  Lx, Ly, Nx, Ny;", text.index("TVS_TAP::subWorldDraw"))
        angle_end = text.index("CenterPoint =", angle_start)
        angle_prefix = text[angle_start:angle_end]
        source = constants + r'''
#include <cmath>
#include <iostream>
#include <cstdlib>
#include "geometry/OffsetAdapter.h"
#define DuctTypeFlex 1
namespace Acad {enum ErrorStatus {eOk};}
struct AcGeVector3d {double x,y,z;AcGeVector3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}};
struct AcGePoint3d {
 double x,y,z;AcGePoint3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}
 AcGePoint3d rotateBy(double angle,const AcGeVector3d&,const AcGePoint3d& origin)const {
  double dx=x-origin.x,dy=y-origin.y;
  return AcGePoint3d(origin.x+dx*std::cos(angle)-dy*std::sin(angle),origin.y+dx*std::sin(angle)+dy*std::cos(angle),z);
 }
};
class TVS_TAP {
public:
 double SizeA,SizeB,Radius,RadiusConst,RadiusVariableParameter,RadiusTypeRound,RadiusTypeRect,units,Swectangle,startangle;
 bool ThisRound;int DuctType,Form;
 AcGePoint3d CenterPoint,MiddlePoint,A,D,MA,MC;AcGeVector3d Startvector;
 TVS_TAP():SizeA(400),SizeB(200),Radius(-987654),RadiusConst(150),RadiusVariableParameter(.5),RadiusTypeRound(3),RadiusTypeRect(2),units(1),Swectangle(0),startangle(0),ThisRound(false),DuctType(0),Form(1),MA(-999,-888,-777),MC(999,888,777){}
 void assertReadEnabled()const{}void assertWriteEnabled(){}
 double get_radius(void)const;void setRadius();Acad::ErrorStatus put_Form(int);void project();
};
namespace tvsUnits {double mm(double value,const TVS_TAP* p){return value*p->units;}}
''' + methods + '\nvoid TVS_TAP::project(){setRadius();\n' + angle_prefix + projection + '\n}\n'
        source += (ROOT / 'tests/offset_adapter_cases.inc').read_text()
        with tempfile.TemporaryDirectory() as folder:
            cpp, binary = pathlib.Path(folder) / 'adapter.cpp', pathlib.Path(folder) / 'adapter'
            cpp.write_text(source)
            subprocess.run(['g++', '-std=c++98', '-Wall', '-Wextra', '-Werror', '-pedantic',
                            '-I', str(ROOT), str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    unittest.main(verbosity=2)
