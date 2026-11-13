#pragma once
// T komadi iz dinamickog bloka "T komad" (orlovij -> TK-O, shtanopodibnij -> TK-G), osnova.
// Local frame (drawing units): origin = centre of the inlet face, +y into the fitting
// (flow direction of the inlet), +x = side of the (first) branch.
//  TK-O: straight run inlet A (y=0) -> outlet C (y=L), branch w1 on +x with radius R1.
//  TK-G: inlet A (y=0), branches w1 on +x (R1) and w2 on -x (R2), no straight run.
// Branch: centre (a+Ri, l) with Ri = R - w; inner arc Ri from (a+Ri, l+Ri) to (a, l);
// outer arc R from (a+Ri, l+R) down to where it meets the run wall (TK-O, the wall below it is
// open) or the axis (TK-G); straight collar l at the branch face. Centre-line radius = w.
// Every face carries a flange tick extended by `flange` beyond the duct edges.
// With collars equal to l, the TVS_WYE port model holds: LengthPl = l + R - w.
#include <vector>
#include <cmath>
namespace tvsTee {
enum Type {Straight=0,Branch=1,Pants=2,Tap=3};   // 0 = existing TVS_WYE drawing
// Tap = stucna, ASHRAE SR5-3: rectangular branch on a straight main with a 45 deg entry on the
// upstream side; collar L = 0.25 w (branch width), at least 75 mm. TVS_WYE LengthPl = L.
struct P {double x,y;P(double a=0,double b=0):x(a),y(b){}};
struct Seg {P a,b;Seg(P p=P(),P q=P()):a(p),b(q){}};
struct Arc {P c;double r,a0,a1;Arc(P p=P(),double rr=0,double s=0,double e=0):c(p),r(rr),a0(s),a1(e){}}; // CCW a0->a1
struct Shape {std::vector<Seg> lines;std::vector<Arc> arcs;std::vector<Seg> axis;};
struct Tee {int type;double A,C,w1,w2,R1,R2,l,flange,e1,e2,xo;   // e1 / e2: TK-G "vrat", extra straight inlet wall
    // xo: TK-O prolaz axis across the run (+x = branch side; prolaz edge in line with the inlet: +-(a - c))
    Tee():type(Branch),A(0),C(0),w1(0),w2(0),R1(0),R2(0),l(0),flange(0),e1(0),e2(0),xo(0){}};   // before branch 1 / 2 (its axis that much farther)
inline double pi(){return 3.14159265358979323846;}
inline double defaultRadius(double w){return 1.5*w;}           // outer radius: centre line R - w/2 = w
inline double lengthPl(double l,double R,double w){return l+R-w;} // TVS_WYE equivalent
inline double runLength(const Tee& t){return t.type==Tap?2*t.l+t.w1:2*lengthPl(t.l,t.R1,t.w1)+t.w1;} // inlet->outlet
inline double tapLength(double w,double minimum){return 0.25*w>minimum?0.25*w:minimum;} // SR5-3: L = 0.25 w, >= 75 mm
inline bool valid(const Tee& t){
    if(t.type==Tap)return t.A>0&&t.w1>0&&t.l>0;
    if(!(t.A>0)||!(t.w1>0)||!(t.l>0)||!(t.R1>t.w1))return false;
    if(t.type==Branch&&!(t.C>0))return false;
    if(t.type==Pants&&(!(t.w2>0)||!(t.R2>t.w2)))return false;
    return t.type==Branch||t.type==Pants;
}
// One branch on side s (+1 = +x, -1 = -x): inlet wall extension d (TK-G, narrower branch),
// inner arc Ri = R - w centred at (s(a+Ri), l+d), outer arc R from the branch outer wall to
// endAngle (s>0: in [pi/2, pi], s<0: in [0, pi/2]), collar up to the face x = s*X.
inline void branchAt(Shape& out,double a,double l,double d,double w,double R,double X,double flange,int s,double endAngle){
    const double Ri=R-w,cx=s*(a+Ri),cy=l+d;
    if(d>1e-12)out.lines.push_back(Seg(P(s*a,l),P(s*a,cy)));
    if(s>0){out.arcs.push_back(Arc(P(cx,cy),Ri,pi()/2,pi()));out.arcs.push_back(Arc(P(cx,cy),R,pi()/2,endAngle));}
    else   {out.arcs.push_back(Arc(P(cx,cy),Ri,0,pi()/2));out.arcs.push_back(Arc(P(cx,cy),R,endAngle,pi()/2));}
    const double x0=s*(a+Ri),x1=s*X,y0=cy+Ri,y1=cy+R;
    out.lines.push_back(Seg(P(x0,y0),P(x1,y0)));out.lines.push_back(Seg(P(x0,y1),P(x1,y1)));
    out.lines.push_back(Seg(P(x0,y0),P(x0,y1)));
    out.lines.push_back(Seg(P(x1,y0-flange),P(x1,y1+flange)));
    out.axis.push_back(Seg(P(0,y0+w/2),P(x1,y0+w/2)));
}
// TK-G: both branch centre lines on one axis Y = l + max(Rc1, Rc2) (Rc = R - w/2); the
// branch with the smaller Rc starts after a straight inlet wall d. Collars are fixed (l) on
// both branches, so each face is at x = +-(a + Ri + l); X = a + Y - w1/2 is the TVS run end
// (a narrower branch ends short of it; the neighbour duct is adjusted later, faza 3).
struct PantsFrame {double Y,X,d1,d2;};
inline PantsFrame pantsFrame(const Tee& t){
    PantsFrame p;const double a=t.A/2,rc1=t.R1-t.w1/2,rc2=t.R2-t.w2/2;
    p.Y=t.l+(rc1>rc2?rc1:rc2);p.d1=p.Y-t.l-rc1+t.e1;p.d2=p.Y-t.l-rc2+t.e2;p.X=a+p.Y-t.w1/2;return p;   // Y: axis line without vrat
}
inline double pantsLengthPl(double l,double w1,double w2){return l+(w1>w2?w1:w2)-w1/2;} // defaults Rc = w
inline bool shape(const Tee& t,Shape& out);
// TK-G with default radii (Rc = w) is drawable: the outer arcs meet (about w1 + w2 >= A).
inline bool pantsFeasible(double A,double w1,double w2,double l){
    Tee t;t.type=Pants;t.A=A;t.w1=w1;t.w2=w2;t.R1=defaultRadius(w1);t.R2=defaultRadius(w2);t.l=l;
    Shape s;return shape(t,s);
}
inline bool shape(const Tee& t,Shape& out){
    out=Shape();if(!valid(t))return false;
    const double a=t.A/2,l=t.l,f=t.flange;
    if(t.type==Tap){
        // The main looks continuous (SR5-3 sits on an unbroken duct): both walls y = 0..2L+w,
        // no end lines, nothing inside the main. On it the stucna only: 45 deg entry
        // (a,0) -> (a+L,L) upstream, downstream wall at y = L + w, face x = a + L.
        const double w=t.w1,Lr=2*l+w;
        out.lines.push_back(Seg(P(-a,0),P(-a,Lr)));out.lines.push_back(Seg(P(a,0),P(a,Lr)));
        out.lines.push_back(Seg(P(a,0),P(a+l,l)));out.lines.push_back(Seg(P(a,l+w),P(a+l,l+w)));
        out.lines.push_back(Seg(P(a+l,l-f),P(a+l,l+w+f)));
        return true;
    }
    // inlet collar and flange
    out.lines.push_back(Seg(P(-a,0),P(-a,l)));out.lines.push_back(Seg(P(a,0),P(a,l)));
    out.lines.push_back(Seg(P(-a,l),P(a,l)));out.lines.push_back(Seg(P(-a-f,0),P(a+f,0)));
    if(t.type==Branch){
        const double L=runLength(t),c=t.C/2,o=t.xo,cl=o-c,cr=o+c;   // prolaz walls x = cl / cr
        // Outer branch arc meets the branch-side run wall (a,l)->(cr,L-l) at t; that wall is
        // open from the inner arc (a,l) up to there.
        const double Ri=t.R1-t.w1,dx=cr-a,dy=L-2*l,qa=dx*dx+dy*dy,qb=-2*Ri*dx,qc=Ri*Ri-t.R1*t.R1;
        double u=(-qb+std::sqrt(qb*qb-4*qa*qc))/(2*qa);if(u>1)u=1;
        const P q(a+u*dx,l+u*dy);double end=std::atan2(q.y-l,q.x-(a+Ri));if(end<pi()/2)end=pi()/2;
        out.lines.push_back(Seg(P(-a,l),P(cl,L-l)));out.lines.push_back(Seg(q,P(cr,L-l)));   // run walls (taper A->C; xo: one may be straight)
        out.lines.push_back(Seg(P(cl,L-l),P(cl,L)));out.lines.push_back(Seg(P(cr,L-l),P(cr,L)));
        out.lines.push_back(Seg(P(cl,L-l),P(cr,L-l)));out.lines.push_back(Seg(P(cl-f,L),P(cr+f,L)));
        out.axis.push_back(Seg(P(0,0),P(o,L)));
        branchAt(out,a,l,0,t.w1,t.R1,a+(t.R1-t.w1)+l,f,+1,end);
    }else{
        const PantsFrame pf=pantsFrame(t);
        const double ri1=t.R1-t.w1,ri2=t.R2-t.w2;
        // Cusp = upper intersection of the two outer circles; it must lie on both quarter arcs,
        // otherwise the branches are too narrow for the inlet (not used in practice).
        const P c1(a+ri1,l+pf.d1),c2(-(a+ri2),l+pf.d2);
        const double ddx=c2.x-c1.x,ddy=c2.y-c1.y,D=std::sqrt(ddx*ddx+ddy*ddy);
        double e1=pi(),e2=0;bool cusp=false;
        if(D>1e-12&&D<=t.R1+t.R2&&D>=std::fabs(t.R1-t.R2)){
            const double m=(t.R1*t.R1-t.R2*t.R2+D*D)/(2*D),h=std::sqrt((std::max)(0.0,t.R1*t.R1-m*m));
            const double mx=c1.x+m*ddx/D,my=c1.y+m*ddy/D;
            P k1(mx-h*ddy/D,my+h*ddx/D),k2(mx+h*ddy/D,my-h*ddx/D);const P k=k1.y>k2.y?k1:k2;
            const double a1=std::atan2(k.y-c1.y,k.x-c1.x),a2=std::atan2(k.y-c2.y,k.x-c2.x);
            if(a1>=pi()/2-1e-9&&a1<=pi()+1e-9&&a2>=-1e-9&&a2<=pi()/2+1e-9){e1=a1;e2=a2;cusp=true;}
        }
        // Long vrat (TVS_SPOJ_TKG): the outer arcs no longer meet. The farther branch keeps its
        // whole outer quarter, ending at S = (s(a - w), its centre height); the nearer outer arc
        // stops on the line x = S.x, a straight wall goes from there to S, and a line across at
        // S's height closes to the inlet wall on the farther side (as drawn by hand, 13.11.).
        std::vector<Seg> flat;
        if(!cusp&&(t.e1>1e-9||t.e2>1e-9)){
            const bool far1=t.e1>t.e2;const int sf=far1?1:-1;
            const P cf=far1?c1:c2,cn=far1?c2:c1;const double wf=far1?t.w1:t.w2,Rn=far1?t.R2:t.R1;
            const double xs=sf*(a-wf),q=(xs-cn.x)/Rn;
            if(q>=-1&&q<=1){const double th=std::acos(q),yc=cn.y+Rn*std::sin(th);   // th in [0, pi]: upper half
                const bool inRange=far1?(th<=pi()/2+1e-9):(th>=pi()/2-1e-9);         // nearer arc: s=-1 [0,pi/2], s=+1 [pi/2,pi]
                if(inRange&&yc<=cf.y+1e-9){
                    if(far1){e1=pi();e2=th;}else{e2=0;e1=th;}
                    flat.push_back(Seg(P(xs,yc),P(xs,cf.y)));flat.push_back(Seg(P(xs,cf.y),P(sf*a,cf.y)));cusp=true;}}
        }
        if(!cusp)return false;   // branches too narrow for the inlet: not a real TK-G
        out.axis.push_back(Seg(P(0,0),P(0,pf.Y+(t.e1>t.e2?t.e1:t.e2))));
        branchAt(out,a,l,pf.d1,t.w1,t.R1,a+ri1+l,f,+1,e1);
        branchAt(out,a,l,pf.d2,t.w2,t.R2,a+ri2+l,f,-1,e2);
        for(size_t k=0;k<flat.size();++k)out.lines.push_back(flat[k]);
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
    if(t.type==Tap){   // main minus opening (w + L) + two trapezoids, downstream wall, 45 deg wall
        const double L=runLength(t),w=t.w1,l=t.l;
        s=perimeter(t.A,b)*L-(w+l)*bb+l*(2*w+l)+bb*l+bb*l*std::sqrt(2.0);}
    else if(t.type==Branch){const double L=runLength(t);
        s=(perimeter(t.A,b)+perimeter(t.C,b))/2*L-t.w1*bb+arm(t.w1,t.R1,bb,t.l);}
    else{const PantsFrame p=pantsFrame(t);
        s=perimeter(t.A,b)*t.l+arm(t.w1,t.R1,bb,t.l)+arm(t.w2,t.R2,bb,t.l)
         +perimeter(t.w1,bb)*p.d1+perimeter(t.w2,bb)*p.d2;}
    return s/1e6;
}
}
