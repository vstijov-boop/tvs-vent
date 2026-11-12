#pragma once
// TVS_PROTOK, brzine: velocity in a duct and the size proposal for a maximum velocity.
// Sizes in mm, flow in m3/h, velocity in m/s. tests/test_duct_sizing.cpp.
//   Rectangular, width only: the height stays, the smallest standard width that is fast enough.
//   Rectangular, width and height: the smallest standard section with width >= height,
//   width / height <= 4 and height at least the current one (the ceiling decides the rest).
//   Round: the smallest standard diameter (spiro series, SOKO catalogue sizes included).
//   Friction R (Pa/m): Darcy-Weisbach, lambda from Colebrook-White (iterated), air 20 C
//   (rho 1.2 kg/m3, nu 15.1e-6 m2/s), galvanised sheet k = 0.15 mm; rectangular by the
//   hydraulic diameter 2ab/(a+b) at the real velocity.
//   Candidates (dialog): standard sizes around the limit, smallest first, two too fast ones too.
//   Dionica: a chain of elements between branchings (same flow), from the parent array of
//   the flow network.
#include <cmath>
#include <cwchar>
#include <string>
#include <vector>
namespace tvsDuctSizing {
inline const std::vector<double>& rectangular(){
    static const std::vector<double> s=[]{std::vector<double> v;for(double a=100;a<=1000;a+=50)v.push_back(a);for(double a=1100;a<=3000;a+=100)v.push_back(a);return v;}();
    return s;
}
inline const std::vector<double>& diameters(){
    static const std::vector<double> s={100,125,140,150,160,180,200,224,250,280,300,315,355,400,450,500,560,630,710,800,900,1000,1120,1250,1400,1600};
    return s;
}
inline double area(double a,double b,bool isRound){const double pi=3.14159265358979323846;return isRound?pi*a*a/4e6:a*b/1e6;}
inline double velocity(double flow,double a,double b,bool isRound){const double s=area(a,b,isRound);return s>0?std::fabs(flow)/3600.0/s:0;}
struct Size {bool ok;double a,b;double v,r;Size():ok(false),a(0),b(0),v(0),r(0){}};   // r: Pa/m (friction)
// Rectangular, height b kept.
inline Size widthOnly(double flow,double b,double vmax){
    Size r;if(!(b>0)||!(vmax>0))return r;
    for(size_t i=0;i<rectangular().size();++i){const double a=rectangular()[i];const double v=velocity(flow,a,b,false);
        if(v<=vmax+1e-9){r.ok=true;r.a=a;r.b=b;r.v=v;return r;}}
    return r;
}
// Rectangular, width and height (height not below `minHeight`).
inline Size widthHeight(double flow,double minHeight,double vmax,double ratio=4){
    Size r;if(!(vmax>0))return r;double best=0;
    const std::vector<double>& s=rectangular();
    for(size_t j=0;j<s.size();++j){const double b=s[j];if(b<minHeight-1e-9)continue;
        for(size_t i=0;i<s.size();++i){const double a=s[i];if(a<b-1e-9||a>ratio*b+1e-9)continue;
            const double v=velocity(flow,a,b,false);if(v>vmax+1e-9)continue;
            const double ar=a*b;
            if(!r.ok||ar<best-1e-9||(std::fabs(ar-best)<=1e-9&&a<r.a)){r.ok=true;r.a=a;r.b=b;r.v=v;best=ar;}
            break;}}   // the first width fast enough is the smallest for this height
    return r;
}
inline Size diameter(double flow,double vmax){
    Size r;if(!(vmax>0))return r;
    for(size_t i=0;i<diameters().size();++i){const double d=diameters()[i];const double v=velocity(flow,d,0,true);
        if(v<=vmax+1e-9){r.ok=true;r.a=d;r.v=v;return r;}}
    return r;
}
inline double friction(double flow,double a,double b,bool isRound,double roughnessMm=0.15){
    const double v=velocity(flow,a,b,isRound);if(!(v>0))return 0;
    const double dh=(isRound?a:2*a*b/(a+b))/1000.0;if(!(dh>0))return 0;
    const double rho=1.2,nu=15.1e-6,k=roughnessMm/1000.0,re=v*dh/nu;
    double lambda;
    if(re<2300)lambda=64/re;
    else{double x=1/std::sqrt(0.02);   // 1/sqrt(lambda)
        for(int i=0;i<50;++i){const double n=-2*std::log10(k/(3.71*dh)+2.51*x/re);if(std::fabs(n-x)<1e-12){x=n;break;}x=n;}
        lambda=1/(x*x);}
    return lambda/dh*rho*v*v/2;
}
inline Size sized(double flow,double a,double b,bool isRound){Size s;s.ok=true;s.a=a;s.b=isRound?0:b;s.v=velocity(flow,a,b,isRound);s.r=friction(flow,a,b,isRound);return s;}
// mode: 0 height b kept, 1 width / height <= 4 (width >= height); round ducts: diameters.
inline void candidates(double flow,bool isRound,double b,int mode,double vmax,std::vector<Size>& out,int before=2,int after=4){
    out.clear();std::vector<Size> all;
    if(isRound){for(size_t i=0;i<diameters().size();++i)all.push_back(sized(flow,diameters()[i],0,true));}
    else if(mode==0){if(!(b>0))return;for(size_t i=0;i<rectangular().size();++i)all.push_back(sized(flow,rectangular()[i],b,false));}
    else{const std::vector<double>& s=rectangular();
        for(size_t j=0;j<s.size();++j)for(size_t i=0;i<s.size();++i){const double w=s[i],h=s[j];
            if(w>=h-1e-9&&w<=4*h+1e-9&&w<=2000+1e-9)all.push_back(sized(flow,w,h,false));}
        // smallest section first; the same area: the squarer one first
        for(size_t i=1;i<all.size();++i){Size x=all[i];size_t j=i;
            while(j>0&&(all[j-1].a*all[j-1].b>x.a*x.b+1e-9||(std::fabs(all[j-1].a*all[j-1].b-x.a*x.b)<=1e-9&&all[j-1].a-all[j-1].b>x.a-x.b))){all[j]=all[j-1];--j;}
            all[j]=x;}}
    size_t first=all.size();for(size_t i=0;i<all.size();++i)if(all[i].v<=vmax+1e-9){first=i;break;}
    if(first==all.size()){const size_t from=all.size()>static_cast<size_t>(before)?all.size()-before:0;for(size_t i=from;i<all.size();++i)out.push_back(all[i]);return;}
    const size_t from=first>static_cast<size_t>(before)?first-before:0,to=first+after<all.size()?first+after:all.size();
    for(size_t i=from;i<to;++i)out.push_back(all[i]);
}
// Manual size typed by the user: "300x150", "300 x 150", "300*150", "300\u00D7150" (rectangular),
// "d200", "D200", "\u00D8200", "\u00F8200" (round), or a single number: round when the duct is round.
// false: not a size (sizes 10..5000 mm).
inline bool parse(const std::wstring& text,bool roundDuct,Size& out){
    out=Size();std::wstring t;
    for(size_t i=0;i<text.size();++i){const wchar_t c=text[i];if(c!=L' '&&c!=L'\t')t+=c;}
    if(t.empty())return false;
    bool isRound=false;
    if(t[0]==L'd'||t[0]==L'D'||t[0]==L'\u00D8'||t[0]==L'\u00F8'){isRound=true;t.erase(0,1);}
    for(size_t i=0;i<t.size();++i)if(t[i]==L','){t[i]=L'.';}
    size_t sep=std::wstring::npos;
    for(size_t i=0;i<t.size();++i)if(t[i]==L'x'||t[i]==L'X'||t[i]==L'*'||t[i]==L'\u00D7'){sep=i;break;}
    auto number=[](const std::wstring& s,double& v){if(s.empty())return false;wchar_t* end=NULL;v=std::wcstod(s.c_str(),&end);return end&&*end==0&&v>=10&&v<=5000;};
    if(sep==std::wstring::npos){double d=0;if(!number(t,d))return false;
        if(isRound||roundDuct){out.ok=true;out.a=d;out.b=0;return true;}return false;}
    if(isRound)return false;
    double a=0,b=0;if(!number(t.substr(0,sep),a)||!number(t.substr(sep+1),b))return false;
    out.ok=true;out.a=a;out.b=b;return true;
}
// Dionice: group[i] = index of the dionica of element i (-1 unreached). A dionica starts at the
// root or at a child of a branching element (more than one child) and goes on while there is
// exactly one child. children counted from parent[]; reached[] from the flow network.
inline int dionice(const std::vector<int>& parent,const std::vector<char>& reached,std::vector<int>& group){
    const size_t n=parent.size();std::vector<int> count(n,0);
    for(size_t i=0;i<n;++i)if(reached[i]&&parent[i]>=0)++count[parent[i]];
    group.assign(n,-1);int next=0;
    // walk up from every element to the start of its dionica (memoised)
    for(size_t i=0;i<n;++i){if(!reached[i]||group[i]>=0)continue;
        std::vector<int> path;int k=static_cast<int>(i);
        while(k>=0&&group[k]<0){path.push_back(k);const int p=parent[k];if(p<0||count[p]!=1)break;k=p;}
        const int g=group[k]>=0?group[k]:next++;   // joined a dionica found before, or a new one
        for(size_t m=0;m<path.size();++m)group[path[m]]=g;
    }
    return next;
}
}
