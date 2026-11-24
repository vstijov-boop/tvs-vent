#pragma once
// TVS_OPREMA: how an ordinary AutoCAD block (oprema na mrezi, geometry/Equipment.h) that is already
// drawn on a duct is linked into the network - pure plan geometry, no AutoCAD, ASCII;
// tests/test_equipment_link.cpp. The command (TVS_Ventilation_ARX/EquipmentLinkCommand.h) passes
// the block footprint (block-local bounding box mapped to WCS: 4 corners, so rotated blocks work)
// and the straight ducts / flex hoses near it; link() decides:
//  - inline (2 ports, ducts on both sides) or terminal (1 port, block at a free duct end);
//  - case 1 Touching: the duct ends already sit on the block faces (within tol) -> only link;
//    case 2 Adjust: small gap (up to gap) / overlap (duct end under the block) -> the duct ends are
//    trimmed / extended to the faces (edits);
//    case 3 Cut: one continuous duct under the block -> cut by the block length (the original keeps
//    its first point and ends at cutKeep, a new piece goes from cutNew to the original last point);
//  - the axis direction, port points on the duct axis (z = duct level), port dirs pointing OUT of
//    the block (towards the duct), the block length along the axis (block extents projected on the
//    axis, or fixedLength per block name centred on them);
//  - or a refusal with a reason (reasonText). The block itself never moves.
// Terminals by the base point (the 5-argument link, user's drawings 2026-11-23): a block that is
// (or may be) a terminal whose INSERTION point lies on a free end - of a duct, flex hose, short stub
// after a transition / tee branch, or the free vertical outlet of an elbow Gore / Dolje (Outlet) -
// has its port there: nothing is trimmed, extended or cut and the block extents are not used
// (a grille drawn over the last 60 mm of its stub, a diffuser drawn from the flex end on).
// Grille in the bottom face (Options::bottom, kind terminal): block over a continuous duct, centre
// on the axis -> mount Bottom: no cut, port[0] = the block centre projected on the axis, dir (0,0).
// Units: points, levels and lengths in drawing units; duct sizes a / b in mm. Horizontal only.
#include "Equipment.h"
#include <cmath>
#include <vector>
namespace tvsEquipmentLink {

struct Pt {double x,y;Pt():x(0),y(0){}Pt(double px,double py):x(px),y(py){}};
inline Pt add(const Pt& p,const Pt& q){return Pt(p.x+q.x,p.y+q.y);}
inline Pt sub(const Pt& p,const Pt& q){return Pt(p.x-q.x,p.y-q.y);}
inline Pt mul(const Pt& p,double k){return Pt(p.x*k,p.y*k);}
inline double dot(const Pt& p,const Pt& q){return p.x*q.x+p.y*q.y;}
inline double cross(const Pt& p,const Pt& q){return p.x*q.y-p.y*q.x;}
inline double length(const Pt& p){return std::sqrt(dot(p,p));}

struct Duct {
    Pt p0,p1;          // axis ends in plan: FirstPoint, LastPoint
    double a,b;        // mm; round: a = D, b = 0
    bool round,flex;   // flex: flex crijevo (its end may carry a terminal; never cut)
    double level;      // axis level (TVS Elev)
    int others[2];     // other network elements touching end 0 / 1 (this block not counted)
    int partner[2];    // input index of a duct touching end 0 / 1 (-1: none)
    Duct():a(0),b(0),round(false),flex(false),level(0){others[0]=others[1]=0;partner[0]=partner[1]=-1;}
};
// Free vertical outlet of an elbow Gore / Dolje (TVS_TAP Form_Up / Form_Down) that nothing else
// touches (the command checks that): centre in plan, outlet level, size of the elbow.
struct Outlet {Pt at;double level;double a,b;bool round;Outlet():level(0),a(0),b(0),round(false){}};
enum Expect {Any=0,Terminal=1,Inline=2};
struct Options {
    double tol;         // duct end on a block face / same level (drawing units)
    double axisTol;     // block centre from the duct axis
    double gap;         // largest gap closed by extending a duct end
    double minPiece;    // shortest duct piece left after a trim / cut
    double fixedLength; // > 0: block length along the axis (stored per block name)
    double sizeTolMm;   // same duct size on both sides
    int expect;         // Expect: from the kind stored for the block name (Any when none)
    bool bottom;        // a terminal over a continuous duct is a grille in its bottom face (else SideMounted)
    Options():tol(1),axisTol(25),gap(150),minPiece(50),fixedLength(0),sizeTolMm(1),expect(Any),bottom(false){}
};
enum Error {Ok=0,BadBlock,NoDuct,OffAxis,Ambiguous,DifferentSize,DifferentLevel,GapTooLarge,TooShort,
            DuctInside,NotFreeEnd,SideMounted,MissingSide,FlexCut,FlexBottom,errorCount};
enum Case {Touching=1,Adjust=2,Cut=3};
const int NewPiece=-2;   // Result::duct of the port on the piece made by a cut
struct EndEdit {int duct;int end;Pt to;double change;EndEdit():duct(-1),end(0),change(0){}};   // change > 0 extended, < 0 trimmed
struct Result {
    int error;
    int ports;            // 1 terminal, 2 inline
    int linkCase;         // Case
    double ux,uy;         // axis direction, from port[0] towards port[1]
    double length;        // block length along the axis
    tvsEquipment::Port port[2];
    int duct[2];          // input index of the duct at each port (NewPiece: the cut piece)
    std::vector<EndEdit> edits;
    int cutDuct;Pt cutKeep,cutNew;
    double a,b;bool round;double level;
    double offset;        // block centre from the axis
    int detailDuct;       // the duct a refusal is about (-1)
    double detail;        // the number behind a refusal (gap / piece length / offset)
    int mount;            // tvsEquipment::AtEnd / Bottom (terminals)
    int outlet;           // input index of the elbow outlet at the port (-1: a duct end / none)
    bool twin;            // terminal with 2 connections on the box (linijski difuzor): ports = 2, both free ends
    Result():error(Ok),ports(0),linkCase(0),ux(1),uy(0),length(0),cutDuct(-1),a(0),b(0),round(false),level(0),
             offset(0),detailDuct(-1),detail(0),mount(tvsEquipment::AtEnd),outlet(-1),twin(false){duct[0]=duct[1]=-1;}
};

inline const wchar_t* reasonText(int e){
    switch(e){
    case BadBlock:return L"blok nema du\u017einu du\u017e ose kanala";
    case NoDuct:return L"osa kanala ne prolazi kroz blok";
    case OffAxis:return L"osa kanala je udaljena od sredine bloka vi\u0161e od tolerancije";
    case Ambiguous:return L"vi\u0161e kanala kod bloka - veza nije jednozna\u010dna";
    case DifferentSize:return L"kanali sa dvije strane bloka nisu iste dimenzije";
    case DifferentLevel:return L"kanali sa dvije strane bloka nisu na istoj koti";
    case GapTooLarge:return L"razmak izme\u0111u bloka i kanala je ve\u0107i od dozvoljenog";
    case TooShort:return L"komad kanala bi ostao prekratak";
    case DuctInside:return L"kanal je cijeli ispod bloka";
    case NotFreeEnd:return L"kraj kanala je spojen sa drugim elementom";
    case SideMounted:return L"terminal nije na slobodnom kraju kanala (bo\u010dna ugradnja - kasnije)";
    case MissingSide:return L"oprema u liniji nema kanal sa druge strane";
    case FlexCut:return L"flex crijevo se ne sije\u010de";
    case FlexBottom:return L"re\u0161etka na dnu flex crijeva nije podr\u017eana";
    default:return L"";
    }
}

namespace detail {
struct Piece {int i;double lo,hi;int loEnd;double len;};   // interval of a duct along the axis
inline Pt onLine(const Duct& d,const Pt& q){   // q projected on the duct's own axis line
    const Pt v=sub(d.p1,d.p0);const double l=dot(v,v);if(l<=0)return d.p0;
    return add(d.p0,mul(v,dot(sub(q,d.p0),v)/l));
}
inline bool sameSize(const Duct& p,const Duct& q,double t){
    if(p.round!=q.round)return false;
    if(std::fabs(p.a-q.a)>t)return false;
    return p.round||std::fabs(p.b-q.b)<=t;
}
}

// block: footprint corners in plan (>= 3, convex: the block-local bounding box in WCS).
inline Result link(const std::vector<Pt>& block,const std::vector<Duct>& ducts,const Options& o){
    using detail::Piece;
    Result r;
    if(block.size()<3){r.error=BadBlock;return r;}
    // ---- candidates: the duct axis line passes through the block, the duct is near it ----
    struct Cand {int i;Pt u;double off,dist;};
    std::vector<Cand> cand;
    for(size_t i=0;i<ducts.size();++i){
        const Duct& d=ducts[i];const Pt v=sub(d.p1,d.p0);const double len=length(v);
        if(len<=o.tol)continue;
        const Pt u=mul(v,1/len),n(-u.y,u.x);
        double s0=0,s1=0,t0=0,t1=0;
        for(size_t k=0;k<block.size();++k){const Pt q=sub(block[k],d.p0);const double s=dot(q,u),t=dot(q,n);
            if(k==0||s<s0){s0=s;}if(k==0||s>s1){s1=s;}if(k==0||t<t0){t0=t;}if(k==0||t>t1){t1=t;}}
        if(t0>o.tol||t1<-o.tol)continue;   // the axis line misses the block
        double dist=0;if(s0-len>dist){dist=s0-len;}if(-s1>dist){dist=-s1;}
        const double reach=4*o.gap>s1-s0?4*o.gap:s1-s0;
        if(dist>reach)continue;
        Cand c;c.i=static_cast<int>(i);c.u=u;c.off=(t0+t1)/2;c.dist=dist;cand.push_back(c);
    }
    if(cand.empty()){r.error=NoDuct;return r;}
    size_t best=0;
    for(size_t k=1;k<cand.size();++k){
        const double a=std::fabs(cand[k].off),b=std::fabs(cand[best].off);
        if(a<b-o.tol||(a<=b+o.tol&&cand[k].dist<cand[best].dist))best=k;}
    const Cand& B=cand[best];
    r.offset=std::fabs(B.off);
    if(r.offset>o.axisTol){r.error=OffAxis;r.detailDuct=B.i;r.detail=r.offset;return r;}
    const Pt O=ducts[B.i].p0;
    Pt u=B.u;if(u.x<-1e-9||(std::fabs(u.x)<=1e-9&&u.y<0))u=mul(u,-1);   // port[0] on the -x (-y) side
    std::vector<int> line;   // collinear ducts (the axis)
    for(size_t k=0;k<cand.size();++k){
        const bool parallel=std::fabs(cross(cand[k].u,u))<=0.01;
        if(parallel){if(std::fabs(cand[k].off)<=o.axisTol)line.push_back(cand[k].i);}
        else if(std::fabs(cand[k].off)<=o.axisTol&&cand[k].dist<=o.gap){r.error=Ambiguous;r.detailDuct=cand[k].i;return r;}
    }
    // ---- block faces along the axis ----
    double S0=0,S1=0;
    for(size_t k=0;k<block.size();++k){const double s=dot(sub(block[k],O),u);if(k==0||s<S0){S0=s;}if(k==0||s>S1){S1=s;}}
    double F0=S0,F1=S1;
    if(o.fixedLength>0){const double m=(S0+S1)/2;F0=m-o.fixedLength/2;F1=m+o.fixedLength/2;}
    if(F1-F0<=o.tol){r.error=BadBlock;return r;}
    r.length=F1-F0;r.ux=u.x;r.uy=u.y;
    const Pt face0=add(O,mul(u,F0)),face1=add(O,mul(u,F1));
    // ---- classify ----
    std::vector<Piece> spans,lefts,rights;int inside=-1;double farLeft=-1,farRight=-1;int farLeftDuct=-1,farRightDuct=-1;
    for(size_t k=0;k<line.size();++k){
        const int i=line[k];const Duct& d=ducts[i];
        const double A0=dot(sub(d.p0,O),u),A1=dot(sub(d.p1,O),u);
        Piece p;p.i=i;p.loEnd=A0<=A1?0:1;p.lo=A0<=A1?A0:A1;p.hi=A0<=A1?A1:A0;p.len=p.hi-p.lo;
        const double outL=F0-p.lo,outR=p.hi-F1;
        if(outL>o.tol&&outR>o.tol)spans.push_back(p);
        else if(outL>o.tol){
            if(p.hi<F0-o.gap){const double g=F0-p.hi;if(farLeft<0||g<farLeft){farLeft=g;farLeftDuct=i;}}
            else lefts.push_back(p);}
        else if(outR>o.tol){
            if(p.lo>F1+o.gap){const double g=p.lo-F1;if(farRight<0||g<farRight){farRight=g;farRightDuct=i;}}
            else rights.push_back(p);}
        else inside=i;
    }
    if(inside>=0){r.error=DuctInside;r.detailDuct=inside;return r;}
    r.duct[0]=r.duct[1]=-1;
    // ---- case 3: one continuous duct under the block ----
    if(!spans.empty()){
        if(spans.size()>1){r.error=Ambiguous;r.detailDuct=spans[1].i;return r;}
        for(size_t k=0;k<lefts.size();++k)if(lefts[k].hi>F0+o.tol){r.error=Ambiguous;r.detailDuct=lefts[k].i;return r;}
        for(size_t k=0;k<rights.size();++k)if(rights[k].lo<F1-o.tol){r.error=Ambiguous;r.detailDuct=rights[k].i;return r;}
        const Piece& p=spans[0];const Duct& d=ducts[p.i];
        if(o.expect==Terminal&&!o.bottom){r.error=SideMounted;r.detailDuct=p.i;return r;}
        if(o.expect==Terminal){   // grille in the bottom face: tap under the block centre, no cut
            if(d.flex){r.error=FlexBottom;r.detailDuct=p.i;return r;}
            Pt c;for(size_t k=0;k<block.size();++k)c=add(c,block[k]);c=mul(c,1.0/block.size());
            const Pt q=detail::onLine(d,c);
            r.linkCase=Touching;r.mount=tvsEquipment::Bottom;r.ports=1;r.duct[0]=p.i;r.duct[1]=-1;
            r.port[0].x=q.x;r.port[0].y=q.y;r.port[0].z=d.level;r.port[0].dx=0;r.port[0].dy=0;
            r.a=d.a;r.b=d.round?0:d.b;r.round=d.round;r.level=d.level;
            return r;
        }
        if(d.flex){r.error=FlexCut;r.detailDuct=p.i;return r;}
        const double outL=F0-p.lo,outR=p.hi-F1;
        if(outL<o.minPiece||outR<o.minPiece){r.error=TooShort;r.detailDuct=p.i;r.detail=outL<outR?outL:outR;return r;}
        const Pt q0=detail::onLine(d,face0),q1=detail::onLine(d,face1);
        r.linkCase=Cut;r.ports=2;r.cutDuct=p.i;
        if(p.loEnd==0){r.cutKeep=q0;r.cutNew=q1;r.duct[0]=p.i;r.duct[1]=NewPiece;}
        else{r.cutKeep=q1;r.cutNew=q0;r.duct[0]=NewPiece;r.duct[1]=p.i;}
        r.port[0].x=q0.x;r.port[0].y=q0.y;r.port[1].x=q1.x;r.port[1].y=q1.y;
        r.port[0].z=r.port[1].z=d.level;
        r.port[0].dx=-u.x;r.port[0].dy=-u.y;r.port[1].dx=u.x;r.port[1].dy=u.y;
        r.a=d.a;r.b=d.round?0:d.b;r.round=d.round;r.level=d.level;
        return r;
    }
    if(lefts.size()>1){r.error=Ambiguous;r.detailDuct=lefts[1].i;return r;}
    if(rights.size()>1){r.error=Ambiguous;r.detailDuct=rights[1].i;return r;}
    const bool hasL=!lefts.empty(),hasR=!rights.empty();
    if(!hasL&&!hasR){
        if(farLeftDuct>=0||farRightDuct>=0){r.error=GapTooLarge;
            const bool l=farLeftDuct>=0&&(farRightDuct<0||farLeft<=farRight);
            r.detailDuct=l?farLeftDuct:farRightDuct;r.detail=l?farLeft:farRight;return r;}
        r.error=NoDuct;return r;
    }
    if(hasL&&hasR){if(o.expect==Terminal){r.error=SideMounted;r.detailDuct=lefts[0].i;return r;}}
    else if(o.expect!=Terminal){
        const int farDuct=hasL?farRightDuct:farLeftDuct;
        if(farDuct>=0){r.error=GapTooLarge;r.detailDuct=farDuct;r.detail=hasL?farRight:farLeft;return r;}
        if(o.expect==Inline){r.error=MissingSide;r.detailDuct=hasL?lefts[0].i:rights[0].i;return r;}
    }
    if(hasL&&hasR){
        const Duct& dl=ducts[lefts[0].i];const Duct& dr=ducts[rights[0].i];
        if(!detail::sameSize(dl,dr,o.sizeTolMm)){r.error=DifferentSize;r.detailDuct=rights[0].i;return r;}
        if(std::fabs(dl.level-dr.level)>o.tol){r.error=DifferentLevel;r.detailDuct=rights[0].i;return r;}
    }
    // ---- case 1 / 2: the near end of each piece goes to its face ----
    const int sides=(hasL?1:0)+(hasR?1:0);
    int slot=0;
    for(int side=0;side<2;++side){
        if((side==0&&!hasL)||(side==1&&!hasR))continue;
        const Piece& p=side==0?lefts[0]:rights[0];const Duct& d=ducts[p.i];
        const int end=side==0?1-p.loEnd:p.loEnd;   // the end at the block
        const double change=side==0?F0-p.hi:p.lo-F1;   // > 0 gap (extend), < 0 overlap (trim)
        const int other=sides==2?(side==0?rights[0].i:lefts[0].i):-1;
        const int joined=d.others[end];
        if(joined>1||(joined==1&&(other<0||d.partner[end]!=other))){r.error=NotFreeEnd;r.detailDuct=p.i;return r;}
        if(change>o.gap+o.tol){r.error=GapTooLarge;r.detailDuct=p.i;r.detail=change;return r;}
        if(p.len+change<o.minPiece){r.error=TooShort;r.detailDuct=p.i;r.detail=p.len+change;return r;}
        Pt at=end==0?d.p0:d.p1;
        if(std::fabs(change)>o.tol){
            EndEdit e;e.duct=p.i;e.end=end;e.to=detail::onLine(d,side==0?face0:face1);e.change=change;
            r.edits.push_back(e);at=e.to;}
        tvsEquipment::Port& q=r.port[slot];
        q.x=at.x;q.y=at.y;q.z=d.level;q.dx=side==0?-u.x:u.x;q.dy=side==0?-u.y:u.y;
        r.duct[slot]=p.i;++slot;
        if(slot==1){r.a=d.a;r.b=d.round?0:d.b;r.round=d.round;r.level=d.level;}
    }
    r.ports=sides;
    r.linkCase=r.edits.empty()?Touching:Adjust;
    if(sides==1){r.duct[1]=-1;}
    return r;
}
// Size of the footprint (side 0-1 x side 0-3: the block-local x / y extents in the drawing, scale
// included) in mm, rounded to 1 mm - the neck of a bottom grille when nothing else gives it.
inline void footprintMm(const std::vector<Pt>& block,double mmPerUnit,double& width,double& depth){
    width=depth=0;if(block.size()<4)return;
    width=std::floor(length(sub(block[1],block[0]))*mmPerUnit+0.5);depth=std::floor(length(sub(block[3],block[0]))*mmPerUnit+0.5);
}
// The base point rule first (see the top): `base` = the block's insertion point in plan, `outlets` =
// free vertical elbow outlets. Not for Inline; for Any the extents decide when they find ducts on
// both sides (an inline block whose base point happens to be on a duct end stays inline).
// Otherwise (base point not on a free end) the extents logic above.
// Point inside the convex footprint, or within tol of it.
inline bool insideBlock(const std::vector<Pt>& block,const Pt& q,double tol){
    if(block.size()<3)return false;
    double sign=0;
    for(size_t k=0;k<block.size();++k){const Pt& a=block[k];const Pt& b=block[(k+1)%block.size()];
        const double l=length(sub(b,a));if(l<=0)continue;const double c=cross(sub(b,a),sub(q,a))/l;   // signed distance from the edge
        if(sign==0&&std::fabs(c)>tol)sign=c>0?1:-1;
        if(sign!=0&&c*sign<-tol)return false;}
    return true;
}
// Linijski difuzor (user 2026-11-24): exactly two free duct / flex ends inside the block footprint
// (the spigots on its plenum box) -> a terminal with 2 connections (twin). For a block of an unknown
// kind (Any) an inline link through the block wins (a damper between two ducts); two ducts on one
// axis facing each other are never a twin (found = -2: the rules below decide).
inline Result twinLink(const std::vector<Pt>& block,const std::vector<Duct>& ducts,const Options& o,int& found){
    Result t;found=0;
    for(size_t i=0;i<ducts.size();++i)for(int e=0;e<2;++e){
        const Duct& d=ducts[i];if(d.others[e]!=0)continue;
        const Pt at=e==0?d.p0:d.p1;if(!insideBlock(block,at,o.tol))continue;
        const Pt v=e==0?sub(d.p0,d.p1):sub(d.p1,d.p0);const double l=length(v);if(l<=o.tol)continue;
        if(found<2){tvsEquipment::Port& p=t.port[found];p.x=at.x;p.y=at.y;p.z=d.level;p.dx=-v.x/l;p.dy=-v.y/l;t.duct[found]=static_cast<int>(i);
            if(found==0){t.a=d.a;t.b=d.round?0:d.b;t.round=d.round;t.level=d.level;t.ux=v.x/l;t.uy=v.y/l;}}
        ++found;
    }
    if(found!=2)return t;
    // two ducts on one axis facing each other (block between them): not a box with two spigots
    const Pt u0(t.port[0].dx,t.port[0].dy),u1(t.port[1].dx,t.port[1].dy),gap=sub(Pt(t.port[1].x,t.port[1].y),Pt(t.port[0].x,t.port[0].y));
    if(length(gap)<=o.tol||(dot(u0,u1)<-0.9&&std::fabs(cross(u0,gap))<=o.axisTol)){found=-2;return t;}   // one point / one axis
    t.error=Ok;t.ports=2;t.twin=true;t.linkCase=Touching;t.mount=tvsEquipment::AtEnd;t.length=0;t.detailDuct=-1;
    return t;
}
inline Result link(const std::vector<Pt>& block,const Pt& base,const std::vector<Duct>& ducts,
                   const std::vector<Outlet>& outlets,const Options& o){
    if(o.expect!=Inline){
        int ends=0;const Result w=twinLink(block,ducts,o,ends);
        if(ends==2){
            if(o.expect==Any){const Result x=link(block,ducts,o);if(x.error==Ok&&x.ports==2)return x;}
            return w;
        }
    }
    if(o.expect!=Inline){
        Result t;int found=0;
        for(size_t i=0;i<ducts.size();++i)for(int e=0;e<2;++e){
            const Duct& d=ducts[i];if(d.others[e]!=0)continue;
            const Pt at=e==0?d.p0:d.p1;if(length(sub(at,base))>o.tol)continue;
            const Pt v=e==0?sub(d.p0,d.p1):sub(d.p1,d.p0);const double l=length(v);if(l<=o.tol)continue;
            ++found;t.detailDuct=static_cast<int>(i);
            t.port[0].x=at.x;t.port[0].y=at.y;t.port[0].z=d.level;t.port[0].dx=-v.x/l;t.port[0].dy=-v.y/l;   // out of the block
            t.ux=v.x/l;t.uy=v.y/l;t.duct[0]=static_cast<int>(i);t.outlet=-1;
            t.a=d.a;t.b=d.round?0:d.b;t.round=d.round;t.level=d.level;
        }
        for(size_t k=0;k<outlets.size();++k){
            const Outlet& w=outlets[k];if(length(sub(w.at,base))>o.tol)continue;
            ++found;
            t.port[0].x=w.at.x;t.port[0].y=w.at.y;t.port[0].z=w.level;t.port[0].dx=0;t.port[0].dy=0;   // vertical
            t.ux=1;t.uy=0;t.duct[0]=-1;t.outlet=static_cast<int>(k);
            t.a=w.a;t.b=w.round?0:w.b;t.round=w.round;t.level=w.level;
        }
        if(found>1){Result r;r.error=Ambiguous;r.detailDuct=t.detailDuct;return r;}
        if(found==1){
            if(o.expect==Any){const Result x=link(block,ducts,o);if(x.error==Ok&&x.ports==2)return x;}
            t.error=Ok;t.ports=1;t.linkCase=Touching;t.mount=tvsEquipment::AtEnd;t.length=0;t.duct[1]=-1;t.detailDuct=-1;
            return t;
        }
    }
    return link(block,ducts,o);
}
}
