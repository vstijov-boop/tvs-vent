#pragma once
// Moving TVS elements up / down by a fixed difference while keeping their own level changes:
// Elev (axis) of each element, plus the points of offset / bypass members that carry real Z.
// Used by TVS_MATCH (Mreza) and the transition alignment (TVS_PRELAZ_KOTA).
#include "TVSMoveCommand.h"
#include <set>
#include <vector>
namespace tvsElevationShift {
inline int shift(const std::vector<AcDbObjectId>& ids,double delta,const AcDbObjectId& skip=AcDbObjectId::kNull){
    if(std::fabs(delta)<=1e-12)return 0;
#ifdef TVS_OFFSET_INTEGRATION
    tvsOffsetData::WriteScope scope;   // whole assemblies move together, nothing is split
#endif
    AcGeMatrix3d up;up.setToTranslation(AcGeVector3d(0,0,delta));int shifted=0;
    for(size_t i=0;i<ids.size();++i){
        if(ids[i]==skip)continue;
        AcDbObjectPointer<AcDbEntity> e(ids[i],AcDb::kForWrite);if(e.openStatus()!=Acad::eOk)continue;
        TVS_Entity* t=TVS_Entity::cast(e.object());if(!t)continue;
        tvsMove::Node n;bool spatial=false;   // offset/bypass members carry real Z in their points
        if(tvsMove::describe(e.object(),n))for(size_t j=0;j<n.ports.size();++j)if(std::fabs(n.ports[j].z)>1e-9)spatial=true;
        if(t->put_Elevation(t->Elev+delta)!=Acad::eOk)continue;
        if(spatial)e->transformBy(up);
        e->recordGraphicsModified();++shifted;
    }
    return shifted;
}
// Plan translation of elements (whole offset / bypass assemblies may move).
inline int move(const std::vector<AcDbObjectId>& ids,const AcGeVector3d& d,const AcDbObjectId& skip=AcDbObjectId::kNull){
    if(d.length()<=1e-12)return 0;
#ifdef TVS_OFFSET_INTEGRATION
    tvsOffsetData::WriteScope scope;
#endif
    AcGeMatrix3d m;m.setToTranslation(d);int moved=0;
    for(size_t i=0;i<ids.size();++i){if(ids[i]==skip)continue;
        AcDbObjectPointer<AcDbEntity> e(ids[i],AcDb::kForWrite);if(e.openStatus()!=Acad::eOk||!TVS_Entity::cast(e.object()))continue;
        if(e->transformBy(m)==Acad::eOk){e->recordGraphicsModified();++moved;}}
    return moved;
}
// Elements connected to port `port` of element `from`, not through `from`. loop = the side
// reaches `from` again through another port (closed ring): no side can be moved alone.
// seed: where the side touched `from` before a change (the port may have moved); NULL = the port now.
inline bool side(AcDbDatabase* db,const AcDbObjectId& from,int port,double tol,std::vector<AcDbObjectId>& out,bool& loop,const AcGePoint3d* seed=NULL){
    out.clear();loop=false;
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;if(!tvsMove::snapshot(db,nodes,labels))return false;
    int self=-1;for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==from)self=static_cast<int>(i);
    if(self<0||port<0||port>=static_cast<int>(nodes[self].ports.size()))return false;
    auto touch=[&](const tvsMove::Point& a,const tvsMove::Point& b){return std::hypot(a.x-b.x,a.y-b.y)<=tol;};
    std::vector<int> seen(nodes.size(),0),queue;seen[self]=1;
    for(size_t k=0;k<nodes.size();++k){if(seen[k])continue;
        const tvsMove::Point at=seed?tvsMove::point(*seed):nodes[self].ports[port];
        for(size_t j=0;j<nodes[k].ports.size();++j)if(touch(at,nodes[k].ports[j])){seen[k]=1;queue.push_back(static_cast<int>(k));break;}}
    for(size_t q=0;q<queue.size();++q){const tvsMove::Node& a=nodes[queue[q]];
        for(size_t i=0;i<a.ports.size();++i){
            for(size_t j=0;j<nodes[self].ports.size();++j)if(static_cast<int>(j)!=port&&touch(a.ports[i],nodes[self].ports[j]))loop=true;
            for(size_t k=0;k<nodes.size();++k){if(seen[k])continue;bool t=false;
                for(size_t j=0;j<nodes[k].ports.size()&&!t;++j)t=touch(a.ports[i],nodes[k].ports[j]);
                if(t){seen[k]=1;queue.push_back(static_cast<int>(k));}}}}
    for(size_t q=0;q<queue.size();++q)out.push_back(nodes[queue[q]].id);
    return true;
}
}
