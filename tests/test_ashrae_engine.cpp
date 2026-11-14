// geometry/AshraeEngine.h with made-up data (the ASHRAE data is not in this repository; the
// comparison with the user's DuctPressDrop engine is tests/ashrae_golden.cpp, run locally).
#include "../geometry/AshraeEngine.h"
#include <cstdio>
#include <cstdlib>
static int fails=0;
static void eq(double a,double b,const char* what){if(std::fabs(a-b)>1e-9*(1+std::fabs(b))){std::printf("FAIL %s: %.12g vs %.12g\n",what,a,b);++fails;}}
static void yes(bool c,const char* what){if(!c){std::printf("FAIL %s\n",what);++fails;}}
int main(){
    std::map<std::string,std::string> files;
    files["Constants.csv"]="\xEF\xBB\xBFVariable;IPValue;SIValue;Description;Calculated\nVPC;4005;1.2910;x;0\nRHO;0.075;1.204;x;0\n";
    files["Equation.csv"]="Key;Order;Equation;Input;Value\n"
        "T1;20;Co=fcn(x,1);;\nT1;10;x=Q/Qc;;\n"                                   // Order decides, not the file
        "T2;1;Co=fcn(x,y,1);;\nT2;2;Po=Co*Pv;;\nT2;3;bad=1/0;;\nT2;4;Pv=(V/VPC)^2;;\n"
        "T3;1;Co=fcn(x,y,z,1);;\n"
        "J;1;Qbc=Qb/Qc;;\nJ;2;Cb=fcn(Qbc,1);;\nJ;3;Pob=Cb*Pvb;;\nJ;4;Cs=0.1;;\nJ;5;Pos=Cs*Pvs;;\n"
        "E;1;a=-2^2;;\nE;2;b=2^-1;;\nE;3;c=SQR([a+8]);;\nE;4;d=IFLE0(a,1,2);;\nE;5;e=MAX(1,5,3);;\nE;6;f=IFIP(9,7);;\nE;7;g=SQR(a);;\nE;8;h=ATN(1)*4;;\nE;9;i=INT(-2.7);;\nE;10;j=unknown+1;;\nE;11;k=(-8)^(1/3);;\nE;12;l=SQR(k);;\n";
    files["Output.csv"]="Key;Order;Prompt;Variable;IPUnitID;IPDecimals;SIUnitID;SIDecimals\nJ;2;x;Pob;a;0;b;0\nJ;1;x;Cb;a;0;b;0\n";
    files["tables/T1.TBL"]="  ' comment\n #  #  #\n   x->  0.0  1.0  2.0\n        1.0  3.0  O.5\n";
    files["tables/T2.TBL"]="   0.0   1.0\n0.0  0  10\n1.0  2  12\n====\n 5 6\n 1 1\n";
    files["tables/T3.TBL"]=" 0 1\n0 0 0 10\n  1 2 12\n1 0 100 110\n  1 102 112\n";
    files["tables/J.TBL"]="0.1 0.5 1.0\n1.5(38) 60.0 2.0 1.0\n";
    tvsAshrae::Reader rd=[&](const std::string& f,std::string& t){auto i=files.find(f);if(i==files.end())return false;t=i->second;return true;};
    tvsAshrae::Engine e;yes(e.load(rd),"load");
    std::vector<std::string> w;
    // 1D, O.5 -> 0.5, clamp + warning once per axis
    eq(e.calculate("T1",{{"Q",1},{"Qc",2}},&w)["Co"],2.0,"1D");
    eq(e.calculate("T1",{{"Q",5},{"Qc",1}},&w)["Co"],0.5,"1D clamp");yes(w.size()==1,"warning");
    e.clamp=false;eq(e.calculate("T1",{{"Q",3},{"Qc",1}},nullptr)["Co"],-2.0,"1D extrapolation");e.clamp=true;
    // 2D bilinear; a failing line is skipped; ==== starts table 2
    std::map<std::string,double> cv=e.calculate("T2",{{"x",0.5},{"y",0.5},{"V",1.291}},nullptr);
    eq(cv["Co"],6.0,"2D");yes(!cv.count("bad"),"1/0 skipped");yes(!cv.count("Po"),"Pv not yet");eq(cv["Pv"],1.0,"Pv");
    eq(e.table("T2",{5.5},2,nullptr),1.0,"table 2");eq(e.table("NONE",{1},1,nullptr),0.2,"no table");
    // 3D
    eq(e.calculate("T3",{{"x",0.5},{"y",0.5},{"z",0.5}},nullptr)["Co"],(6+106)/2.0,"3D");
    // junction: pv on the first Q/Qc interval (C*x^2), IP(SI) value pair, loss roles
    cv=e.calculate("J",{{"Qb",0.3},{"Qc",1},{"Pvb",10},{"Pvs",20}},nullptr);
    const double x=0.3,pvv=(60*0.01*(1-(x-0.1)/0.4)+2*0.25*((x-0.1)/0.4))/(x*x);
    eq(cv["Cb"],pvv,"pv");e.pv=false;eq(e.calculate("J",{{"Qb",0.3},{"Qc",1}},nullptr)["Cb"],31.0,"linear");e.pv=true;
    tvsAshrae::Engine::Loss b=e.loss("J",{{"Qb",0.3},{"Qc",1},{"Pvb",10},{"Pvs",20}},false,nullptr),m=e.loss("J",{{"Qb",0.3},{"Qc",1},{"Pvb",10},{"Pvs",20}},true,nullptr);
    eq(b.c,pvv,"branch C");eq(b.dp,pvv*10,"branch dP");eq(m.c,0.1,"main C");eq(m.dp,2.0,"main dP");
    yes(e.resultVars("J").size()==2&&e.resultVars("J")[0]=="Cb","output order");
    // expressions
    cv=e.calculate("E",{},nullptr);
    eq(cv["a"],-4,"-2^2");eq(cv["b"],0.5,"2^-1");eq(cv["c"],2,"SQR [ ]");eq(cv["d"],1,"IFLE0");eq(cv["e"],5,"MAX");eq(cv["f"],7,"IFIP SI");
    yes(!cv.count("g"),"sqrt negative skipped");eq(cv["h"],3.14159265358979,"ATN");eq(cv["i"],-2,"INT");yes(!cv.count("j"),"unknown name skipped");
    yes(cv.count("k")&&std::isnan(cv["k"])&&!cv.count("l"),"complex");
    // TBL line clean-ups
    yes(tvsAshrae::cleanLine("1.5(38) - 3873 1242777-10249.51 O.O1")=="1.5 -3873 1242777 -10249.51 0.O1","clean (as engine.py)");
    if(fails)return 1;std::printf("OK\n");return 0;
}
