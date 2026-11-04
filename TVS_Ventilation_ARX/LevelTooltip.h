#pragma once
// Oblacic sa kotama (TVS_SETTINGS "Oblacic sa kotama"): while the cursor is over a TVS element,
// the AutoCAD tooltip shows its top / axis / bottom elevation in mm. Input point monitor on
// every open document; the switch is kept in HKCU\Software\TVS_Vent (per user).
// TVS_KOTE: switches the on-screen elevation labels drawn by the elements (LevelLabels.h).
#include "acedinpt.h"
#include "acdocman.h"
#include "../TVS_Ventilation_DBX/TVS_Entity.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../TVS_Ventilation_DBX/LevelLabels.h"
#include <cwchar>
namespace tvsLevelTooltip {
inline const wchar_t* regKey(){return L"Software\\TVS_Vent";}
inline bool stored(){
    HKEY k=NULL;if(RegOpenKeyExW(HKEY_CURRENT_USER,regKey(),0,KEY_READ,&k)!=ERROR_SUCCESS)return false;
    DWORD v=0,size=sizeof(v),type=0;const bool ok=RegQueryValueExW(k,L"LevelTooltip",NULL,&type,reinterpret_cast<LPBYTE>(&v),&size)==ERROR_SUCCESS&&type==REG_DWORD;
    RegCloseKey(k);return ok&&v!=0;
}
inline void store(bool on){
    HKEY k=NULL;if(RegCreateKeyExW(HKEY_CURRENT_USER,regKey(),0,NULL,0,KEY_WRITE,NULL,&k,NULL)!=ERROR_SUCCESS)return;
    const DWORD v=on?1:0;RegSetValueExW(k,L"LevelTooltip",0,REG_DWORD,reinterpret_cast<const BYTE*>(&v),sizeof(v));RegCloseKey(k);
}
class Monitor : public AcEdInputPointMonitor {
    wchar_t text[160];
public:
    Monitor(){text[0]=0;}
    virtual Acad::ErrorStatus monitorInputPoint(const AcEdInputPoint& input,AcEdInputPointMonitorResult& output){
        if(!input.pointComputed())return Acad::eOk;
        const AcArray<AcDbObjectId>& ids=input.pickedEntities();
        for(int i=0;i<ids.length();++i){
            AcDbEntity* e=NULL;if(acdbOpenAcDbEntity(e,ids[i],AcDb::kForRead)!=Acad::eOk)continue;
            TVS_Entity* t=TVS_Entity::cast(e);
            if(t){const double f=tvsUnits::factor(e),half=(t->SizeB>0?t->SizeB:t->SizeA)/2;
                swprintf_s(text,L"TVS kote: gi %.0f / os %.0f / di %.0f mm",(t->Elev+half)*f,t->Elev*f,(t->Elev-half)*f);
                e->close();output.setAdditionalTooltipString(text);return Acad::eOk;}
            e->close();
        }
        return Acad::eOk;
    }
    virtual bool excludeFromOsnapCalculation(const AcArray<AcDbObjectId>&,Adesk::GsMarker){return false;}
};
inline Monitor& monitor(){static Monitor m;return m;}
inline bool& active(){static bool a=false;return a;}
inline void attach(AcApDocument* d){if(d&&d->inputPointManager())d->inputPointManager()->addPointMonitor(&monitor());}
inline void detach(AcApDocument* d){if(d&&d->inputPointManager())d->inputPointManager()->removePointMonitor(&monitor());}
class DocReactor : public AcApDocManagerReactor {
public:
    virtual void documentCreated(AcApDocument* d){if(active())attach(d);}
    virtual void documentToBeDestroyed(AcApDocument* d){if(active())detach(d);}
};
inline DocReactor& reactor(){static DocReactor r;return r;}
inline void forAll(bool on){
    AcApDocumentIterator* it=acDocManager?acDocManager->newAcApDocumentIterator():NULL;if(!it)return;
    for(;!it->done();it->step()){if(on)attach(it->document());else detach(it->document());}
    delete it;
}
inline void setEnabled(bool on){
    store(on);if(on==active())return;
    active()=on;forAll(on);
}
inline bool enabled(){return active();}
// application load / unload
inline void start(){if(acDocManager)acDocManager->addReactor(&reactor());if(stored()){active()=true;forAll(true);}}
inline void stop(){if(active())forAll(false);active()=false;if(acDocManager)acDocManager->removeReactor(&reactor());}
// TVS_KOTE: labels on screen in the current drawing, on/off.
inline void toggleLabels(){
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(!doc||!doc->database())return;
    const bool on=!tvsLevelLabels::enabled(doc->database());
    if(!tvsLevelLabels::set(doc->database(),on)){acutPrintf(L"\nTVS_KOTE: prikaz nije promijenjen.");return;}
    acutPrintf(on?L"\nTVS_KOTE: kote ukljucene (gi / os / di, ne stampaju se).":L"\nTVS_KOTE: kote iskljucene.");
    acDocManager->sendStringToExecute(doc,L"_.REGENALL\n",false,true,false);
}
}
