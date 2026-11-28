// plan 9: ASHRAE key in the specification name (geometry/AshraeKeys.h)
#include "../geometry/AshraeKeys.h"
#include <cstdio>
using namespace tvsPressure;
static int bad=0;
static void is(const std::wstring& got,const wchar_t* want,const char* what){if(got!=want){++bad;std::printf("FAIL %s: %ls\n",what,got.c_str());}}
int main(){
    is(elbowSpec(Section(400,200,false),90),L"ASHRAE CR3-1","rect elbow");
    is(elbowSpec(Section(200,0,true),90),L"ASHRAE CD3-2","round 90 <=250");
    is(elbowSpec(Section(315,0,true),90),L"ASHRAE CD3-21","round 90 >250");
    is(elbowSpec(Section(200,0,true),45),L"ASHRAE CD3-4","round 45");
    is(elbowSpec(Section(315,0,true),30),L"","round 30: none");
    is(transitionSpec(Section(600,300,false),Section(400,300,false)),L"ASHRAE SR4-1 / ER4-1","rect one side");
    is(transitionSpec(Section(400,300,false),Section(600,400,false)),L"ASHRAE SR4-2 / ER4-2","rect both, order");
    is(transitionSpec(Section(315,0,true),Section(250,0,true)),L"ASHRAE SD4-1 / ED4-1","round");
    is(transitionSpec(Section(600,300,false),Section(250,0,true)),L"ASHRAE SD4-2 / ED4-2","rect large -> round");
    is(transitionSpec(Section(250,0,true),Section(600,300,false)),L"ASHRAE SD4-2 / ED4-2","same, other order");
    is(transitionSpec(Section(400,0,true),Section(200,100,false)),L"ASHRAE SR4-3 / ER4-3","round large -> rect");
    is(junctionSpec(1,Section(600,300,false),Section(300,300,false),false,false),L"ASHRAE SR5-1 / ER5-1","TK-O");
    is(junctionSpec(2,Section(600,300,false),Section(300,300,false),false,false),L"ASHRAE SR5-14 / ER5-4","TK-G");
    is(junctionSpec(3,Section(600,300,false),Section(300,200,false),false,false),L"ASHRAE SR5-13 / ER5-3","stucna");
    is(junctionSpec(3,Section(600,300,false),Section(200,0,true),true,false),L"ASHRAE SR5-12 / ER5-2","konusna");
    is(junctionSpec(0,Section(600,300,false),Section(300,300,false),false,false),L"ASHRAE SR5-5 / ER5-1","TK0");
    is(junctionSpec(3,Section(315,0,true),Section(200,0,true),false,true),L"ASHRAE SD5-1 / ED5-2","kosa TK");
    is(junctionSpec(0,Section(315,0,true),Section(200,0,true),false,false),L"ASHRAE SD5-9 / ED5-3","round T");
    is(junctionSpec(0,Section(315,0,true),Section(200,100,false),false,false),L"","round main, rect branch: none");
    if(!bad)std::printf("OK\n");
    return bad;
}
