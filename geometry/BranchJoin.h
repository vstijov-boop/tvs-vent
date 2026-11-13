#pragma once
// TVS_SPOJ_TKO: inlet duct, prolaz duct (on the other side, parallel) and a branch duct at
// right angles, joined by a TK-O (T komad sa ogrankom). Pure plan geometry, drawing units;
// tests/test_branch_join.cpp. TVS_WYE port model (teePorts, TK-O): Basepoint on the inlet axis
// where the branch axis crosses it; u (Vectpr) along the run in the flow direction, v (Vectotv)
// towards the branch; LengthPl = l + R1 - w (R1 = 1.5 w); inlet face at B - u (LengthPl + w/2),
// prolaz face at B + u (LengthPl + w/2) + v xo, branch face at B + v (A/2 + LengthPl).
//   align: 0 prolaz on the inlet axis (xo = 0; the prolaz duct is moved across when it is not),
//   1 prolaz edge in line with the inlet edge opposite the branch (xo = c - a), 2 on the branch
//   side (xo = a - c). The prolaz duct is moved across to xo when it is not there (only with a
//   free far end). Inlet shortened to its face, prolaz and branch near ends to theirs.
#include <cmath>
namespace tvsBranchJoin {
struct V {double x,y;V():x(0),y(0){}V(double a,double b):x(a),y(b){}};
inline V operator+(V a,V b){return V(a.x+b.x,a.y+b.y);}
inline V operator-(V a,V b){return V(a.x-b.x,a.y-b.y);}
inline V operator*(V a,double k){return V(a.x*k,a.y*k);}
inline double dot(V a,V b){return a.x*b.x+a.y*b.y;}
inline double len(V a){return std::sqrt(dot(a,a));}
inline V unit(V a){const double l=len(a);return l>1e-12?a*(1/l):V();}
struct Duct {V a,b;double width;bool farJoined,nearJoined;Duct():width(0),farJoined(false),nearJoined(false){}};
enum Error {Ok=0,Degenerate=1,NotParallel=2,NotPerpendicular=3,BranchSide=4,ProlazJoined=5,InletShort=6,ProlazShort=7,BranchShort=8,NearJoined=9,ProlazBehind=10};
struct Plan {
    Error error;
    V base,u,v;double lengthPl,xo;      // T komad (record "neck" = xo)
    V inletPort,prolazPort,branchPort;
    V shift;bool moved;                 // prolaz duct moved across
    int inletNear,prolazNear,branchNear;  // end (0 a / 1 b) at the T komad
    double offset;                      // prolaz axis across the run before (+ = branch side)
    int suggested;                      // alignment the drawing already has (0 / 1 / 2), -1 none
    Plan():error(Degenerate),lengthPl(0),xo(0),moved(false),inletNear(0),prolazNear(0),branchNear(0),offset(0),suggested(-1){}
};
inline Plan plan(const Duct& in,const Duct& pr,const Duct& br,int align,double l,double minDuct){
    Plan p;const double tol=1e-6;
    if(len(in.b-in.a)<1e-9||len(pr.b-pr.a)<1e-9||len(br.b-br.a)<1e-9||!(in.width>0)||!(pr.width>0)||!(br.width>0))return p;
    // the inlet end nearest the prolaz, the prolaz end nearest the inlet
    const V pm=(pr.a+pr.b)*0.5,im=(in.a+in.b)*0.5;
    p.inletNear=len(in.a-pm)<=len(in.b-pm)?0:1;p.prolazNear=len(pr.a-im)<=len(pr.b-im)?0:1;
    const V iN=p.inletNear==0?in.a:in.b,iF=p.inletNear==0?in.b:in.a,pN=p.prolazNear==0?pr.a:pr.b,pF=p.prolazNear==0?pr.b:pr.a;
    p.u=unit(iN-iF);   // flow: inlet -> prolaz
    if(std::fabs(dot(unit(pF-pN),p.u))<0.9998||dot(unit(pF-pN),p.u)<0){p.error=NotParallel;return p;}
    if(dot(pN-iN,p.u)<=0){p.error=ProlazBehind;return p;}
    // branch: end nearest the run axis, direction away from it
    const V n(-p.u.y,p.u.x);
    const double da=std::fabs(dot(br.a-iN,n)),db=std::fabs(dot(br.b-iN,n));
    p.branchNear=da<=db?0:1;const V bN=p.branchNear==0?br.a:br.b,bF=p.branchNear==0?br.b:br.a;
    const V bd=unit(bF-bN);
    if(std::fabs(dot(bd,p.u))>0.02){p.error=NotPerpendicular;return p;}
    p.v=dot(bd,n)>0?n:n*-1;
    if(dot(bN-iN,p.v)<0){p.error=BranchSide;return p;}   // the branch starts on the other side of the run
    if(in.nearJoined||pr.nearJoined||br.nearJoined){p.error=NearJoined;return p;}
    const double a=in.width/2,c=pr.width/2,w=br.width;
    p.offset=dot(pN-iN,p.v);
    if(std::fabs(p.offset)<=tol)p.suggested=0;else if(std::fabs(p.offset-(c-a))<=tol)p.suggested=1;else if(std::fabs(p.offset-(a-c))<=tol)p.suggested=2;
    p.xo=align==1?c-a:align==2?a-c:0;
    const double move=p.xo-p.offset;
    if(std::fabs(move)>tol){if(pr.farJoined){p.error=ProlazJoined;return p;}p.moved=true;p.shift=p.v*move;}
    const double tb=dot(bN-iN,p.u);   // branch axis along the run
    p.base=iN+p.u*tb;p.lengthPl=l+0.5*w;
    const double half=p.lengthPl+w/2;
    p.inletPort=p.base-p.u*half;p.prolazPort=p.base+p.u*half+p.v*p.xo;p.branchPort=p.base+p.v*(a+p.lengthPl);
    if(dot(p.inletPort-iF,p.u)<minDuct){p.error=InletShort;return p;}
    if(dot((pF+(p.moved?p.shift:V()))-p.prolazPort,p.u)<minDuct){p.error=ProlazShort;return p;}
    if(dot(bF-p.branchPort,p.v)<minDuct){p.error=BranchShort;return p;}
    p.error=Ok;return p;
}
}
