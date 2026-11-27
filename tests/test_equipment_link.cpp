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
    // ---- a row of blocks (FV 55 - ventilator 800 - FV 55) between two ducts ----
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1910,0,3000,0));
     std::vector<std::vector<Pt> > row;row.push_back(box(1027.5,0,55,700,0));row.push_back(box(1455,0,800,800,0));row.push_back(box(1882.5,0,55,700,0));
     std::vector<Pt> all;for(size_t k=0;k<row.size();++k)all.insert(all.end(),row[k].begin(),row[k].end());
     Result r=link(all,d,o);assert(r.error==Ok&&r.ports==2&&eq(r.length,910));
     std::vector<ChainPart> c;assert(chain(row,r,o.tol,c)&&c.size()==3);
     assert(at(c[0].port[0],1000,0)&&at(c[0].port[1],1055,0)&&at(c[1].port[0],1055,0)&&at(c[1].port[1],1855,0)&&at(c[2].port[1],1910,0));
     assert(eq(c[1].length,800)&&c[1].port[0].dx==-1&&c[1].port[1].dx==1);
     assert(touching(row[0],row[1],o.tol)&&touching(row[1],row[2],o.tol)&&!touching(row[0],row[2],o.tol));
     assert(!touching(box(0,0,100,100,0),box(103,0,100,100,0),1)&&touching(box(0,0,100,100,0),box(100.5,0,100,100,0),1));
     std::vector<std::vector<Pt> > gap=row;gap[1]=box(1460,0,790,800,0);assert(!chain(gap,r,o.tol,c));}
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
    // ---- user's drawing p.dxf (mm, 2026-11-23): exact coordinates (proxy graphics / block extents) ----
    std::vector<Outlet> none;
    {   // 1: duct 250 -> TVS_TRANS 250->450 -> stub 60 mm, 450 wide -> Res_2 (rot 90) based on the stub's free end,
        //    extents (local y -10..60) cover the whole stub
        std::vector<Duct> d;d.push_back(duct(14733.47155,3466.599385,17602.81909,3466.599385,250,200));d[0].others[1]=1;
        d.push_back(duct(17902.81909,3466.599385,17962.81909,3466.599385,450,200));d[1].others[0]=1;
        std::vector<Pt> b;b.push_back(Pt(17972.81909,3229.099385));b.push_back(Pt(17972.81909,3704.099385));
        b.push_back(Pt(17902.81909,3704.099385));b.push_back(Pt(17902.81909,3229.099385));
        const Pt base(17962.81909,3466.599385);
        assert(link(b,d,o).error==DuctInside);   // extents only (before): refused
        for(int x=Any;x<=Terminal;++x){Options t=o;t.expect=x;t.bottom=true;
            Result r=link(b,base,d,none,t);
            assert(r.error==Ok&&r.ports==1&&r.linkCase==Touching&&r.edits.empty()&&r.cutDuct==-1&&r.mount==tvsEquipment::AtEnd);
            assert(at(r.port[0],17962.81909,3466.599385)&&eq(r.port[0].dx,-1)&&eq(r.port[0].dy,0)&&r.port[0].z==2700);
            assert(r.duct[0]==1&&r.duct[1]==-1&&r.outlet==-1&&r.a==450&&r.b==200&&eq(r.ux,1));}
        Options t=o;t.expect=Inline;assert(link(b,base,d,none,t).error==DuctInside);   // inline: extents as before
    }
    {   // 2: duct -> TVS_WYE, branch to -y 450 wide -> stub 63 mm -> Res_2 (rot 0) based on the stub's free end
        std::vector<Duct> d;d.push_back(duct(14733.47155,4881.183363,17602.81909,4881.183363,250,200));d[0].others[1]=1;
        d.push_back(duct(17877.81909,4706.183363,17877.81909,4643.334832,450,200));d[1].others[0]=1;
        std::vector<Pt> b;b.push_back(Pt(17640.31909,4633.334832));b.push_back(Pt(18115.31909,4633.334832));
        b.push_back(Pt(18115.31909,4703.334832));b.push_back(Pt(17640.31909,4703.334832));
        const Pt base(17877.81909,4643.334832);
        assert(link(b,d,o).error!=Ok);
        Result r=link(b,base,d,none,o);
        assert(r.error==Ok&&r.ports==1&&r.edits.empty()&&at(r.port[0],17877.81909,4643.334832)&&eq(r.port[0].dx,0)&&eq(r.port[0].dy,1));
        assert(r.duct[0]==1&&eq(r.uy,-1)&&r.level==2700);
        // the stub end joined to something else: not free -> the extents decide (refused here)
        d[1].others[1]=1;assert(link(b,base,d,none,o).error!=Ok);
    }
    {   // 3: flex end -> Dif_1 based on the flex end (extents start at the base point): same as before
        std::vector<Duct> d;d.push_back(duct(6368.006598,5237.543708,6368.006598,5857.852233,220,0));d[0].round=true;d[0].flex=true;d[0].others[0]=1;
        std::vector<Pt> b;b.push_back(Pt(6068.006598,5857.852233));b.push_back(Pt(6668.006598,5857.852233));
        b.push_back(Pt(6668.006598,6537.852233));b.push_back(Pt(6068.006598,6537.852233));
        Result x=link(b,d,o),r=link(b,Pt(6368.006598,5857.852233),d,none,o);
        assert(x.error==Ok&&r.error==Ok&&x.ports==1&&r.ports==1&&r.round&&r.b==0);
        assert(at(r.port[0],x.port[0].x,x.port[0].y)&&eq(r.port[0].dx,x.port[0].dx)&&eq(r.port[0].dy,x.port[0].dy)&&eq(r.port[0].dy,-1));
    }
    {   // 4: duct -> TVS_TAP elbow Dolje at the duct end -> LVS-100-RETURN (circle D307) based on the vertical outlet
        std::vector<Duct> d;d.push_back(duct(11123.55944,5137.346914,11123.55944,6412.968924,125,0));d[0].round=true;
        d[0].others[0]=1;d[0].others[1]=1;   // tee / elbow
        std::vector<Pt> b;b.push_back(Pt(11276.97952,6384.549521));b.push_back(Pt(10970.13922,6384.549521));
        b.push_back(Pt(10970.13922,6691.389824));b.push_back(Pt(11276.97952,6691.389824));
        std::vector<Outlet> w(1);w[0].at=Pt(11123.559435060974,6537.968923746606);w[0].level=2700-125;w[0].a=125;w[0].round=true;
        const Pt base(11123.55943506097,6537.968923746607);
        assert(link(b,d,o).error==NotFreeEnd);   // before: the duct end under the block is joined to the elbow
        Result r=link(b,base,d,w,o);
        assert(r.error==Ok&&r.ports==1&&r.linkCase==Touching&&r.edits.empty()&&r.outlet==0&&r.duct[0]==-1);
        assert(at(r.port[0],11123.559435060974,6537.968923746606)&&r.port[0].z==2575&&r.port[0].dx==0&&r.port[0].dy==0);
        assert(r.a==125&&r.b==0&&r.round&&r.level==2575);
        // the second one: elbow Gore below the other duct (31B / 72D)
        std::vector<Duct> e;e.push_back(duct(11123.55944,4912.346914,11123.55944,3639.610502,125,0));e[0].round=true;e[0].others[0]=1;e[0].others[1]=1;
        std::vector<Pt> c;c.push_back(Pt(11276.97952,3361.191099));c.push_back(Pt(10970.13922,3361.191099));
        c.push_back(Pt(10970.13922,3668.031402));c.push_back(Pt(11276.97952,3668.031402));
        std::vector<Outlet> v(1);v[0].at=Pt(11123.559435060974,3514.6105015774497);v[0].level=2700+125;v[0].a=125;v[0].round=true;
        r=link(c,Pt(11123.55943506097,3514.61050157745),e,v,o);
        assert(r.error==Ok&&r.outlet==0&&r.port[0].z==2825);
        // base point beside the outlet (> tol): not on a free end -> extents -> refused
        assert(link(b,Pt(base.x+5,base.y),d,w,o).error==NotFreeEnd);
    }
    {   // 5: Res_1 (490 x 190 frame) over a duct 250 wide, centred on the axis, 80 mm before the closed end
        std::vector<Duct> d;d.push_back(duct(14733.47155,6245.650885,18245.04994,6245.650885,250,200));
        std::vector<Pt> b;b.push_back(Pt(17675.67907,6150.546218));b.push_back(Pt(18165.17166,6150.546218));
        b.push_back(Pt(18165.17166,6340.755552));b.push_back(Pt(17675.67907,6340.755552));
        const Pt base(17920.42537,6245.650885);
        Options t=o;t.bottom=true;t.expect=Terminal;
        Result r=link(b,base,d,none,t);
        assert(r.error==Ok&&r.ports==1&&r.mount==tvsEquipment::Bottom&&r.linkCase==Touching&&r.edits.empty()&&r.cutDuct==-1);
        assert(at(r.port[0],(17675.67907+18165.17166)/2,6245.650885,1e-6)&&r.port[0].dx==0&&r.port[0].dy==0&&r.port[0].z==2700);
        assert(r.duct[0]==0&&r.duct[1]==-1&&r.a==250&&r.b==200);
        t.bottom=false;assert(link(b,base,d,none,t).error==SideMounted);   // as before without the bottom option
        t.bottom=true;t.expect=Any;r=link(b,base,d,none,t);            // no kind: the duct is cut (the command asks Dno / Linija)
        assert(r.error==Ok&&r.linkCase==Cut&&r.ports==2&&eq(r.cutNew.x,18165.17166,1e-6));
        t.expect=Inline;assert(link(b,base,d,none,t).linkCase==Cut);
        d[0].flex=true;t.expect=Terminal;assert(link(b,base,d,none,t).error==FlexBottom);
        double w=0,h=0;footprintMm(b,1,w,h);assert(w==489&&h==190);
        footprintMm(b,10,w,h);assert(w==4895&&h==1902);
    }
    // base point on a duct end but ducts on both sides (no kind): the extents keep it inline
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1500,0,3000,0));
     Result r=link(box(1250,0,500,450,0),Pt(1000,0),d,none,o);assert(r.error==Ok&&r.ports==2&&r.linkCase==Touching);
     Options t=o;t.expect=Terminal;r=link(box(1250,0,500,450,0),Pt(1000,0),d,none,t);
     assert(r.error==Ok&&r.ports==1&&at(r.port[0],1000,0)&&r.port[0].dx==-1);}
    // two free ends at the base point: ambiguous
    {std::vector<Duct> d;d.push_back(duct(0,0,1000,0));d.push_back(duct(1000,0,1000,-800));
     assert(link(box(1100,0,200,600,0),Pt(1000,0),d,none,o).error==Ambiguous);}
    // linijski difuzor: plenum box 1200 x 150 at (0,0), two flex / ducts from +y ending over the box
    {std::vector<Outlet> no;std::vector<Duct> d;d.push_back(duct(-300,1000,-300,20,200,0));d.push_back(duct(300,1000,300,-10,200,0));
     d[0].round=d[1].round=true;
     Options t=o;t.expect=Terminal;Result r=link(box(0,0,1200,150,0),Pt(-600,0),d,no,t);
     assert(r.error==Ok&&r.twin&&r.ports==2&&r.linkCase==Touching&&r.edits.empty());
     assert(at(r.port[0],-300,20)&&at(r.port[1],300,-10)&&eq(r.port[0].dy,1)&&eq(r.port[1].dy,1)&&r.duct[0]==0&&r.duct[1]==1);
     Options any=o;Result q=link(box(0,0,1200,150,0),Pt(-600,0),d,no,any);assert(q.error==Ok&&q.twin);   // not inline: kind unknown, still twin
     Options in=o;in.expect=Inline;assert(!link(box(0,0,1200,150,0),Pt(-600,0),d,no,in).twin);
     // one end only over the box: an ordinary terminal (base point rule)
     std::vector<Duct> one(1,d[0]);Result s1=link(box(0,0,1200,150,0),Pt(-300,20),one,no,t);assert(s1.error==Ok&&!s1.twin&&s1.ports==1);
     // inline damper between two ducts (Any): inline wins over twin
     std::vector<Duct> l;l.push_back(duct(0,0,1000,0));l.push_back(duct(1500,0,3000,0));
     Result x=link(box(1250,0,500,450,0),Pt(1000,0),l,no,any);assert(x.error==Ok&&x.ports==2&&!x.twin);}
    for(int e=1;e<errorCount;++e)assert(reasonText(e)[0]!=0);
    std::puts("equipment link OK");
    return 0;
}
