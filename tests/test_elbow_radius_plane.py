"""Exercise actual radius/form methods and the rectangular up/down projection prefix."""
import pathlib
import re
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ElbowRadiusPlane(unittest.TestCase):
    def test_actual_radius_and_projection(self):
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
        source = constants + r'''
#include <cassert>
#include <cmath>
#define DuctTypeFlex 1
namespace Acad {enum ErrorStatus {eOk};}
struct Vector {double x,y,z;Vector(double a,double b,double c):x(a),y(b),z(c){}};
struct AcGePoint3d {
 double x,y,z;AcGePoint3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}
 AcGePoint3d rotateBy(double a,const Vector& axis,const AcGePoint3d& origin)const {
  (void)axis;double dx=x-origin.x,dy=y-origin.y;
  return AcGePoint3d(origin.x+dx*std::cos(a)-dy*std::sin(a),origin.y+dx*std::sin(a)+dy*std::cos(a),z);
 }
};
class TVS_TAP {
public:
 double SizeA,SizeB,Radius,RadiusConst,RadiusVariableParameter,RadiusTypeRound,RadiusTypeRect,units,Swectangle,startangle;
 bool ThisRound;int DuctType,Form;
 AcGePoint3d CenterPoint,MiddlePoint,A,D,MA,MC;
 TVS_TAP():SizeA(400),SizeB(200),Radius(9999),RadiusConst(150),RadiusVariableParameter(.5),RadiusTypeRound(3),RadiusTypeRect(2),units(1),Swectangle(1.570796326794897),startangle(0),ThisRound(false),DuctType(0),Form(1){}
 void assertReadEnabled()const{}void assertWriteEnabled(){}
 double get_radius(void)const;void setRadius();Acad::ErrorStatus put_Form(int);
 void project();
};
namespace tvsUnits {double mm(double value,const TVS_TAP* p){return value*p->units;}}
''' + methods + '\nvoid TVS_TAP::project(){setRadius();Vector vectr(0,0,1);\n' + projection + '\n}\n' + r'''
static void equal(double a,double b){assert(std::fabs(a-b)<1e-8*(1+std::fabs(b)));}
int main(){
 const double pi=3.14159265358979323846,units[]={1,10,1000};
 const double dimensions[][2]={{400,200},{200,600},{600,200},{250,250}};
 for(int u=0;u<3;++u)for(int d=0;d<4;++d){
  TVS_TAP t;double f=units[u];t.units=f;t.SizeA=dimensions[d][0]/f;t.SizeB=dimensions[d][1]/f;t.RadiusConst=150/f;
  equal(t.get_radius()*f,dimensions[d][0]*.5);
  for(int form=Form_Up;form<=Form_Down;++form){
   t.put_Form(form);equal(t.Radius*f,dimensions[d][1]*.5);
   for(int rotation=0;rotation<4;++rotation)for(int angle=1;angle<=2;++angle){
    t.startangle=rotation*pi/4;t.Swectangle=angle*pi/4;t.MiddlePoint=AcGePoint3d(12000/f,-5000/f,30/f);
    t.project();double reach=dimensions[d][1]/f*std::sin(t.Swectangle);
    equal(t.MA.x,t.MiddlePoint.x+reach*std::sin(t.startangle));
    equal(t.MA.y,t.MiddlePoint.y-reach*std::cos(t.startangle));
    equal(t.MC.x,t.MiddlePoint.x);equal(t.MC.y,t.MiddlePoint.y);
   }
   equal(t.SizeA*f,dimensions[d][0]);equal(t.SizeB*f,dimensions[d][1]);
  }
  t.put_Form(Form_Direct);equal(t.Radius*f,dimensions[d][0]*.5);
  t.RadiusTypeRect=RadiusTypeRect_RadiusConstant;t.put_Form(Form_Up);equal(t.Radius*f,150);
  t.RadiusTypeRect=RadiusTypeRect_RadiusSpecific;t.put_Form(Form_Down);
  equal(t.Radius*f,dimensions[d][1]<355?dimensions[d][1]:dimensions[d][1]/2);
  t.ThisRound=true;t.SizeA=400/f;t.SizeB=999/f;t.RadiusTypeRound=RadiusTypeRound_RadiusVariable;
  for(int form=1;form<=3;++form){t.put_Form(form);equal(t.Radius*f,200);}
 }
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp, binary = pathlib.Path(folder) / 'radius.cpp', pathlib.Path(folder) / 'radius'
            cpp.write_text(source)
            subprocess.run(['g++', '-std=c++98', '-Wall', '-Wextra', '-Werror', '-pedantic',
                            str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_kgtez_vertical_fallback_and_native_area(self):
        from test_kgtez_units import Harness
        h = Harness()
        h.env['tvs-elbow-rg'] = lambda obj: (_ for _ in ()).throw(AssertionError('Vertical projection must not be used as a developed arc'))
        for form in (2, 3):
            obj = dict(dpr=400.0, sizeb=200.0, angle=90.0, form=form)
            info = h.call('tvs-info', obj, 'TVS_TAP')
            self.assertAlmostEqual(info[3], .12 * h.env['pi'])
            # Constant Ri=150 mm uses the native calculation, not the fallback k=.5.
            obj['sheetarea'] = .15 * h.env['pi']
            info = h.call('tvs-info', obj, 'TVS_TAP')
            self.assertAlmostEqual(info[3], .15 * h.env['pi'])
            obj['sheetarea'] = {'variant': .15 * h.env['pi']}
            self.assertAlmostEqual(h.call('tvs-info', obj, 'TVS_TAP')[3], .15 * h.env['pi'])


if __name__ == '__main__':
    unittest.main()
