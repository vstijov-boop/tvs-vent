#include "StdAfx.h"
#ifdef TVS_OFFSET_INTEGRATION
#include "TVS_Entity.h"
#include "TVS_Pipe.h"
#include "TVS_TAP.h"
#include "OffsetAssemblyData.h"
#include "dbdict.h"
#include "dbxrecrd.h"
#include "dbobjptr.h"
#include "../geometry/OffsetAdapter.h"
#include <set>
#include <map>
#include <limits>
#include "../geometry/OffsetRecordRules.h"
namespace tvsOffsetData {
static int writeDepth=0;static std::set<AcDbObjectId> clones;
WriteScope::WriteScope(){++writeDepth;}WriteScope::~WriteScope(){--writeDepth;}
void permitClone(const AcDbObjectId& id){clones.insert(id);}void clearClonePermits(){clones.clear();}
const ACHAR* statusText(Status s){switch(s){case Legacy:return L"Legacy";case Valid:return L"Valid";case Incomplete:return L"Incomplete";case UnsupportedVersion:return L"UnsupportedVersion";case GeometryMismatch:return L"GeometryMismatch";default:return L"Corrupt";}}
CString newGuid(){GUID id;OLECHAR value[40]={0};if(FAILED(CoCreateGuid(&id)))return CString();StringFromGUID2(id,value,40);return CString(value);}
Status read(const AcDbObject* object,Record& out){
    if(!object)return Corrupt;
    AcDbObjectId dictId=object->extensionDictionary();if(dictId.isNull())return Legacy;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return Corrupt;
    AcDbObjectId id;if(dict->getAt(L"TVS_OFFSET_MEMBER",id)!=Acad::eOk)return Legacy;
    AcDbObjectPointer<AcDbXrecord> record(id,AcDb::kForRead);if(record.openStatus()!=Acad::eOk)return Corrupt;
    resbuf* data=NULL;if(record->rbChain(&data)!=Acad::eOk||!data)return Corrupt;
    Record r;resbuf* p=data;bool ok=true;
    const int types[]={90,1,70,330,330,330,40};
    for(int j=0;j<7;++j){if(!p||p->restype!=types[j]){ok=false;break;}
        if(j==0)r.version=p->resval.rlong;
        if(j==1){if(!p->resval.rstring){ok=false;break;}r.guid=p->resval.rstring;}
        if(j==2)r.role=p->resval.rint;
        if(j>=3&&j<=5){if(p->resval.rlname[0]||p->resval.rlname[1])
            if(acdbGetObjectId(r.members[j-3],p->resval.rlname)!=Acad::eOk){ok=false;break;}}
        if(j==6)r.rise=p->resval.rreal;p=p->rbnext;
    }
    if(p)ok=false;acutRelRb(data);
    if(!ok)return Corrupt;out=r;
    bool guidShape=r.guid.GetLength()==38&&r.guid[0]==L'{'&&r.guid[37]==L'}';
    tvsOffsetRecordRules::ScalarStatus scalar=tvsOffsetRecordRules::validate(r.version,r.role,guidShape,r.rise);
    if(scalar==tvsOffsetRecordRules::ScalarUnsupported)return UnsupportedVersion;
    if(scalar!=tvsOffsetRecordRules::ScalarValid)return Corrupt;
    if(r.members[r.role]!=object->objectId())return Corrupt;
    return Valid;
}
Acad::ErrorStatus write(AcDbObject* object,const Record& r){
    if(!object||!object->isWriteEnabled()||r.guid.IsEmpty())return Acad::eInvalidInput;
    Acad::ErrorStatus es=object->createExtensionDictionary();if(es!=Acad::eOk)return es;
    AcDbObjectPointer<AcDbDictionary> dict(object->extensionDictionary(),AcDb::kForWrite);if(dict.openStatus()!=Acad::eOk)return dict.openStatus();
    AcDbObjectId id;AcDbXrecord* record=NULL;
    if(dict->getAt(L"TVS_OFFSET_MEMBER",id)==Acad::eOk){es=acdbOpenObject(record,id,AcDb::kForWrite);if(es!=Acad::eOk)return es;}
    else {record=new AcDbXrecord();es=dict->setAt(L"TVS_OFFSET_MEMBER",record,id);if(es!=Acad::eOk){delete record;return es;}}
    resbuf nodes[7]={};const int types[]={90,1,70,330,330,330,40};
    for(int j=0;j<7;++j){nodes[j].restype=static_cast<short>(types[j]);nodes[j].rbnext=j<6?&nodes[j+1]:NULL;}
    nodes[0].resval.rlong=r.version;nodes[1].resval.rstring=const_cast<ACHAR*>(r.guid.GetString());nodes[2].resval.rint=static_cast<short>(r.role);
    for(int j=0;j<3;++j)if(!r.members[j].isNull())acdbGetAdsName(nodes[j+3].resval.rlname,r.members[j]);
    nodes[6].resval.rreal=r.rise;record->setXlateReferences(true);es=record->setFromRbChain(nodes[0]);record->close();return es;
}
bool marked(const AcDbObject* o){Record r;return read(o,r)!=Legacy;}
bool editable(const AcDbObject* o){return writeDepth>0||!marked(o)||clones.find(o->objectId())!=clones.end();}
bool allowTransform(const AcDbObject* o,const AcGeMatrix3d& m){
    if(!marked(o)||writeDepth>0)return true;
    if(!o->objectId().isNull()&&clones.find(o->objectId())==clones.end())return false;
    // Only positioning translations of known newly cloned/scratch objects, never scale/rotate.
    for(int r=0;r<4;++r)for(int c=0;c<4;++c){
        if(!tvsOffset::finite(m.entry[r][c]))return false;
        if(c==3&&r<3)continue;
        if(std::fabs(m.entry[r][c]-(r==c?1.0:0.0))>1e-10)return false;
    }
    return true;
}
static bool nearPoint(const AcGePoint3d& a,const AcGePoint3d& b,double f){return a.distanceTo(b)*f<=.001;}
Status validate(const AcDbObject* o,bool geometry){
    Record r;Status s=read(o,r);if(s!=Valid)return s;
    for(int j=0;j<3;++j){if(r.members[j].isNull()||r.members[j].isErased())return Incomplete;
        if(r.members[j].database()!=o->database())return Corrupt;
        for(int k=0;k<j;++k)if(r.members[k]==r.members[j])return Corrupt;}
    AcDbObjectPointer<TVS_TAP> a(r.members[0],AcDb::kForRead),b(r.members[2],AcDb::kForRead);
    AcDbObjectPointer<TVS_Pipe> p(r.members[1],AcDb::kForRead);
    if(a.openStatus()!=Acad::eOk||b.openStatus()!=Acad::eOk||p.openStatus()!=Acad::eOk)return Incomplete;
    const AcDbObject* objects[]={a.object(),p.object(),b.object()};Record members[3];
    for(int j=0;j<3;++j){Status q=read(objects[j],members[j]);if(q!=Valid)return q;
        if(members[j].role!=j||members[j].guid!=r.guid)return Corrupt;
        for(int k=0;k<3;++k)if(members[j].members[k]!=r.members[k])return Corrupt;}
    if(!geometry)return Valid;
    double f=tvsUnits::factor(o);double delta=(b->get_Elevation()-a->get_Elevation())*f;
    if(a->ThisRound||b->ThisRound||p->ThisRound||a->DuctType!=DuctTypeStill||b->DuctType!=DuctTypeStill||p->DuctType!=DuctTypeStill)return GeometryMismatch;
    if(std::fabs(a->SizeA-p->SizeA)*f>.001||std::fabs(a->SizeB-p->SizeB)*f>.001||std::fabs(b->SizeA-p->SizeA)*f>.001||std::fabs(b->SizeB-p->SizeB)*f>.001
        ||std::fabs(a->get_radius()-b->get_radius())*f>.001||std::fabs(a->Swectangle-b->Swectangle)>1e-8)return GeometryMismatch;
    AcGeVector3d axis=p->LastPoint-p->FirstPoint;if(axis.length()<=0)return GeometryMismatch;
    double heading=std::atan2(axis.y,axis.x);
    tvsOffset::Input in(delta,p->SizeA*f,p->SizeB*f,a->get_radius()*f,a->Swectangle,0);
    tvsOffset::Result result=tvsOffset::solve(in);if(!result.hasGeometry())return GeometryMismatch;
    tvsOffsetAdapter::Placement place(tvsOffset::WorldPoint(a->MiddlePoint.x,a->MiddlePoint.y,a->MiddlePoint.z),heading,f,a->get_Elevation()*f);
    tvsOffsetAdapter::Result d=tvsOffsetAdapter::adapt(in,result,place);if(!d.ready())return GeometryMismatch;
    if(!nearPoint(p->FirstPoint,AcGePoint3d(d.middle.first.x,d.middle.first.y,d.middle.first.z),f)
        ||!nearPoint(p->LastPoint,AcGePoint3d(d.middle.last.x,d.middle.last.y,d.middle.last.z),f)
        ||!nearPoint(b->MiddlePoint,AcGePoint3d(d.elbows[1].middle.x,d.elbows[1].middle.y,d.elbows[1].middle.z),f)
        ||std::fabs(members[1].rise-(result.axis[2].z-result.axis[1].z))>.001
        ||a->get_Form()!=(delta>0?Form_Up:Form_Down)||b->get_Form()!=(delta>0?Form_Down:Form_Up))return GeometryMismatch;
    return Valid;
}
bool physicalLength(const AcDbObject* o,double projected,double& physical){
    Record r;Status s=read(o,r);physical=projected;if(s==Legacy)return true;
    if(s!=Valid||r.role!=SlopedStraight){acutPrintf(L"\nTVS OFFSET: neispravni podaci fizičke dužine (%s).",statusText(s));return false;}
    // Incomplete membership still permits the standalone member's own valid quantity.
    Status integrity=validate(o,writeDepth==0&&clones.find(o->objectId())==clones.end());if(integrity!=Valid&&integrity!=Incomplete){acutPrintf(L"\nTVS OFFSET: neispravno članstvo (%s).",statusText(integrity));return false;}
    double h=r.rise/tvsUnits::factor(o);return tvsOffsetRecordRules::physical(projected,h,physical);
}
Acad::ErrorStatus remapClones(AcDbIdMapping& map){
    std::map<CString,CString> guids;AcDbIdMappingIter iter(map);AcDbIdPair pair;
    WriteScope scope;
    for(iter.start();!iter.done();iter.next()){
        if(!iter.getMap(pair)||!pair.isCloned()||pair.key()==pair.value())continue;
        AcDbObjectPointer<AcDbObject> source(pair.key(),AcDb::kForRead);
        if(source.openStatus()!=Acad::eOk)continue;Record original;Status status=read(source.object(),original);
        if(status==Legacy)continue;if(status!=Valid)return Acad::eInvalidInput;
        AcDbObjectPointer<AcDbObject> dest(pair.value(),AcDb::kForWrite);if(dest.openStatus()!=Acad::eOk)return dest.openStatus();
        Record copy=original;
        if(guids.find(original.guid)==guids.end())guids[original.guid]=newGuid();copy.guid=guids[original.guid];
        for(int j=0;j<3;++j){AcDbIdPair member;member.setKey(original.members[j]);
            copy.members[j]=(map.compute(member)&&member.isCloned())?member.value():AcDbObjectId::kNull;}
        copy.members[copy.role]=pair.value();Acad::ErrorStatus es=write(dest.object(),copy);if(es!=Acad::eOk)return es;permitClone(pair.value());
    }
    return Acad::eOk;
}
}
#endif
