#pragma once
// Production helpers used by TVS_OFFSET / etaz, TVS_BYPASS, TVS_ETAZ_KOTA and TVS_DIM_PRATI, taken
// out of TVSTestOffset.h (review 2026-11b, plan 8): that file stays a development-only test
// (TVS_OFFSET_PROOF + TVS_DEV), these do not depend on it. Namespace kept for the callers.
#include "../geometry/OffsetAdapter.h"
#include "../TVS_Ventilation_DBX/TVS_Entity.h"
#include "dbobjptr.h"
#include <dbents.h>
#include <actrans.h>
namespace tvsOffsetProof {
inline AcGePoint3d pt(const tvsOffset::WorldPoint& p){return AcGePoint3d(p.x,p.y,p.z);}
inline AcDbObjectId modelId(AcDbDatabase* db){
    AcDbObjectId id;AcDbObjectPointer<AcDbBlockTable> table(db->blockTableId(),AcDb::kForRead);
    if(table.openStatus()==Acad::eOk)table->getAt(ACDB_MODEL_SPACE,id);return id;
}
class BuildTransaction {
    AcTransactionManager* manager;bool active;
public:
    AcDbBlockTableRecord* space;
    Acad::ErrorStatus status; // last SDK status on a failed step (diagnostics only)
    BuildTransaction(AcDbDatabase* db):manager(actrTransactionManager),active(false),space(NULL),status(Acad::eOk){
        if(!manager||!manager->startTransaction())return;active=true;
        AcDbObject* object=NULL;
        status=manager->getObject(object,modelId(db),AcDb::kForWrite);
        if(status==Acad::eOk)
            space=AcDbBlockTableRecord::cast(object);
    }
    ~BuildTransaction(){if(active)manager->abortTransaction();}
    bool append(AcDbEntity* entity,AcDbObjectId& id){
        if(!space){delete entity;return false;}
        status=space->appendAcDbEntity(id,entity);
        if(status!=Acad::eOk){delete entity;return false;}
        status=manager->addNewlyCreatedDBRObject(entity,true);
        if(status!=Acad::eOk){
            entity->erase();entity->close();return false;}
        return true;
    }
    bool commit(){if(!active||!space)return false;status=manager->endTransaction();if(status!=Acad::eOk)return false;active=false;return true;}
};
inline void initialize(TVS_Entity* e){
    e->Direct=0;e->Param=0;e->IsPipe=false;e->ShowText=false;e->Axis=0;
    e->Form=0;e->activeStatus=false;e->Tag1=NULL;e->Tag2=NULL;
    e->Type1=0;e->Type2=0;e->connectorInteractionStatus=false;e->connectorCounts=0;
}

}
