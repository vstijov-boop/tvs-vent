#pragma once
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetAssemblyData.h"
#include "TVS_Pipe.h"
inline bool offsetSelectionValid(ads_name set){
    Adesk::Int32 count=0;acedSSLength(set,&count);
    for(Adesk::Int32 j=0;j<count;++j){ads_name name;AcDbObjectId id;
        if(acedSSName(set,j,name)!=RTNORM||acdbGetObjectId(id,name)!=Acad::eOk)return false;
        AcDbObjectPointer<AcDbObject> object(id,AcDb::kForRead);if(object.openStatus()!=Acad::eOk)return false;
        tvsOffsetData::Status s=tvsOffsetData::validate(object.object());
        if(s!=tvsOffsetData::Legacy&&s!=tvsOffsetData::Valid&&s!=tvsOffsetData::Incomplete){
            acutPrintf(L"\nTVS OFFSET: izvoz/obračun prekinut — %s. Nema horizontalnog fallback-a.",tvsOffsetData::statusText(s));return false;}
        if(s==tvsOffsetData::Incomplete)acutPrintf(L"\nTVS OFFSET: nepotpun sklop; obračun samo preostalih pojedinačnih elemenata.");
    }
    return true;
}
#endif
