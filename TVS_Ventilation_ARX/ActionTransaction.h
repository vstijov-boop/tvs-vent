#pragma once
// Review 2026-11b, proposal 3: one user action = one UNDO step and, where possible, all or nothing.
// UNDO: every TVS command is registered ACRX_CMD_MODAL, so AutoCAD already records one command
// as one UNDO step (also the TVS_TK_TIP / TVS_BYPASS dialogs opened inside a command); the work
// that used to be split over a posted command (Svojstva -> TVS_DIM_PRATI) is now done inside the
// command (Edit.inl, TVS_Change). All or nothing, two shared tools:
//  - Transaction: AcTransaction (actrTransactionManager); aborted in the destructor unless
//    commit() - every object obtained through it and every entity appended through it is rolled
//    back. Nested use is fine (AcTransactions nest). Same contract as tvsMove::Transaction and
//    tvsOffsetProof::BuildTransaction, for any owner (model, paper space, block).
//  - writable(): preflight for code that writes with open / close (no rollback there): every
//    element about to be written exists and is on an unlocked layer, else nothing is written.
#include <actrans.h>
#include "dbobjptr.h"
#include "dbsymtb.h"
#include <vector>
namespace tvsAction {
class Transaction {
    AcTransactionManager* manager;bool active;
    Transaction(const Transaction&);Transaction& operator=(const Transaction&);
public:
    Acad::ErrorStatus status;   // last SDK status of a failed step (messages only)
    Transaction():manager(actrTransactionManager),active(false),status(Acad::eOk){
        if(manager&&manager->startTransaction())active=true;else status=Acad::eNotApplicable;}
    ~Transaction(){if(active)manager->abortTransaction();}
    bool good()const{return active;}
    // NULL on failure (status says why); never close() what it returns
    AcDbObject* open(const AcDbObjectId& id,AcDb::OpenMode mode){
        if(!active){status=Acad::eNotApplicable;return NULL;}
        AcDbObject* object=NULL;status=manager->getObject(object,id,mode);
        return status==Acad::eOk?object:NULL;
    }
    // Takes `entity`: in the owner and in the transaction, or deleted / erased on failure.
    bool append(AcDbBlockTableRecord* owner,AcDbEntity* entity,AcDbObjectId& id){
        if(!active||!owner){delete entity;status=Acad::eNotApplicable;return false;}
        status=owner->appendAcDbEntity(id,entity);
        if(status!=Acad::eOk){delete entity;return false;}
        status=manager->addNewlyCreatedDBRObject(entity,true);
        if(status!=Acad::eOk){entity->erase();entity->close();return false;}
        return true;
    }
    bool commit(){
        if(!active)return false;
        status=manager->endTransaction();if(status!=Acad::eOk)return false;
        active=false;return true;
    }
};
// The element can be written: it opens and its layer is not locked (null id: nothing to write).
inline bool writable(const AcDbObjectId& id){
    if(id.isNull())return true;
    AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbLayerTableRecord> layer(e->layerId(),AcDb::kForRead);
    return layer.openStatus()==Acad::eOk&&!layer->isLocked();
}
inline bool writable(const std::vector<AcDbObjectId>& ids){
    for(size_t k=0;k<ids.size();++k)if(!writable(ids[k]))return false;
    return true;
}
}
