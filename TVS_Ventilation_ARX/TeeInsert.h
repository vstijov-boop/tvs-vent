#pragma once
// T komad faza 2: TK-O / TK-G inserted in the duct being drawn (TVS_DRAW option "Tkomad").
// The cursor picks the inlet face on the line of the current duct (ahead: the duct is
// extended, behind: shortened) and, for TK-O, the side of the branch. A preview of the
// fitting and of the duct follows the cursor. Geometry only; the command glue (creating the
// TVS entities with the drawing properties) is in acrxEntryPoint.cpp.
#include "../geometry/TeeShape.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include <dbjig.h>
#include <acgi.h>
#include <cmath>
namespace tvsTeeInsert {
struct Input {
    AcGePoint3d start,end;   // current duct (plan direction start -> end)
    double A,B;              // duct width / height
    int type;                // tvsTee::Branch or tvsTee::Pants
    double w1,second,l;      // TK-O: branch w1, outlet C; TK-G: right w1, left second; collar l
    double minimum;          // shortest duct that stays before the T komad
};
struct Plan {
    bool ok;int side;double t;
    AcGePoint3d face;AcGeVector3d dir;
    // TVS_WYE parameters
    double SizeApr,SizeAotv,LengthPl;AcGeVector3d Vectpr,Vectotv;AcGePoint3d Basepoint;
    // drawn frame and shape (TeeShape local frame: origin = inlet face, ey = flow, ex = branch side)
    tvsTee::Tee tee;AcGeVector3d ex,ey;
    // continuation ports: 0 = TK-O run outlet / TK-G right, 1 = TK-O branch / TK-G left
    AcGePoint3d port[2];AcGeVector3d portDir[2];double portWidth[2];
    Plan():ok(false),side(1),t(0),SizeApr(0),SizeAotv(0),LengthPl(0){for(int k=0;k<2;++k)portWidth[k]=0;}
};
inline bool plan(const Input& in,const AcGePoint3d& cursor,Plan& p){
    p=Plan();
    AcGeVector3d d=in.end-in.start;d.z=0;if(d.length()<1e-9)return false;d.normalize();p.dir=d;
    AcGeVector3d c=cursor-in.start;c.z=0;
    p.t=c.dotProduct(d);if(p.t<in.minimum)p.t=in.minimum;
    p.face=in.start+d*p.t;p.face.z=in.end.z;
    const AcGeVector3d left(-d.y,d.x,0),right(d.y,-d.x,0);
    tvsTee::Tee& t=p.tee;t.type=in.type;t.A=in.A;t.l=in.l;t.flange=0;
    if(in.type==tvsTee::Branch){
        p.side=c.dotProduct(left)>=0?1:-1;const AcGeVector3d n=p.side>0?left:right;
        t.w1=in.w1;t.C=in.second>0?in.second:in.A;t.R1=tvsTee::defaultRadius(in.w1);
        p.SizeApr=in.A;p.SizeAotv=in.w1;p.LengthPl=tvsTee::lengthPl(in.l,t.R1,in.w1);
        p.Vectpr=d;p.Vectotv=n;p.Basepoint=p.face+d*(p.LengthPl+in.w1/2);
        p.ey=d;p.ex=n;
        p.port[0]=p.face+d*(2*p.LengthPl+in.w1);p.portDir[0]=d;p.portWidth[0]=t.C;
        p.port[1]=p.Basepoint+n*(in.A/2+p.LengthPl);p.portDir[1]=n;p.portWidth[1]=in.w1;
    }else{
        t.w1=in.w1;t.w2=in.second>0?in.second:in.w1;t.R1=tvsTee::defaultRadius(t.w1);t.R2=tvsTee::defaultRadius(t.w2);
        p.SizeApr=t.w1;p.SizeAotv=in.A;p.LengthPl=tvsTee::pantsLengthPl(in.l,t.w1,t.w2);
        p.Vectpr=right;p.Vectotv=-d;p.Basepoint=p.face+d*(t.w1/2+p.LengthPl);
        p.ey=d;p.ex=right;
        p.port[0]=p.Basepoint+right*(in.A/2+t.R1-t.w1+in.l);p.portDir[0]=right;p.portWidth[0]=t.w1;
        p.port[1]=p.Basepoint-right*(in.A/2+t.R2-t.w2+in.l);p.portDir[1]=-right;p.portWidth[1]=t.w2;
    }
    p.Basepoint.z=p.face.z;for(int k=0;k<2;++k)p.port[k].z=p.face.z;
    tvsTee::Shape s;p.ok=tvsTee::shape(t,s);
    return p.ok;
}
// Preview: the fitting outline and the duct from its start to the inlet face.
class Preview : public AcDbEntity {
public:
    Input in;Plan p;bool valid;
    Preview():valid(false){}
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* draw){
        draw->subEntityTraits().setColor(static_cast<Adesk::UInt16>(valid?3:1));
        const AcGeVector3d side(-p.dir.y*in.A/2,p.dir.x*in.A/2,0);
        AcGePoint3d a[2]={in.start+side,p.face+side},b[2]={in.start-side,p.face-side};
        draw->geometry().polyline(2,a);draw->geometry().polyline(2,b);
        tvsTee::Shape s;if(!valid||!tvsTee::shape(p.tee,s))return Adesk::kTrue;
        auto world=[&](const tvsTee::P& q){AcGePoint3d w=p.face+p.ex*q.x+p.ey*q.y;w.z=p.face.z;return w;};
        for(size_t j=0;j<s.lines.size();++j){AcGePoint3d l[2]={world(s.lines[j].a),world(s.lines[j].b)};draw->geometry().polyline(2,l);}
        for(size_t j=0;j<s.arcs.size();++j){
            const tvsTee::Arc& c=s.arcs[j];
            const AcGeVector3d v0=p.ex*std::cos(c.a0)+p.ey*std::sin(c.a0);
            const double ccw=p.ex.crossProduct(p.ey).z>0?1:-1;
            draw->geometry().circularArc(world(c.c),c.r,AcGeVector3d(0,0,ccw),v0,c.a1-c.a0);
        }
        return Adesk::kTrue;
    }
};
class Jig : public AcEdJig {
    Preview preview;AcGePoint3d last;bool sampled;
public:
    explicit Jig(const Input& in):sampled(false){
        preview.in=in;last=in.end;preview.valid=plan(in,last,preview.p);
        setDispPrompt(in.type==tvsTee::Branch?L"\nPolozaj T komada na kanalu (strana kursora = strana ogranka; Esc: odustani): "
                                              :L"\nPolozaj gacastog T komada na kanalu (Esc: odustani): ");
        setUserInputControls(static_cast<UserInputControls>(kAccept3dCoordinates|kGovernedByOrthoMode));
    }
    virtual AcDbEntity* entity()const{return const_cast<Preview*>(&preview);}
    virtual DragStatus sampler(){
        AcGePoint3d next;const DragStatus status=acquirePoint(next);
        if(status!=kNormal)return status;
        if(sampled&&next.distanceTo(last)<1e-9)return kNoChange;
        sampled=true;last=next;return kNormal;
    }
    virtual Adesk::Boolean update(){preview.valid=plan(preview.in,last,preview.p);return Adesk::kTrue;}
    const Plan& result()const{return preview.p;}
    bool valid()const{return preview.valid;}
};
}
