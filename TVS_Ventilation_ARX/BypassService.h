#pragma once
#if defined(TVS_BYPASS_PREVIEW) && defined(TVS_OFFSET_INTEGRATION)
#include "TVSMoveCommand.h"
#include "TVSTestOffset.h"
#include "OffsetService.h"
#include "BypassAssemblyData.h"
#include "../geometry/bypass/PreviewModel.h"
#include <cwchar>
#include <string>
namespace tvsBypassService {
using tvsBypassPreview::View;using tvsBypassPreview::Options;
struct Snapshot {tvsOffsetService::Snapshot route;AcDbObjectId obstacle;std::wstring obstacleState;};
struct Commit {AcDbObjectId output,parent;TVS_Pipe* resident;double elevation,angle;Commit():resident(NULL),elevation(0),angle(0){}};
inline bool obstacleState(const AcDbObjectId& id,std::wstring& value){
    AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForRead);
    // Rectangular or round rigid obstacle (round: SizeB = 0); its state is fingerprinted below.
    if(p.openStatus()!=Acad::eOk||p->isA()!=TVS_Pipe::desc()||p->DuctType!=DuctTypeStill
        ||p->ownerId()!=tvsOffsetProof::modelId(p->database())||tvsOffsetData::marked(p.object()))return false;
    std::wostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(17);
    tvsOffsetService::putPoint(s,p->FirstPoint);tvsOffsetService::putPoint(s,p->LastPoint);
    s<<p->SizeA<<L';'<<p->SizeB<<L';'<<p->Elev;value=s.str();return true;
}
inline bool capture(const AcDbObjectId& route,const AcDbObjectId& obstacle,bool reverse,Snapshot& s){
    if(route==obstacle||tvsBypassData::pinned(route,reverse))return false;
    s.obstacle=obstacle;
    return tvsOffsetService::capture(route,reverse?tvsOffsetOperation::First:tvsOffsetOperation::Last,s.route,true)
        &&obstacle.database()==s.route.db&&obstacleState(obstacle,s.obstacleState);
}
inline bool current(const Snapshot& s){std::wstring now;
    return !tvsBypassData::pinned(s.route.source,s.route.physical.end==tvsOffsetOperation::First)
        &&tvsOffsetService::current(s.route,true)&&obstacleState(s.obstacle,now)&&now==s.obstacleState;
}
inline int connector(TVS_Entity* p,int role){
    int result=-1;
    for(size_t j=0;j<p->connectors.size();++j){
        if(p->connectors[j].cTypeCurrent==role){if(result>=0)return -1;result=static_cast<int>(j);}
    }
    return result;
}
#ifdef TVS_BYPASS_FAULT_INJECTION
// DEV-only rollback probe (defined only for the TVSBypassPreview package). The
// variable is read once per process: set TVS_BYPASS_FAIL before AutoCAD starts.
inline bool inject(int stage){static const int configured=[]{wchar_t text[24]={0};GetEnvironmentVariableW(L"TVS_BYPASS_FAIL",text,24);return _wtoi(text);}();return configured==stage;}
#else
inline bool inject(int){return false;}
#endif
// Every rejection names its phase; SDK failures add the real ErrorStatus.
inline std::wstring& lastFailure(){static std::wstring value;return value;}
inline bool fail(const wchar_t* phase,const wchar_t* reason){
    wchar_t text[400];std::swprintf(text,400,L"[%ls] %ls",phase,reason);
    lastFailure()=text;acutPrintf(L"\nTVS BYPASS %ls Nema izmjena.",text);return false;}
inline bool fail(const wchar_t* phase,const wchar_t* reason,Acad::ErrorStatus status){
    wchar_t text[400];std::swprintf(text,400,L"[%ls] %ls (ErrorStatus %d: %ls)",phase,reason,static_cast<int>(status),acadErrorStatusText(status));
    lastFailure()=text;acutPrintf(L"\nTVS BYPASS %ls Nema izmjena.",text);return false;}
inline bool write(const Snapshot& s,const View& view,const Options& options,Commit& output){
    lastFailure().clear();
    if(!view.ready||view.axis.size()!=8||!view.offsets[0].ready()||!view.offsets[1].ready())return fail(L"snapshot",L"Kandidat nije potpun; ponovite Pronadji prolaz.");
    // Same checks as current(), split so the stale cause is named.
    if(tvsBypassData::pinned(s.route.source,s.route.physical.end==tvsOffsetOperation::First))
        return fail(L"snapshot",L"Izabrani kraj je granica postojeceg bypass sklopa.");
    if(!tvsOffsetService::current(s.route,true))return fail(L"snapshot",L"Kanal ili njegove veze promijenjeni poslije izbora.");
    {std::wstring now;if(!obstacleState(s.obstacle,now)||now!=s.obstacleState)return fail(L"snapshot",L"Prepreka promijenjena poslije izbora.");}
    const auto& base=s.route;const double f=base.physical.factor;const bool reverse=base.physical.end==tvsOffsetOperation::First;
    if(view.frame.factor!=f||view.frame.width!=base.physical.a*f||view.frame.height!=base.physical.b*f
        ||view.frame.z0!=base.physical.elevation*f)return fail(L"snapshot",L"Jedinice/snapshot kandidata ne odgovaraju bazi.");
    const auto& first=view.offsets[0];const auto& second=view.offsets[1];
    const AcGePoint3d oldFirst=tvsOffsetProof::pt(base.physical.first),oldLast=tvsOffsetProof::pt(base.physical.last);
    const AcGePoint3d fixed=reverse?oldLast:oldFirst,outerPoint=reverse?oldFirst:oldLast;
    const AcGeVector3d delta=outerPoint-fixed;const double available=delta.length();if(!(available>0))return fail(L"duzina",L"Izabrani kanal nema duzinu.");
    const AcGeVector3d u=delta/available;
    const AcGePoint3d cutIn=tvsOffsetProof::pt(first.incomingLast),cutOut=tvsOffsetProof::pt(second.outgoingFirst);
    const double a=(cutIn-fixed).dotProduct(u),b=(cutOut-fixed).dotProduct(u);
    if(a*f<1||(available-b)*f<1||b<=a||(cutIn-(fixed+u*a)).length()*f>.001||(cutOut-(fixed+u*b)).length()*f>.001)
        return fail(L"duzina",L"Kandidat mora biti unutar izabranog kanala uz oba ostatka >=1 mm.");
    tvsOffsetProof::BuildTransaction tx(base.db);if(!tx.space)return tx.status==Acad::eOk?fail(L"append",L"Transakcija ili Model prostor nije dostupan."):fail(L"append",L"Model prostor se ne moze otvoriti za upis.",tx.status);tvsOffsetData::WriteScope allow;
    AcDbObject* raw=NULL;{const Acad::ErrorStatus es=actrTransactionManager->getObject(raw,base.source,AcDb::kForWrite);
        if(es!=Acad::eOk)return fail(L"snapshot",L"Izabrani kanal se ne moze otvoriti za upis.",es);}
    TVS_Pipe* source=TVS_Pipe::cast(raw);if(!source)return fail(L"snapshot",L"Izabrani objekat nije TVS kanal.");
    tvsOffsetService::Snapshot inside=base;inside.manufacturer.clear();inside.note.clear();
    if(!tvsOffsetService::pipeState(source,inside)||inside.fingerprint!=base.fingerprint)return fail(L"atributi",L"Atributi ili stanje kanala promijenjeni poslije izbora.");
    // Legacy connectors are not filed (TVS_Pipe::dwgOutFields/dwgInFields skip them), so
    // neither user UNDO nor save/reopen can restore a changed link. Same contract as
    // TVS_OFFSET: the writer never modifies connectors. An explicit link on the selected
    // outer end would need relinking, so it is refused instead.
    const int selectedRole=reverse?conPipeStart:conPipeEnd;const int selected=connector(source,selectedRole);
    if(selected>=0&&!source->connectors[selected].connectionID.isNull())
        return fail(L"konektori",L"Izabrani spoljasnji kraj ima eksplicitnu konektorsku vezu; prevezivanje nije podrzano jer se konektori ne snimaju i Undo ih ne vraca.");
    AcDbObject* members[7]={NULL,NULL,NULL,NULL,NULL,NULL,NULL};tvsBypassData::Record record;
    record.factor=f;record.reverse=reverse;record.inlet=base.source;record.obstacle=s.obstacle;
    for(int j=0;j<2;++j){const int role=j?(reverse?conPipeStart:conPipeEnd):(reverse?conPipeEnd:conPipeStart);
        const int ix=connector(source,role);if(ix>=0&&!source->connectors[ix].connectionID.isNull()){
            record.neighbors[j]=source->connectors[ix].connectionID;record.ports[j]=source->connectors[ix].cTypeAnother;}}
    record.policy[0]=options.clearance;record.policy[1]=options.insulation;record.policy[2]=options.obstacleInsulation;record.policy[3]=options.reserve;
    {AcDbObjectPointer<TVS_Pipe> obstacle(s.obstacle,AcDb::kForRead);if(obstacle.openStatus()!=Acad::eOk)return fail(L"snapshot",L"Prepreka se ne moze procitati.",obstacle.openStatus());
     const double values[]={obstacle->FirstPoint.x,obstacle->FirstPoint.y,obstacle->LastPoint.x,obstacle->LastPoint.y,obstacle->Elev,obstacle->SizeA,obstacle->SizeB,obstacle->FirstPoint.z};
     for(int j=0;j<8;++j)record.obstacleSnapshot[j]=values[j];}
    for(int group=0;group<2;++group){const int start=group?4:0;const auto& dto=view.offsets[group];
        for(int k=0;k<2;++k){auto tap=tvsOffsetService::makeElbow(source,base,dto.elbows[k]);if(!tap)return fail(L"atributi",L"Koljeno nije kreirano (atributi/kota).");
            const int role=start+(k?2:0);TVS_TAP* p=tap.release();if(!tx.append(p,record.members[role]))return fail(L"append",L"Koljeno nije dodato u crtez.",tx.status);members[role]=p;}
        auto slope=tvsOffsetService::makePipe(source,base,dto.middle.first,dto.middle.last,(dto.middle.firstElevationMm+dto.middle.lastElevationMm)/2/f);
        if(!slope)return fail(L"atributi",L"Kosi clan nije kreiran (atributi/kota).");
        TVS_Pipe* p=slope.release();
        if(!tx.append(p,record.members[start+1]))return fail(L"append",L"Kosi clan nije dodat u crtez.",tx.status);
        members[start+1]=p;
        CString guid=tvsOffsetData::newGuid();if(guid.IsEmpty())return fail(L"metapodaci",L"GUID V1 trojke nije kreiran.");
        for(int k=0;k<3;++k){tvsOffsetData::Record v1;v1.guid=guid;v1.role=k;
            for(int j=0;j<3;++j)v1.members[j]=record.members[start+j];
            v1.rise=k==1?dto.middle.lastElevationMm-dto.middle.firstElevationMm:0;
            const Acad::ErrorStatus es=tvsOffsetData::write(members[start+k],v1);if(es!=Acad::eOk)return fail(L"metapodaci",L"V1 zapis clana nije upisan.",es);}
    }
    auto bridge=tvsOffsetService::makePipe(source,base,first.outgoingFirst,second.incomingLast,view.zPass/f);
    if(!bridge)return fail(L"atributi",L"Srednji clan H nije kreiran (atributi/kota).");
    TVS_Pipe* h=bridge.release();
    if(!tx.append(h,record.members[3]))return fail(L"append",L"Srednji clan H nije dodat u crtez.",tx.status);
    members[3]=h;
    if(inject(1))return fail(L"DEV",L"Proba prekida 1.");
    const tvsOffset::WorldPoint farWorld(outerPoint.x,outerPoint.y,outerPoint.z);
    auto tail=tvsOffsetService::makePipe(source,base,reverse?farWorld:second.outgoingFirst,reverse?second.outgoingFirst:farWorld,base.physical.elevation);
    if(!tail)return fail(L"atributi",L"Spoljasnji ostatak nije kreiran (atributi/kota).");
    TVS_Pipe* exit=tail.release();
    if(!tx.append(exit,record.outlet))return fail(L"append",L"Spoljasnji ostatak nije dodat u crtez.",tx.status);
    // Connectors stay untouched: source keeps its (unlinked) outer connector, the new
    // outlet keeps the fresh unlinked pair from makePipe. Continuity inside the assembly
    // is geometric and recorded by the filed parent, not by connectors.
    source->assertWriteEnabled();if(reverse)source->FirstPoint=cutIn;else source->LastPoint=cutIn;source->recordGraphicsModified();
    if(inject(2))return fail(L"DEV",L"Proba prekida 2.");
    AcDbObjectId parent;const Acad::ErrorStatus parentStatus=tvsBypassData::write(base.db,record,members,parent);
    if(parentStatus!=Acad::eOk){
        return fail(L"metapodaci",L"Upis roditelja/backlink-ova nije uspio.",parentStatus);
    }
    if(inject(3))return fail(L"DEV",L"Proba prekida 3.");
    if(!tx.commit())return fail(L"commit",L"Potvrda transakcije nije uspjela.",tx.status);
    output.output=record.outlet;output.parent=parent;output.resident=exit;output.elevation=base.physical.elevation;output.angle=options.angle;
    return true;
}
}
#endif
