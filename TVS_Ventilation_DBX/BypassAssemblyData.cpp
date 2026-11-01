#include "StdAfx.h"
#ifdef TVS_OFFSET_INTEGRATION
#include "BypassAssemblyData.h"
#include "TVS_Pipe.h"
#include "TVS_TAP.h"
#include "DrawingUnits.h"
#include "dbdict.h"
#include "dbxrecrd.h"
#include "dbobjptr.h"
#include "dbtrans.h"
#include "../geometry/OffsetGeometry.h"
#include <set>
#include <memory>
namespace tvsBypassData {
using namespace tvsOffsetData;
static const wchar_t* memberKey(){return L"TVS_BYPASS_PARENT";}
static int typeAt(int j){return j==0||j==2?90:j==1?1:(j>=4&&j<=9)||j==23||j==24?330:j==10||j>=25?70:40;}
static Status link(const AcDbObject* o,AcDbObjectId& parent,int& role){
    if(!o)return Corrupt;if(o->extensionDictionary().isNull())return Legacy;
    AcDbObjectPointer<AcDbDictionary> d(o->extensionDictionary(),AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return Corrupt;
    AcDbObjectId id;const Acad::ErrorStatus es=d->getAt(memberKey(),id);if(es==Acad::eKeyNotFound)return Legacy;if(es!=Acad::eOk)return Corrupt;
    AcDbObjectPointer<AcDbXrecord> r(id,AcDb::kForRead);if(r.openStatus()!=Acad::eOk)return Corrupt;
    resbuf* b=NULL;if(r->rbChain(&b)!=Acad::eOk||!b)return Corrupt;
    Status status=Corrupt;
    if(b->restype==90&&b->rbnext&&b->rbnext->restype==330&&b->rbnext->rbnext
        &&b->rbnext->rbnext->restype==70&&!b->rbnext->rbnext->rbnext){
        if(b->resval.rlong!=1)status=UnsupportedVersion;
        else if(acdbGetObjectId(parent,b->rbnext->resval.rlname)==Acad::eOk){role=b->rbnext->rbnext->resval.rint;if(role>=0&&role<7)status=Valid;}
    }
    acutRelRb(b);return status;
}
bool marked(const AcDbObject* o){AcDbObjectId p;int role=-1;return link(o,p,role)!=Legacy;}
static Status read(const AcDbObjectId& id,Record& out){
    if(id.isNull()||id.isErased())return Incomplete;
    AcDbObjectPointer<AcDbXrecord> r(id,AcDb::kForRead);if(r.openStatus()!=Acad::eOk)return Corrupt;
    resbuf* b=NULL;if(r->rbChain(&b)!=Acad::eOk||!b)return Corrupt;
    resbuf* p=b;bool ok=true;int version=0;
    for(int j=0;j<27;++j){const int type=typeAt(j);
        if(!p||p->restype!=type){ok=false;break;}
        if(j==0)version=p->resval.rlong;
        else if(j==1){if(!p->resval.rstring){ok=false;break;}out.guid=p->resval.rstring;}
        else if(j==2)out.revision=p->resval.rlong;
        else if(j==3)out.factor=p->resval.rreal;
        else if(j>=4&&j<=9){AcDbObjectId value;if(acdbGetObjectId(value,p->resval.rlname)!=Acad::eOk){ok=false;break;}
            if(j==4)out.members[0]=value;if(j==5)out.members[3]=value;if(j==6)out.members[4]=value;
            if(j==7)out.inlet=value;if(j==8)out.outlet=value;if(j==9)out.obstacle=value;}
        else if(j==10){if(p->resval.rint!=0&&p->resval.rint!=1)ok=false;out.reverse=p->resval.rint!=0;}
        else if(j<15)out.policy[j-11]=p->resval.rreal;
        else if(j<23)out.obstacleSnapshot[j-15]=p->resval.rreal;
        else if(j<25){if(p->resval.rlname[0]||p->resval.rlname[1]){if(acdbGetObjectId(out.neighbors[j-23],p->resval.rlname)!=Acad::eOk)ok=false;}}
        else out.ports[j-25]=p->resval.rint;
        p=p->rbnext;
    }
    if(p)ok=false;acutRelRb(b);if(!ok)return Corrupt;if(version!=1)return UnsupportedVersion;
    GUID parsed;if(out.guid.GetLength()!=38||FAILED(CLSIDFromString(const_cast<ACHAR*>(out.guid.GetString()),&parsed))||out.revision<1
        ||!tvsUnits::validFactor(out.factor)||out.factor!=tvsUnits::factor(r.object()))return Corrupt;
    for(double v:out.policy)if(!tvsOffset::finite(v)||v<0)return Corrupt;
    for(double v:out.obstacleSnapshot)if(!tvsOffset::finite(v))return Corrupt;
    for(int j=0;j<2;++j){const int anchor=j?4:0;
        if(out.members[anchor].isNull()||out.members[anchor].isErased())return Incomplete;
        AcDbObjectPointer<AcDbObject> o(out.members[anchor],AcDb::kForRead);if(o.openStatus()!=Acad::eOk)return Corrupt;
        tvsOffsetData::Record triple;Status s=tvsOffsetData::read(o.object(),triple);if(s!=Valid)return s==Legacy?Corrupt:s;
        if(triple.role!=0)return Corrupt;for(int k=0;k<3;++k)out.members[anchor+k]=triple.members[k];
    }
    return Valid;
}
static AcGePoint3d horizontalPort(TVS_TAP* p){
    return p->MiddlePoint-AcGeVector3d(-p->Startvector.y,p->Startvector.x,0)*((p->get_radius()+p->SizeB/2)*std::sin(p->Swectangle));
}
static bool boundaryPinned(const Record& r,const AcDbObjectId& source,bool first){
    if(r.inlet==source&&(first==r.reverse||!r.neighbors[0].isNull()))return true;
    if(r.outlet==source&&(first!=r.reverse||!r.neighbors[1].isNull()))return true;
    for(int k=0;k<2;++k){
        if(r.neighbors[k]==source&&r.ports[k]==(first?1:2))return true;
    }
    return false;
}
bool pinned(const AcDbObjectId& source,bool first){
    if(source.isNull()||!source.database())return true;
    AcDbObjectPointer<AcDbDictionary> nod(source.database()->namedObjectsDictionaryId(),AcDb::kForRead);
    if(nod.openStatus()!=Acad::eOk)return true;
    std::unique_ptr<AcDbDictionaryIterator> it(nod->newIterator());
    if(!it)return true;
    for(;!it->done();it->next()){
        if(_wcsnicmp(it->name(),L"TVS_BYPASS_",11))continue;
        Record r;const Status s=read(it->objectId(),r);if(s!=Valid)return true;
        if(boundaryPinned(r,source,first))return true;
    }
    return false;
}
static bool nearPoint(const AcGePoint3d& a,const AcGePoint3d& b,double f){return a.distanceTo(b)*f<=.001;}
Status validate(const AcDbObject* object,bool geometry){
    AcDbObjectId parent;int role=-1;Status s=link(object,parent,role);if(s!=Valid)return s;
    Record r;s=read(parent,r);if(s!=Valid)return s;
    std::set<AcDbObjectId> ids;
    for(int j=0;j<7;++j){const AcDbObjectId id=r.members[j];if(id.isNull()||id.isErased())return Incomplete;
        if(id.database()!=object->database()||!ids.insert(id).second)return Corrupt;
        AcDbObjectPointer<AcDbObject> member(id,AcDb::kForRead);if(member.openStatus()!=Acad::eOk)return Corrupt;
        AcDbObjectId other;int slot=-1;if(link(member.object(),other,slot)!=Valid||other!=parent||slot!=j)return Corrupt;
    }
    if(r.members[role]!=object->objectId())return Corrupt;
    for(int j=0;j<2;++j){AcDbObjectPointer<AcDbObject> anchor(r.members[j?4:0],AcDb::kForRead);
        s=tvsOffsetData::validateCore(anchor.object(),geometry);if(s!=Valid)return s;}
    if(!geometry)return Valid;
    AcDbObjectPointer<TVS_TAP> a(r.members[0],AcDb::kForRead),b(r.members[2],AcDb::kForRead),c(r.members[4],AcDb::kForRead),d(r.members[6],AcDb::kForRead);
    AcDbObjectPointer<TVS_Pipe> h(r.members[3],AcDb::kForRead),in(r.inlet,AcDb::kForRead),out(r.outlet,AcDb::kForRead);
    if(a.openStatus()!=Acad::eOk||b.openStatus()!=Acad::eOk||c.openStatus()!=Acad::eOk||d.openStatus()!=Acad::eOk||h.openStatus()!=Acad::eOk)return Corrupt;
    if(in.openStatus()!=Acad::eOk||out.openStatus()!=Acad::eOk)return Incomplete;
    const double f=r.factor;
    if(h->ThisRound||in->ThisRound||out->ThisRound||h->DuctType!=DuctTypeStill||in->DuctType!=DuctTypeStill||out->DuctType!=DuctTypeStill||h->FirstPoint.distanceTo(h->LastPoint)*f<=.001
        ||std::fabs(a->Elev-d->Elev)*f>.001||std::fabs(b->Elev-c->Elev)*f>.001||std::fabs(h->Elev-b->Elev)*f>.001
        ||std::fabs(a->SizeA-c->SizeA)*f>.001||std::fabs(a->SizeB-c->SizeB)*f>.001
        ||std::fabs(h->SizeA-a->SizeA)*f>.001||std::fabs(h->SizeB-a->SizeB)*f>.001
        ||std::fabs(a->get_radius()-c->get_radius())*f>.001||std::fabs(a->Swectangle-c->Swectangle)>1e-8
        ||!nearPoint(h->FirstPoint,horizontalPort(b.object()),f)||!nearPoint(h->LastPoint,horizontalPort(c.object()),f)
        ||!nearPoint(r.reverse?in->FirstPoint:in->LastPoint,horizontalPort(a.object()),f)
        ||!nearPoint(r.reverse?out->LastPoint:out->FirstPoint,horizontalPort(d.object()),f)
        ||std::fabs(in->Elev-a->Elev)*f>.001||std::fabs(out->Elev-a->Elev)*f>.001)return GeometryMismatch;
    AcGeVector3d direction=h->LastPoint-h->FirstPoint;direction.normalize();
    for(int k=0;k<2;++k){AcDbObjectPointer<TVS_Pipe> slope(r.members[k?5:1],AcDb::kForRead);if(slope.openStatus()!=Acad::eOk)return Corrupt;
        AcGeVector3d v=slope->LastPoint-slope->FirstPoint;if(v.length()*f<=.001)return GeometryMismatch;v.normalize();
        if(v.dotProduct(direction)<1-1e-8)return GeometryMismatch;}
    for(int k=0;k<2;++k){if(r.neighbors[k].isNull()){if(r.ports[k])return Corrupt;continue;}
        if(r.ports[k]!=1&&r.ports[k]!=2)return Corrupt;
        AcDbObjectPointer<TVS_Pipe> neighbor(r.neighbors[k],AcDb::kForRead);if(neighbor.openStatus()!=Acad::eOk)return Incomplete;
        const AcGePoint3d expected=k?(r.reverse?out->FirstPoint:out->LastPoint):(r.reverse?in->LastPoint:in->FirstPoint);
        if(!nearPoint(r.ports[k]==1?neighbor->FirstPoint:neighbor->LastPoint,expected,f)||std::fabs(neighbor->Elev-a->Elev)*f>.001
            ||std::fabs(neighbor->SizeA-a->SizeA)*f>.001||std::fabs(neighbor->SizeB-a->SizeB)*f>.001)return GeometryMismatch;
    }
    return Valid;
}
static Acad::ErrorStatus newRecord(AcDbDictionary* dict,const ACHAR* key,const resbuf& data,AcDbTransactionManager* tx,AcDbObjectId& id){
    AcDbXrecord* r=new AcDbXrecord();Acad::ErrorStatus es=r->setFromRbChain(data);if(es!=Acad::eOk){delete r;return es;}
    r->setXlateReferences(true);es=dict->setAt(key,r,id);if(es!=Acad::eOk){delete r;return es;}
    return tx->addNewlyCreatedDBRObject(r,true);
}
Acad::ErrorStatus write(AcDbDatabase* db,Record& r,AcDbObject* members[7],AcDbObjectId& parent){
    AcDbTransactionManager* tx=db?db->transactionManager():NULL;if(!tx||!tx->numActiveTransactions())return Acad::eInvalidInput;
    r.guid=tvsOffsetData::newGuid();if(r.guid.IsEmpty())return Acad::eInvalidInput;
    resbuf nodes[27]={};for(int j=0;j<27;++j){nodes[j].restype=static_cast<short>(typeAt(j));nodes[j].rbnext=j<26?&nodes[j+1]:NULL;}
    nodes[0].resval.rlong=1;nodes[1].resval.rstring=const_cast<ACHAR*>(r.guid.GetString());nodes[2].resval.rlong=r.revision;nodes[3].resval.rreal=r.factor;
    const AcDbObjectId refs[]={r.members[0],r.members[3],r.members[4],r.inlet,r.outlet,r.obstacle};
    for(int j=0;j<6;++j)if(acdbGetAdsName(nodes[j+4].resval.rlname,refs[j])!=Acad::eOk)return Acad::eInvalidInput;
    nodes[10].resval.rint=r.reverse?1:0;for(int j=0;j<4;++j)nodes[11+j].resval.rreal=r.policy[j];for(int j=0;j<8;++j)nodes[15+j].resval.rreal=r.obstacleSnapshot[j];
    for(int j=0;j<2;++j){if(!r.neighbors[j].isNull())acdbGetAdsName(nodes[23+j].resval.rlname,r.neighbors[j]);nodes[25+j].resval.rint=static_cast<short>(r.ports[j]);}
    AcDbObject* raw=NULL;Acad::ErrorStatus es=tx->getObject(raw,db->namedObjectsDictionaryId(),AcDb::kForWrite);if(es!=Acad::eOk)return es;
    AcDbDictionary* nod=AcDbDictionary::cast(raw);if(!nod)return Acad::eInvalidInput;
    CString key=L"TVS_BYPASS_"+r.guid;es=newRecord(nod,key,nodes[0],tx,parent);if(es!=Acad::eOk)return es;
    for(int j=0;j<7;++j){es=members[j]->createExtensionDictionary();if(es!=Acad::eOk)return es;
        raw=NULL;es=tx->getObject(raw,members[j]->extensionDictionary(),AcDb::kForWrite);if(es!=Acad::eOk)return es;
        AcDbDictionary* dict=AcDbDictionary::cast(raw);if(!dict||dict->has(memberKey()))return Acad::eInvalidInput;
        resbuf linkData[3]={};linkData[0].restype=90;linkData[0].resval.rlong=1;linkData[0].rbnext=&linkData[1];
        linkData[1].restype=330;acdbGetAdsName(linkData[1].resval.rlname,parent);linkData[1].rbnext=&linkData[2];linkData[2].restype=70;linkData[2].resval.rint=static_cast<short>(j);
        AcDbObjectId id;es=newRecord(dict,memberKey(),linkData[0],tx,id);if(es!=Acad::eOk)return es;
    }
    return Acad::eOk;
}
}
#endif
