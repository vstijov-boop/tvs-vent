#pragma once
// TVS_SPOJ_TKG for round ducts (user 2026-11-27): three already drawn round ducts joined by
//  - racva Y45 (geometry/TeeShape.h Pants + lateral, Soko "Spiro Y45"): inlet + two branches at
//    +-45 deg to it on opposite sides. Each branch axis must cross the inlet axis where the Y45
//    puts it (O_k = inlet face + yO_k); one branch is kept, the other is moved along the inlet
//    direction onto its place (the one with a free far end; both free: the narrower one; both
//    joined and off: refused). The inlet duct is shortened / extended to the inlet face, the
//    branch ducts end on the branch faces;
//  - kosa racva TK (Tap + lateral): two collinear ducts of one size (the main) + a branch at 45 deg;
//    the T komad sits where the branch axis crosses the main axis, the main ducts end on its run
//    ports, the branch duct on its face.
// Pure plan geometry, drawing units; tests/test_wye_join.cpp. Ends: the end of each duct nearer
// the joint ("near") must be free; far ends stay (a joined far end is never moved).
#include "PantsJoin.h"
#include "TeeShape.h"
#include <cmath>
namespace tvsWyeJoin {
typedef tvsPantsJoin::V V;
using tvsPantsJoin::dot;using tvsPantsJoin::len;using tvsPantsJoin::unit;
typedef tvsPantsJoin::Duct Duct;
enum Error {Ok=0,Degenerate=1,NotAngled=2,SameSide=3,BothJoined=4,InletShort=5,BranchShort=6,NearJoined=7,NoMain=8,MainSizes=9,MainShort=10};
inline double cross(V a,V b){return a.x*b.y-a.y*b.x;}
// t on line p + d t where it meets line q + o s; false: parallel
inline bool meet(V p,V d,V q,V o,double& t){const double den=cross(d,o);if(std::fabs(den)<1e-12)return false;t=cross(q-p,o)/den;return true;}
struct Plan {
    Error error;
    int moved;V shift;         // input branch moved along the inlet axis, -1 none
    int right;                 // input branch on the right of the flow (w1, TVS_WYE +Vectpr)
    V base,u,v;                // TVS_WYE: Basepoint = O of the right branch, Vectpr = u, Vectotv = v
    double lengthPl;           // TVS_WYE LengthPl = yO1 - w1/2
    V inletPort,port[2];       // port[k]: where input branch k starts (the Y45 face, or the exit of its elbow)
    bool elbow[2];V face[2],faceDir[2];   // branch k perpendicular to the inlet: koljeno 45 on the Y45 face
    int inletNear,branchNear[2];
    Plan():error(Degenerate),moved(-1),right(0),lengthPl(0),inletNear(0){branchNear[0]=branchNear[1]=0;elbow[0]=elbow[1]=false;}
};
// A: inlet diameter, w[k] = br[k].width; s: inlet straight; L2: Soko L2 (both branches);
// tolDeg: angle tolerance; minDuct: shortest duct left; preferMoved: -1 automatic, 0 / 1.
// A branch perpendicular to the inlet (user's drawing 2026-11-27: inlet + two opposite branches at
// right angles) gets a koljeno 45 on the Y45 face: T[k] = tangent length of its axis
// ((R + w/2) tan 22.5); its duct line then lies (L2 + T) cos 45 beyond O_k along the inlet.
inline Plan wye(const Duct& in,const Duct br[2],double s,double L2,double tolDeg,double minDuct,int preferMoved=-1,const double* T=0){
    Plan p;const double deg=45,c=std::cos(deg*3.14159265358979323846/180),tc=std::sin(tolDeg*3.14159265358979323846/180);
    if(len(in.b-in.a)<1e-9||len(br[0].b-br[0].a)<1e-9||len(br[1].b-br[1].a)<1e-9||!(in.width>0)||!(br[0].width>0)||!(br[1].width>0))return p;
    const V mid=(br[0].a+br[0].b+br[1].a+br[1].b)*0.25;
    p.inletNear=len(in.a-mid)<=len(in.b-mid)?0:1;
    const V inNear=p.inletNear==0?in.a:in.b,farIn=p.inletNear==0?in.b:in.a,d=unit(inNear-farIn);   // flow towards the branches
    const V ex(d.y,-d.x);   // right of the flow
    V bn[2],o[2];double t[2];
    for(int k=0;k<2;++k){
        // near end: nearer the inlet axis line
        const double da=std::fabs(cross(br[k].a-inNear,d)),db=std::fabs(cross(br[k].b-inNear,d));
        p.branchNear[k]=da<=db?0:1;bn[k]=p.branchNear[k]==0?br[k].a:br[k].b;
        o[k]=unit((p.branchNear[k]==0?br[k].b:br[k].a)-bn[k]);
        p.elbow[k]=std::fabs(dot(o[k],d))<=tc&&T&&T[k]>=0;
        if(!p.elbow[k]&&std::fabs(dot(o[k],d)-c)>tc){p.error=NotAngled;return p;}
        if(!meet(inNear,d,bn[k],o[k],t[k]))return p;
    }
    const bool r0=dot(o[0],ex)>0,r1=dot(o[1],ex)>0;
    if(r0==r1){p.error=SameSide;return p;}
    if(br[0].nearJoined||br[1].nearJoined||in.nearJoined){p.error=NearJoined;return p;}
    p.right=r0?0:1;const int left=1-p.right;
    const double w1=br[p.right].width,w2=br[left].width,A=in.width;
    double yO[2];yO[p.right]=tvsTee::wyeAxisY(A,w1,s,deg);yO[left]=tvsTee::wyeAxisY(A,w2,s,deg);
    for(int k=0;k<2;++k)if(p.elbow[k])yO[k]+=(L2+T[k])*c;   // where the branch duct line crosses the inlet axis
    // which branch keeps its place: the other moves along d by (tface + yO_m) - t_m
    const double off01=(t[1]-yO[1])-(t[0]-yO[0]);   // inlet face of branch 1 minus that of branch 0
    int keep=0;
    if(std::fabs(off01)>1e-9){
        int m=preferMoved;
        if(m<0){if(br[0].farJoined&&br[1].farJoined)m=-2;else if(br[0].farJoined)m=1;else if(br[1].farJoined)m=0;else m=br[0].width<br[1].width?0:1;}
        if(m==-2||br[m].farJoined){p.error=BothJoined;return p;}
        keep=1-m;p.moved=m;
        const double tface=t[keep]-yO[keep],delta=(tface+yO[m])-t[m];
        p.shift=d*delta;
    }
    const double tface=t[keep]-yO[keep];
    p.inletPort=inNear+d*tface;
    p.u=ex;p.v=d*-1;
    const double yR=yO[p.right]-(p.elbow[p.right]?(L2+T[p.right])*c:0);
    p.base=inNear+d*(tface+yR);p.lengthPl=yR-w1/2;
    for(int k=0;k<2;++k){
        const double y=yO[k]-(p.elbow[k]?(L2+T[k])*c:0);
        p.faceDir[k]=p.elbow[k]?unit(d+o[k]):o[k];
        p.face[k]=inNear+d*(tface+y)+p.faceDir[k]*L2;
        p.port[k]=p.elbow[k]?p.face[k]+p.faceDir[k]*T[k]+o[k]*T[k]:p.face[k];}
    if(dot(farIn-p.inletPort,d*-1)<minDuct){p.error=InletShort;return p;}
    for(int k=0;k<2;++k){const V f=(p.branchNear[k]==0?br[k].b:br[k].a)+(p.moved==k?p.shift:V());
        if(dot(f-p.port[k],o[k])<minDuct){p.error=BranchShort;return p;}}
    p.error=Ok;return p;
}
// Kosa racva: which two of the three ducts are the main (collinear: parallel, on one line within
// tol). main[0], main[1], branch: indices; false: none or more than one pair.
inline bool lateralParts(const Duct d[3],double tol,int main[2],int& branch){
    int found=0;
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){
        const V u=unit(d[i].b-d[i].a),w=unit(d[j].b-d[j].a);if(len(u)<0.5||len(w)<0.5)continue;
        if(std::fabs(cross(u,w))>0.005)continue;
        if(std::fabs(cross(d[j].a-d[i].a,u))>tol||std::fabs(cross(d[j].b-d[i].a,u))>tol)continue;
        main[0]=i;main[1]=j;branch=3-i-j;++found;}
    return found==1;
}
struct Lateral {
    Error error;
    V base,u,d;           // TVS_WYE: Basepoint (branch axis on the main axis), Vectpr = u (downstream), Vectotv = d (branch)
    V run0,run1,face;     // run ports (-u / +u side) and the branch face
    int up;               // main duct (0 / 1 of the input pair) on the -u side
    int nearEnd[3];       // input order main0, main1, branch: 0 = end a, 1 = end b
    Lateral():error(Degenerate),up(0){nearEnd[0]=nearEnd[1]=nearEnd[2]=0;}
};
// m[2]: the main ducts, b: the branch; before / after: run length upstream / downstream of the
// branch axis (tvsTee::tapRun), faceDist: A/2 + LengthPl (branch face from the main axis along it).
inline Lateral lateral(const Duct m[2],const Duct& b,double before,double after,double faceDist,double tolDeg,double minDuct){
    Lateral p;const double c=std::cos(3.14159265358979323846/4),tc=std::sin(tolDeg*3.14159265358979323846/180);
    if(len(m[0].b-m[0].a)<1e-9||len(m[1].b-m[1].a)<1e-9||len(b.b-b.a)<1e-9)return p;
    if(std::fabs(m[0].width-m[1].width)>1e-6){p.error=MainSizes;return p;}
    V u=unit(m[0].b-m[0].a);const V q=m[0].a;
    // branch: near end nearer the main line
    const double da=std::fabs(cross(b.a-q,u)),db=std::fabs(cross(b.b-q,u));
    p.nearEnd[2]=da<=db?0:1;const V bn=p.nearEnd[2]==0?b.a:b.b,bf=p.nearEnd[2]==0?b.b:b.a;
    const V o=unit(bf-bn);if(dot(o,u)<0)u=u*-1;
    if(std::fabs(dot(o,u)-c)>tc){p.error=NotAngled;return p;}
    double t;if(!meet(q,u,bn,o,t))return p;
    const V X=q+u*t;
    p.base=X;p.u=u;p.d=o;p.run0=X-u*before;p.run1=X+u*after;p.face=X+o*faceDist;
    // each main duct: near end = nearer X; one on each side
    double side[2];V farEnd[2];
    for(int k=0;k<2;++k){const bool aNear=len(m[k].a-X)<=len(m[k].b-X);p.nearEnd[k]=aNear?0:1;
        const V nr=aNear?m[k].a:m[k].b;farEnd[k]=aNear?m[k].b:m[k].a;side[k]=dot(farEnd[k]-X,u);(void)nr;}
    if((side[0]<0)==(side[1]<0)){p.error=SameSide;return p;}
    if(m[0].nearJoined||m[1].nearJoined||b.nearJoined){p.error=NearJoined;return p;}
    p.up=side[0]<0?0:1;
    if(dot(farEnd[p.up]-p.run0,u*-1)<minDuct||dot(farEnd[1-p.up]-p.run1,u)<minDuct){p.error=MainShort;return p;}
    if(dot(bf-p.face,o)<minDuct){p.error=BranchShort;return p;}
    p.error=Ok;return p;
}
}
