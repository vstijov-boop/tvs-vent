#pragma once
#ifdef TVS_OFFSET_INTEGRATION
namespace tvsOffsetDiagnostics {
inline void policy(){
    tvsOffsetIntegration::IntegrationPolicy next=tvsOffsetIntegration::integrationPolicy();double value;
    acutPrintf(L"\nRazvojne postavke — nisu zahtjev standarda niti konačno proizvodno pravilo.");
    acutPrintf(L"\nTrenutno: Lmin=%.6f mm, minimumProjection=%.6f mm",next.minimumStraightMm,next.minimumProjectionMm);
    int status=acedGetReal(L"\nLmin fizičkog međukomada u mm <zadrži>: ",&value);if(status==RTCAN)return;
    if(status==RTNORM){if(!(value>=0&&value<=DBL_MAX))return;next.minimumStraightMm=value;}
    status=acedGetReal(L"\nMinimalna XY projekcija u mm (>0) <zadrži>: ",&value);if(status==RTCAN)return;
    if(status==RTNORM){if(!(value>0&&value<=DBL_MAX))return;next.minimumProjectionMm=value;}
    tvsOffsetIntegration::integrationPolicy()=next;
    acutPrintf(L"\nDEV session policy: Lmin=%.6f, minimumProjection=%.6f mm. Nema upisa u DWG.",next.minimumStraightMm,next.minimumProjectionMm);
}
inline void status(){
    ads_name name;ads_point picked;if(tvsCoordinates::selectEntity(L"\nIzaberite član etažiranja: ",name,picked)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    AcDbObjectPointer<AcDbObject> object(id,AcDb::kForRead);if(object.openStatus()!=Acad::eOk)return;
    tvsOffsetData::Record record;tvsOffsetData::Status scalar=tvsOffsetData::read(object.object(),record);
    tvsOffsetData::Status inside=tvsOffsetData::validate(object.object());
    acutPrintf(L"\nOFFSET INTERNAL=%s; record=%s; role=%d; GUID=%s; riseStraightMm=%.9f",tvsOffsetData::statusText(inside),tvsOffsetData::statusText(scalar),record.role,record.guid.GetString(),record.rise);
    for(int j=0;j<3;++j)acutPrintf(L"\n role %d: %s",j,record.members[j].isNull()?L"null":record.members[j].isErased()?L"erased":L"present");
    TVS_Pipe* pipe=TVS_Pipe::cast(object.object());
    if(pipe&&scalar==tvsOffsetData::Valid){double length=0;
        if(pipe->getPhysicalLength(length)){double area=0,thickness=0,mass=0;pipe->getSheetMetal(area,thickness,mass);
            acutPrintf(L"\nP=%.9f mm; L=%.9f mm; quantity=%.9f m; area=%.9f m2; t=%.6f mm; mass=%.9f kg",pipe->get_Length()*tvsUnits::factor(pipe),length*tvsUnits::factor(pipe),length*tvsUnits::factor(pipe)/1000,area,thickness,mass);}}
    if(inside!=tvsOffsetData::Valid){acutPrintf(L"\nNEIGHBORS=Unknown (unutrašnji sklop nije Valid). Ne vrši se popravka.");return;}
    AcDbObjectPointer<TVS_TAP> first(record.members[0],AcDb::kForRead),last(record.members[2],AcDb::kForRead);
    AcDbObjectPointer<TVS_Pipe> middle(record.members[1],AcDb::kForRead);if(first.openStatus()!=Acad::eOk||last.openStatus()!=Acad::eOk||middle.openStatus()!=Acad::eOk)return;
    double f=tvsUnits::factor(middle.object());AcGeVector3d along=middle->LastPoint-middle->FirstPoint;along.normalize();
    AcGePoint3d ports[2]={first->MiddlePoint-along*((first->get_radius()+first->SizeB/2)*std::sin(first->Swectangle)),
                         last->MiddlePoint+along*((last->get_radius()+last->SizeB/2)*std::sin(last->Swectangle))};
    // Connection diagnostic only: no stored neighboring IDs and no member repairs.
    AcDbObjectPointer<AcDbBlockTableRecord> space(object->ownerId(),AcDb::kForRead);AcDbBlockTableRecordIterator* it=NULL;
    int counts[2]={0,0};if(space.openStatus()!=Acad::eOk||space->newIterator(it)!=Acad::eOk)return;
    for(;!it->done();it->step()){AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
        TVS_Pipe* p=TVS_Pipe::cast(e);
        if(p&&p->objectId()!=record.members[1]&&!p->ThisRound&&p->DuctType==DuctTypeStill
            &&std::fabs(p->SizeA-middle->SizeA)*f<=.1&&std::fabs(p->SizeB-middle->SizeB)*f<=.1){
            AcGeVector3d direction=p->LastPoint-p->FirstPoint;if(direction.length()>0){direction.normalize();
                if(std::fabs(direction.dotProduct(along))>=1-1e-8)for(int j=0;j<2;++j)
                    if(std::fabs(p->get_Elevation()-(j==0?first->get_Elevation():last->get_Elevation()))*f<=.1
                        &&(p->FirstPoint.distanceTo(ports[j])*f<=.1||p->LastPoint.distanceTo(ports[j])*f<=.1))++counts[j];}}
        e->close();}
    delete it;acutPrintf(L"\nNEIGHBORS: incoming=%d outgoing=%d; %s. Ovo je odvojeno od INTERNAL statusa.",counts[0],counts[1],counts[0]==1&&counts[1]==1?L"Connected":L"Disconnected/Ambiguous");
}
inline void corruptTest(){
    ads_name name;ads_point point;if(tvsCoordinates::selectEntity(L"\nDEV test: izaberite NOVI označeni član za upis nepoznate verzije (UNDO vraća): ",name,point)!=RTNORM)return;
    AcDbObjectId id;acdbGetObjectId(id,name);AcDbObjectPointer<AcDbObject> object(id,AcDb::kForWrite);if(object.openStatus()!=Acad::eOk)return;
    tvsOffsetData::Record record;if(tvsOffsetData::read(object.object(),record)!=tvsOffsetData::Valid)return;
    record.version=999;tvsOffsetData::WriteScope scope;
    acutPrintf(L"\nDEV bad-version write=%d. Provjerite STATUS, COM/tabele, pa UNDO.",tvsOffsetData::write(object.object(),record));
}
}
#endif
