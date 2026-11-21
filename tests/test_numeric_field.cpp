// geometry/NumericField.h: numeric dialog fields (review 2026-11b, proposal 4).
#include "../geometry/NumericField.h"
#include <cassert>
#include <cstdio>
#include <string>
using namespace tvsNumeric;
static bool eq(double a,double b){return std::fabs(a-b)<1e-12;}
int main(){
    double v=-1;
    // parse: comma or point, spaces, no text
    assert(parse(L"1,5",v)&&eq(v,1.5));
    assert(parse(L" 2.25 ",v)&&eq(v,2.25));
    assert(parse(L"\t315\r\n",v)&&eq(v,315));
    assert(parse(L"-12,75",v)&&eq(v,-12.75));
    assert(parse(L"1e3",v)&&eq(v,1000));
    v=7;assert(!parse(L"abc",v)&&v==7);
    assert(!parse(L"",v));assert(!parse(L"   ",v));assert(!parse((const wchar_t*)0,v));
    assert(!parse(L"1,5 m",v));assert(!parse(L"1,000,5",v));assert(!parse(L"inf",v));assert(!parse(L"nan",v));assert(!parse(L"0x10",v));
    assert(!parse(L"Različito",v));
    assert(parse(L"-0,0",v)&&v==0&&!std::signbit(v));
    assert(formatG(-0.0,15)==L"0");
    // format
    assert(formatG(315,15)==L"315");assert(formatG(0.315,15)==L"0.315");assert(formatG(1.5,15)==L"1.5");
    assert(format(2.755,2)==L"2.76"||format(2.755,2)==L"2.75");   // binary 2.755: either, but two decimals
    assert(format(3,2)==L"3.00");assert(format(-0.001,2)==L"0.00");assert(format(12.5,0)==L"12"||format(12.5,0)==L"13");
    assert(format(-1.25,1)==L"-1.2"||format(-1.25,1)==L"-1.3");
    // unit factor round trip: m drawing (1000 mm / unit), cm (10), mm (1)
    const double factors[3]={1000,10,1};
    const double sizes[5]={0.315,0.125,2.755,0.1+0.2,0.355};
    for(int f=0;f<3;++f)for(int k=0;k<5;++k){
        const double stored=sizes[k]*(f==0?1:f==1?100:1000);   // 0.315 m = 31.5 cm = 315 mm
        const std::wstring shown=show(exact(stored).c_str(),factors[f],15);
        double back=0;assert(!edited(exact(stored).c_str(),shown.c_str(),factors[f],15,back));   // OK without change
        assert(parse(shown,back));assert(same(fromDisplay(back,factors[f]),stored));
        assert(same(fromDisplay(toDisplay(stored,factors[f]),factors[f]),stored));
    }
    assert(show(L"0.315",1000,15)==L"315");   // Ø315 in a m drawing stays 315 (was 320, N1)
    double nv=0;assert(edited(L"0.315",L"320",1000,15,nv)&&same(nv,0.32));
    // angles: 30 deg -> rad -> text -> deg unchanged; 44.99999 stays and compares equal
    {
        const double r=radians(30);const std::wstring t=exact(r);double back=0;
        assert(parse(t,back)&&back==r);assert(eq(degrees(back),30));
        assert(show(t.c_str(),degPerRad(),10)==L"30");
        assert(!edited(t.c_str(),L"30",degPerRad(),10,back));
        assert(edited(t.c_str(),L"45",degPerRad(),10,back)&&same(back,radians(45),1e-15));
        const double r2=radians(44.99999);const std::wstring s2=show(exact(r2).c_str(),degPerRad(),10);
        assert(s2==L"44.99999");
        assert(!edited(exact(r2).c_str(),s2.c_str(),degPerRad(),10,back));
        assert(!edited(exact(r2).c_str(),L"44,99999",degPerRad(),10,back));   // comma: same number
        assert(edited(exact(r2).c_str(),L"45",degPerRad(),10,back));         // 45 is a change
        // an elbow stored as 1.57 rad (old rounding) shows 89.95437384 and is not rewritten on OK
        const std::wstring s3=show(L"1.57",degPerRad(),10);assert(s3==L"89.95437384");
        assert(!edited(L"1.57",s3.c_str(),degPerRad(),10,back));
    }
    // edited: texts that are not numbers
    assert(show(L"Različito",1000,15)==L"Različito");
    assert(!edited(L"Različito",L"Različito",1000,15,nv));
    assert(edited(L"Različito",L"400",1000,15,nv)&&eq(nv,0.4));       // different values -> one typed value
    assert(!edited(L"0.4",L"",1000,15,nv));                                // emptied field: no change
    assert(edited(L"",L"250",1,15,nv)&&eq(nv,250));
    // differs (command side) and changed by tolerance
    assert(!differs(L"1.5",L"1,50",nv));assert(!differs(L"0.315",L"0.315000000000001",nv));
    assert(differs(L"0.315",L"0.32",nv)&&eq(nv,0.32));
    assert(!differs(L"Različito",L"Različito",nv));assert(!differs(L"1",L"x",nv));
    assert(differs(L"Različito",L"2",nv)&&eq(nv,2));
    assert(!changed(1.0,1.0+1e-10));assert(changed(1.0,1.0+1e-8));
    assert(!changed(2755,2755+1e-7));assert(changed(2755,2755.001));
    assert(!changed(0,1e-10));assert(changed(0,1e-8));
    assert(changed(1,2,0.5));assert(!changed(1,1.4,0.5));
    // range
    assert(inside(45,0,180,false));assert(!inside(0,0,180,false));assert(!inside(180,0,180,false));assert(!inside(-5,0,180,false));
    assert(inside(0,0,1e9,true));assert(!inside(-1,0,1e9,true));
    std::puts("OK");
    return 0;
}
