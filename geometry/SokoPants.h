#pragma once
// Soko inzenjering "Spiro Y45 - racva" (katalog "Kanali za razvod vazduha", str. 36; user
// 2026-11-26): d1 ulaz, d3 = d4 ogranci, L2 od presjeka osa (ogranak / ulaz) do lica ogranka,
// with the spigot. Model (geometry/TeeShape.h, Pants + lateral 45): inlet straight s = spigot(d1)
// (the user's block KTP_htany: ring 10 + collar 30 = 40 for 200), the outer branch walls start at
// the inlet corners at 45 deg. Sizes not in the table (or d3 != d4): L2 = d3 + spigot(d3) (fits
// the equal-size rows 80..160 within 5 mm), at least spigot(d3) of inner wall beyond the crotch.
#include <cmath>
namespace tvsSokoPants {
struct Row {int d1,d3,L2;};
inline const Row* rows(int& n){
    static const Row r[]={
    {80,80,120},{100,100,140},{125,125,170},{150,100,190},{160,160,205},{180,125,215},
    {200,140,280},{250,180,320},{300,200,365},{355,250,410},{400,300,440},{450,315,490},
    {500,355,550},{560,400,580},{630,400,610},{710,450,670},{800,500,740},{900,560,825}};
    n=static_cast<int>(sizeof(r)/sizeof(r[0]));return r;
}
// same rule as tvsSpiro::spigot (TVS_Ventilation_DBX/SpiroCatalog.h), mm
inline double spigotMm(double d){const double r=std::floor(d+0.5);return r<=224?40:r<=355?55:r<=630?75:r<=900?100:115;}
// The catalogue branch size for an inlet d1 (0: not in the table).
inline int branchFor(double d1){int n=0;const Row* r=rows(n);for(int i=0;i<n;++i)if(std::fabs(r[i].d1-d1)<0.5)return r[i].d3;return 0;}
// L2 (mm) for inlet d1 and branches d3 / d4; true: Soko row. minL2: the shortest L2 the shape allows
// for this branch (inner wall reaches the crotch), from tvsTee::wyeMinL2.
inline bool length(double d1,double d3,double d4,double minL2,double& L2){
    int n=0;const Row* r=rows(n);
    if(std::fabs(d3-d4)<0.5)for(int i=0;i<n;++i)if(std::fabs(r[i].d1-d1)<0.5&&std::fabs(r[i].d3-d3)<0.5&&r[i].L2>=minL2+spigotMm(d3)){L2=r[i].L2;return true;}
    const double sp=spigotMm(d3);L2=d3+sp;if(L2<minL2+sp)L2=std::ceil(minL2+sp);return false;
}
}
