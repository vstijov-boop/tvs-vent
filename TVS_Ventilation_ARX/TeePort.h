#pragma once
// T komad TK-O / TK-G: the T komad keeps its own sizes; every port has the inlet height B.
// A TVS duct on a port with another size or shape gets a TVS prelaz right at the port (the duct
// is shortened from the T komad side). Used by the TVS_TK_TIP dialog and by the size follow
// (a duct on a TK-O / TK-G port changed: the T komad stays, the prelaz is inserted / adjusted).
// A prelaz already on the port takes the port size on its T komad side; when both of its
// sides become equal it is removed and the duct goes back to the port.
#include "TeeNeighbours.h"
#include "../TVS_Ventilation_DBX/SpiroCatalog.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include <cmath>
#include <vector>
namespace tvsTeePort {
struct Section {double a,b;bool round;Section():a(0),b(0),round(false){}};
inline bool same(const Section& x,const Section& y,double tol){
    return x.round==y.round&&std::fabs(x.a-y.a)<=tol&&(x.round||std::fabs(x.b-y.b)<=tol);
}
struct Port {AcGePoint3d p;AcGeVector3d out;Section s;};
inline int type(const TVS_WYE* w){tvsTeeData::Record r;return tvsTeeData::read(w,r)?r.type:0;}
inline bool keeps(const TVS_WYE* w){const int t=type(w);return t==tvsTee::Branch||t==tvsTee::Pants;}
// Port k (0 run start, 1 run end, 2 branch: TVS order, as tvsTeeFit::ports) with its section.
inline bool port(const TVS_WYE* w,int k,Port& out){
    if(!w||k<0||k>2)return false;const tvsTeeFit::Ports ps=tvsTeeFit::ports(w);if(!ps.ok)return false;
    AcGeVector3d u=w->Vectpr,v=w->Vectotv;u.z=v.z=0;if(u.length()<1e-12||v.length()<1e-12)return false;u.normalize();v.normalize();
    out.p=ps.p[k];out.out=k==0?-u:k==1?u:v;
    const bool run=k<2;out.s.a=ps.w[k];out.s.round=run?w->ThisRoundpr:w->ThisRoundotv;out.s.b=out.s.round?0:(run?w->SizeBpr:w->SizeBotv);
    return true;
}
inline const wchar_t* portName(const TVS_WYE* w,int k){
    const int t=type(w);
    if(t==tvsTee::Pants)return k==2?L"ulaz":k==1?L"ogranak B":L"ogranak C";
    return k==2?L"ogranak B":k==1?L"prolaz C":L"ulaz";
}
// What sits on a port: one TVS duct end, one prelaz end, something else, several or nothing.
struct Neighbour {int kind;AcDbObjectId id;int end;Section s;Neighbour():kind(0),end(-1){}};   // 0 none, 1 duct, 2 prelaz, 3 other, 4 several
inline Neighbour at(const TVS_WYE* w,const AcGePoint3d& p,double tol){
    Neighbour n;int count=0;
    AcDbObjectPointer<AcDbBlockTableRecord> space(w->ownerId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return n;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return n;
    for(;!it->done();it->step()){
        AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id==w->objectId())continue;
        AcDbEntity* e=NULL;if(acdbOpenAcDbEntity(e,id,AcDb::kForRead)!=Acad::eOk)continue;
        tvsMove::Node node;
        if(TVS_Entity::cast(e)&&tvsMove::describe(e,node))
            for(size_t j=0;j<node.ports.size()&&j<node.sections.size();++j)
                if(tvsTeeFit::planDistance(tvsMove::acad(node.ports[j]),p)<=tol){
                    ++count;n.id=id;n.end=static_cast<int>(j);
                    n.s.a=node.sections[j].a;n.s.round=node.sections[j].round;n.s.b=n.s.round?0:node.sections[j].b;
                    const TVS_Pipe* d=TVS_Pipe::cast(e);
                    n.kind=d&&d->DuctType==DuctTypeStill?1:TVS_TRANS::cast(e)?2:3;break;}
        e->close();
    }
    delete it;if(count>1){n.kind=4;n.id=AcDbObjectId::kNull;}
    return n;
}
inline void sizeText(wchar_t* out,size_t n,const Section& s,double f){
    if(s.round)swprintf_s(out,n,L"Ø%.0f",s.a*f);else swprintf_s(out,n,L"%.0fx%.0f",s.a*f,s.b*f);
}
// Prelaz length (drawing units): spiro by catalogue, otherwise TVS_SETTINGS "Prelaz L" (or `lengthMm` > 0).
inline double prelazLength(AcDbDatabase* db,const Section& a,const Section& b,double f,double lengthMm=0){
    const double fallback=(lengthMm>0?lengthMm:tvsTeeFit::transitionMm())/f;
    return tvsSpiro::drawingLength(db,a.a,a.round,b.a,b.round,fallback);
}
// Make port k of the T komad `teeId` match what is attached to it. Returns false only when a
// needed prelaz could not be made (reason printed). lengthMm: rectangular prelaz length (0 = setting).
inline bool ensure(const AcDbObjectId& teeId,int k,double lengthMm=0){
    Port P;double f=1,tol=0;Neighbour n;AcDbDatabase* db=NULL;const wchar_t* name=L"";
    {AcDbObjectPointer<TVS_WYE> w(teeId,AcDb::kForRead);if(w.openStatus()!=Acad::eOk||!port(w.object(),k,P))return false;
     f=tvsUnits::factor(w.object());tol=2.0/f;db=w->database();name=portName(w.object(),k);n=at(w.object(),P.p,tol);}
    wchar_t ps[32],ns[32];sizeText(ps,32,P.s,f);
    if(n.kind==0)return true;
    if(n.kind==4){acutPrintf(L"\nT komad, %ls: više elemenata na priključku; prelaz nije provjeren.",name);return true;}
    sizeText(ns,32,n.s,f);
    if(n.kind==3){if(!same(P.s,n.s,1e-6))acutPrintf(L"\nT komad, %ls (%ls): susjedni element %ls nije kanal ni prelaz; uskladite ručno.",name,ps,ns);return true;}
    if(n.kind==2){   // prelaz on the port: its T komad side takes the port size
        AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)return false;
        const int e=n.end;Section other;other.a=e==0?t->SizeAp2:t->SizeAp1;other.round=e==0?t->ThisRoundp2:t->ThisRoundp1;other.b=other.round?0:(e==0?t->SizeBp2:t->SizeBp1);
        if(same(P.s,n.s,1e-6)&&!same(P.s,other,1e-6))return true;
        if(same(P.s,other,1e-6)){   // no step left: remove it, the duct beyond goes back to the port
            tvsMove::Node tn;if(!tvsMove::describe(t.object(),tn)||tn.ports.size()!=2)return true;
            const AcGePoint3d beyond=tvsMove::acad(tn.ports[1-e]);
            AcDbObjectPointer<TVS_WYE> w(teeId,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return true;
            t.close();const Neighbour d=at(w.object(),beyond,tol);w.close();
            if(d.kind!=1){acutPrintf(L"\nT komad, %ls: prelaz više ne mijenja dimenziju (%ls); uklonite ga ručno.",name,ps);return true;}
            AcDbObjectPointer<TVS_Pipe> p(d.id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)return true;
            AcGePoint3d to=P.p;to.z=(d.end==0?p->FirstPoint:p->LastPoint).z;
            if((d.end==0?p->put_FirstPoint(to):p->put_Lastpoint(to))!=Acad::eOk)return true;p->recordGraphicsModified();p.close();
            AcDbObjectPointer<TVS_TRANS> gone(n.id,AcDb::kForWrite);if(gone.openStatus()==Acad::eOk)gone->erase();
            acutPrintf(L"\nT komad, %ls: prelaz uklonjen, kanal %ls ide do priključka.",name,ps);return true;
        }
        t->assertWriteEnabled();
        if(e==0){t->SizeAp1=P.s.a;t->SizeBp1=P.s.b;t->ThisRoundp1=P.s.round;}else{t->SizeAp2=P.s.a;t->SizeBp2=P.s.b;t->ThisRoundp2=P.s.round;}
        t->recordGraphicsModified();
        acutPrintf(L"\nT komad, %ls: postojeći prelaz preuzima dimenziju priključka %ls.",name,ps);return true;
    }
    // duct on the port
    if(same(P.s,n.s,1e-6))return true;
    AcGePoint3d E,N;double elev=0;bool oneD=false;Section ds=n.s;
    {AcDbObjectPointer<TVS_Pipe> d(n.id,AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return false;
     N=n.end==0?d->FirstPoint:d->LastPoint;E=n.end==0?d->LastPoint:d->FirstPoint;elev=d->Elev;oneD=d->This1D;}
    AcGeVector3d dir=E-N;dir.z=0;const double length=dir.length();if(length<1e-12)return false;dir.normalize();
    if(dir.dotProduct(P.out)<1-1e-6){acutPrintf(L"\nT komad, %ls: kanal %ls nije u pravcu priključka %ls; prelaz nije umetnut.",name,ns,ps);return false;}
    double L=prelazLength(db,P.s,ds,f,lengthMm);
    if(length<L+50.0/f){acutPrintf(L"\nT komad, %ls: kanal je prekratak za prelaz %.0f mm (%ls -> %ls); uskladite ručno.",name,L*f,ps,ns);return false;}
    AcGePoint3d start=P.p;start.z=N.z;AcGePoint3d cut=start+dir*L;
    {AcDbObjectPointer<TVS_Pipe> d(n.id,AcDb::kForWrite);if(d.openStatus()!=Acad::eOk)return false;
     if((n.end==0?d->put_FirstPoint(cut):d->put_Lastpoint(cut))!=Acad::eOk)return false;d->recordGraphicsModified();}
    double a1=P.s.a,b1=P.s.b,a2=ds.a,b2=ds.b;AcGeVector3d v=dir*L;
    TVS_TRANS* t=TVS_TRANS::add_new(a1,b1,a2,b2,L,v,start,P.s.round,ds.round,1,oneD);
    if(!t||t->objectId().isNull()){acutPrintf(L"\nT komad, %ls: prelaz nije napravljen.",name);return false;}
    {AcDbObjectPointer<TVS_TRANS> o(t->objectId(),AcDb::kForWrite);
     if(o.openStatus()==Acad::eOk){AcDbObjectPointer<TVS_Pipe> d(n.id,AcDb::kForRead);if(d.openStatus()==Acad::eOk)o->setPropertiesFrom(d.object());
        o->put_Elevation(elev);o->recordGraphicsModified();}}
    acutPrintf(L"\nT komad, %ls: umetnut prelaz %ls -> %ls mm, dužine %.0f mm.",name,ps,ns,L*f);
    return true;
}
// Size follow: a changed duct with an end on a TK-O / TK-G port gets the prelaz there.
inline void afterDuct(AcDbDatabase* db,const AcDbObjectId& ductId){
    AcGePoint3d ends[2];
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return;ends[0]=d->FirstPoint;ends[1]=d->LastPoint;}
    const double tol=2.0/tvsUnits::factor(db);
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return;
    std::vector<std::pair<AcDbObjectId,int> > found;
    for(;!it->done();it->step()){
        AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id.objectClass()!=TVS_WYE::desc())continue;
        AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk||!keeps(w.object()))continue;
        for(int k=0;k<3;++k){Port P;if(!port(w.object(),k,P))continue;
            for(int j=0;j<2;++j)if(tvsTeeFit::planDistance(P.p,ends[j])<=tol)found.push_back(std::make_pair(id,k));}
    }
    delete it;space.close();
    for(size_t k=0;k<found.size();++k)ensure(found[k].first,found[k].second);
}
}
