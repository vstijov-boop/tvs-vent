#pragma once
// ASHRAE DFDB keys of the TVS fittings (the user's mapping 2026-11-14), shared by TVS_PAD
// (geometry/PressureDrop.h) and the specification (plan 9, 2026-11-28: SPEC.cpp names the key of
// each fitting, supply / exhaust, e.g. "ASHRAE SR4-1 / ER4-1"). Pure C++, ASCII, sizes in mm.
//   Specification: no network there, so the usual flow direction is taken - a prelaz from the
//   larger section to the smaller one in supply, the other way in exhaust (the air goes to the fan).
#include <cmath>
#include <string>
namespace tvsPressure {
struct Section {double a,b;bool round;Section():a(0),b(0),round(false){}Section(double x,double y,bool r):a(x),b(y),round(r){}};
enum TeeType {TeePlain=0,TeeBranch=1,TeePants=2,TeeTap=3};   // TK0, TK-O, TK-G, stucna (TeeData.h)
// ---- ASHRAE keys (the user's mapping) ----
inline std::string elbowKey(const Section& s,double angle){
    if(!s.round)return "CR3-1";
    if(std::fabs(angle-90)<=7.5)return s.a<=250?"CD3-2":"CD3-21";
    if(std::fabs(angle-45)<=7.5&&s.a<=250)return "CD3-4";
    return std::string();
}
inline std::string transitionKey(bool exhaust,const Section& up,const Section& down){
    if(up.round&&down.round)return exhaust?"ED4-1":"SD4-1";
    if(up.round)return exhaust?"ED4-2":"SR4-3";     // round -> rectangular
    if(down.round)return exhaust?"ER4-3":"SD4-2";   // rectangular -> round
    const bool oneSide=std::fabs(up.a-down.a)<0.5||std::fabs(up.b-down.b)<0.5;
    return exhaust?(oneSide?"ER4-1":"ER4-2"):(oneSide?"SR4-1":"SR4-2");
}
inline std::string junctionKey(bool exhaust,int tee,const Section& common,const Section& branch,bool cone=false,bool lateral=false){
    if(common.round&&branch.round&&lateral)return exhaust?"ED5-2":"SD5-1";   // kosa stucna: wye 45; Y45 (TeePants): each leg as its branch
    if(common.round)return branch.round?(exhaust?"ED5-3":"SD5-9"):std::string();
    if(tee==TeePants)return exhaust?"ER5-4":"SR5-14";
    if(branch.round)return exhaust?"ER5-2":cone?"SR5-12":"SR5-11";   // round tap: cylindrical / conical
    if(exhaust)return tee==TeeTap?"ER5-3":"ER5-1";   // stucna: 45 deg entry branch (ASHRAE: Ab/Ac = 0.5, As = Ac)
    return tee==TeePlain?"SR5-5":tee==TeeBranch?"SR5-1":"SR5-13";   // TK0 plain tee, TK-O smooth radius, stucna 45 deg entry
}
// ---- specification labels ("" no ASHRAE piece) ----
inline std::wstring wideKey(const std::string& k){return std::wstring(k.begin(),k.end());}
inline std::wstring specPair(const std::string& supply,const std::string& exhaust){
    if(supply.empty()&&exhaust.empty())return std::wstring();
    if(supply==exhaust||exhaust.empty())return L"ASHRAE "+wideKey(supply);
    if(supply.empty())return L"ASHRAE "+wideKey(exhaust);
    return L"ASHRAE "+wideKey(supply)+L" / "+wideKey(exhaust);
}
inline std::wstring elbowSpec(const Section& s,double angle){const std::string k=elbowKey(s,angle);return specPair(k,k);}
inline double sectionArea(const Section& s){return s.round?3.14159265358979323846*s.a*s.a/4:s.a*s.b;}
inline std::wstring transitionSpec(const Section& x,const Section& y){
    const bool xLarger=sectionArea(x)>=sectionArea(y);const Section& large=xLarger?x:y;const Section& smaller=xLarger?y:x;
    return specPair(transitionKey(false,large,smaller),transitionKey(true,smaller,large));
}
// tee: 0 TK0, 1 TK-O, 2 TK-G, 3 stucna (TeeType; plain ints: SPEC.cpp has macros of these names)
inline std::wstring junctionSpec(int tee,const Section& common,const Section& branch,bool cone,bool lateral){
    return specPair(junctionKey(false,tee,common,branch,cone,lateral),junctionKey(true,tee,common,branch,cone,lateral));
}
}
