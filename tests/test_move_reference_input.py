"""Check the actual TVS_MOVE reference prompt with deterministic AutoCAD input stubs."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class MoveReferenceInput(unittest.TestCase):
    def test_enter_default_and_options(self):
        text = (ROOT / 'TVS_Ventilation_ARX/TVSMoveCommand.h').read_text(encoding='utf-8-sig')
        start = text.index('inline bool destinationPoint(')
        brace = text.index('{', start)
        depth, end = 1, brace + 1
        while depth:
            depth += (text[end] == '{') - (text[end] == '}')
            end += 1
        helper = text[start:end]
        source = r'''
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cwchar>
#include <vector>
#include "MoveGeometry.h"
using tvsMoveGeometry::Point;
typedef double ads_point[3];typedef wchar_t ACHAR;
const int RTNORM=5100,RTNONE=5000,RTCAN=-5002,RSG_NONULL=1;
struct Node {std::vector<Point> ports;double a;};
static int keywordStatus=RTNONE,pointStatus=RTNORM,pointCalls=0,initCalls=0;
static const ACHAR* response=L"";static ads_point baseSeen;
int acedInitGet(int flags,const ACHAR* keys){
 if(initCalls++==0){assert(flags==0);assert(std::wcscmp(keys,L"Izabrana Druga Osa")==0);}
 else {assert(flags==RSG_NONULL);assert(keys==NULL);}return RTNORM;
}
int acedGetKword(const ACHAR* prompt,ACHAR* value){
 assert(std::wcsstr(prompt,L"<Izabrana>")!=NULL);
 if(keywordStatus==RTNORM)std::wcscpy(value,response);return keywordStatus;
}
int _wcsicmp(const ACHAR* a,const ACHAR* b){return std::wcscmp(a,b);}
void acutPrintf(const ACHAR*,...){}
namespace tvsCoordinates {int getPoint(const ads_point base,const ACHAR*,ads_point destination){
 ++pointCalls;for(int i=0;i<3;++i){baseSeen[i]=base[i];destination[i]=base[i]+200;}return pointStatus;
}}
''' + helper + r'''
int main(){Node n;n.a=250;n.ports.push_back(Point(0,1000));n.ports.push_back(Point(2000,1000));
 ads_point picked={600,1150,0},destination;Point origin;
 assert(destinationPoint(n,picked,origin,destination));assert(std::fabs(origin.y-1125)<1e-9);assert(pointCalls==1);
 keywordStatus=RTNORM;response=L"Druga";initCalls=0;
 assert(destinationPoint(n,picked,origin,destination));assert(std::fabs(origin.y-875)<1e-9);
 response=L"Osa";initCalls=0;assert(destinationPoint(n,picked,origin,destination));assert(origin.y==1000);
 response=L"Izabrana";initCalls=0;assert(destinationPoint(n,picked,origin,destination));assert(origin.y==1125);
 keywordStatus=RTCAN;initCalls=0;int before=pointCalls;assert(!destinationPoint(n,picked,origin,destination));assert(pointCalls==before);
 keywordStatus=RTNONE;pointStatus=RTCAN;initCalls=0;assert(!destinationPoint(n,picked,origin,destination));
}
'''
        # Keep the stub readable without triggering GCC's indentation warning.
        source = source.replace('std::wcscpy(value,response);return keywordStatus;', 'std::wcscpy(value,response);\n return keywordStatus;')
        with tempfile.TemporaryDirectory() as folder:
            cpp, binary = pathlib.Path(folder) / 'input.cpp', pathlib.Path(folder) / 'input'
            cpp.write_text(source)
            subprocess.run(['g++', '-std=c++98', '-Wall', '-Wextra', '-Werror', '-pedantic',
                            '-I', str(ROOT / 'TVS_Ventilation_ARX'), str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
