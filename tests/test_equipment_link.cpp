// geometry/EquipmentLink.h: linking a drawn block (oprema) to the ducts under it (TVS_OPREMA).
#include "../geometry/EquipmentLink.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace tvsEquipmentLink;
static const double PI=3.14159265358979323846;
static bool eq(double p,double q,double t=1e-6){return std::fabs(p-q)<=t;}
static bool at(const tvsEquipment::Port& p,double x,double y,double t=1e-6){return eq(p.x,x,t)&&eq(p.y,y,t);}
static bool at(const Pt& p,double x,double y,double t=1e-6){return eq(p.x,x,t)&&eq(p.y,y,t);}
// rectangle: centre, length along angle deg, width across
static std::vector<Pt> box(double cx,double cy,double len,double wid,double deg){
    const double c=std::cos(deg*PI/180),s=std::sin(deg*PI/180);std::vector<Pt> b;
    const double h[4][2]={{-len/2,-wid/2},{len/2,-wid/2},{len/2,wid/2},{-len/2,wid/2}};
    for(int k=0;k<4;++k)b.push_back(Pt(cx+h[k][0]*c-h[k][1]*s,cy+h[k][0]*s+h[k][1]*c));
    return b;
}
static Duct duct(double x0,double y0,double x1,double y1,double a=400,double b=250,double level=2700){
    Duct d;d.p0=Pt(x0,y0);d.p1=Pt(x1,y1);d.a=a;d.b=b;d.level=level;return d;
}
// a point along angle deg from (cx,cy)
static Pt along(double cx,double cy,double dist,double deg){return Pt(cx+dist*std::cos(deg*PI/180),cy+dist*std::sin(deg*PI/180));}
int main(){
    Options o;   // mm drawing: tol 1, axis 25, gap 150, min piece 50
    // ---- case 1: inline, both pieces touch the faces (block 500 long at x 1000..1500) ----
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1500,0,3000,0));
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&r.ports==2&&r.linkCase==Touching&&r.edits.empty());
     assert(at(r.port[0],1000,0)&&at(r.port[1],1500,0)&&r.port[0].dx==-1&&r.port[1].dx==1);
     assert(r.port[0].z==2700&&eq(r.length,500)&&r.duct[0]==0&&r.duct[1]==1&&r.a==400&&r.b==250&&!r.round);}
    // both duct orientations (drawn right to left), also within tol
    {std::vector<Duct> d;d.push_back(duct(1000.5,0,0,0));d.push_back(duct(3000,0,1500,0));
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&r.linkCase==Touching&&at(r.port[0],1000.5,0)&&at(r.port[1],1500,0)&&r.ux==1);}
    // ---- case 2: gap on one side, overlap on the other ----
    {std::vector<Duct> d;d.push_back(duct(0,0,900,0));d.push_back(duct(3000,0,1400,0));
     d[0].others[0]=1;   // the far end is joined: does not matter
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&r.ports==2&&r.linkCase==Adjust&&r.edits.size()==2);
     assert(r.edits[0].duct==0&&r.edits[0].end==1&&at(r.edits[0].to,1000,0)&&eq(r.edits[0].change,100));
     assert(r.edits[1].duct==1&&r.edits[1].end==1&&at(r.edits[1].to,1500,0)&&eq(r.edits[1].change,-100));
     assert(at(r.port[0],1000,0)&&at(r.port[1],1500,0));}
    // two pieces joined to each other under the block centre: both trimmed
    {std::vector<Duct> d;d.push_back(duct(0,0,1250,0));d.push_back(duct(1250,0,3000,0));
     d[0].others[1]=1;d[0].partner[1]=1;d[1].others[0]=1;d[1].partner[0]=0;
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&r.linkCase==Adjust&&r.edits.size()==2&&at(r.edits[0].to,1000,0)&&at(r.edits[1].to,1500,0));}
    // ---- case 3: one duct under the block, both orientations ----
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&r.linkCase==Cut&&r.ports==2&&r.cutDuct==0&&at(r.cutKeep,1000,0)&&at(r.cutNew,1500,0));
     assert(r.duct[0]==0&&r.duct[1]==NewPiece&&at(r.port[0],1000,0)&&at(r.port[1],1500,0));}
    {std::vector<Duct> d;d.push_back(duct(3000,0,0,0));
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&r.linkCase==Cut&&at(r.cutKeep,1500,0)&&at(r.cutNew,1000,0));
     assert(r.duct[0]==NewPiece&&r.duct[1]==0&&at(r.port[0],1000,0)&&at(r.port[1],1500,0));}
    // vertical duct in plan (along y), block rotated 90 deg
    {std::vector<Duct> d;d.push_back(duct(200,5000,200,0,250,0));d[0].round=true;
     Result r=link(box(200,2000,400,300,90),d,o);
     assert(r.error==Ok&&r.linkCase==Cut&&at(r.cutKeep,200,2200)&&at(r.cutNew,200,1800)&&r.round&&r.b==0);
     assert(eq(r.uy,1)&&at(r.port[0],200,1800)&&eq(r.port[0].dy,-1)&&at(r.port[1],200,2200));}
    // rotated 30 deg: cut and adjust
    {const double a=30;Pt s=along(0,0,-2000,a),e=along(0,0,2000,a);
     std::vector<Duct> d;d.push_back(duct(s.x,s.y,e.x,e.y));
     Result r=link(box(0,0,600,450,a),d,o);
     Pt k=along(0,0,-300,a),n=along(0,0,300,a);
     assert(r.error==Ok&&r.linkCase==Cut&&at(r.cutKeep,k.x,k.y,1e-6)&&at(r.cutNew,n.x,n.y,1e-6)&&eq(r.length,600));
     assert(eq(r.port[1].dx,std::cos(a*PI/180))&&eq(r.port[1].dy,std::sin(a*PI/180)));}
    {const double a=-135;Pt s=along(0,0,-2000,a),e=along(0,0,-350,a),s2=along(0,0,280,a),e2=along(0,0,2000,a);
     std::vector<Duct> d;d.push_back(duct(s.x,s.y,e.x,e.y));d.push_back(duct(e2.x,e2.y,s2.x,s2.y));
     Result r=link(box(0,0,600,450,a),d,o);
     Pt k=along(0,0,-300,a),n=along(0,0,300,a);
     assert(r.error==Ok&&r.linkCase==Adjust&&r.edits.size()==2);
     // the axis points to +x: port[0] is the piece on the -x side (d[1], overlapping 20: trimmed)
     assert(r.edits[0].duct==1&&r.edits[0].end==1&&at(r.edits[0].to,n.x,n.y,1e-6)&&eq(r.edits[0].change,-20,1e-6));
     assert(r.edits[1].duct==0&&r.edits[1].end==1&&at(r.edits[1].to,k.x,k.y,1e-6)&&eq(r.edits[1].change,50,1e-6));
     assert(r.ux>0&&at(r.port[0],n.x,n.y,1e-6)&&at(r.port[1],k.x,k.y,1e-6));}
    // duct drawn a little off the block centre (10 mm): ok, ports on the duct axis
    {std::vector<Duct> d;d.push_back(duct(0,10,3000,10));
     Result r=link(box(1250,0,500,450,0),d,o);
     assert(r.error==Ok&&at(r.port[0],1000,10)&&eq(r.offset,10));}
    // fixed length per block name (400 instead of the drawn 500)
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));Options f=o;f.fixedLength=400;
     Result r=link(box(1250,0,500,450,0),d,f);
     assert(r.error==Ok&&eq(r.length,400)&&at(r.port[0],1050,0)&&at(r.port[1],1450,0));}
    // ---- terminals ----
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));   // grille touching
     Result r=link(box(1100,0,200,600,0),d,o);
     assert(r.error==Ok&&r.ports==1&&r.linkCase==Touching&&at(r.port[0],1000,0)&&r.port[0].dx==-1&&r.duct[0]==0&&r.duct[1]==-1);}
    {std::vector<Duct> d;d.push_back(duct(0,3000,0,1100));d[0].flex=true;   // flex end overlapping the diffuser (vertical in plan)
     Result r=link(box(0,1000,600,600,90),d,o);
     assert(r.error==Ok&&r.ports==1&&r.linkCase==Adjust&&r.edits.size()==1&&r.edits[0].end==1&&at(r.edits[0].to,0,1300));
     assert(at(r.port[0],0,1300)&&eq(r.port[0].dy,1));}
    {std::vector<Duct> d;d.push_back(duct(1100,0,3000,0));   // gap at a terminal on the left side of the duct
     Result r=link(box(900,0,200,600,0),d,o);
     assert(r.error==Ok&&r.ports==1&&r.edits.size()==1&&r.edits[0].end==0&&at(r.edits[0].to,1000,0)&&r.port[0].dx==1);}
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d[0].others[1]=1;   // the end is joined (elbow)
     Result r=link(box(1100,0,200,600,0),d,o);assert(r.error==NotFreeEnd&&r.detailDuct==0);}
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));Options t=o;t.expect=Terminal;   // grille on a continuous duct
     assert(link(box(1100,0,200,600,0),d,t).error==SideMounted);}
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1200,0,3000,0));Options t=o;t.expect=Terminal;
     assert(link(box(1100,0,200,600,0),d,t).error==SideMounted);}
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));Options t=o;t.expect=Inline;   // damper with one duct only
     assert(link(box(1250,0,500,450,0),d,t).error==MissingSide);}
    // ---- refusals ----
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1500,0,3000,0,500,250));
     assert(link(box(1250,0,500,450,0),d,o).error==DifferentSize);}
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1500,0,3000,0,400,250,2900));
     assert(link(box(1250,0,500,450,0),d,o).error==DifferentLevel);}
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1800,0,3000,0));   // 300 > 150 on the right
     Result r=link(box(1250,0,500,450,0),d,o);assert(r.error==GapTooLarge&&r.detailDuct==1&&eq(r.detail,300));}
    {std::vector<Duct> d;d.push_back(duct(0,0,700,0));   // only a far duct
     Result r=link(box(1250,0,500,450,0),d,o);assert(r.error==GapTooLarge&&eq(r.detail,300));}
    {std::vector<Duct> d;d.push_back(duct(0,100,3000,100));   // axis 100 off the centre, still inside the block
     Result r=link(box(1250,0,500,450,0),d,o);assert(r.error==OffAxis&&eq(r.detail,100));}
    {std::vector<Duct> d;d.push_back(duct(0,1000,3000,1000));   // beside the block
     assert(link(box(1250,0,500,450,0),d,o).error==NoDuct);}
    {std::vector<Duct> d;assert(link(box(1250,0,500,450,0),d,o).error==NoDuct);}
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));d.push_back(duct(1250,-2000,1250,2000));   // crossing duct
     assert(link(box(1250,0,500,450,0),d,o).error==Ambiguous);}
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));d.push_back(duct(500,0,2000,0,400,250,3500));   // stacked in plan
     assert(link(box(1250,0,500,450,0),d,o).error==Ambiguous);}
    {std::vector<Duct> d;d.push_back(duct(970,0,3000,0));   // 30 mm left of the face after the cut
     Result r=link(box(1250,0,500,450,0),d,o);assert(r.error==TooShort&&eq(r.detail,30));}
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));d[0].flex=true;
     assert(link(box(1250,0,500,450,0),d,o).error==FlexCut);}
    {std::vector<Duct> d;d.push_back(duct(1100,0,1400,0));
     assert(link(box(1250,0,500,450,0),d,o).error==DuctInside);}
    {std::vector<Duct> d;d.push_back(duct(0,0,3000,0));std::vector<Pt> b;b.push_back(Pt(0,0));b.push_back(Pt(1,1));
     assert(link(b,d,o).error==BadBlock);}
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1500,0,3000,0));
     d[0].others[1]=1;d[0].partner[1]=-1;   // the face end is joined to a third element
     assert(link(box(1250,0,500,450,0),d,o).error==NotFreeEnd);}
    for(int e=1;e<errorCount;++e)assert(reasonText(e)[0]!=0);
    std::puts("equipment link OK");
    return 0;
}
