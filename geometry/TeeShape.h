#pragma once
// T komadi iz dinamickog bloka "T komad" (orlovij -> TK-O, shtanopodibnij -> TK-G), osnova.
// Local frame (drawing units): origin = centre of the inlet face, +y into the fitting
// (flow direction of the inlet), +x = side of the (first) branch.
//  TK-O: straight run inlet A (y=0) -> outlet C (y=L), branch w1 on +x with radius R1.
//  TK-G: inlet A (y=0), branches w1 on +x (R1) and w2 on -x (R2), no straight run.
// Branch: centre (a+Ri, l) with Ri = R - w; inner arc Ri from (a+Ri, l+Ri) to (a, l);
// outer arc R from (a+Ri, l+R) towards the far side; straight collar l at the branch face.
// Every face carries a flange tick extended by `flange` beyond the duct edges.
// With collars equal to l, the TVS_WYE port model holds: LengthPl = l + R - w.
#include <vector>
#include <cmath>
namespace tvsTee {
enum Type {Straight=0,Branch=1,Pants=2};   // 0 = existing TVS_WYE drawing
struct P {double x,y;P(double a=0,double b=0):x(a),y(b){}};
struct Seg {P a,b;Seg(P p=P(),P q=P()):a(p),b(q){}};
struct Arc {P c;double r,a0,a1;Arc(P p=P(),double rr=0,double s=0,double e=0):c(p),r(rr),a0(s),a1(e){}}; // CCW a0->a1
struct Shape {std::vector<Seg> lines;std::vector<Arc> arcs;std::vector<Seg> axis;};
struct Tee {int type;double A,C,w1,w2,R1,R2,l,flange;
    Tee():type(Branch),A(0),C(0),w1(0),w2(0),R1(0),R2(0),l(0),flange(0){}};
inline double pi(){return 3.14159265358979323846;}
inline double defaultRadius(double w){return 1.75*w;}          // outer radius, as in the block
inline double lengthPl(double l,double R,double w){return l+R-w;} // TVS_WYE equivalent
inline double runLength(const Tee& t){return 2*lengthPl(t.l,t.R1,t.w1)+t.w1;} // TK-O inlet->outlet
inline bool valid(const Tee& t){
    if(!(t.A>0)||!(t.w1>0)||!(t.l>0)||!(t.R1>t.w1))return false;
    if(t.type==Branch&&!(t.C>0))return false;
    if(t.type==Pants&&(!(t.w2>0)||!(t.R2>t.w2)))return false;
    return t.type==Branch||t.type==Pants;
}
// One branch on side s (+1 = +x, -1 = -x). `farX` = where the outer arc stops (x of the far
// wall for TK-O, the axis x=0 for TK-G); it stops at 180 deg if it cannot reach it.
inline void branch(Shape& out,double a,double l,double w,double R,double flange,int s,double farX){
    const double Ri=R-w,cx=s*(a+Ri),cy=l,k=(farX-cx)/R;
    const double clamp=k<-1?-1:k>1?1:k,phi=std::acos(clamp); // angle where the outer arc meets x = farX
    if(s>0){out.arcs.push_back(Arc(P(cx,cy),Ri,pi()/2,pi()));               // inner: (cx,l+Ri) -> (a,l)
            out.arcs.push_back(Arc(P(cx,cy),R,pi()/2,phi<pi()/2?pi()/2:phi));} // outer: (cx,l+R) -> far side
    else   {out.arcs.push_back(Arc(P(cx,cy),Ri,0,pi()/2));
            out.arcs.push_back(Arc(P(cx,cy),R,phi>pi()/2?pi()/2:phi,pi()/2));}
    // collar x from a+Ri to a+Ri+l, y from l+Ri to l+R; flange tick at the face
    const double x0=s*(a+Ri),x1=s*(a+Ri+l),y0=l+Ri,y1=l+R;
    out.lines.push_back(Seg(P(x0,y0),P(x1,y0)));out.lines.push_back(Seg(P(x0,y1),P(x1,y1)));
    out.lines.push_back(Seg(P(x0,y0),P(x0,y1)));
    out.lines.push_back(Seg(P(x1,y0-flange),P(x1,y1+flange)));
    out.axis.push_back(Seg(P(0,y0+w/2),P(x1,y0+w/2)));
}
inline bool shape(const Tee& t,Shape& out){
    out=Shape();if(!valid(t))return false;
    const double a=t.A/2,l=t.l,f=t.flange;
    // inlet collar and flange
    out.lines.push_back(Seg(P(-a,0),P(-a,l)));out.lines.push_back(Seg(P(a,0),P(a,l)));
    out.lines.push_back(Seg(P(-a,l),P(a,l)));out.lines.push_back(Seg(P(-a-f,0),P(a+f,0)));
    if(t.type==Branch){
        const double L=runLength(t),c=t.C/2;
        out.lines.push_back(Seg(P(-a,l),P(-c,L-l)));out.lines.push_back(Seg(P(a,l),P(c,L-l)));   // run walls (taper A->C)
        out.lines.push_back(Seg(P(-c,L-l),P(-c,L)));out.lines.push_back(Seg(P(c,L-l),P(c,L)));
        out.lines.push_back(Seg(P(-c,L-l),P(c,L-l)));out.lines.push_back(Seg(P(-c-f,L),P(c+f,L)));
        out.axis.push_back(Seg(P(0,0),P(0,L)));
        branch(out,a,l,t.w1,t.R1,f,+1,-a);
    }else{
        out.axis.push_back(Seg(P(0,0),P(0,l+(std::max)(t.R1-t.w1+t.w1/2,t.R2-t.w2+t.w2/2))));
        branch(out,a,l,t.w1,t.R1,f,+1,0);
        branch(out,a,l,t.w2,t.R2,f,-1,0);
    }
    return true;
}
// Sheet area, m2 (all sizes mm). b = duct height of run/inlet, bb = height of the branches.
// Branch = quarter of an elbow (Rc = R - w/2) + straight collar; run of TK-O by mean
// perimeter A->C over L minus the branch opening; inlet collar of TK-G.
inline double perimeter(double a,double b){return 2*(a+b);}
inline double arm(double w,double R,double bb,double l){return perimeter(w,bb)*(R-w/2)*pi()/2+perimeter(w,bb)*l;}
inline double area(const Tee& t,double b,double bb){
    if(!valid(t))return 0;
    double s=0;
    if(t.type==Branch){const double L=runLength(t);
        s=(perimeter(t.A,b)+perimeter(t.C,b))/2*L-t.w1*bb+arm(t.w1,t.R1,bb,t.l);}
    else s=perimeter(t.A,b)*t.l+arm(t.w1,t.R1,bb,t.l)+arm(t.w2,t.R2,bb,t.l);
    return s/1e6;
}
}
