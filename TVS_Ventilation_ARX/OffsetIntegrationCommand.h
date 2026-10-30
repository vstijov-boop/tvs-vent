#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetAssemblyData.h"
#include "../geometry/OffsetAdapter.h"
namespace tvsOffsetIntegration {
enum Outcome { Applied,NoChange,Cancelled,Failed };
// Explicit DEVELOPMENT settings, not standards or approved production minima.
struct IntegrationPolicy {double minimumStraightMm,minimumProjectionMm;IntegrationPolicy():minimumStraightMm(100),minimumProjectionMm(.1){}};
inline IntegrationPolicy& integrationPolicy(){static IntegrationPolicy policy;return policy;}
inline void initializePipe(TVS_Pipe* pipe){
    tvsOffsetProof::initialize(pipe);TVS_Connector first,last;
    first.connectionID=NULL;first.cTypeCurrent=conPipeStart;last.connectionID=NULL;last.cTypeCurrent=conPipeEnd;
    pipe->connectors.resize(2);pipe->connectors[0]=first;pipe->connectors[1]=last;
}
inline const ACHAR* errorText(tvsOffset::Status s){switch(s){
case tvsOffset::Unsupported90:return L"90° nije podržano: vertikalni međukomad ima nultu XY projekciju. Ugao nije zamijenjen.";
case tvsOffset::TooShort:return L"Premala razlika kota za lukove i Lmin (ili strogo numeričko odbijanje na granici). Završna kota nije promijenjena.";
case tvsOffset::ProjectionTooSmall:return L"Premala horizontalna projekcija međukomada.";
default:return L"Nevažeći podaci ili nepodržana geometrija promjene kote.";}}
inline void appearance(TVS_Pipe* source,TVS_Entity* target){
    target->setLayer(source->layerId());target->setColor(source->color());target->setLinetype(source->linetypeId());
    target->setLinetypeScale(source->linetypeScale());target->setLineWeight(source->lineWeight());
    target->Flow=source->Flow;target->Grani=source->Grani;target->Wipeout=source->Wipeout;target->This1D=source->This1D;
}
inline Outcome execute(TVS_Pipe*& current,double& acceptedElevation,double& acceptedAngle){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db||!current)return Failed;
    AcDbObjectId original=current->objectId();double a,b,z0,f=tvsUnits::factor(db);AcGePoint3d first,last;
    {
        AcDbObjectPointer<TVS_Pipe> source(original,AcDb::kForRead);
        if(source.openStatus()!=Acad::eOk||!tvsMove::editable(source.object())||tvsOffsetData::marked(source.object())){
            acutPrintf(L"\nTVS OFFSET: aktivni kanal nije dostupan, zaključan je ili je član sklopa.");return Failed;}
        if(source->ThisRound||source->SizeA<=0||source->SizeB<=0||source->DuctType!=DuctTypeStill
            ||std::fabs(source->LastPoint.z-source->FirstPoint.z)*f>.001||db->currentSpaceId()!=tvsOffsetProof::modelId(db)){
            acutPrintf(L"\nTVS OFFSET: podržan je samo horizontalni pravougaoni kruti kanal u Model prostoru.");return Failed;}
        a=source->SizeA;b=source->SizeB;z0=source->get_Elevation();first=source->FirstPoint;last=source->LastPoint;
    }
    dgChangeZ dg;dg.startZ=z0;dg.nextZ=z0;dg.Axis=globalAxis;
    if(dg.DoModal()!=IDOK)return Cancelled;
    if(std::fabs((dg.nextZ-z0)*f)<=.001)return NoChange;
    double ri=0;
    if(globalRadiusTypeRect==RadiusTypeRect_RadiusConstant)ri=globalTapRadiusConst*f;
    else if(globalRadiusTypeRect==RadiusTypeRect_RadiusVariable)ri=globalTapRadiusVariableParameter*b*f;
    else {acutPrintf(L"\nTVS OFFSET: izaberite konstantni ili promjenljivi radijus pravougaonog koljena.");return Failed;}
    if(!(ri>0&&ri<=DBL_MAX)){acutPrintf(L"\nTVS OFFSET: potreban je pozitivan unutrašnji radijus.");return Failed;}
    AcGeVector3d direction=last-first;double available=std::sqrt(direction.x*direction.x+direction.y*direction.y)*f;
    if(!(available>0&&available<=DBL_MAX))return Failed;
    IntegrationPolicy settings=integrationPolicy();tvsOffset::Policy policy;policy.minimumProjection=settings.minimumProjectionMm;
    tvsOffset::Input input((dg.nextZ-z0)*f,a*f,b*f,ri,dg.Axis*tvsOffset::pi()/180,settings.minimumStraightMm);
    tvsOffset::Result result=tvsOffset::solve(input,policy);
    if(result.status==tvsOffset::NoChange)return NoChange;
    if(result.hasGeometry()&&result.projectedLength<=0){acutPrintf(L"\nTVS OFFSET: nulti ravanski međukomad nije podržan u integraciji.");return Failed;}
    if(!result.hasGeometry()){
        acutPrintf(L"\nTVS OFFSET DEV: Lmin=%.3f mm, minimumProjection=%.3f mm. %s",settings.minimumStraightMm,settings.minimumProjectionMm,errorText(result.status));return Failed;}
    if(available-result.axis[1].s<1){acutPrintf(L"\nTVS OFFSET: dolazni kanal je prekratak za koljeno i preostali segment od 1 mm.");return Failed;}
    tvsOffsetAdapter::Placement placement(tvsOffset::WorldPoint(last.x,last.y,last.z),std::atan2(direction.y,direction.x),f,z0*f);
    tvsOffsetAdapter::Result dto=tvsOffsetAdapter::adapt(input,result,placement);
    if(!dto.ready()){acutPrintf(L"\nTVS OFFSET: rezultat nije moguće pouzdano pozicionirati.");return Failed;}
    AcDbObjectId output;bool committed=false;
    {
        tvsOffsetProof::BuildTransaction tx(db);if(!tx.space)return Failed;
        tvsOffsetData::WriteScope allow;
        AcDbObject* base=NULL;if(actrTransactionManager->getObject(base,original,AcDb::kForWrite)!=Acad::eOk)return Failed;
        TVS_Pipe* source=TVS_Pipe::cast(base);if(!source||source->FirstPoint.distanceTo(first)*f>.001||source->LastPoint.distanceTo(last)*f>.001
            ||source->SizeA!=a||source->SizeB!=b||source->get_Elevation()!=z0)return Failed;
        TVS_TAP* taps[2]={NULL,NULL};AcDbObjectId ids[3];
        for(int j=0;j<2;++j){const tvsOffsetAdapter::Elbow& e=dto.elbows[j];
            TVS_TAP* tap=new TVS_TAP();tvsOffsetProof::initialize(tap);tap->setDatabaseDefaults();appearance(source,tap);
            tap->SizeA=e.sizeA;tap->SizeB=e.sizeB;tap->ThisRound=false;tap->DuctType=DuctTypeStill;
            tap->TypeRoundTap=TypeRoundTap_TapSection;tap->RadiusTypeRect=RadiusTypeRect_RadiusConstant;
            tap->RadiusTypeRound=RadiusTypeRound_RadiusConstant;tap->RadiusVariableParameter=.5;tap->RadiusConst=e.innerRadius;
            tap->Swectangle=e.angle;tap->Startvector=AcGeVector3d(e.startVector.x,e.startVector.y,0);tap->Normvector=AcGeVector3d(0,0,1);
            tap->MiddlePoint=tvsOffsetProof::pt(e.middle);tap->CenterPoint=tap->MiddlePoint;tap->WipeoutLength=50/f;
            tap->put_Form(e.form==tvsOffsetAdapter::Up?Form_Up:Form_Down);tap->put_Elevation(e.elevation);
            if(!tx.append(tap,ids[j==0?0:2]))return Failed;taps[j]=tap;
        }
        TVS_Pipe* slope=new TVS_Pipe();initializePipe(slope);slope->setDatabaseDefaults();appearance(source,slope);
        slope->SizeA=a;slope->SizeB=b;slope->ThisRound=false;slope->DuctType=DuctTypeStill;slope->WipeoutLength=50/f;
        slope->FirstPoint=tvsOffsetProof::pt(dto.middle.first);slope->LastPoint=tvsOffsetProof::pt(dto.middle.last);
        slope->put_Elevation((dto.middle.firstElevationMm+dto.middle.lastElevationMm)/2/f);
        if(!tx.append(slope,ids[1]))return Failed;
        TVS_Pipe* exit=new TVS_Pipe();initializePipe(exit);exit->setDatabaseDefaults();appearance(source,exit);
        exit->SizeA=a;exit->SizeB=b;exit->ThisRound=false;exit->DuctType=DuctTypeStill;exit->WipeoutLength=50/f;
        exit->FirstPoint=tvsOffsetProof::pt(dto.outgoingFirst);
        AcGeVector3d along(std::cos(placement.heading),std::sin(placement.heading),0);
        exit->LastPoint=exit->FirstPoint+along*(300/f);exit->put_Elevation(dg.nextZ);
        if(!tx.append(exit,output))return Failed;
        CString guid=tvsOffsetData::newGuid();if(guid.IsEmpty())return Failed;
        AcDbObject* members[3]={taps[0],slope,taps[1]};
        for(int j=0;j<3;++j){tvsOffsetData::Record record;record.guid=guid;record.role=j;
            for(int k=0;k<3;++k)record.members[k]=ids[k];
            record.rise=j==1?result.axis[2].z-result.axis[1].z:0;
            if(tvsOffsetData::write(members[j],record)!=Acad::eOk){acutPrintf(L"\nTVS OFFSET: podaci nisu upisani; cijela operacija se poništava.");return Failed;}}
        if(source->put_Lastpoint(tvsOffsetProof::pt(dto.incomingLast))!=Acad::eOk)return Failed;
        source->recordGraphicsModified();committed=tx.commit();
    }
    if(!committed)return Failed;
    TVS_Pipe* next=NULL;if(acdbOpenObject(next,output,AcDb::kForRead)!=Acad::eOk){acutPrintf(L"\nTVS OFFSET: upis je završen, ali nastavak trase nije moguće otvoriti.");return Failed;}
    current=next;next->close();acceptedElevation=dg.nextZ;acceptedAngle=dg.Axis;
    tvsOffsetSession::applied(db,original,output,z0);
    acutPrintf(L"\nTVS OFFSET DEV: P=%.6f, riseStraight=%.6f, Lphysical=%.6f mm. Lmin=%.3f / minimumProjection=%.3f su razvojne postavke.",result.projectedLength,result.axis[2].z-result.axis[1].z,result.straightLength,settings.minimumStraightMm,settings.minimumProjectionMm);
    return Applied;
}
}
#endif
