#pragma once
// TVS_MATCH: TVS properties of one source duct onto selected TVS elements (MATCHPROP does not
// know them). Groups (Podesavanja): Dimenzije (ducts only), Kota (axis / top / bottom of the
// source), Tip (kruti / flex, ducts only), Prikaz (maskiranje, ivice, 1D), Protok, Sloj (layer,
// colour, linetype). After a width change, each end of a changed duct that meets an element of
// another width gets a transition (TeeNeighbours.h); elevation steps to elements outside the
// selection are listed.
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "TeeNeighbours.h"
#include "TVSMoveCommand.h"
#include <set>
#include <vector>
namespace tvsMatch {
struct Options {bool size,elevation,type,display,flow,layer;int reference; // 0 osa, 1 vrh, 2 dno
    Options():size(true),elevation(true),type(true),display(true),flow(false),layer(true),reference(0){}};
inline Options& options(){static Options o;return o;}
inline const wchar_t* yes(bool v){return v?L"Da":L"Ne";}
inline const wchar_t* refName(int r){return r==1?L"Vrh":r==2?L"Dno":L"Osa";}
inline void settings(){
    for(;;){
        Options& o=options();
        acutPrintf(L"\nPrenosi: Dimenzije=%ls Kota=%ls (%ls) Tip=%ls Prikaz=%ls Protok=%ls Sloj=%ls",
            yes(o.size),yes(o.elevation),refName(o.reference),yes(o.type),yes(o.display),yes(o.flow),yes(o.layer));
        acedInitGet(0,L"Dimenzije Kota Referenca Tip Prikaz pRotok Sloj");ACHAR kw[32]=L"";
        const int rc=acedGetKword(L"\nPromijeni [Dimenzije/Kota/Referenca/Tip/Prikaz/pRotok/Sloj] <Gotovo>: ",kw);
        if(rc!=RTNORM)return;
        if(!_wcsicmp(kw,L"Dimenzije"))o.size=!o.size;
        else if(!_wcsicmp(kw,L"Kota"))o.elevation=!o.elevation;
        else if(!_wcsicmp(kw,L"Referenca"))o.reference=(o.reference+1)%3;
        else if(!_wcsicmp(kw,L"Tip"))o.type=!o.type;
        else if(!_wcsicmp(kw,L"Prikaz"))o.display=!o.display;
        else if(!_wcsicmp(kw,L"pRotok"))o.flow=!o.flow;
        else if(!_wcsicmp(kw,L"Sloj"))o.layer=!o.layer;
    }
}
struct Source {double A,B,elev,flow,wipeLength;bool round,wipe,grani,oneD;int ductType;AcDbObjectId id;};
inline double height(double A,double B){return B>0?B:A;}   // round: diameter
inline double targetAxis(const Source& s,double A,double B,int ref){
    if(ref==1)return s.elev+height(s.A,s.B)/2-height(A,B)/2;
    if(ref==2)return s.elev-height(s.A,s.B)/2+height(A,B)/2;
    return s.elev;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=2.0/f;
    Source src;
    for(;;){
        ads_name name;ads_point pick;acedInitGet(0,L"Podesavanja");
        const int rc=acedEntSel(L"\nIzaberite izvorni TVS kanal ili [Podesavanja]: ",name,pick);
        if(rc==RTKWORD){settings();continue;}
        if(rc!=RTNORM)return;
        AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
        AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForRead);
        if(p.openStatus()!=Acad::eOk){acutPrintf(L"\nIzvor mora biti TVS kanal.");continue;}
        src.A=p->SizeA;src.B=p->SizeB;src.elev=p->Elev;src.flow=p->Flow;src.wipeLength=p->WipeoutLength;
        src.round=p->ThisRound;src.wipe=p->Wipeout;src.grani=p->Grani;src.oneD=p->This1D;src.ductType=p->DuctType;src.id=id;
        break;
    }
    ads_name ss;acutPrintf(L"\nIzaberite ciljne TVS elemente: ");
    if(acedSSGet(NULL,NULL,NULL,NULL,ss)!=RTNORM)return;
    Adesk::Int32 count=0;acedSSLength(ss,&count);
    const Options o=options();
    std::set<AcDbObjectId> selected;std::vector<AcDbObjectId> widened,raised;
    int changed=0,skippedShape=0;
    for(Adesk::Int32 i=0;i<count;++i){
        ads_name en;if(acedSSName(ss,i,en)!=RTNORM)continue;
        AcDbObjectId id;if(acdbGetObjectId(id,en)!=Acad::eOk||id==src.id)continue;
        AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForWrite);if(e.openStatus()!=Acad::eOk)continue;
        TVS_Entity* t=TVS_Entity::cast(e.object());if(!t)continue;
        selected.insert(id);
        TVS_Pipe* pipe=TVS_Pipe::cast(e.object());
        bool any=false;
        if(o.size&&pipe){
            if(pipe->ThisRound!=src.round)++skippedShape;
            else if(std::fabs(pipe->SizeA-src.A)>1e-9||std::fabs(pipe->SizeB-src.B)>1e-9){
                if(pipe->put_SizeA(src.A)==Acad::eOk&&pipe->put_SizeB(src.B)==Acad::eOk){widened.push_back(id);any=true;}
            }
        }
        if(o.type&&pipe&&pipe->DuctType!=src.ductType){pipe->setFlex(src.ductType==DuctTypeFlex);any=true;}
        if(o.display){t->put_Wipeout(src.wipe);t->put_WipeoutLength(src.wipeLength);t->put_Grani(src.grani);t->put_This1D(src.oneD);any=true;}
        if(o.flow){t->put_Flow(src.flow);any=true;}
        if(o.layer){AcDbObjectPointer<TVS_Pipe> s(src.id,AcDb::kForRead);if(s.openStatus()==Acad::eOk){e->setPropertiesFrom(s.object());any=true;}}
        if(o.elevation){
            const double axis=targetAxis(src,t->SizeA,t->SizeB,o.reference);
            if(std::fabs(axis-t->Elev)>1e-9&&t->put_Elevation(axis)==Acad::eOk){raised.push_back(id);any=true;}
        }
        if(any){e->recordGraphicsModified();++changed;}
    }
    acedSSFree(ss);
    // width steps at the ends of changed ducts
    for(size_t i=0;i<widened.size();++i){tvsTeeFit::farEnd(AcDbObjectId::kNull,widened[i],true,tol,f);tvsTeeFit::farEnd(AcDbObjectId::kNull,widened[i],false,tol,f);}
    // elevation steps to elements outside the selection
    int steps=0;
    if(!raised.empty()){
        std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;tvsMove::snapshot(db,nodes,labels);
        std::set<AcDbObjectId> moved(raised.begin(),raised.end());
        for(size_t i=0;i<nodes.size();++i){if(!moved.count(nodes[i].id))continue;
            for(size_t j=0;j<nodes[i].ports.size();++j)for(size_t k=0;k<nodes.size();++k){
                if(k==i||selected.count(nodes[k].id))continue;
                for(size_t m=0;m<nodes[k].ports.size();++m){
                    const AcGePoint3d a=tvsMove::acad(nodes[i].ports[j]),b=tvsMove::acad(nodes[k].ports[m]);
                    if(std::hypot(a.x-b.x,a.y-b.y)>tol)continue;
                    const double dz=nodes[i].elevation-nodes[k].elevation;
                    if(std::fabs(dz)>1e-9){++steps;acutPrintf(L"\nZ razlika %.0f mm prema elementu van izbora kod (%.1f, %.1f): potreban je Z prelaz.",dz*f,a.x,a.y);}
                }
            }}
    }
    acutPrintf(L"\nTVS_MATCH: promijenjeno elemenata %d.",changed);
    if(skippedShape)acutPrintf(L" Dimenzije nisu prenesene na %d kanala drugog oblika (okrugli/pravougaoni).",skippedShape);
    if(steps)acutPrintf(L"\nZ razlika na %d mjesta; izaberite i susjedne elemente ili dodajte Z prelaz (TVS_OFFSET).",steps);
}
}
