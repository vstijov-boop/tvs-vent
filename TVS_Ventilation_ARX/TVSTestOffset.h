#pragma once
#ifndef TVS_OFFSET_PROOF
#error TVSTestOffset is restricted to explicit development builds
#endif
#include "../geometry/OffsetAdapter.h"
#include <dbents.h>
#include <actrans.h>
#include <vector>

namespace tvsOffsetProof {
struct Sample {AcDbObjectId id;AcGePoint3d ma,mc;double f;int number,elbow;};
struct Session {AcDbDatabase* db;std::vector<Sample> samples;AcDbObjectIdArray created;Session():db(NULL){}};
inline Session& session(){static Session s;return s;}
inline AcGePoint3d pt(const tvsOffset::WorldPoint& p){return AcGePoint3d(p.x,p.y,p.z);}
inline void warning(){acutPrintf(L"\nRazvojni geometrijski test — fizički obračun nagibnog segmenta još nije integrisan. Rezultat nije za specifikaciju.");}
inline AcDbObjectId modelId(AcDbDatabase* db){
    AcDbObjectId id;AcDbObjectPointer<AcDbBlockTable> table(db->blockTableId(),AcDb::kForRead);
    if(table.openStatus()==Acad::eOk)table->getAt(ACDB_MODEL_SPACE,id);return id;
}
inline bool emptyModel(AcDbDatabase* db){
    if(db->currentSpaceId()!=modelId(db))return false;
    AcDbObjectPointer<AcDbBlockTableRecord> space(modelId(db),AcDb::kForRead);
    if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return false;
    bool empty=it->done();delete it;return empty;
}
class BuildTransaction {
    AcTransactionManager* manager;bool active;
public:
    AcDbBlockTableRecord* space;
    BuildTransaction(AcDbDatabase* db):manager(actrTransactionManager),active(false),space(NULL){
        if(!manager||!manager->startTransaction())return;active=true;
        AcDbObject* object=NULL;
        if(manager->getObject(object,modelId(db),AcDb::kForWrite)==Acad::eOk)
            space=AcDbBlockTableRecord::cast(object);
    }
    ~BuildTransaction(){if(active)manager->abortTransaction();}
    bool append(AcDbEntity* entity,AcDbObjectId& id){
        if(!space){delete entity;return false;}
        if(space->appendAcDbEntity(id,entity)!=Acad::eOk){delete entity;return false;}
        manager->addNewlyCreatedDBRObject(entity,true);return true;
    }
    bool commit(){if(!active||!space||manager->endTransaction()!=Acad::eOk)return false;active=false;return true;}
};
inline bool add(BuildTransaction& tx,AcDbEntity* e,Session& s){AcDbObjectId id;
    if(!tx.append(e,id))return false;s.created.append(id);return true;}
inline bool label(BuildTransaction& tx,Session& s,const AcGePoint3d& p,const CString& text,double f){
    AcDbText* t=new AcDbText();t->setDatabaseDefaults();t->setPosition(p);t->setHeight(70/f);t->setTextString(text);t->setColorIndex(2);return add(tx,t,s);
}
inline bool marker(BuildTransaction& tx,Session& s,const AcGePoint3d& p,int n,int port,double f){
    AcGeVector3d x(35/f,0,0),y(0,35/f,0);
    AcDbLine* a=new AcDbLine(p-x,p+x);a->setColorIndex(1);if(!add(tx,a,s))return false;
    AcDbLine* b=new AcDbLine(p-y,p+y);b->setColorIndex(1);if(!add(tx,b,s))return false;
    CString text;text.Format(L"C%d P%d",n,port);return label(tx,s,p+AcGeVector3d(45/f,80/f,0),text,f);
}
inline void initialize(TVS_Entity* e){
    e->Direct=0;e->Param=0;e->IsPipe=false;e->ShowText=false;e->Axis=0;
    e->Form=0;e->activeStatus=false;e->Tag1=NULL;e->Tag2=NULL;
    e->Type1=0;e->Type2=0;e->connectorInteractionStatus=false;e->connectorCounts=0;
}
inline bool pipe(BuildTransaction& tx,Session& s,const AcGePoint3d& a,const AcGePoint3d& b,double f,double elevation,bool wipe,bool axis){
    TVS_Pipe* p=new TVS_Pipe();initialize(p);p->setDatabaseDefaults();p->FirstPoint=a;p->LastPoint=b;
    p->SizeA=400/f;p->SizeB=200/f;p->ThisRound=false;p->This1D=axis;p->DuctType=DuctTypeStill;
    p->Flow=0;p->Wipeout=wipe;p->Grani=false;p->WipeoutLength=50/f;
    p->put_Elevation(elevation/f);return add(tx,p,s);
}
inline void printPoint(const ACHAR* name,const AcGePoint3d& p,double f){
    acutPrintf(L"\n  %s model=(%.9f, %.9f, %.9f), WCS-mm=(%.6f, %.6f, %.6f)",name,p.x,p.y,p.z,p.x*f,p.y*f,p.z*f);
}
inline void create(bool forceFailure=false){
    warning();AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
    if(!db||!emptyModel(db)){acutPrintf(L"\nTVS_TEST_OFFSET: dozvoljen je samo prazan MODEL probnog DWG-a. Nema izmjena.");return;}
    ACHAR mode[133]={0};acedInitGet(0,L"Kontura Maska Osa");
    int status=acedGetKword(L"\nPrikaz [Kontura/Maska/Osa] <Kontura>: ",mode);
    if(status!=RTNORM&&status!=RTNONE)return;
    bool wipe=status==RTNORM&&_wcsicmp(mode,L"Maska")==0;
    bool axis=status==RTNORM&&_wcsicmp(mode,L"Osa")==0;
    const double f=tvsUnits::factor(db);Session pending;pending.db=db;bool failed=false;
    {
        BuildTransaction tx(db);if(!tx.space){acutPrintf(L"\nNije moguće započeti transakciju.");return;}
        int number=0;const double angles[]={45,30,60},headings[]={0,37};
        for(int sign=1;sign>=-1&&!failed;sign-=2)for(int a=0;a<3&&!failed;++a)for(int h=0;h<2&&!failed;++h){
            ++number;tvsOffset::Input in(sign*400,400,200,100,angles[a]*tvsOffset::pi()/180,0);
            tvsOffset::Result geometry=tvsOffset::solve(in); // diagnostic Lmin=0; no production policy
            tvsOffsetAdapter::Placement place(tvsOffset::WorldPoint((1000+h*6000)/f,((sign==1?0:3)+a)*4000/f,37/f),headings[h]*tvsOffset::pi()/180,f,-400);
            tvsOffsetAdapter::Result dto=tvsOffsetAdapter::adapt(in,geometry,place);
            if(!dto.ready()){failed=true;break;}
            acutPrintf(L"\nCASE %d: A=400 B=200 Ri=100 mm, angle=%.0f deg heading=%.0f deg; z0=-400 delta=%.0f z1=%.0f mm; F=%.0f mm/unit; planeZ=%.9f model",number,angles[a],headings[h],in.deltaZ,-400+in.deltaZ,f,37/f);
            acutPrintf(L"\n  Lphysical=%.9f mm; Lprojected=%.9f mm; run=%.9f mm",geometry.straightLength,geometry.projectedLength,geometry.totalRun);
            CString title;title.Format(L"C%d %+.0f / %.0f deg / heading %.0f | P=%.6f L=%.6f RUN=%.6f mm",number,in.deltaZ,angles[a],headings[h],geometry.projectedLength,geometry.straightLength,geometry.totalRun);
            if(!label(tx,pending,pt(place.firstMiddle)+AcGeVector3d(-800/f,900/f,0),title,f)){failed=true;break;}
            AcGePoint3d ports[4]={pt(dto.incomingLast),pt(dto.middle.first),pt(dto.middle.last),pt(dto.outgoingFirst)};
            for(int j=0;j<4;++j){CString name;name.Format(L"Expected P%d",j);printPoint(name,ports[j],f);
                acutPrintf(L" ; physical elevation=%.6f mm",-400+geometry.axis[j].z);
                if(!marker(tx,pending,ports[j],number,j,f)){failed=true;break;}}
            if(failed)break;
            for(int j=0;j<2;++j){const tvsOffsetAdapter::Elbow& e=dto.elbows[j];
                TVS_TAP* tap=new TVS_TAP();initialize(tap);tap->setDatabaseDefaults();
                // Existing entity public API; explicit append/transaction enrollment avoids add_new's
                // internally-posted/closed raw pointer and gives rollback ownership to this test.
                tap->SizeA=e.sizeA;tap->SizeB=e.sizeB;tap->ThisRound=false;tap->DuctType=DuctTypeStill;
                tap->This1D=axis;tap->Wipeout=wipe;tap->WipeoutLength=50/f;tap->Grani=false;tap->Flow=0;
                tap->TypeRoundTap=TypeRoundTap_TapSection;tap->RadiusTypeRound=RadiusTypeRound_RadiusConstant;
                tap->RadiusTypeRect=RadiusTypeRect_RadiusConstant;tap->RadiusConst=e.innerRadius;
                tap->RadiusVariableParameter=.5;tap->Swectangle=e.angle;
                tap->Normvector=AcGeVector3d(e.normal.x,e.normal.y,e.normal.z);
                tap->Startvector=AcGeVector3d(e.startVector.x,e.startVector.y,e.startVector.z);
                tap->MiddlePoint=pt(e.middle);tap->CenterPoint=tap->MiddlePoint;tap->put_Form(e.form==tvsOffsetAdapter::Up?Form_Up:Form_Down);
                tap->put_Elevation(e.elevation);
                // Deliberately poison diagnostic cache: only actual drawing may refresh MA/MC.
                tap->MA=AcGePoint3d(-999999,-999999,-999999);tap->MC=tap->MA;
                AcDbObjectId id;if(!tx.append(tap,id)){failed=true;break;}pending.created.append(id);
                Sample sample;sample.id=id;sample.ma=pt(e.expectedMA);sample.mc=pt(e.expectedMC);
                sample.f=f;sample.number=number;sample.elbow=j+1;pending.samples.push_back(sample);
                if(forceFailure){acutPrintf(L"\nINJECTED FAILURE after first TAP; transaction must abort.");failed=true;break;}
            }
            if(failed)break;
            AcGeVector3d along(std::cos(place.heading),std::sin(place.heading),0);
            if(!pipe(tx,pending,ports[0]-along*(600/f),ports[0],f,-400,wipe,axis)
                ||!pipe(tx,pending,ports[1],ports[2],f,dto.middle.firstElevationMm,wipe,axis)
                ||!pipe(tx,pending,ports[3],ports[3]+along*(600/f),f,-400+in.deltaZ,wipe,axis)){failed=true;break;}
        }
        if(!failed&&!tx.commit())failed=true;
    }
    if(failed){acutPrintf(L"\nTVS_TEST_OFFSET: kreiranje nije završeno; prazni model poslije aborta: %s",emptyModel(db)?L"DA":L"NE — provjeriti rollback");return;}
    session()=pending;
    acutPrintf(L"\nTVS_TEST_OFFSET: 12 slučajeva kreirano, %d objekata. ZOOM Extents, REGEN, pa TVS_TEST_OFFSET_CHECK. Jedan UNDO treba ukloniti cijelu operaciju.",pending.created.length());
}
inline void check(){
    warning();Session& s=session();AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
    if(s.db!=db||s.samples.empty()){acutPrintf(L"\nNema testne sesije u ovom DWG-u. Kreirajte TVS_TEST_OFFSET u praznom modelu.");return;}
    int pass=0,missing=0;const double tolerance=.001;
    for(size_t i=0;i<s.samples.size();++i){const Sample& sample=s.samples[i];
        AcDbObjectPointer<TVS_TAP> tap(sample.id,AcDb::kForRead);
        if(tap.openStatus()!=Acad::eOk){++missing;acutPrintf(L"\nC%d E%d: missing/erased (Undo/brisanje).",sample.number,sample.elbow);continue;}
        // TEST ONLY: observed redraw cache AFTER user REGEN, never used for construction.
        AcGePoint3d ma=tap->MA,mc=tap->MC;
        double dma=ma.distanceTo(sample.ma)*sample.f,dmc=mc.distanceTo(sample.mc)*sample.f;
        acutPrintf(L"\nC%d E%d: MA/MC redraw-cache diagnostic ONLY",sample.number,sample.elbow);
        printPoint(L"Expected MA",sample.ma,sample.f);printPoint(L"Actual MA",ma,sample.f);
        printPoint(L"Expected MC",sample.mc,sample.f);printPoint(L"Actual MC",mc,sample.f);
        bool ok=dma<=tolerance&&dmc<=tolerance;if(ok)++pass;
        acutPrintf(L"\n  errorMA=%.9f mm errorMC=%.9f mm [%s], Elev=%.6f mm",dma,dmc,ok?L"PASS":L"FAIL",tap->get_Elevation()*sample.f);
    }
    int alive=0;for(int i=0;i<s.created.length();++i){AcDbObject* o=NULL;
        if(acdbOpenObject(o,s.created[i],AcDb::kForRead)==Acad::eOk){++alive;o->close();}}
    acutPrintf(L"\nSUMMARY: %d/%d TAP pass; missing=%d; live objects=%d/%d; tolerance=0.001 mm. Visual/OSNAP checks are manual.",pass,static_cast<int>(s.samples.size()),missing,alive,s.created.length());
}
}
