#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetService.h"
#include "acgiTransient.h"
#include "dbpl.h"
#include "dbents.h"
namespace tvsOffsetIntegration {
using tvsOffsetService::Outcome;
using tvsOffsetService::Applied;using tvsOffsetService::NoChange;
using tvsOffsetService::Cancelled;using tvsOffsetService::Failed;
typedef tvsOffsetService::Policy IntegrationPolicy;
inline IntegrationPolicy& integrationPolicy(){return tvsOffsetService::policy();}
inline bool& failNext(){return tvsOffsetService::failNext();}
inline const ACHAR* errorText(tvsOffset::Status s){switch(s){
case tvsOffset::Unsupported90:return L"90 stepeni nije podrzano; ugao nije zamijenjen.";
case tvsOffset::TooShort:return L"Premala razlika kota za lukove i Lmin (ukljucujuci strogu numericku granicu).";
case tvsOffset::ProjectionTooSmall:return L"Premala horizontalna projekcija medjukomada.";
default:return L"Nevazeci podaci ili nepodrzana geometrija promjene kote.";}}
class Preview {
    AcGiTransientManager* manager;AcDbIntArray viewports;std::vector<AcDbEntity*> entities;
    bool add(std::unique_ptr<AcDbEntity> e){
        if(!manager)return false;
        entities.push_back(e.get());
        bool added=false;
        try {added=manager->addTransient(e.get(),kAcGiDirectShortTerm,130,viewports);}
        catch(...){entities.pop_back();throw;}
        if(!added){entities.pop_back();return false;}
        e.release();return true;
    }
public:
    Preview():manager(acgiGetTransientManager()){}
    ~Preview(){for(auto e:entities){manager->eraseTransient(e,viewports);delete e;}}
    bool show(const tvsOffsetService::Snapshot& snapshot,const tvsOffsetOperation::Plan& p){
        const double f=snapshot.physical.factor;
        std::unique_ptr<AcDbPolyline> line(new AcDbPolyline());line->setColorIndex(3);
        const tvsOffset::WorldPoint points[]={p.dto.incomingLast,p.dto.middle.first,p.dto.middle.last,p.exitFirst,p.exitLast};
        for(int j=0;j<5;++j)if(line->addVertexAt(j,AcGePoint2d(points[j].x,points[j].y))!=Acad::eOk)return false;
        line->setElevation(points[0].z);
        if(line->elevation()!=points[0].z||!add(std::unique_ptr<AcDbEntity>(line.release())))return false;
        const double elevations[]={snapshot.physical.elevation*f,p.dto.middle.firstElevationMm,p.dto.middle.lastElevationMm,p.acceptedElevation*f};
        for(int j=0;j<4;++j){
            std::unique_ptr<AcDbText> text(new AcDbText());CString label;label.Format(L"P%d Z=%.3f mm",j,elevations[j]);
            if(text->setPosition(tvsOffsetProof::pt(points[j])+AcGeVector3d(0,(80+80*(j%2))/f,0))!=Acad::eOk
                ||text->setHeight(35/f)!=Acad::eOk||text->setTextString(label)!=Acad::eOk)return false;
            text->setColorIndex(2);if(!add(std::unique_ptr<AcDbEntity>(text.release())))return false;
        }
        return true;
    }
};
// Input acquisition is separate from the service. Only the drawing adapter may
// update the active drawing continuation and its globals, after Applied.
inline Outcome interact(const AcDbObjectId& source,tvsOffsetOperation::End end,double defaultAngle,tvsOffsetService::Commit& output){
    tvsOffsetService::Snapshot snapshot;if(!tvsOffsetService::capture(source,end,snapshot))return Failed;
    dgChangeZ dg;dg.startZ=snapshot.physical.elevation;dg.nextZ=dg.startZ;dg.Axis=defaultAngle;
    if(dg.DoModal()!=IDOK)return Cancelled;
    if(!tvsOffset::finite(dg.startZ)||std::fabs(dg.startZ-snapshot.physical.elevation)*snapshot.physical.factor>.001){
        tvsOffsetService::reject(L"Pocetna apsolutna kota mora odgovarati izabranom kanalu.");return Failed;}
    tvsOffsetOperation::Request request;request.elevation=dg.nextZ;request.angleDegrees=dg.Axis;
    request.minimumStraight=integrationPolicy().minimumStraightMm;request.minimumProjection=integrationPolicy().minimumProjectionMm;
    if(globalRadiusTypeRect==RadiusTypeRect_RadiusConstant)request.ri=globalTapRadiusConst*snapshot.physical.factor;
    else if(globalRadiusTypeRect==RadiusTypeRect_RadiusVariable)request.ri=globalTapRadiusVariableParameter*snapshot.physical.b*snapshot.physical.factor;
    else {tvsOffsetService::reject(L"Izaberite konstantni ili k*B radijus pravougaonog koljena.");return Failed;}
    const auto plan=tvsOffsetOperation::prepare(snapshot.physical,request);
    if(plan.status==tvsOffsetOperation::NoChange)return NoChange;
    if(plan.status!=tvsOffsetOperation::Ready){
        if(plan.status==tvsOffsetOperation::TooShort)tvsOffsetService::reject(L"Ulaz je prekratak za koljeno i ostatak od 1 mm.");
        else acutPrintf(L"\nTVS OFFSET DEV: Lmin=%.3f, minProjection=%.3f mm. %s",request.minimumStraight,request.minimumProjection,errorText(plan.geometryStatus));
        return Failed;
    }
    acutPrintf(L"\nDEV preview: zelena osa/spojevi u osnovi; zute oznake su apsolutne fizicke kote, nisu WCS Z.");
    acutPrintf(L"\nDva glatka pravougaona koljena: Ri=%.3f, ugao=%.3f; A x B=%.3f x %.3f mm.",request.ri,request.angleDegrees,snapshot.physical.a*snapshot.physical.factor,snapshot.physical.b*snapshot.physical.factor);
    acutPrintf(L"\nZ %.3f -> %.3f mm; P=%.6f, rise=%.6f, L=%.6f mm; izlazni nastavak 300 mm.",
        snapshot.physical.elevation*snapshot.physical.factor,plan.acceptedElevation*snapshot.physical.factor,
        plan.geometry.projectedLength,plan.geometry.axis[2].z-plan.geometry.axis[1].z,plan.geometry.straightLength);
    int answer;ACHAR value[32]=L"";
    {Preview preview;if(!preview.show(snapshot,plan))return Failed;
        acedInitGet(0,L"Primijeni Otkazi");answer=acedGetKword(L"\nPotvrda offset-a [Primijeni/Otkazi] <Otkazi>: ",value);}
    if(answer!=RTNORM||_wcsicmp(value,L"Primijeni"))return Cancelled;
    return tvsOffsetService::write(snapshot,request,plan,output);
}
inline Outcome execute(TVS_Pipe*& current,double& acceptedElevation,double& acceptedAngle){
    if(!current)return Failed;
    tvsOffsetService::Commit output;Outcome result;
    const AcDbObjectId original=current->objectId();
    try {result=interact(original,tvsOffsetOperation::Last,globalAxis,output);}
    catch(...){acutPrintf(L"\nTVS OFFSET: izuzetak; nepotvrdjena transakcija je ponistena.");return Failed;}
    if(result==Applied){
        current=output.residentOutput;acceptedElevation=output.elevation;acceptedAngle=output.angle;
        try {tvsOffsetSession::applied(acdbHostApplicationServices()->workingDatabase(),original,output.output,output.before);}
        catch(...){acutPrintf(L"\nDEV: geometrija je upisana; sesijska evidencija nije sacuvana.");}
    }
    return result;
}
inline void standalone(){
    try {
        ads_name entity;ads_point pick;
        if(tvsCoordinates::selectEntity(L"\nIzaberite obican horizontalni pravougaoni TVS kanal: ",entity,pick)!=RTNORM)return;
        AcDbObjectId id;if(acdbGetObjectId(id,entity)!=Acad::eOk)return;
        ACHAR value[32]=L"";acedInitGet(0,L"Prvi Posljednji");
        const int status=acedGetKword(L"\nSlobodan kraj [Prvi/Posljednji] <Posljednji>: ",value);
        if(status!=RTNORM&&status!=RTNONE)return;
        const auto end=status==RTNORM&&_wcsicmp(value,L"Prvi")==0?tvsOffsetOperation::First:tvsOffsetOperation::Last;
        tvsOffsetService::Commit output;const Outcome result=interact(id,end,globalAxis,output);
        if(result==NoChange)acutPrintf(L"\nTVS_OFFSET: NoChange; nema upisa.");
        else if(result==Cancelled)acutPrintf(L"\nTVS_OFFSET: otkazano; nema upisa.");
        else if(result==Failed)acutPrintf(L"\nTVS_OFFSET: operacija nije primijenjena.");
        // Standalone intentionally does not touch current pipe, globals or drawing-session history.
    }catch(...){acutPrintf(L"\nTVS_OFFSET: izuzetak; nepotvrdjena transakcija je ponistena.");}
}
}
#endif
