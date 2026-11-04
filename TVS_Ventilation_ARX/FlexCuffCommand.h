#pragma once
// TVS_FLEX_MANZETNE: marks cuffs on every flex hose end in Model space that does NOT continue
// into another flex piece, i.e. an end at a rigid duct/fitting or a free end (air terminal).
// Purely geometric (connectors are not filed), so it can be re-run any time, e.g. before
// printing. Only the cuff flags change; geometry, length and links stay as they are.
#include "TVSMoveCommand.h"
#include "../TVS_Ventilation_DBX/FlexCuffData.h"
#include <vector>
#include <memory>
#include <cmath>
namespace tvsFlexCuffCommand {
struct Port {AcGePoint3d p;AcDbObjectId id;bool flex;};
// Plan ports of a flex elbow (same parameterisation as TVS_TAP::subWorldDraw, Form_Direct).
inline bool elbowPorts(TVS_TAP* t,AcGePoint3d& ma,AcGePoint3d& mc){
    const double sa=std::atan2(t->Startvector.y,t->Startvector.x),s=t->Swectangle,rc=t->get_radius()+t->SizeA/2;
    if(!(s>0)||!(rc>0)||t->get_Form()!=Form_Direct)return false;
    const double k=rc/std::cos(s/2);const AcGePoint3d m=t->MiddlePoint;
    const AcGePoint3d c(m.x-k*std::cos(sa+s/2),m.y-k*std::sin(sa+s/2),m.z);
    ma=AcGePoint3d(c.x+rc*std::cos(sa),c.y+rc*std::sin(sa),m.z);mc=AcGePoint3d(c.x+rc*std::cos(sa+s),c.y+rc*std::sin(sa+s),m.z);
    return true;
}
inline bool flexEnds(AcDbEntity* e,AcGePoint3d ends[2]){
    if(TVS_Pipe* p=TVS_Pipe::cast(e)){if(p->DuctType!=DuctTypeFlex)return false;ends[0]=p->FirstPoint;ends[1]=p->LastPoint;return true;}
    if(TVS_TAP* t=TVS_TAP::cast(e)){if(t->DuctType!=DuctTypeFlex)return false;return elbowPorts(t,ends[0],ends[1]);}
    return false;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db||!db->tilemode()){acutPrintf(L"\nTVS_FLEX_MANZETNE radi u Model prostoru.");return;}
    const double tol=2.0/tvsUnits::factor(db); // 2 mm in plan
    std::vector<Port> ports;std::vector<AcDbObjectId> hoses;
    {AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return;
     AcDbBlockTableRecordIterator* raw=NULL;if(space->newIterator(raw)!=Acad::eOk)return;std::unique_ptr<AcDbBlockTableRecordIterator> it(raw);
     for(;!it->done();it->step()){AcDbEntity* ent=NULL;if(it->getEntity(ent,AcDb::kForRead)!=Acad::eOk)continue;
        AcGePoint3d ends[2];
        if(flexEnds(ent,ends)){hoses.push_back(ent->objectId());for(int k=0;k<2;++k)ports.push_back(Port{ends[k],ent->objectId(),true});}
        else if(TVS_Entity::cast(ent)){tvsMove::Node n;if(tvsMove::describe(ent,n))for(const auto& q:n.ports)ports.push_back(Port{AcGePoint3d(q.x,q.y,q.z),ent->objectId(),false});}
        ent->close();}}
    int marked=0,elements=0,changed=0;
    for(const AcDbObjectId& id:hoses){
        AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForWrite);if(e.openStatus()!=Acad::eOk)continue;
        AcGePoint3d ends[2];if(!flexEnds(e.object(),ends))continue;bool cuff[2]={true,true};
        for(int k=0;k<2;++k)for(const Port& q:ports)
            if(q.id!=id&&q.flex&&std::hypot(q.p.x-ends[k].x,q.p.y-ends[k].y)<=tol)cuff[k]=false; // continues into flex
        bool oldStart=false,oldEnd=false;tvsFlexCuff::read(e.object(),oldStart,oldEnd);
        if(oldStart!=cuff[0]||oldEnd!=cuff[1]){if(tvsFlexCuff::write(e.object(),cuff[0],cuff[1])==Acad::eOk)++changed;}
        marked+=int(cuff[0])+int(cuff[1]);++elements;
    }
    acutPrintf(L"\nTVS_FLEX_MANZETNE: %d flex elemenata, %d manzetni (izmijenjeno %d elemenata). Geometrija i duzine nisu mijenjane.",elements,marked,changed);
}
// TVS_FLEX_MANZETNA: manual toggle. Pick a flex piece near one of its ends; that end's cuff
// is switched on/off. Repeats until Enter/Esc.
inline void toggle(){
    for(;;){
        ads_name name;ads_point pick;
        const int rc=acedEntSel(L"\nIzaberite flex kanal/koljeno blizu kraja (Enter za kraj): ",name,pick);
        if(rc!=RTNORM)return;
        AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)continue;
        AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForWrite);
        AcGePoint3d ends[2];if(e.openStatus()!=Acad::eOk||!flexEnds(e.object(),ends)){acutPrintf(L"\nNije fleksibilni kanal ili flex koljeno u osnovi.");continue;}
        ads_point wcs;resbuf from={},to={};from.restype=RTSHORT;from.resval.rint=1;to.restype=RTSHORT;to.resval.rint=0;
        if(acedTrans(pick,&from,&to,0,wcs)!=RTNORM)continue;
        const int k=std::hypot(wcs[0]-ends[0].x,wcs[1]-ends[0].y)<=std::hypot(wcs[0]-ends[1].x,wcs[1]-ends[1].y)?0:1;
        bool flags[2]={false,false};tvsFlexCuff::read(e.object(),flags[0],flags[1]);flags[k]=!flags[k];
        const Acad::ErrorStatus es=tvsFlexCuff::write(e.object(),flags[0],flags[1]);
        if(es!=Acad::eOk)acutPrintf(L"\nManzetna nije upisana (%ls).",acadErrorStatusText(es));
        else acutPrintf(L"\nManzetna na %ls kraju: %ls.",k?L"zavrsnom":L"pocetnom",flags[k]?L"DA":L"NE");
    }
}
}
