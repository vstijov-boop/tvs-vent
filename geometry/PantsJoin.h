#pragma once
// TVS_SPOJ_TKG: inlet duct + two branch ducts (opposite, at right angles to it) joined by a
// TK-G (gacasti T komad). Pure plan geometry, drawing units; tests/test_pants_join.cpp.
//   The branch axes must be one line: one branch is moved onto the other's axis (the one whose
//   far end is free; both free: the narrower one). The T komad sits where the inlet axis meets
//   that line (Basepoint), its inlet port at l + max(w1, w2) before it (TK-G with default radii,
//   TVS_WYE::teePorts); the inlet duct is shortened to it. Branch faces at A/2 + w/2 + l from the
//   axis crossing. u = direction of branch 1 (w1, TVS_WYE +Vectpr), v = towards the inlet.
//   neck ("Vrat"): no branch moves; the T komad is put on the axis nearer the inlet and the
//   farther branch gets that much more straight inlet wall (TeeShape.h e1 / e2, record "neck").
#include "TeeShape.h"
#include <cmath>
namespace tvsPantsJoin {
struct V {double x,y;V():x(0),y(0){}V(double a,double b):x(a),y(b){}};
inline V operator+(V a,V b){return V(a.x+b.x,a.y+b.y);}
inline V operator-(V a,V b){return V(a.x-b.x,a.y-b.y);}
inline V operator*(V a,double k){return V(a.x*k,a.y*k);}
inline double dot(V a,V b){return a.x*b.x+a.y*b.y;}
inline double len(V a){return std::sqrt(dot(a,a));}
inline V unit(V a){const double l=len(a);return l>1e-12?a*(1/l):V();}
struct Duct {V a,b;double width;bool farJoined;bool nearJoined;   // a, b: the two ends (order free)
    Duct():width(0),farJoined(false),nearJoined(false){}};
enum Error {Ok=0,Degenerate=1,NotPerpendicular=2,NotOpposite=3,BothJoined=4,InletShort=5,BranchShort=6,NearJoined=7,NeckShape=8};
struct Plan {
    Error error;
    int moved;            // branch moved onto the other's axis: 0 / 1, -1 none
    V shift;              // its displacement
    V base,u,v;           // T komad
    double w1,w2;         // w1: branch on +u
    int branchOnU;        // which input branch is on +u (0 / 1)
    V inletPort,port[2];  // port[k]: face of input branch k
    int inletNear;        // 0: inlet end a is at the T komad, 1: end b
    int branchNear[2];
    double e[2];          // vrat of input branch k (0 unless neck)
    double neck;          // record value: e[1] - e[0] (branch 1 = w2 side)
    Plan():error(Degenerate),moved(-1),w1(0),w2(0),branchOnU(0),inletNear(0),neck(0){branchNear[0]=branchNear[1]=0;e[0]=e[1]=0;}
};
// l: collar (tvsTeeData::collarMm in drawing units); minDuct: shortest duct left; preferMoved: -1
// automatic, 0 / 1 move that branch (the "Zamijeni" option).
inline Plan plan(const Duct& in,const Duct br[2],double l,double minDuct,int preferMoved=-1,bool neck=false){
    Plan p;
    if(len(in.b-in.a)<1e-9||len(br[0].b-br[0].a)<1e-9||len(br[1].b-br[1].a)<1e-9||!(in.width>0)||!(br[0].width>0)||!(br[1].width>0))return p;
    // the branch ends nearest the inlet axis, the inlet end nearest the branches
    const V mid=(br[0].a+br[0].b+br[1].a+br[1].b)*0.25;
    p.inletNear=len(in.a-mid)<=len(in.b-mid)?0:1;
    const V inNear=p.inletNear==0?in.a:in.b,farIn=p.inletNear==0?in.b:in.a,d=unit(inNear-farIn);   // towards the branches
    const V n(-d.y,d.x);   // across the inlet
    V o[2],bn[2];
    for(int k=0;k<2;++k){
        const double da=std::fabs(dot(br[k].a-inNear,n)),db=std::fabs(dot(br[k].b-inNear,n));
        p.branchNear[k]=da<=db?0:1;bn[k]=p.branchNear[k]==0?br[k].a:br[k].b;
        const V f=p.branchNear[k]==0?br[k].b:br[k].a;o[k]=unit(f-bn[k]);
        if(std::fabs(dot(o[k],d))>0.02){p.error=NotPerpendicular;return p;}
    }
    if(dot(o[0],o[1])>-0.999){p.error=NotOpposite;return p;}
    if(br[0].nearJoined||br[1].nearJoined||in.nearJoined){p.error=NearJoined;return p;}
    // which branch moves onto the other's axis
    int m=preferMoved;
    if(m<0){if(br[0].farJoined&&br[1].farJoined)m=-2;else if(br[0].farJoined)m=1;else if(br[1].farJoined)m=0;else m=br[0].width<br[1].width?0:1;}
    const double offset=dot(bn[1]-bn[0],d);   // branch 1 axis minus branch 0 axis, along d
    if(neck){if(offset>0)p.e[1]=offset;else p.e[0]=-offset;p.neck=p.e[1]-p.e[0];}
    else if(std::fabs(offset)>1e-9){
        if(m==-2){p.error=BothJoined;return p;}
        if(m>=0&&br[m].farJoined){p.error=BothJoined;return p;}
        p.moved=m;p.shift=d*(m==0?offset:-offset);
    }
    const int keep=neck?(offset>0?0:1):p.moved==0?1:0;   // neck: the axis nearer the inlet
    const double t=dot(bn[keep]-inNear,d);
    p.base=inNear+d*t;p.v=d*-1;
    p.branchOnU=0;p.u=o[0];p.w1=br[0].width;p.w2=br[1].width;
    const double a=in.width/2,Y=l+(p.w1>p.w2?p.w1:p.w2);
    p.inletPort=p.base+p.v*Y;
    p.port[0]=p.base+p.u*(a+p.w1/2+l)+d*p.e[0];p.port[1]=p.base-p.u*(a+p.w2/2+l)+d*p.e[1];
    if(neck){tvsTee::Tee t;t.type=tvsTee::Pants;t.A=in.width;t.w1=p.w1;t.w2=p.w2;t.R1=tvsTee::defaultRadius(p.w1);t.R2=tvsTee::defaultRadius(p.w2);
        t.l=l;t.e1=p.e[0];t.e2=p.e[1];tvsTee::Shape s;if(!tvsTee::shape(t,s)){p.error=NeckShape;return p;}}
    if(dot(farIn-p.inletPort,d*-1)<minDuct){p.error=InletShort;return p;}
    for(int k=0;k<2;++k){const V f=(p.branchNear[k]==0?br[k].b:br[k].a)+(p.moved==k?p.shift:V());
        if(dot(f-p.port[k],o[k])<minDuct){p.error=BranchShort;return p;}}
    p.error=Ok;return p;
}
}
