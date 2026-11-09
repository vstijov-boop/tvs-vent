#pragma once
// "Connector" grips on the FREE ports of fittings (koljeno in plan, T komad, prelaz), as the ducts
// have at their ends: a hot grip a little outside the port starts TVS_Draw from that port with its
// size, shape and elevation (appData 200 + port; TVS_Pipe::SetParamsForDraw / Draw.inl).
// A port is free when no other TVS element has a port / duct end there (plan, 2 mm).
// Ports: koljeno MA / MC (Form_Direct only), prelaz FirstPoint / LastPoint, T komad teePorts.
#include "TVS_Entity.h"
#include "TVS_Pipe.h"
#include "TVS_TAP.h"
#include "TVS_TRANS.h"
#include "TVS_WYE.h"
#include "GripImp.h"
#include "DrawingUnits.h"
#include "dbobjptr.h"
#include <cmath>
#include <unordered_map>
#include <utility>
#include <vector>
namespace tvsPortGrip {
const int base=200;
struct Port {AcGePoint3d p;AcGeVector3d dir;double a,b;bool round;Port():a(0),b(0),round(false){}};
inline double plan(const AcGePoint3d& a,const AcGePoint3d& b){return std::hypot(a.x-b.x,a.y-b.y);}
inline bool unit(AcGeVector3d v,AcGeVector3d& out){v.z=0;if(v.length()<1e-12)return false;out=v.normal();return true;}
// Ports of a fitting usable for drawing on (plan); count, 0 when none.
inline int ports(const TVS_Entity* e,Port out[3]){
    if(const TVS_TAP* t=TVS_TAP::cast(e)){
        if(t->Form!=Form_Direct||std::fabs(std::fabs(t->Normvector.z)-1)>1e-9)return 0;
        const AcGePoint3d end[2]={t->MA,t->MC};
        for(int k=0;k<2;++k){out[k].p=end[k];if(!unit(end[k]-t->MiddlePoint,out[k].dir))return 0;
            out[k].a=t->SizeA;out[k].round=t->ThisRound;out[k].b=t->ThisRound?0:t->SizeB;}
        return 2;
    }
    if(const TVS_TRANS* t=TVS_TRANS::cast(e)){
        AcGeVector3d d;if(!unit(t->VectTr,d))return 0;
        out[0].p=t->FirstPoint;out[0].dir=-d;out[0].a=t->SizeAp1;out[0].round=t->ThisRoundp1;out[0].b=t->ThisRoundp1?0:t->SizeBp1;
        out[1].p=t->LastPoint;out[1].dir=d;out[1].a=t->SizeAp2;out[1].round=t->ThisRoundp2;out[1].b=t->ThisRoundp2?0:t->SizeBp2;
        return 2;
    }
    if(const TVS_WYE* w=TVS_WYE::cast(e)){
        AcGeVector3d u,v;if(!unit(w->Vectpr,u)||!unit(w->Vectotv,v))return 0;
        AcGePoint3d p[3];double wd[3];
        if(!w->teePorts(p,wd)){const double half=w->LengthPl+w->SizeAotv/2;
            p[0]=w->Basepoint-u*half;p[1]=w->Basepoint+u*half;p[2]=w->Basepoint+v*(w->SizeApr/2+w->LengthPl);wd[0]=wd[1]=w->SizeApr;wd[2]=w->SizeAotv;}
        const AcGeVector3d dir[3]={-u,u,v};
        for(int k=0;k<3;++k){out[k].p=p[k];out[k].dir=dir[k];out[k].a=wd[k];
            out[k].round=k<2?w->ThisRoundpr:w->ThisRoundotv;out[k].b=out[k].round?0:(k<2?w->SizeBpr:w->SizeBotv);}
        return 3;
    }
    return 0;
}
// Every connection point of another TVS element (duct ends, fitting ports, elbow ends of any form).
inline int points(const TVS_Entity* e,AcGePoint3d out[3]){
    if(const TVS_Pipe* p=TVS_Pipe::cast(e)){out[0]=p->FirstPoint;out[1]=p->LastPoint;return 2;}
    if(const TVS_TAP* t=TVS_TAP::cast(e)){out[0]=t->MA;out[1]=t->MC;return 2;}
    Port q[3];const int n=ports(e,q);for(int k=0;k<n;++k)out[k]=q[k].p;return n;
}
// All connection points of the space, on a grid; rebuilt at most every 1.5 s (grips are asked in
// bursts when fittings are selected, so a selection of many fittings scans the drawing once).
struct Cache {AcDbObjectId space;ULONGLONG at;double cell;std::unordered_map<long long,std::vector<std::pair<AcDbObjectId,AcGePoint3d> > > grid;Cache():at(0),cell(0){}};
inline Cache& cache(){static Cache c;return c;}
inline long long cellKey(double x,double y,double cell){const long long i=static_cast<long long>(std::floor(x/cell)),j=static_cast<long long>(std::floor(y/cell));return i*1000003LL+j;}
inline void rebuild(const AcDbObjectId& spaceId,double tol){
    Cache& c=cache();const ULONGLONG now=GetTickCount64();
    if(c.space==spaceId&&c.cell==tol&&now-c.at<1500)return;
    c.grid.clear();c.space=spaceId;c.cell=tol>0?tol:1;c.at=now;
    AcDbObjectPointer<AcDbBlockTableRecord> space(spaceId,AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return;
    for(;!it->done();it->step()){
        AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk)continue;
        AcDbEntity* e=NULL;if(acdbOpenAcDbEntity(e,id,AcDb::kForRead)!=Acad::eOk)continue;
        if(const TVS_Entity* o=TVS_Entity::cast(e)){AcGePoint3d q[3];const int n=points(o,q);
            for(int k=0;k<n;++k)c.grid[cellKey(q[k].x,q[k].y,c.cell)].push_back(std::make_pair(id,q[k]));}
        e->close();
    }
    delete it;
}
inline bool isFree(const TVS_Entity* self,const AcGePoint3d& at,double tol){
    rebuild(self->ownerId(),tol);const Cache& c=cache();
    for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy){
        auto f=c.grid.find(cellKey(at.x+dx*c.cell,at.y+dy*c.cell,c.cell));if(f==c.grid.end())continue;
        for(size_t k=0;k<f->second.size();++k)if(f->second[k].first!=self->objectId()&&plan(f->second[k].second,at)<=tol)return false;
    }
    return true;
}
// Appends the connector grips of the free ports (call from the new-style subGetGripPoints).
inline void add(const TVS_Entity* e,AcDbGripDataPtrArray& grips,double viewUnit){
    if(!e->database())return;
    Port q[3];const int n=ports(e,q);if(n==0)return;
    const double tol=2.0/tvsUnits::factor(e->database());
    for(int k=0;k<n;++k){
        if(!isFree(e,q[k].p,tol))continue;
        AcDbGripData* g=new AcDbGripData();
        g->setGripPoint(q[k].p+q[k].dir*(10*viewUnit));
        g->setAppData(reinterpret_cast<void*>(static_cast<intptr_t>(base+k)));
        g->setHotGripFunc(GripCback::hotGripfunc);g->setWorldDraw(GripCback::WorldDrawfunc);
        grips.append(g);
    }
}
// The ordinary grips of the old-style getter, as grip data with their index (moved by index).
inline void ordinary(const AcGePoint3dArray& points,AcDbGripDataPtrArray& grips){
    for(int i=0;i<points.length();++i){AcDbGripData* g=new AcDbGripData();g->setGripPoint(points[i]);
        g->setAppData(reinterpret_cast<void*>(static_cast<intptr_t>(i)));grips.append(g);}
}
inline void indices(const AcDbVoidPtrArray& data,AcDbIntArray& out){
    for(int i=0;i<data.length();++i){const intptr_t k=reinterpret_cast<intptr_t>(data[i]);if(k>=0&&k<base)out.append(static_cast<int>(k));}
}
}
