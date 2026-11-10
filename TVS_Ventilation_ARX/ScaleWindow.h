#pragma once
// Drawing units between drawings (mm / cm / m, TVS_SETTINGS) when TVS elements are pasted, inserted
// or exploded. TVS sizes are in drawing units and the factor lives in the drawing only, so:
//  - WBLOCK / COPYCLIP: the factor of the source goes with the copied objects (otherWblock),
//  - PASTE / INSERT / EXPLODE: a uniform scaling of TVS elements is taken (points and sizes,
//    TransformGuard.h; env TVS_SCALE_OK = 1 only while such a command runs; SCALE stays refused),
//  - otherInsert: when AutoCAD's INSUNITS scaling does not match the TVS units of the two
//    drawings, a warning names TVS_PRERACUNAJ (scales the selection by the units ratio).
#include "aced.h"
#include "rxevent.h"
#include "dbidmap.h"
#include "dbobjptr.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../TVS_Ventilation_DBX/TVS_Entity.h"
#include <cmath>
#include <windows.h>
namespace tvsScaleWindow {
inline void allow(bool on){SetEnvironmentVariableW(L"TVS_SCALE_OK",on?L"1":L"0");}
inline bool scalingCommand(const ACHAR* c){
    if(!c)return false;
    static const wchar_t* list[]={L"PASTECLIP",L"PASTEORIG",L"PASTEBLOCK",L"INSERT",L"-INSERT",L"CLASSICINSERT",L"EXPLODE",L"XPLODE",L"BURST",L"DROPGEOM",L"BINDXREF",L"XBIND"};
    for(size_t i=0;i<sizeof(list)/sizeof(list[0]);++i)if(!_wcsicmp(c,list[i]))return true;
    return false;
}
inline const wchar_t* unitName(double f){return f==1000?L"m":f==10?L"cm":L"mm";}
inline bool hasRecord(AcDbDatabase* db){
    AcDbDictionary* d=NULL;if(!db||db->getNamedObjectsDictionary(d,AcDb::kForRead)!=Acad::eOk)return false;
    const bool has=d->has(tvsUnits::recordName());d->close();return has;
}
class Reactor : public AcEditorReactor {
public:
    virtual void commandWillStart(const ACHAR* c){if(scalingCommand(c))allow(true);}
    virtual void commandEnded(const ACHAR* c){if(scalingCommand(c))allow(false);}
    virtual void commandCancelled(const ACHAR* c){if(scalingCommand(c))allow(false);}
    virtual void commandFailed(const ACHAR* c){if(scalingCommand(c))allow(false);}
    // the copy (clipboard, WBLOCK) keeps the TVS units of its drawing
    virtual void otherWblock(AcDbDatabase* to,AcDbIdMapping&,AcDbDatabase* from){
        if(to&&from&&to!=from&&hasRecord(from))tvsUnits::store(tvsUnits::factor(from),to);
    }
    virtual void otherInsert(AcDbDatabase* to,AcDbIdMapping& map,AcDbDatabase* from){
        if(!to||!from||to==from)return;
        const double a=tvsUnits::factor(from),b=tvsUnits::factor(to);if(a==b)return;
        bool tvs=false;AcDbIdMappingIter it(map);
        for(it.start();!it.done()&&!tvs;it.next()){AcDbIdPair p;if(it.getMap(p)&&p.value().objectClass()&&p.value().objectClass()->isDerivedFrom(TVS_Entity::desc()))tvs=true;}
        if(!tvs)return;
        double ratio=1;if(acdbGetUnitsConversion(from->insunits(),to->insunits(),ratio)!=Acad::eOk)ratio=1;
        const double wanted=a/b;
        if(std::fabs(ratio-wanted)<=1e-9*wanted)return;   // INSUNITS scaling = TVS units: sizes follow (TransformGuard.h)
        acutPrintf(L"\nTVS: umetnuti elementi su iz crteža u %ls, ovaj crtež je u %ls (TVS_SETTINGS), a jedinice umetanja (INSUNITS) ih ne preračunavaju."
                   L"\nPoslije umetanja pokrenite TVS_PRERACUNAJ nad njima (izvor %ls), ili prije kopiranja izjednačite INSUNITS sa TVS jedinicama oba crteža.",
                   unitName(a),unitName(b),unitName(a));
    }
};
inline Reactor& reactor(){static Reactor r;return r;}
inline void start(){allow(false);if(acedEditor)acedEditor->addReactor(&reactor());}
inline void stop(){if(acedEditor)acedEditor->removeReactor(&reactor());allow(false);}
// TVS_PRERACUNAJ: selection drawn in other units (pasted 1:1) -> this drawing's units, scaled
// about a base point (positions and TVS sizes together).
inline void recalculate(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double here=tvsUnits::factor(db);
    ads_name ss;acutPrintf(L"\nIzaberite elemente crtane u drugim jedinicama: ");
    if(acedSSGet(NULL,NULL,NULL,NULL,ss)!=RTNORM)return;
    acedInitGet(0,L"mm cm m");ACHAR kw[16]=L"";
    const int k=acedGetKword(L"\nU kojim jedinicama su crtani? [mm/cm/m]: ",kw,16);
    if(k!=RTNORM){acedSSFree(ss);return;}
    const double from=!_wcsicmp(kw,L"m")?1000:!_wcsicmp(kw,L"cm")?10:1;
    if(from==here){acutPrintf(L"\nIste jedinice kao crtež (%ls): ništa za preračunavanje.",unitName(here));acedSSFree(ss);return;}
    ads_point bp;if(acedGetPoint(NULL,L"\nBazna tačka: ",bp)!=RTNORM){acedSSFree(ss);return;}
    const double ratio=from/here;const AcGeMatrix3d m=AcGeMatrix3d::scaling(ratio,asPnt3d(bp));
    Int32 n=0;acedSSLength(ss,&n);int done=0,failed=0;
    allow(true);
    for(Int32 i=0;i<n;++i){ads_name e;AcDbObjectId id;if(acedSSName(ss,i,e)!=RTNORM||acdbGetObjectId(id,e)!=Acad::eOk)continue;
        AcDbObjectPointer<AcDbEntity> p(id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk){++failed;continue;}
        if(p->transformBy(m)==Acad::eOk)++done;else ++failed;}
    allow(false);acedSSFree(ss);
    acutPrintf(L"\nPreračunato %ls → %ls (×%g): elemenata %d.",unitName(from),unitName(here),ratio,done);
    if(failed)acutPrintf(L"\nNije preračunato (zaključan sloj, dio etaža / obilaska): %d.",failed);
}
}
