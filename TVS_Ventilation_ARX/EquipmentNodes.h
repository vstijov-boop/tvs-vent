#pragma once
// Oprema na mrezi in the network readers: every block reference of the current space with a
// TVS_OPREMA record (EquipmentStore.h, geometry/Equipment.h) as a tvsMove::Node of kind
// tvsEquipmentFlow::nodeKind() (5): its record ports (1 terminal / 2 inline) in the current drawing
// units, the record section on every port, z of a port = z of the duct end it lies on
// (geometry/EquipmentFlow.h). Appended AFTER tvsMove::snapshot by TVS_PROTOK / TVS_PAD /
// TVS_DPD_IZVOZ (FlowCommand.h tvsFlow::read), TVS_DIM_PRATI (SizeFollow.h: a boundary "oprema")
// and TVS_PROVJERA (CheckCommand.h); TVS_MOVE / TVS_STRETCH do not see the blocks.
#include "TVSMoveCommand.h"
#include "EquipmentStore.h"
#include "../geometry/EquipmentFlow.h"
#include "dbents.h"
#include <vector>
namespace tvsEquipmentNodes {
inline bool isNode(const tvsMove::Node& n){return n.kind==tvsEquipmentFlow::nodeKind();}
// f: mm per unit of the current drawing
inline bool describe(const AcDbBlockReference* b,double f,tvsMove::Node& n,tvsEquipment::Record& r){
    if(!b||!tvsEquipmentStore::read(b,r)||r.ports<1||r.ports>2||!(f>0))return false;
    const double s=tvsEquipmentFlow::unitScale(r.mmPerUnit,f);
    n=tvsMove::Node();n.id=b->objectId();n.kind=tvsEquipmentFlow::nodeKind();
    n.round=r.round;n.a=r.a/f;n.b=r.round?0:r.b/f;
    for(int k=0;k<r.ports;++k){const tvsEquipment::Port& p=r.port[k];
        const tvsMove::Point q(p.x*s,p.y*s,p.z*s);n.ports.push_back(q);n.axes.push_back(tvsMove::Point(p.dx,p.dy,0));
        n.sections.push_back(tvsMoveNetwork::Section(n.a,n.b,n.round,q.z));}
    n.elevation=n.ports[0].z;n.middle=n.ports.size()==2?(n.ports[0]+n.ports[1])*0.5:n.ports[0];
    return true;
}
// Appends the linked blocks to `nodes` (records: same order, optional). snapTol > 0: the port z
// is taken from the end of another node lying on it in plan (within snapTol, drawing units).
inline bool append(AcDbDatabase* db,std::vector<tvsMove::Node>& nodes,std::vector<tvsEquipment::Record>* records,double snapTol){
    if(!db)return false;const double f=tvsUnits::factor(db);const size_t before=nodes.size();
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return false;
    for(;!it->done();it->step()){
        AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
        if(const AcDbBlockReference* b=AcDbBlockReference::cast(e)){tvsMove::Node n;tvsEquipment::Record r;
            if(describe(b,f,n,r)){nodes.push_back(n);if(records)records->push_back(r);}}
        e->close();
    }
    delete it;
    if(snapTol>0&&nodes.size()>before){
        std::vector<tvsEquipmentFlow::Candidate> c;
        for(size_t k=0;k<before;++k)for(size_t j=0;j<nodes[k].ports.size();++j){const tvsMove::Point& p=nodes[k].ports[j];
            const double level=j<nodes[k].sections.size()?nodes[k].sections[j].elevation:nodes[k].elevation;
            const tvsEquipmentFlow::Candidate x={p.x,p.y,p.z,level};c.push_back(x);}
        for(size_t i=before;i<nodes.size();++i)for(size_t j=0;j<nodes[i].ports.size();++j){tvsMove::Point& p=nodes[i].ports[j];
            const int k=tvsEquipmentFlow::pickLevel(c,p.x,p.y,nodes[i].sections[j].elevation,snapTol);if(k>=0)p.z=c[k].z;}
    }
    return true;
}
}
