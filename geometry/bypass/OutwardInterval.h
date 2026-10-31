#ifndef TVS_BYPASS_OUTWARD_INTERVAL_H
#define TVS_BYPASS_OUTWARD_INTERVAL_H
#include <algorithm>
#include <cmath>
#include <cfenv>
#include <cfloat>
#include <limits>
#include <stdexcept>
#if defined(__FAST_MATH__) || FLT_EVAL_METHOD != 0
#error Bypass bounds require strict IEEE binary64 evaluation without fast-math.
#endif
namespace tvsBypass { namespace bounded {
struct I {
 double lo,hi;
 explicit I(double x=0):lo(x),hi(x){}
 I(double a,double b):lo(a),hi(b){if(!(a<=b))throw std::runtime_error("invalid interval");}
 bool point()const{return lo==hi;}
};
inline bool supported(){return std::numeric_limits<double>::is_iec559 &&
 sizeof(double)==8 && std::fegetround()==FE_TONEAREST;}
inline double down(double x){return std::nextafter(x,-std::numeric_limits<double>::infinity());}
inline double up(double x){return std::nextafter(x,std::numeric_limits<double>::infinity());}
inline I rounded(double p,double residual,bool exactSafe=true){
 if(!std::isfinite(p)||!std::isfinite(residual))throw std::runtime_error("arithmetic overflow");
 if(!exactSafe)return I(down(p),up(p));
 return residual==0?I(p):residual>0?I(p,up(p)):I(down(p),p);
}
inline I sum(double a,double b){
 double p=a+b,bb=p-a;
 return rounded(p,(a-(p-bb))+(b-bb));
}
inline I product(double a,double b){
 if(a==0||b==0)return I(0);
 double p=a*b,e=std::fma(a,b,-p);
 return rounded(p,e,std::fabs(p)>=1e-200&&std::fabs(p)<=1e200);
}
inline I quotient(double a,double b){
 if(b==0)throw std::runtime_error("zero divisor");
 if(a==0)return I(0);
 double p=a/b,e=std::fma(p,b,-a);
 bool safe=std::fabs(a)>=1e-100&&std::fabs(a)<=1e100 &&
 std::fabs(b)>=1e-100&&std::fabs(b)<=1e100 &&
 std::fabs(p)>=1e-100&&std::fabs(p)<=1e100;
 return rounded(p,b>0?-e:e,safe);
}
inline I operator+(I a,I b){return I(sum(a.lo,b.lo).lo,sum(a.hi,b.hi).hi);}
inline I operator-(I a){return I(-a.hi,-a.lo);}
inline I operator-(I a,I b){return a+(-b);}
inline I operator*(I a,I b){
 I v[]={product(a.lo,b.lo),product(a.lo,b.hi),product(a.hi,b.lo),product(a.hi,b.hi)};
 double l=v[0].lo,h=v[0].hi;for(int j=1;j<4;++j){l=std::min(l,v[j].lo);h=std::max(h,v[j].hi);}return I(l,h);
}
inline I operator/(I a,I b){
 if(b.lo<=0&&b.hi>=0)throw std::runtime_error("interval divisor contains zero");
 I v[]={quotient(a.lo,b.lo),quotient(a.lo,b.hi),quotient(a.hi,b.lo),quotient(a.hi,b.hi)};
 double l=v[0].lo,h=v[0].hi;for(int j=1;j<4;++j){l=std::min(l,v[j].lo);h=std::max(h,v[j].hi);}return I(l,h);
}
inline I hull(I a,I b){return I(std::min(a.lo,b.lo),std::max(a.hi,b.hi));}
inline I absolute(I a){return a.lo>=0?a:a.hi<=0?-a:I(0,std::max(-a.lo,a.hi));}
inline I square(I a){
 I p=product(a.lo,a.lo),q=product(a.hi,a.hi);
 return I(a.lo<=0&&a.hi>=0?0:std::min(p.lo,q.lo),std::max(p.hi,q.hi));
}
inline I root(I a){
 if(a.lo<0)throw std::runtime_error("negative square root");
 double l=std::sqrt(a.lo),h=std::sqrt(a.hi);
 I lb=a.lo==0?I(0):rounded(l,-std::fma(l,l,-a.lo),a.lo>=1e-200&&a.lo<=1e200);
 I hb=a.hi==0?I(0):rounded(h,-std::fma(h,h,-a.hi),a.hi>=1e-200&&a.hi<=1e200);
 return I(std::max(0.0,lb.lo),hb.hi);
}
// Alternating Taylor series; all arithmetic is outward enclosed. No libm sin/cos.
inline I sinePoint(double x){
 if(x<0||x>1.1)throw std::runtime_error("unsupported trig domain");
 if(x==0)return I(0);
 I a(x),xx=a*a,term=a,total=term;
 for(int j=1;j<=14;++j){term=-(term*xx/I((2*j)*(2*j+1)));total=total+term;}
 I next=absolute(term*xx/I(30*31));return total+I(-next.hi,next.hi);
}
inline I cosinePoint(double x){
 if(x<0||x>1.1)throw std::runtime_error("unsupported trig domain");
 if(x==0)return I(1);
 I xx=I(x)*I(x),term(1),total(1);
 for(int j=1;j<=14;++j){term=-(term*xx/I((2*j-1)*(2*j)));total=total+term;}
 I next=absolute(term*xx/I(29*30));return total+I(-next.hi,next.hi);
}
inline I sine(I x){return I(sinePoint(x.lo).lo,sinePoint(x.hi).hi);}
inline I cosine(I x){return I(cosinePoint(x.hi).lo,cosinePoint(x.lo).hi);}
struct V {I s,z;V(I a=I(),I b=I()):s(a),z(b){}};
inline V operator+(V a,V b){return V(a.s+b.s,a.z+b.z);}
inline V operator-(V a,V b){return V(a.s-b.s,a.z-b.z);}
inline V operator*(V a,I k){return V(a.s*k,a.z*k);}
inline I dot(V a,V b){return a.s*b.s+a.z*b.z;}
inline I distance(V a,V b){return root(square(a.s-b.s)+square(a.z-b.z));}
struct Box {I s,z;Box(I a=I(),I b=I()):s(a),z(b){}};
inline Box boxHull(Box a,Box b){return Box(hull(a.s,b.s),hull(a.z,b.z));}
inline Box pointBox(V a){return Box(a.s,a.z);}
inline I boxDistance(Box a,Box b){
 I ds(0),dz(0);
 if(a.s.hi<b.s.lo)ds=I(b.s.lo)-I(a.s.hi);else if(b.s.hi<a.s.lo)ds=I(a.s.lo)-I(b.s.hi);
 if(a.z.hi<b.z.lo)dz=I(b.z.lo)-I(a.z.hi);else if(b.z.hi<a.z.lo)dz=I(a.z.lo)-I(b.z.hi);
 return root(square(ds)+square(dz));
}
} }
#endif
