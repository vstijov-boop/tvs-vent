#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetAssemblyData.h"
#include "TVSMoveCommand.h"
#include "../geometry/OffsetOperation.h"
#include "TVSProperty.h"
#include <sstream>
#include <iomanip>
#include <memory>
#include <set>

namespace tvsOffsetService {
enum Outcome { Applied,NoChange,Cancelled,Failed };
struct Policy {double minimumStraightMm,minimumProjectionMm;Policy():minimumStraightMm(100),minimumProjectionMm(.1){}};
inline Policy& policy(){static Policy p;return p;}
inline bool& failNext(){static bool value=false;return value;}
struct Snapshot {
    AcDbDatabase* db;AcDbObjectId source,owner;
    tvsOffsetOperation::Snapshot physical;
    std::wstring fingerprint,network,manufacturer,note;
    Snapshot():db(NULL){}
};
struct Commit {AcDbObjectId output,members[3];TVS_Pipe* residentOutput;double elevation,angle,before;
    Commit():residentOutput(NULL),elevation(0),angle(0),before(0){}};
inline bool reject(const wchar_t* reason){acutPrintf(L"\nTVS OFFSET: %s Nema izmjena.",reason);return false;}
inline bool checked(Acad::ErrorStatus e){return e==Acad::eOk;}
inline void putPoint(std::wostringstream& s,const AcGePoint3d& p){s<<p.x<<L','<<p.y<<L','<<p.z<<L';';}
inline void putId(std::wostringstream& s,const AcDbObjectId& id){s<<id.asOldId()<<L';';}
inline void putString(std::wostringstream& s,const std::wstring& value){s<<value.size()<<L':'<<value<<L';';}
// Do not inspect uninitialized legacy free-connector cTypeAnother/point fields.
inline bool connections(TVS_Entity* entity,std::wostringstream& out){
    out<<entity->connectors.size()<<L';';
    for(const auto& c:entity->connectors){out<<int(c.cTypeCurrent)<<L';';putId(out,c.connectionID);
        if(!c.connectionID.isNull()){out<<int(c.cTypeAnother)<<L';';putPoint(out,c.point);}}
    return true;
}
inline bool properties(TVS_Pipe* source,Snapshot& s,std::wostringstream& out){
    // No unknown dictionary payload can be safely interpreted after changing length.
    if(!source->extensionDictionary().isNull()){
        AcDbObjectPointer<AcDbDictionary> d(source->extensionDictionary(),AcDb::kForRead);
        if(d.openStatus()!=Acad::eOk||d->numEntries()!=0)return reject(L"Nepodrzani extension podaci kanala.");
    }
    resbuf* raw=source->xData(NULL);
    struct Release {resbuf* p;~Release(){if(p)acutRelRb(p);}} release={raw};
    bool app=false;std::set<int> seen;
    for(resbuf* r=raw;r;){
        if(r->restype==1001){
            if(app||!r->resval.rstring||wcscmp(r->resval.rstring,CTVSProperty)!=0)
                return reject(L"Nepoznata XData aplikacija; atributi nisu kopirani niti izgubljeni.");
            app=true;out<<L"TVS_Property;";r=r->rbnext;continue;
        }
        if(!app||r->restype!=1070||!r->rbnext)return reject(L"Neispravni korisnicki podaci.");
        const int tag=r->resval.rint;resbuf* v=r->rbnext;
        if(tag<0||tag>9||!seen.insert(tag).second)return reject(L"Nepoznata/dupla korisnicka osobina.");
        out<<tag<<L';';
        if(tag==7||tag==8){
            if(v->restype!=1040||v->resval.rreal!=0)return reject(L"Rucna kolicina/masa nije podrzana pri podjeli kanala.");
            out<<v->resval.rreal<<L';';
        }else{
            if(v->restype!=1000||!v->resval.rstring)return reject(L"Neispravna tekstualna osobina.");
            std::wstring value=v->resval.rstring;putString(out,value);
            // Position remains only on the original object. Manufacturer and a shared
            // note are non-unique. Product/size/unit/name overrides are ambiguous.
            if(tag==5)s.manufacturer=value;
            else if(tag==9)s.note=value;
            else if(tag!=0&&!value.empty())return reject(L"Rucni naziv/tip/artikal/presjek/jedinica zahtijeva posebnu politiku prenosa.");
        }
        r=v->rbnext;
    }
    return true;
}
inline bool pipeState(TVS_Pipe* p,Snapshot& s){
    if(!p||p->isA()!=TVS_Pipe::desc()||!tvsMove::editable(p)||tvsOffsetData::marked(p))
        return reject(L"Potreban je obican editabilan TVS kanal, ne clan sklopa.");
    if(p->ThisRound||p->DuctType!=DuctTypeStill||p->Direct!=0||p->Form!=0
        ||!tvsOffset::finite(p->Flow)||p->Flow<0)
        return reject(L"Podrzan je kruti pravougaoni kanal bez posebnog smjera Direct/Form.");
    // Direct=0 is the only documented legacy drawing value. Flow is a nonnegative
    // scalar magnitude; reversing the selected end never negates it or swaps source ports.
    s.physical.first=tvsOffset::WorldPoint(p->FirstPoint.x,p->FirstPoint.y,p->FirstPoint.z);
    s.physical.last=tvsOffset::WorldPoint(p->LastPoint.x,p->LastPoint.y,p->LastPoint.z);
    s.physical.a=p->SizeA;s.physical.b=p->SizeB;s.physical.elevation=p->get_Elevation();
    std::wostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(17);
    putPoint(out,p->FirstPoint);putPoint(out,p->LastPoint);putId(out,p->ownerId());
    out<<p->SizeA<<L';'<<p->SizeB<<L';'<<p->Elev<<L';'<<p->ElevUp<<L';'<<p->ElevDown<<L';'
       <<p->Flow<<L';'<<p->Direct<<L';'<<p->Param<<L';'<<p->Form<<L';'<<p->IsPipe<<L';'
       <<p->Wipeout<<L';'<<p->WipeoutLength<<L';'<<p->This1D<<L';'<<p->Grani<<L';';
    putId(out,p->layerId());putId(out,p->linetypeId());putId(out,p->materialId());
    AcDbObjectId plotStyle;out<<int(p->getPlotStyleNameId(plotStyle))<<L';';putId(out,plotStyle);
    const AcCmColor color=p->color();out<<int(color.colorMethod())<<L';'<<color.colorIndex()<<L';'
       <<int(color.red())<<L';'<<int(color.green())<<L';'<<int(color.blue())<<L';'
       <<p->linetypeScale()<<L';'<<int(p->lineWeight())<<L';'<<int(p->visibility())<<L';'
       <<p->transparency().serializeOut()<<L';';
    connections(p,out);
    if(!properties(p,s,out))return false;
    s.fingerprint=out.str();return true;
}
inline bool describe(TVS_Entity* e,tvsMove::Node& n){
    if(!tvsMove::describe(e,n))return false;
    TVS_TAP* tap=TVS_TAP::cast(e);
    if(tap&&(tap->Form==Form_Up||tap->Form==Form_Down)){
        const double r=tap->get_radius()+tap->SizeB/2;
        const AcGeVector3d start=tap->Startvector;
        if(tap->ThisRound||tap->DuctType!=DuctTypeStill||!tvsOffset::finite(r)||r<=0
            ||tap->Swectangle<=0||tap->Swectangle>=tvsOffset::pi()/2
            ||std::fabs(start.length()-1)>1e-8||std::fabs(start.z)>1e-8
            ||(tap->Normvector-AcGeVector3d(0,0,1)).length()>1e-8)return false;
        const AcGeVector3d u(-start.y,start.x,0);
        n.ports.clear();n.axes.clear();n.sections.clear();n.kind=2;
        n.ports.push_back(tvsMove::point(tap->MiddlePoint-u*(r*std::sin(tap->Swectangle))));
        n.ports.push_back(tvsMove::point(tap->MiddlePoint));
        n.axes.push_back(tvsMoveGeometry::Point(u.x,u.y,0));
        n.axes.push_back(tvsMoveGeometry::Point(0,0,1)); // sloping port cannot join a horizontal pipe
        n.sections.push_back(tvsMoveNetwork::Section(tap->SizeA,tap->SizeB,false,tap->Elev));
        n.sections.push_back(tvsMoveNetwork::Section(tap->SizeA,tap->SizeB,false,
            tap->Elev+(tap->Form==Form_Up?1:-1)*r*(1-std::cos(tap->Swectangle))));
    }
    return true;
}
inline bool topology(const Snapshot& s,TVS_Pipe* source,std::wstring& signature){
    const double tol=.001/s.physical.factor;
    const int selected=s.physical.end==tvsOffsetOperation::First?0:1,opposite=1-selected;
    const AcGePoint3d ends[]={source->FirstPoint,source->LastPoint};
    int ownIndex[2]={-1,-1};
    for(size_t k=0;k<source->connectors.size();++k){
        const int type=source->connectors[k].cTypeCurrent;
        if(type!=conPipeStart&&type!=conPipeEnd)return reject(L"Nepoznata uloga konektora.");
        const int port=type==conPipeStart?0:1;
        if(ownIndex[port]>=0)return reject(L"Dupli konektor kanala.");
        ownIndex[port]=static_cast<int>(k);
    }
    if(!source->connectors.empty()&&(ownIndex[0]<0||ownIndex[1]<0))return reject(L"Nepotpun par konektora.");
    if(ownIndex[selected]>=0&&!source->connectors[ownIndex[selected]].connectionID.isNull())
        return reject(L"Izabrani izlaz ima vezu; mreza se ne pomjera.");
    AcDbObjectId contactId[2];int contactPort[2]={-1,-1},count[2]={0,0},inbound=0;
    std::wostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(17);
    AcDbObjectPointer<AcDbBlockTableRecord> space(s.owner,AcDb::kForRead);
    if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* raw=NULL;if(space->newIterator(raw)!=Acad::eOk)return false;
    std::unique_ptr<AcDbBlockTableRecordIterator> it(raw);
    for(;!it->done();it->step()){
        AcDbEntity* rawEntity=NULL;if(it->getEntity(rawEntity,AcDb::kForRead)!=Acad::eOk)return false;
        AcDbObjectPointer<AcDbEntity> object;object.acquire(rawEntity);
        TVS_Entity* e=TVS_Entity::cast(object.object());if(!e||e->objectId()==s.source)continue;
        tvsMove::Node node;if(!describe(e,node)||!node.kind||node.ports.size()!=node.axes.size()
            ||node.ports.size()!=node.sections.size())return reject(L"Nepodrzana TVS geometrija u Model prostoru.");
        putId(out,e->objectId());connections(e,out);
        for(const auto& c:e->connectors)if(c.connectionID==s.source){
            ++inbound;
            if(ownIndex[opposite]<0)return reject(L"Jednosmjerna ulazna konektorska veza.");
            const TVS_Connector& own=source->connectors[ownIndex[opposite]];
            if(own.connectionID!=e->objectId()||c.cTypeAnother!=own.cTypeCurrent||own.cTypeAnother!=c.cTypeCurrent
                ||c.point.distanceTo(ends[opposite])>tol||own.point.distanceTo(ends[opposite])>tol)
                return reject(L"Pokvarena ili nedvosmislena reciprocna veza nije potvrdjena.");
        }
        out<<node.kind<<L';';
        for(size_t j=0;j<node.ports.size();++j){
            const auto& p=node.ports[j];const auto& sec=node.sections[j];const auto& axis=node.axes[j];
            out<<p.x<<L';'<<p.y<<L';'<<p.z<<L';'<<sec.a<<L';'<<sec.b<<L';'<<sec.elevation<<L';'
               <<sec.round<<L';'<<axis.x<<L';'<<axis.y<<L';'<<axis.z<<L';';
            if(std::fabs(sec.elevation-source->Elev)>tol)continue;
            for(int k=0;k<2;++k)if(tvsMoveGeometry::pointsNear(p,tvsMove::point(ends[k]),tol)){
                if(!node.kind||!tvsMoveNetwork::compatible(sec,tvsMoveNetwork::Section(source->SizeA,source->SizeB,false,source->Elev),tol))
                    return reject(L"Nepodrzan ili nesaglasan prikljucak.");
                tvsMoveGeometry::Point u,v;
                if(!tvsMoveGeometry::unit(axis,u)||!tvsMoveGeometry::unit(tvsMoveGeometry::Point(ends[1].x-ends[0].x,ends[1].y-ends[0].y,ends[1].z-ends[0].z),v)
                    ||std::fabs(u.dot(v))<1-1e-8)return reject(L"Prikljucak nije tangentan.");
                ++count[k];contactId[k]=e->objectId();contactPort[k]=static_cast<int>(j);
            }
            if(tvsMoveNetwork::interior(p,tvsMove::point(ends[0]),tvsMove::point(ends[1]),tol))
                return reject(L"Prikljucak/racva u unutrasnjosti kanala.");
        }
        if(TVS_Pipe* other=TVS_Pipe::cast(e)){
            if(std::fabs(other->Elev-source->Elev)<=tol)for(int k=0;k<2;++k)
                if(tvsMoveNetwork::interior(tvsMove::point(ends[k]),tvsMove::point(other->FirstPoint),tvsMove::point(other->LastPoint),tol))
                    return reject(L"Kraj dodiruje unutrasnjost drugog kanala.");
        }
    }
    if(count[selected]!=0||count[opposite]>1||inbound>1)return reject(L"Izlaz nije slobodan ili je suprotni prikljucak dvosmislen.");
    if(ownIndex[opposite]>=0){const auto& own=source->connectors[ownIndex[opposite]];
        if(!own.connectionID.isNull()){
            // First version supports explicit reciprocal pipe ports; other stored
            // fitting port conventions are rejected rather than guessed.
            AcDbObjectPointer<TVS_Pipe> neighbor(own.connectionID,AcDb::kForRead);
            if(count[opposite]!=1||inbound!=1||contactId[opposite]!=own.connectionID
                ||neighbor.openStatus()!=Acad::eOk||own.cTypeAnother!=(contactPort[opposite]==0?conPipeStart:conPipeEnd))
                return reject(L"Suprotna reciprokna pipe veza nije geometrijski potvrdjena.");
        }else if(inbound)return false;
    }
    signature=out.str();return true;
}
inline bool capture(const AcDbObjectId& id,tvsOffsetOperation::End end,Snapshot& s){
    s=Snapshot();s.db=acdbHostApplicationServices()->workingDatabase();s.source=id;s.physical.end=end;
    if(!s.db||id.isNull()||id.database()!=s.db)return false;
    s.physical.factor=tvsUnits::factor(s.db);
    if(s.physical.factor!=1&&s.physical.factor!=10&&s.physical.factor!=1000)return reject(L"Podrzane su mm/cm/m jedinice.");
    AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForRead);
    if(p.openStatus()!=Acad::eOk)return reject(L"Kanal nije dostupan.");
    s.owner=p->ownerId();
    if(s.owner!=tvsOffsetProof::modelId(s.db)||s.db->currentSpaceId()!=s.owner)return reject(L"Potreban je direktan kanal u Model prostoru.");
    if(!pipeState(p.object(),s))return false;
    tvsOffsetOperation::Request validation;validation.elevation=s.physical.elevation;
    if(tvsOffsetOperation::prepare(s.physical,validation).status!=tvsOffsetOperation::NoChange)
        return reject(L"Neispravan ili nehorizontalan kanal.");
    return topology(s,p.object(),s.network);
}
inline bool current(const Snapshot& before){
    Snapshot now;if(!capture(before.source,before.physical.end,now))return false;
    if(now.db!=before.db||now.owner!=before.owner||now.physical.factor!=before.physical.factor
        ||now.fingerprint!=before.fingerprint||now.network!=before.network)
        return reject(L"Izvor, svojstva, veze ili jedinice su promijenjeni poslije snapshot-a.");
    return true;
}
inline void initialize(TVS_Entity* e){tvsOffsetProof::initialize(e);e->ElevUp=0;e->ElevDown=0;}
inline void initializePipe(TVS_Pipe* p){
    initialize(p);TVS_Connector a,b;a.connectionID=AcDbObjectId::kNull;b.connectionID=AcDbObjectId::kNull;
    a.cTypeCurrent=conPipeStart;b.cTypeCurrent=conPipeEnd;a.cTypeAnother=b.cTypeAnother=0;
    a.point=b.point=AcGePoint3d::kOrigin;p->connectors.clear();p->connectors.push_back(a);p->connectors.push_back(b);
}
inline bool appearance(TVS_Pipe* source,TVS_Entity* target,const Snapshot& s){
    if(!checked(target->setPropertiesFrom(source)))return false;
    // setPropertiesFrom copies color/layer/linetype/scale/visibility only.
    AcDbObjectId plotStyle;const AcDb::PlotStyleNameType plotType=source->getPlotStyleNameId(plotStyle);
    if(!checked(target->setLineWeight(source->lineWeight()))||!checked(target->setMaterial(source->materialId()))
        ||!checked(target->setTransparency(source->transparency()))||!checked(target->setPlotStyleName(plotType,plotStyle)))return false;
    target->Flow=source->Flow;target->Direct=0;target->Grani=source->Grani;
    target->Wipeout=source->Wipeout;target->This1D=source->This1D;target->WipeoutLength=source->WipeoutLength;
    // Only known shared properties. Never clone position, quantity overrides or identities.
    if(!s.manufacturer.empty()||!s.note.empty()){
        resbuf* data=acutBuildList(1001,CTVSProperty,1070,5,1000,s.manufacturer.c_str(),
            1070,9,1000,s.note.c_str(),0);
        if(!data)return false;
        const Acad::ErrorStatus es=target->setXData(data);acutRelRb(data);
        if(!checked(es))return false;
    }
    return true;
}
inline Outcome write(const Snapshot& s,const tvsOffsetOperation::Request& request,const tvsOffsetOperation::Plan& plan,Commit& result){
    if(plan.status!=tvsOffsetOperation::Ready||!current(s))return Failed;
    // Recompute only the deterministic plan from the same snapshot/request; reject a
    // mismatched DTO rather than accepting caller-modified geometry. No second solver path.
    const auto verified=tvsOffsetOperation::prepare(s.physical,request);
    if(verified.status!=tvsOffsetOperation::Ready||verified.geometry.straightLength!=plan.geometry.straightLength
        ||verified.acceptedElevation!=plan.acceptedElevation||verified.angleDegrees!=plan.angleDegrees)return Failed;
    const auto& d=verified.dto;const auto& g=verified.geometry;
    tvsOffsetProof::BuildTransaction tx(s.db);if(!tx.space)return Failed;
    tvsOffsetData::WriteScope scope;AcDbObject* base=NULL;
    if(actrTransactionManager->getObject(base,s.source,AcDb::kForWrite)!=Acad::eOk)return Failed;
    TVS_Pipe* source=TVS_Pipe::cast(base);if(!source)return Failed;
    Snapshot inside=s;inside.manufacturer.clear();inside.note.clear();
    if(!pipeState(source,inside)||inside.fingerprint!=s.fingerprint)return Failed;
    AcDbObject* members[3]={NULL,NULL,NULL};Commit pending;
    for(int j=0;j<2;++j){
        const auto& e=d.elbows[j];std::unique_ptr<TVS_TAP> tap(new TVS_TAP());initialize(tap.get());tap->setDatabaseDefaults();
        if(!appearance(source,tap.get(),s))return Failed;
        tap->SizeA=e.sizeA;tap->SizeB=e.sizeB;tap->ThisRound=false;tap->DuctType=DuctTypeStill;
        tap->TypeRoundTap=TypeRoundTap_TapSection;tap->RadiusTypeRect=RadiusTypeRect_RadiusConstant;
        tap->RadiusTypeRound=RadiusTypeRound_RadiusConstant;tap->RadiusVariableParameter=.5;tap->RadiusConst=e.innerRadius;
        tap->Swectangle=e.angle;tap->Startvector=AcGeVector3d(e.startVector.x,e.startVector.y,0);tap->Normvector=AcGeVector3d(0,0,1);
        tap->MiddlePoint=tvsOffsetProof::pt(e.middle);tap->CenterPoint=tap->MiddlePoint;
        if(!checked(tap->put_Form(e.form==tvsOffsetAdapter::Up?Form_Up:Form_Down))||!checked(tap->put_Elevation(e.elevation)))return Failed;
        const int role=j==0?0:2;TVS_TAP* raw=tap.release();
        if(!tx.append(raw,pending.members[role]))return Failed;
        members[role]=raw;
    }
    std::unique_ptr<TVS_Pipe> slope(new TVS_Pipe());initializePipe(slope.get());slope->setDatabaseDefaults();
    if(!appearance(source,slope.get(),s))return Failed;
    slope->SizeA=s.physical.a;slope->SizeB=s.physical.b;slope->ThisRound=false;slope->DuctType=DuctTypeStill;
    slope->FirstPoint=tvsOffsetProof::pt(d.middle.first);slope->LastPoint=tvsOffsetProof::pt(d.middle.last);
    if(!checked(slope->put_Elevation((d.middle.firstElevationMm+d.middle.lastElevationMm)/2/s.physical.factor)))return Failed;
    slope->connectors[0].point=slope->FirstPoint;slope->connectors[1].point=slope->LastPoint;
    TVS_Pipe* rawSlope=slope.release();
    if(!tx.append(rawSlope,pending.members[1]))return Failed;
    members[1]=rawSlope;
    std::unique_ptr<TVS_Pipe> exit(new TVS_Pipe());initializePipe(exit.get());exit->setDatabaseDefaults();
    if(!appearance(source,exit.get(),s))return Failed;
    exit->SizeA=s.physical.a;exit->SizeB=s.physical.b;exit->ThisRound=false;exit->DuctType=DuctTypeStill;
    exit->FirstPoint=tvsOffsetProof::pt(verified.exitFirst);exit->LastPoint=tvsOffsetProof::pt(verified.exitLast);
    if(!checked(exit->put_Elevation(verified.acceptedElevation)))return Failed;
    exit->connectors[0].point=exit->FirstPoint;exit->connectors[1].point=exit->LastPoint;
    TVS_Pipe* rawExit=exit.release();
    if(!tx.append(rawExit,pending.output))return Failed;
    pending.residentOutput=rawExit;
    const CString guid=tvsOffsetData::newGuid();if(guid.IsEmpty())return Failed;
    for(int j=0;j<3;++j){tvsOffsetData::Record record;record.guid=guid;record.role=j;
        for(int k=0;k<3;++k)record.members[k]=pending.members[k];
        record.rise=j==1?g.axis[2].z-g.axis[1].z:0;
        if(tvsOffsetData::write(members[j],record)!=Acad::eOk)return Failed;
    }
    // Source vector and its opposite connector are intentionally never modified:
    // legacy connectors are not filed, so they must not rely on DB undo/rollback.
    source->assertWriteEnabled();
    source->FirstPoint=tvsOffsetProof::pt(verified.remainingFirst);source->LastPoint=tvsOffsetProof::pt(verified.remainingLast);
    source->recordGraphicsModified();
    if(failNext()){failNext()=false;acutPrintf(L"\nDEV INJECTED FAILURE after source endpoints; aborting full offset transaction.");return Failed;}
    if(!tx.commit())return Failed;
    pending.elevation=verified.acceptedElevation;pending.angle=verified.angleDegrees;pending.before=s.physical.elevation;
    result=pending;
    acutPrintf(L"\nTVS OFFSET DEV: P=%.6f, riseStraight=%.6f, Lphysical=%.6f mm.",g.projectedLength,g.axis[2].z-g.axis[1].z,g.straightLength);
    return Applied;
}
}
#endif
