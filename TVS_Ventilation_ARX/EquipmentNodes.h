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
#include "../geometry/RoundElbow.h"
#include "dbents.h"
#include <cmath>
#include <vector>
namespace tvsEquipmentNodes {
inline bool isNode(const tvsMove::Node& n){return n.kind==tvsEquipmentFlow::nodeKind();}
// koljeno gore / dolje (TVS_TAP Form_Up / Form_Down; tvsMove::describe: kind 0, ports MA and MC,
// port 1 = the vertical outlet): the outlet in plan as TVS_OPREMA records it (rectangular:
// MiddlePoint; round with a constant / variable radius: tvsRoundElbow::ports; else MC) and its
// level (geometry/EquipmentFlow.h outletLevel, rc = get_radius() + side in the bend plane / 2).
struct Outlet {double x,y,level;Outlet():x(0),y(0),level(0){}};
inline bool outlet(const tvsMove::Node& n,Outlet& o){
    if(n.kind!=0||n.flex||n.ports.size()!=2)return false;
    AcDbObjectPointer<TVS_TAP> t(n.id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)return false;
    const bool up=t->get_Form()==Form_Up;if(!up&&t->get_Form()!=Form_Down)return false;
    const bool isRound=t->ThisRound;
    const double rc=t->get_radius()+(isRound?t->SizeA:t->SizeB)/2;
    o.level=tvsEquipmentFlow::outletLevel(t->Elev,up,rc,std::fabs(t->Swectangle));
    o.x=n.ports[1].x;o.y=n.ports[1].y;
    if(!isRound){o.x=t->MiddlePoint.x;o.y=t->MiddlePoint.y;}
    else if(t->RadiusTypeRound==RadiusTypeRound_RadiusConstant||t->RadiusTypeRound==RadiusTypeRound_RadiusVariable){
        tvsRoundElbow::P ma,mc;tvsRoundElbow::ports(tvsRoundElbow::P(t->MiddlePoint.x,t->MiddlePoint.y),t->Startvector.x,t->Startvector.y,t->Swectangle,rc,ma,mc);
        o.x=mc.x;o.y=mc.y;}
    return true;
}
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
// A twin terminal (linijski difuzor, 2 ports on the box) is appended as two 1-port terminals.
// detached (optional): linked blocks moved / copied after linking - left out of the network.
inline bool append(AcDbDatabase* db,std::vector<tvsMove::Node>& nodes,std::vector<tvsEquipment::Record>* records,double snapTol,std::vector<AcDbObjectId>* detached=NULL){
    if(!db)return false;const double f=tvsUnits::factor(db);const size_t before=nodes.size();
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return false;
    for(;!it->done();it->step()){
        AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
        if(const AcDbBlockReference* b=AcDbBlockReference::cast(e)){tvsMove::Node n;tvsEquipment::Record r;
            if(describe(b,f,n,r)){
                if(tvsEquipmentStore::detached(b,r)){if(detached)detached->push_back(b->objectId());}   // moved / copied: not in the network
                else if(r.twin&&n.ports.size()==2){   // linijski difuzor: two 1-port terminals (same block id), Equipment.h half()
                    for(int k=0;k<2;++k){tvsMove::Node h=n;h.ports.assign(1,n.ports[k]);h.axes.assign(1,n.axes[k]);h.sections.assign(1,n.sections[k]);
                        h.elevation=n.ports[k].z;h.middle=n.ports[k];nodes.push_back(h);if(records)records->push_back(tvsEquipment::half(r,k));}}
                else{nodes.push_back(n);if(records)records->push_back(r);}}}
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
    // terminal at the vertical outlet of a koljeno gore / dolje: within 1 mm of the outlet in plan
    // (nearest level when several) its port becomes the elbow's port 1 (plan point and z), so the
    // network joins it there (also without snapTol: TVS_PROVJERA, TVS_DIM_PRATI)
    if(nodes.size()>before){
        const double reach=1.0/f;std::vector<size_t> at;std::vector<Outlet> outs;
        for(size_t k=0;k<before;++k){Outlet o;if(outlet(nodes[k],o)){at.push_back(k);outs.push_back(o);}}
        for(size_t i=before;i<nodes.size()&&!outs.empty();++i){if(nodes[i].ports.size()!=1||nodes[i].sections.empty())continue;
            tvsMove::Point& p=nodes[i].ports[0];int best=-1;double off=0;
            for(size_t m=0;m<outs.size();++m){if(std::hypot(outs[m].x-p.x,outs[m].y-p.y)>reach)continue;
                const double d=std::fabs(outs[m].level-nodes[i].sections[0].elevation);if(best<0||d<off){best=static_cast<int>(m);off=d;}}
            if(best>=0){p=nodes[at[best]].ports[1];nodes[i].middle=p;}}
    }
    return true;
}
}
