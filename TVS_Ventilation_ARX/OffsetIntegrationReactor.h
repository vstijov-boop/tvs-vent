#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "aced.h"
#include "rxevent.h"
#include "OffsetAssemblyData.h"
#include "AssemblyMoveGuard.h"
// AcEditorReactor inherits the audited AcRxEventReactor clone callbacks.
class OffsetIntegrationReactor : public AcEditorReactor {
public:
    virtual void endDeepClone(AcDbIdMapping& mapping){
        // SDK contract: all IDs have been translated at this callback.
        Acad::ErrorStatus es=tvsOffsetData::remapClones(mapping);
        if(es!=Acad::eOk)acutPrintf(L"\nTVS OFFSET: COPY podaci nisu završeni; kopija nije potvrđen operativni sklop (%d).",es);
    }
    virtual void commandWillStart(const ACHAR*){tvsAssemblyMove::begin();}
    virtual void commandEnded(const ACHAR* command){tvsAssemblyMove::end();tvsOffsetData::clearClonePermits();tvsOffsetSession::afterUndoRedo(command);}
    virtual void commandCancelled(const ACHAR*){tvsAssemblyMove::end();tvsOffsetData::clearClonePermits();}
    virtual void commandFailed(const ACHAR*){tvsAssemblyMove::end();tvsOffsetData::clearClonePermits();}
};
inline OffsetIntegrationReactor& offsetReactor(){static OffsetIntegrationReactor reactor;return reactor;}
#endif
