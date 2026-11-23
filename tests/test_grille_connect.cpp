// geometry/GrilleConnect.h: TVS_RESETKA, grille block connected to a duct (side T komad + stub, or
// bottom face). Numbers from the user's example drawing p.dxf (mm): Res_2 on the side of a 250 duct
// (TK0 with a 450 branch, stub ~63 mm, block base point at the stub end, rotation 0), Res_2 at the
// end of a duct drawn to +x (rotation 90), Res_1 490 x 190 over a 250 duct (bottom face).
#include "../geometry/GrilleConnect.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace tvsGrille;
static const double PI=3.14159265358979323846;
static bool eq(double p,double q,double t=1e-6){return std::fabs(p-q)<=t;}
static bool at(const V& p,double x,double y,double t=1e-6){return eq(p.x,x,t)&&eq(p.y,y,t);}
static Duct duct(double x0,double y0,double x1,double y1,double a,double b,bool round=false){
    Duct d;d.p0=V(x0,y0);d.p1=V(x1,y1);d.a=a;d.b=b;d.round=round;return d;
}
static Choice choice(int side,int method,double stub,double a,double b,bool round=false){
    Choice c;c.side=side;c.method=method;c.stub=stub;c.a=a;c.b=b;c.round=round;return c;
}
int main(){
    Options o;   // mm: TK0 collar 50, stucna L >= 75, cone L 100, margin 25, piece 50
    // ---- side, example (handle 694 / 6AA / 6A9 / Res_2 6A8): axis y 4881.18, duct drawn to +x ----
    {const Duct d=duct(14733.47,4881.18,18500,4881.18,250,200);
     const V ins(17877.81908863922,4643.334832090462);
     const Place p=place(d,ins);
     assert(p.ok&&p.side==Right&&eq(p.offset,ins.y-4881.18)&&at(p.tap,ins.x,4881.18));
     const Choice c=choice(Right,Tee,4706.18-ins.y,450,200);
     const Layout y=layout(d,p,c,o);
     assert(y.ok&&y.teeType==tvsTee::Straight&&eq(y.lengthPl,50)&&eq(y.before,275)&&eq(y.after,275));
     assert(at(y.outward,0,-1));
     assert(at(y.runStart,17602.81908863922,4881.18)&&at(y.runEnd,18152.81908863922,4881.18));   // the TVS_WYE 6AA extents in x
     assert(at(y.branchPort,ins.x,4706.18));                                     // = stub 6A9 start (wye bottom)
     assert(at(y.stubEnd,ins.x,ins.y));                                          // = Res_2 base point
     assert(eq(y.rotation,0));                                                   // Res_2 rotation 0 in the example
     assert(validate(d,p,c,o).error==Ok);
     // left of the axis: mirrored
     const Place q=place(d,V(ins.x,4881.18+237.85));assert(q.side==Left);
     const Layout z=layout(d,q,choice(Left,Tee,100,450,200),o);
     assert(at(z.outward,0,1)&&at(z.stubEnd,ins.x,4881.18+125+50+100)&&eq(std::fabs(z.rotation),PI)&&eq(std::fabs(blockRotation(V(0,1))),PI));
     // too close to the end: run 275 + piece 50 needed after the tap point
     const Duct shortD=duct(14733.47,4881.18,17602.82+275+275+40,4881.18,250,200);
     const Check k=validate(shortD,place(shortD,ins),c,o);assert(k.error==NearEnd&&eq(k.detail,325));
     // the user's drawing has the duct ending at the T komad: the tap is beyond the duct
     const Duct real=duct(14733.47,4881.18,17602.82,4881.18,250,200);
     assert(validate(real,place(real,ins),c,o).error==Outside);
     assert(y.fit==Split);
     // ... and with that end FREE it is the typical case (coordinator 2026-11-23): T komad at the end,
     // branch axis on the projection, duct trimmed to the inlet (here exactly its end), no new piece,
     // the straight outlet beyond the old duct end stays free (cap)
     {Duct freeD=real;freeD.freeEnd[1]=true;const Place q=place(freeD,ins);
      assert(eq(q.t,ins.x-14733.47)&&q.t>freeD.len());
      const Layout e=layout(freeD,q,c,o);
      assert(e.ok&&e.fit==AtLast&&validate(freeD,q,c,o).error==Ok);
      assert(at(e.tap,ins.x,4881.18)&&at(e.runStart,17602.81908863922,4881.18)&&eq(e.runStart.x,17602.82,0.01));   // = the duct end of 694
      assert(at(e.runEnd,18152.81908863922,4881.18)&&at(e.branchPort,ins.x,4706.18)&&at(e.stubEnd,ins.x,ins.y));
      // free start instead: nothing to do there (the short side is the last end)
      Duct startFree=real;startFree.freeEnd[0]=true;assert(validate(startFree,place(startFree,ins),c,o).error==Outside);
      // grille a bit before the free end: inlet inside the duct (duct trimmed), still AtLast
      const Place q2=place(freeD,V(17500,4643.33));const Layout e2=layout(freeD,q2,c,o);
      assert(e2.fit==AtLast&&eq(e2.runStart.x,17225)&&validate(freeD,q2,c,o).error==Ok);
      // the duct too short for the inlet piece: refused even with a free end
      Duct tiny=duct(17580,4881.18,17602.82,4881.18,250,200);tiny.freeEnd[0]=tiny.freeEnd[1]=true;
      assert(validate(tiny,place(tiny,ins),c,o).error==Outside||validate(tiny,place(tiny,ins),c,o).error==NearEnd);
      assert(layout(tiny,place(tiny,ins),c,o).fit==Refused);}
     // the same at the START of a duct drawn the other way (free p0): FirstPoint goes to runEnd
     {Duct rev=duct(17602.82,4881.18,14733.47,4881.18,250,200);rev.freeEnd[0]=true;
      const Place q=place(rev,ins);assert(q.side==Left&&q.t<0);
      const Choice cl=choice(Left,Tee,4706.18-ins.y,450,200);
      const Layout e=layout(rev,q,cl,o);
      assert(e.fit==AtFirst&&validate(rev,q,cl,o).error==Ok&&eq(e.runEnd.x,17602.82,0.01)&&at(e.stubEnd,ins.x,ins.y,1e-6));
      rev.freeEnd[0]=false;assert(validate(rev,q,cl,o).error==Outside);}
     // stucna near a free end: run 25 + L + w/2 upstream
     {Duct freeD=real;freeD.freeEnd[1]=true;const Layout e=layout(freeD,place(freeD,ins),choice(Right,Tap,60,400,200),o);
      assert(e.fit==AtLast&&eq(e.before,25+100+200)&&eq(e.runStart.x,ins.x-325));}
     // branch higher than the side face
     assert(validate(d,p,choice(Right,Tee,60,450,250),o).error==TooTall&&eq(validate(d,p,choice(Right,Tee,60,450,250),o).detail,200));
     assert(validate(d,p,choice(Right,Tee,0,450,200),o).error==NoStub);
     assert(validate(d,p,choice(Right,Tee,60,0,200),o).error==BadSize);
     assert(validate(d,p,choice(Right,Direct,60,450,200),o).error==MethodSide);}
    // ---- end of a duct drawn to +x: outward +x -> rotation 90 (Res_2 6AF) ----
    {assert(eq(blockRotation(V(1,0)),PI/2));
     assert(eq(turnAngle(V(0,-1),V(1,0)),PI/2));        // block at rotation 0 turned to +x
     assert(eq(turnAngle(V(1,0),V(0,-1)),-PI/2));
     assert(eq(turnAngle(V(0,-1),V(0,-1)),0));
     assert(eq(std::fabs(turnAngle(V(0,1),V(0,-1))),PI));}
    // ---- stucna on a rectangular duct: rect branch, round branch, cone ----
    {const Duct d=duct(0,0,3000,0,600,300);const Place p=place(d,V(1500,-800));assert(p.side==Right);
     const Layout r=layout(d,p,choice(Right,Tap,100,400,250),o);   // L = max(0.25*400, 75) = 100
     assert(r.teeType==tvsTee::Tap&&eq(r.lengthPl,100)&&eq(r.before,25+100+200)&&eq(r.after,25+200)&&eq(r.cone,0));
     assert(at(r.runStart,1500-325,0)&&at(r.runEnd,1500+225,0)&&at(r.branchPort,1500,-400)&&at(r.stubEnd,1500,-500));
     const Layout s=layout(d,p,choice(Right,Tap,100,250,0,true),o);   // L = 75, run 25 + D/2 both sides
     assert(eq(s.lengthPl,75)&&eq(s.before,150)&&eq(s.after,150)&&at(s.branchPort,1500,-375));
     const Layout c=layout(d,p,choice(Right,Cone,100,200,0,true),o);   // L 100, cone base 200 + 2*100*tan15
     assert(eq(c.lengthPl,100)&&eq(c.cone,tvsTee::coneBase(200,100))&&eq(c.before,25+c.cone/2)&&eq(c.after,c.before));
     assert(validate(d,p,choice(Right,Cone,100,200,0,true),o).error==Ok);
     assert(validate(d,p,choice(Right,Cone,100,300,0,true),o).error==TooTall);   // cone base 353.6 > 300
     assert(validate(d,p,choice(Right,Cone,100,200,150),o).error==ConeRect);
     const Duct rd=duct(0,0,3000,0,400,0,true);
     assert(validate(rd,place(rd,V(1500,-800)),choice(Right,Tap,100,200,0,true),o).error==RoundMain);
     assert(validate(rd,place(rd,V(1500,-800)),choice(Right,Tee,100,200,0,true),o).error==Ok);
     assert(validate(rd,place(rd,V(1500,-800)),choice(Right,Tee,100,450,0,true),o).error==TooTall);}
    // ---- bottom face, example (handle 4DF / Res_1 693): duct 250 wide, axis y 6245.65 ----
    {const Duct d=duct(14733.47,6245.65,18245.05,6245.65,250,200);
     const V centre(17920.42536703968,6245.650885062965);
     const Place p=place(d,centre);
     assert(p.side==Bottom&&at(p.tap,centre.x,6245.65));
     const Choice c=choice(Bottom,Direct,0,490,190);   // default neck = block footprint
     assert(validate(d,p,c,o).error==Ok);
     assert(!layout(d,p,c,o).ok);                       // nothing drawn
     assert(validate(d,p,choice(Bottom,Direct,0,490,260),o).error==TooWide);    // across 260 > 250
     assert(validate(d,p,choice(Bottom,Tee,0,490,190),o).error==MethodSide);
     // 80 mm before the closed end as drawn: centre 324.6 from the end, half of 490 = 245 fits
     const Duct shortD=duct(14733.47,6245.65,centre.x+200,6245.65,250,200);
     const Check k=validate(shortD,place(shortD,centre),c,o);assert(k.error==NearEnd&&eq(k.detail,245));
     // over the duct but off-centre still counts as bottom; beyond the half width it is a side
     assert(place(d,V(centre.x,6245.65+120)).side==Bottom&&place(d,V(centre.x,6245.65+130)).side==Left);
     assert(place(d,V(centre.x,6245.65-130)).side==Right);}
    // ---- rotated duct: everything in its frame ----
    {const Duct d=duct(0,0,0,3000,400,250);   // drawn to +y: left = -x
     const Place p=place(d,V(-500,1000));assert(p.side==Left&&eq(p.t,1000)&&eq(p.offset,500));
     const Layout y=layout(d,p,choice(Left,Tee,100,300,200),o);
     assert(at(y.outward,-1,0)&&at(y.branchPort,-250,1000)&&at(y.stubEnd,-350,1000)&&eq(y.rotation,-PI/2));}
    for(int e=1;e<errorCount;++e)assert(errorText(e)[0]!=0);
    // Kraj kanala (user 2026-11-23): transition 450x150 -> duct 248 mm -> grille at its free end
    {Duct d=duct(1000,0,1248,0,450,150);d.freeEnd[1]=true;
     assert(nearFreeEnd(d,V(1248,0),600)==1&&nearFreeEnd(d,V(1240,30),600)==1&&nearFreeEnd(d,V(1000,0),100)==-1);
     Choice c;c.side=End;c.method=Direct;c.end=1;c.stub=100;c.a=450;c.b=150;
     assert(validate(d,place(d,V(1248,0)),c,Options()).error==Ok);   // 100 mm is enough (no 225 mm minimum)
     c.stub=50;assert(validate(d,place(d,V(1248,0)),c,Options()).error==Ok);
     const EndLayout e=endLayout(d,1,100);assert(e.ok&&std::fabs(e.at.x-1100)<1e-9&&std::fabs(e.at.y)<1e-9&&e.outward.x==1);
     const EndLayout k=endLayout(d,1,0);assert(k.ok&&std::fabs(k.at.x-1248)<1e-9);   // 0: keep the drawn end
     const EndLayout f0=endLayout(d,0,0);assert(f0.ok&&f0.outward.x==-1);
     d.freeEnd[1]=false;assert(validate(d,place(d,V(1248,0)),c,Options()).error==NotFreeEnd);}
    std::puts("grille connect OK");
    return 0;
}
