#pragma once
// TVS_PROTOK, brzine: velocity in a duct and the size proposal for a maximum velocity.
// Sizes in mm, flow in m3/h, velocity in m/s. tests/test_duct_sizing.cpp.
//   Rectangular, width only: the height stays, the smallest standard width that is fast enough.
//   Rectangular, width and height: the smallest standard section with width >= height,
//   width / height <= 4 and height at least the current one (the ceiling decides the rest).
//   Round: the smallest standard diameter (spiro series, SOKO catalogue sizes included).
//   Dionica: a chain of elements between branchings (same flow), from the parent array of
//   the flow network.
#include <cmath>
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
struct Size {bool ok;double a,b;double v;Size():ok(false),a(0),b(0),v(0){}};
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
