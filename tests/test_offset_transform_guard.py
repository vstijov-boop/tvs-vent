"""Execute actual transform-permission predicate with API stubs, not a COPY runtime proof."""
import pathlib
import re
import subprocess
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class OffsetTransformGuard(unittest.TestCase):
    def test_actual_permission_predicate(self):
        text=(ROOT/'TVS_Ventilation_DBX/OffsetAssemblyData.cpp').read_text(encoding='utf-8-sig')
        body=re.search(r'bool allowTransform\([\s\S]*?\n}\n',text).group()
        source=r'''
#include <cmath>
#include <cfloat>
#include <set>
#include <cassert>
#include <iostream>
struct ID {int value;ID(int v=0):value(v){}bool isNull()const{return value==0;}bool operator<(const ID& b)const{return value<b.value;}};
struct AcDbObject {ID id;bool offset;AcDbObject(int i,bool m):id(i),offset(m){}ID objectId()const{return id;}};
struct AcGeMatrix3d {double entry[4][4];AcGeMatrix3d(){for(int r=0;r<4;++r)for(int c=0;c<4;++c)entry[r][c]=r==c?1:0;}};
namespace tvsOffset {bool finite(double x){return x>=-DBL_MAX&&x<=DBL_MAX;}}
static int writeDepth=0;static std::set<ID> clones;static bool marked(const AcDbObject* o){return o->offset;}
''' + body + r'''
int main(){
 AcDbObject original(1,true),copy(2,true),legacy(3,false),scratch(0,true);
 AcGeMatrix3d translation;translation.entry[0][3]=123;translation.entry[1][3]=-456;translation.entry[2][3]=37;
 assert(!allowTransform(&original,translation));
 clones.insert(copy.id);assert(allowTransform(&copy,translation));assert(allowTransform(&scratch,translation));
 AcGeMatrix3d scale;scale.entry[0][0]=2;scale.entry[1][1]=2;scale.entry[2][2]=2;
 assert(!allowTransform(&original,scale));assert(!allowTransform(&copy,scale));assert(allowTransform(&legacy,scale));
 AcGeMatrix3d rotation;rotation.entry[0][0]=0;rotation.entry[0][1]=-1;rotation.entry[1][0]=1;rotation.entry[1][1]=0;
 assert(!allowTransform(&copy,rotation));
 AcGeMatrix3d mirror;mirror.entry[0][0]=-1;assert(!allowTransform(&copy,mirror));
 clones.clear();assert(!allowTransform(&copy,translation));
 writeDepth=1;assert(allowTransform(&original,translation));writeDepth=0;
 std::cout<<"PASS actual permission predicate: originals blocked, clone/scratch translation allowed, copy SCALE/ROTATE/MIRROR blocked, permit expiry; SDK stubs only\n";
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/'guard.cpp';binary=pathlib.Path(folder)/'guard';cpp.write_text(source)
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-pedantic',str(cpp),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
if __name__=='__main__':unittest.main(verbosity=2)
