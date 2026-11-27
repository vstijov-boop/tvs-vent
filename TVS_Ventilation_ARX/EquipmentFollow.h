#pragma once
// Oprema na mrezi follows TVS_MOVE (network) / TVS_STRETCH (user 2026-11-24): before,
// the commands did not see the linked blocks, so a block whose ducts moved became "odvojen" (red).
// Hooks of TVSMoveCommand.h (install() at load):
//  read   - the linked blocks as nodes of kind 5 after the TVS elements (EquipmentNodes.h, port z
//           from the duct end it lies on); an inline block is a rigid body of the planner, the
//           terminals are hidden from it (tvsMove::hideTerminals);
//  follow - in the command's transaction: an inline block moved by the plan (already transformed)
//           gets its record ports shifted and the anchor renewed; a terminal moves with the end of
//           the duct / flex it sits on (or, resetka na dnu, with a duct moved whole); both halves of
//           a linijski difuzor must move alike, otherwise the block stays (warning: odvojen).
#include "TVSMoveCommand.h"
#include "EquipmentNodes.h"
#include "EquipmentStore.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include <cmath>
#include <vector>
namespace tvsEquipmentFollow {
inline bool read(AcDbDatabase* db,std::vector<tvsMove::Node>& nodes){
    if(!db)return false;const double f=tvsUnits::factor(db);
    return tvsEquipmentNodes::append(db,nodes,NULL,1.0/f);
}
// record ports + anchor of a block already at its new place (d in drawing units)
inline bool shift(AcDbEntity* e,const tvsMove::Point& d,double f){
    AcDbBlockReference* b=AcDbBlockReference::cast(e);if(!b)return false;
    tvsEquipment::Record r;if(!tvsEquipmentStore::read(b,r))return false;
    const double s=tvsEquipmentFlow::unitScale(r.mmPerUnit,f);
    for(int k=0;k<r.ports&&k<2;++k){r.port[k].x+=d.x/s;r.port[k].y+=d.y/s;r.port[k].z+=d.z/s;}
    tvsEquipmentStore::anchor(b,r);
    return tvsEquipmentStore::write(b,r)==Acad::eOk;
}
inline bool sameMove(const tvsMove::Point& a,const tvsMove::Point& b,double tol){return (a-b).length()<=tol;}
inline int follow(tvsMove::Transaction& tx,const std::vector<tvsMove::Node>& nodes,const std::vector<tvsMoveNetwork::Body>& bodies,
                  const tvsMoveNetwork::Result& result,const std::vector<int>* flexMask,const tvsMove::Point& delta,double reach,const ACHAR* command){
    AcDbDatabase* db=NULL;for(size_t i=0;i<nodes.size()&&!db;++i)if(tvsMove::isEquipment(nodes[i]))db=nodes[i].id.database();
    if(!db)return 0;
    const double f=tvsUnits::factor(db);
    auto moved=[&](size_t k){return k<result.masks.size()&&result.masks[k]!=0&&k<result.ports.size()&&result.ports[k].size()==bodies[k].ports.size();};
    // the new place of a flex end (as tvsMove::followFlex)
    auto flexEnd=[&](size_t i,int j,tvsMove::Point& to){
        if(flexMask&&i<flexMask->size()&&(*flexMask)[i]>0&&((*flexMask)[i]&(1<<j))){to=nodes[i].ports[j]+delta;return true;}
        for(size_t k=0;k<bodies.size();++k){if(k==i||!moved(k))continue;
            for(size_t q=0;q<bodies[k].ports.size();++q)if(tvsMoveGeometry::pointsNear(bodies[k].ports[q],nodes[i].ports[j],reach)){to=result.ports[k][q];return true;}}
        return false;};
    // displacement of the network end at p (duct / fitting port, flex end); false: nothing moved there
    auto endMove=[&](const tvsMove::Point& p,tvsMove::Point& d){
        for(size_t k=0;k<bodies.size();++k){if(!moved(k))continue;
            for(size_t q=0;q<bodies[k].ports.size();++q)if(tvsMoveGeometry::pointsNear(bodies[k].ports[q],p,reach)){d=result.ports[k][q]-bodies[k].ports[q];return true;}
            if(k<nodes.size()&&tvsMove::followsOutlet(nodes[k],bodies[k],p,reach)){d=result.ports[k][1]-bodies[k].ports[1];return true;}}   // terminal on a koljeno gore / dolje
        for(size_t i=0;i<nodes.size();++i){if(!nodes[i].flex||nodes[i].ports.size()!=2)continue;
            for(int j=0;j<2;++j)if(tvsMoveGeometry::pointsNear(nodes[i].ports[j],p,reach)){tvsMove::Point to;if(flexEnd(i,j,to)){d=to-nodes[i].ports[j];return true;}}}
        return false;};
    struct Move {AcDbObjectId id;tvsMove::Point d;bool conflict,rigid;};
    std::vector<Move> moves;
    auto entry=[&](const AcDbObjectId& id,const tvsMove::Point& d,bool rigid){
        for(size_t k=0;k<moves.size();++k)if(moves[k].id==id){if(!sameMove(moves[k].d,d,reach))moves[k].conflict=true;return;}
        Move m;m.id=id;m.d=d;m.conflict=false;m.rigid=rigid;moves.push_back(m);};
    for(size_t i=0;i<nodes.size();++i){const tvsMove::Node& n=nodes[i];if(!tvsMove::isEquipment(n))continue;
        if(n.ports.size()==2){if(moved(i))entry(n.id,delta,true);continue;}   // inline: moved by the plan (transformBy done)
        if(n.ports.size()!=1)continue;
        tvsMove::Point d(0,0,0);
        if(!endMove(n.ports[0],d)){   // resetka na dnu: on a duct moved whole
            for(size_t k=0;k<bodies.size();++k)if(moved(k)&&bodies[k].kind==tvsMoveNetwork::Pipe&&bodies[k].ports.size()==2&&result.masks[k]==3
                &&tvsMoveGeometry::onSegment(n.ports[0],bodies[k].ports[0],bodies[k].ports[1],reach)){d=result.ports[k][0]-bodies[k].ports[0];break;}}
        entry(n.id,d,false);
    }
    int count=0;
    for(size_t k=0;k<moves.size();++k){const Move& m=moves[k];
        if(m.conflict){acutPrintf(L"\n%ls: linijski difuzor – priključci se ne pomjeraju jednako; blok ostaje (odvojen, ponovo TVS_OPREMA).",command);continue;}
        if(m.d.length()<=reach*1e-3)continue;
        AcDbEntity* e=tx.write(m.id);
        if(!e){acutPrintf(L"\n%ls: blok opreme je na zaključanom sloju. Izmjena je poništena.",command);return -1;}
        if(!m.rigid){AcGeMatrix3d t;t.setToTranslation(AcGeVector3d(m.d.x,m.d.y,m.d.z));
            if(e->transformBy(t)!=Acad::eOk){acutPrintf(L"\n%ls: blok opreme se ne može pomjeriti. Izmjena je poništena.",command);return -1;}}
        if(!shift(e,m.d,f)){acutPrintf(L"\n%ls: zapis opreme nije upisan. Izmjena je poništena.",command);return -1;}
        e->recordGraphicsModified();++count;
    }
    return count;
}
inline void install(){tvsMove::equipmentReadHook()=&read;tvsMove::equipmentFollowHook()=&follow;}
}
