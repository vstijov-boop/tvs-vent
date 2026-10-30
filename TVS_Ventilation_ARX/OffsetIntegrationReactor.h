#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "aced.h"
#include "rxevent.h"
#include "OffsetAssemblyData.h"
// AcEditorReactor inherits the audited AcRxEventReactor clone callbacks.
class OffsetIntegrationReactor : public AcEditorReactor {
public:
    virtual void endDeepClone(AcDbIdMapping& mapping){
        // SDK contract: all IDs have been translated at this callback.
        Acad::ErrorStatus es=tvsOffsetData::remapClones(mapping);
        if(es!=Acad::eOk)acutPrintf(L"\nTVS OFFSET: COPY podaci nisu završeni; kopija nije potvrđen operativni sklop (%d).",es);
    }
    virtual void commandEnded(const ACHAR* command){tvsOffsetData::clearClonePermits();tvsOffsetSession::afterUndoRedo(command);}
    virtual void commandCancelled(const ACHAR*){tvsOffsetData::clearClonePermits();}
    virtual void commandFailed(const ACHAR*){tvsOffsetData::clearClonePermits();}
};
inline OffsetIntegrationReactor& offsetReactor(){static OffsetIntegrationReactor reactor;return reactor;}
#endif
