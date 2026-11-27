#pragma once
// TVS_RESETKA (prikljucenje resetke): pure plan geometry of connecting a grille block to a duct,
// no AutoCAD, ASCII; tests/test_grille_connect.cpp. The command is
// TVS_Ventilation_ARX/GrilleCommand.h (dialog IDD_GRILLE).
//  - Lijevo / Desno (side of the duct axis seen in the drawing direction FirstPoint -> LastPoint):
//    tap point = projection of the block insertion point on the duct axis; a TVS T komad on the
//    duct there (ravni TK0, stucna SR5-3 / SR5-11 or konusna stucna SR5-12) with the branch to
//    the chosen side, a straight stub duct of the given length from the branch port outward, the
//    grille block moved and turned so its BASE POINT is at the free end of the stub.
//  - Dno (grille in the bottom face, drawn over the duct): nothing is drawn; port = projection of
//    the block centre on the axis (mount Bottom, geometry/Equipment.h).
// Block orientation (user's examples, p.dxf: Res_2 on the side with the branch to -y has rotation
// 0, Res_2 at the end of a duct drawn to +x has rotation 90; its extents are local y -10..60, i.e.
// the block lies on the stub): the block's local -y axis points OUTWARD (away from the duct), local
// +y into the stub. Unmirrored block in plan: rotation = atan2(ox, -oy) for the outward unit
// vector o. The command turns the block by turnAngle(current WCS direction of local -y, o), so a
// mirrored / scaled block follows the same rule.
// Units: points, sizes and lengths in drawing units (the command converts the dialog mm).
#include "TeeShape.h"
#include <cmath>
namespace tvsGrille {

struct V {double x,y;V():x(0),y(0){}V(double a,double b):x(a),y(b){}};
inline V add(const V& p,const V& q){return V(p.x+q.x,p.y+q.y);}
inline V sub(const V& p,const V& q){return V(p.x-q.x,p.y-q.y);}
inline V mul(const V& p,double k){return V(p.x*k,p.y*k);}
inline double dot(const V& p,const V& q){return p.x*q.x+p.y*q.y;}
inline double cross(const V& p,const V& q){return p.x*q.y-p.y*q.x;}
inline double length(const V& p){return std::sqrt(dot(p,p));}

// End (user 2026-11-23): grille on a FREE end of the duct (after a transition, a short duct of
// 50 / 100 mm - the grille's damper - is enough): no T komad; the duct end may be set to a given
// length from its other end, the block goes on the end facing outward.
enum Side {Left=0,Right=1,Bottom=2,End=3};
enum Method {Tee=0,Tap=1,Cone=2,Direct=3};   // ravni T komad TK0, stucna, konusna stucna, direktno na stranicu

struct Duct {
    V p0,p1;          // axis in plan: FirstPoint, LastPoint
    double a,b;       // width, height; round: a = D, b = 0
    bool round;
    bool freeEnd[2];  // nothing else on p0 / p1: the T komad may sit at that end (outlet left free = cap)
    Duct():a(0),b(0),round(false){freeEnd[0]=freeEnd[1]=false;}
    double len()const{return length(sub(p1,p0));}
    double height()const{return round?a:b;}   // the side face the branch enters
    double width()const{return a;}            // the bottom face (plan width)
};
// Where a plan point lies against the duct axis.
struct Place {
    bool ok;
    V u,left;          // axis direction p0 -> p1, its left normal
    double t;          // projection along the axis from p0
    double offset;     // signed distance from the axis (+ = left)
    V tap;             // projection on the axis (also beyond the duct ends)
    int side;          // suggested Side: Bottom over the duct, else Left / Right of the axis
    Place():ok(false),t(0),offset(0),side(Left){}
};
inline Place place(const Duct& d,const V& at){
    Place p;const double l=d.len();if(!(l>0))return p;
    p.u=mul(sub(d.p1,d.p0),1.0/l);p.left=V(-p.u.y,p.u.x);
    const V r=sub(at,d.p0);p.t=dot(r,p.u);p.offset=dot(r,p.left);
    p.tap=add(d.p0,mul(p.u,p.t));
    const bool over=p.t>=0&&p.t<=l&&std::fabs(p.offset)<d.a/2;
    p.side=over?Bottom:p.offset>=0?Left:Right;p.ok=true;
    return p;
}
// What the dialog chose; sizes in drawing units.
struct Choice {
    int side,method;
    double stub;      // straight part of the branch (Lijevo / Desno)
    double a,b;bool round;   // branch / neck size; round: a = D
    int end;          // End: 0 = FirstPoint, 1 = LastPoint; stub = duct length to the grille (0 = keep)
    int aAcross;      // Dno: 1 = side a lies across the duct (the block turned so), 0 = along, -1 unknown
                      // (the smaller side across); user 2026-11-27: 489 x 190 grille, 489 across a 650 duct
    Choice():side(Left),method(Tee),stub(0),a(0),b(0),round(false),end(-1),aAcross(-1){}
};
struct Options {
    double collar;    // TK0: TVS_WYE LengthPl (TVS_SETTINGS T komad length)
    double tapMinimum;// stucna: L = 0.25 w, at least this (75 mm)
    double coneLength;// konusna stucna: L (100 mm)
    double margin;    // stucna run beyond its footprint (25 mm, record radius2)
    double minPiece;  // shortest duct piece left on each side of the T komad
    Options():collar(50),tapMinimum(75),coneLength(100),margin(25),minPiece(50){}
};
// Split: the duct is cut at the T komad (original up to runStart, new piece from runEnd).
// AtLast / AtFirst (user 2026-11-23, the typical case of p.dxf): the projection is closer than
// half the run + minPiece to a FREE end of the duct -> the T komad sits at that end, its branch
// axis on the projection; the duct is trimmed (or extended) to the T komad inlet, no new piece,
// the straight outlet of the T komad stays free (the network treats a free tee port as a cap). The
// run may reach beyond the original duct end (p.dxf: duct end 17603, T komad 17603-18153, branch
// axis 17878). Refused: the short end is not free, or both ends are short.
enum Fit {Refused=-1,Split=0,AtLast=1,AtFirst=2};
// The T komad, stub and block of Lijevo / Desno.
struct Layout {
    bool ok;
    V outward;        // unit, from the axis towards the chosen side
    V tap;            // T komad Basepoint (on the branch axis)
    V runStart,runEnd;// run ends of the T komad on the axis (upstream / downstream in drawing direction)
    V branchPort;     // branch port of the T komad = stub start
    V stubEnd;        // free end of the stub = block base point = grille port
    double lengthPl;  // TVS_WYE LengthPl (TK0 collar, stucna L)
    double before,after;   // run from the branch axis, upstream / downstream
    double cone;      // konusna: cone base diameter (TVS_TEE_V1 second), else 0
    int teeType;      // tvsTee::Straight / tvsTee::Tap
    double rotation;  // block rotation for an unmirrored block (radians)
    int fit;          // Fit: how the duct meets the T komad
    Layout():ok(false),lengthPl(0),before(0),after(0),cone(0),teeType(tvsTee::Straight),rotation(0),fit(Refused){}
};
// Rotation of an unmirrored block so that its local -y points along o (outward).
inline double blockRotation(const V& o){return std::atan2(o.x,-o.y);}
// Turn (radians, CCW) that takes the plan direction c (the block's local -y now, in WCS) to o.
inline double turnAngle(const V& c,const V& o){return std::atan2(cross(c,o),dot(c,o));}
inline Layout layout(const Duct& d,const Place& p,const Choice& c,const Options& o){
    Layout r;if(!p.ok||c.side==Bottom||c.method==Direct)return r;
    r.outward=c.side==Left?p.left:mul(p.left,-1);r.tap=p.tap;
    const double w=c.a;
    if(c.method==Tee){r.teeType=tvsTee::Straight;r.lengthPl=o.collar;r.before=r.after=o.collar+w/2;}
    else{
        r.teeType=tvsTee::Tap;
        tvsTee::Tee t;t.type=tvsTee::Tap;t.A=d.a;t.C=d.a;t.w1=w;t.round=c.round;t.margin=o.margin;
        if(c.method==Cone){t.l=o.coneLength;t.cone=tvsTee::coneBase(w,t.l);r.cone=t.cone;}
        else t.l=tvsTee::tapLength(w,o.tapMinimum);
        r.lengthPl=t.l;tvsTee::tapRun(t,r.before,r.after);
    }
    r.runStart=sub(p.tap,mul(p.u,r.before));r.runEnd=add(p.tap,mul(p.u,r.after));
    r.branchPort=add(p.tap,mul(r.outward,d.a/2+r.lengthPl));
    r.stubEnd=add(r.branchPort,mul(r.outward,c.stub));
    r.rotation=blockRotation(r.outward);r.ok=true;
    const double l=d.len();
    const bool shortStart=p.t<r.before+o.minPiece-1e-9,shortEnd=l-p.t<r.after+o.minPiece-1e-9;
    r.fit=!shortStart&&!shortEnd?Split:shortEnd&&!shortStart&&d.freeEnd[1]?AtLast:shortStart&&!shortEnd&&d.freeEnd[0]?AtFirst:Refused;
    return r;
}
// The free end nearest to `at` within reach (drawing units); -1 none.
inline int nearFreeEnd(const Duct& d,const V& at,double reach){
    int best=-1;double bd=reach;
    for(int e=0;e<2;++e){if(!d.freeEnd[e])continue;const double x=length(sub(at,e==0?d.p0:d.p1));if(x<=bd){bd=x;best=e;}}
    return best;
}
struct EndLayout {bool ok;V at,outward;double rotation;EndLayout():ok(false),rotation(0){}};
// End: the new end point (length > 0: measured from the other end along the axis) and the outward direction.
inline EndLayout endLayout(const Duct& d,int end,double length){
    EndLayout r;const double l=d.len();if(!(l>0)||end<0||end>1)return r;
    const V u=mul(sub(d.p1,d.p0),1.0/l);r.outward=end==1?u:mul(u,-1);
    const V other=end==1?d.p0:d.p1;r.at=length>0?add(other,mul(r.outward,length)):(end==1?d.p1:d.p0);
    r.rotation=blockRotation(r.outward);r.ok=true;return r;
}
enum Error {Ok=0,NoDuct,BadSize,NoStub,MethodSide,RoundMain,ConeRect,TooTall,TooWide,Outside,NearEnd,NotFreeEnd,errorCount};
// detail (drawing units): TooTall = duct side height, TooWide = duct width, NearEnd = length needed
// on the short side, Outside = how far beyond the duct end.
struct Check {int error;double detail;Check():error(Ok),detail(0){}};
// p: place of the insertion point (Lijevo / Desno) or of the block centre (Dno).
inline Check validate(const Duct& d,const Place& p,const Choice& c,const Options& o){
    Check k;const double l=d.len();
    if(!p.ok||!(l>0)||!(d.a>0)){k.error=NoDuct;return k;}
    if(!(c.a>0)||(!c.round&&!(c.b>0))){k.error=BadSize;return k;}
    if(c.side==End){
        if(c.end<0||c.end>1||!d.freeEnd[c.end]){k.error=NotFreeEnd;return k;}
        if(c.stub<0){k.error=NoStub;return k;}
        return k;
    }
    const bool bottom=c.side==Bottom;
    if(bottom!=(c.method==Direct)){k.error=MethodSide;return k;}
    if(!bottom&&(c.method==Tap||c.method==Cone)&&(d.round||!(d.b>0))){k.error=RoundMain;return k;}
    if(c.method==Cone&&!c.round){k.error=ConeRect;return k;}
    if(bottom&&(p.t<0||p.t>l)){k.error=Outside;k.detail=p.t<0?-p.t:p.t-l;return k;}
    if(bottom){
        // neck in the bottom face: the smaller side across the duct, the larger along it
        double across=c.round?c.a:(c.a<c.b?c.a:c.b),along=c.round?c.a:(c.a<c.b?c.b:c.a);
        if(!c.round&&c.aAcross>=0){across=c.aAcross?c.a:c.b;along=c.aAcross?c.b:c.a;}   // as the block lies
        if(across>d.width()+1e-9){k.error=TooWide;k.detail=d.width();return k;}
        const double need=along/2,shortSide=p.t<l-p.t?p.t:l-p.t;
        if(shortSide+1e-9<need){k.error=NearEnd;k.detail=need;return k;}
        return k;
    }
    if(!(c.stub>0)){k.error=NoStub;return k;}
    const double h=c.method==Cone?tvsTee::coneBase(c.a,o.coneLength):c.round?c.a:c.b;   // konusna: the cone base sits on the side
    if(h>d.height()+1e-9){k.error=TooTall;k.detail=d.height();return k;}
    const Layout y=layout(d,p,c,o);
    if(!y.ok){k.error=NoDuct;return k;}
    if(y.fit!=Refused)return k;
    if(p.t<0||p.t>l){k.error=Outside;k.detail=p.t<0?-p.t:p.t-l;return k;}   // beyond an end that is not free
    const double needBefore=y.before+o.minPiece,needAfter=y.after+o.minPiece;
    k.error=NearEnd;k.detail=p.t+1e-9<needBefore?needBefore:needAfter;
    return k;
}
// Messages (BCS, \u escapes keep this file ASCII); the command adds the numbers in mm.
inline const wchar_t* errorText(int e){
    switch(e){
    case NoDuct:return L"kanal nije prepoznat (du\u017eina 0)";
    case BadSize:return L"dimenzija ogranka mora biti ve\u0107a od 0";
    case NoStub:return L"du\u017eina ravnog dijela ogranka mora biti ve\u0107a od 0";
    case MethodSide:return L"na\u010din priklju\u010denja ne odgovara strani (dno: direktno na stranicu; lijevo / desno: T komad ili \u0161tucna)";
    case RoundMain:return L"\u0161tucna je za pravougaoni kanal (okrugli kanal: ravni T komad)";
    case ConeRect:return L"konusna \u0161tucna je samo za okrugli ogranak";
    case TooTall:return L"ogranak je vi\u0161i od bo\u010dne stranice kanala";
    case TooWide:return L"priklju\u010dak re\u0161etke je \u0161iri od dna kanala";
    case Outside:return L"blok nije naspram kanala (projekcija je van kanala)";
    case NearEnd:return L"mjesto priklju\u010denja je preblizu kraju kanala (kraj nije slobodan ili je kanal prekratak)";
    case NotFreeEnd:return L"re\u0161etka nije kod slobodnog kraja kanala (na kraju je drugi element)";
    default:return L"";
    }
}
}
