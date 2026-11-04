#pragma once
// Poravnanje prelaza po visini (TransAlignData.h): default for new transitions (TVS_SETTINGS,
// per user in HKCU\Software\TVS_Vent), and TVS_PRELAZ_KOTA: after the alignment (or the heights)
// of a transition changed, the axis at its end moves by a difference; the command asks which
// side stays and moves the other side of the network by that difference (its own level
// changes kept, ElevationShift.h). Properties and the Svojstva dialog queue the change and run
// the command, so the question is asked in the command line.
#include "../TVS_Ventilation_DBX/TVS_TRANS.h"
#include "../TVS_Ventilation_DBX/TransAlignData.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "ElevationShift.h"
#include "acdocman.h"
#include <vector>
namespace tvsTransAlignCmd {
inline const wchar_t* regKey(){return L"Software\\TVS_Vent";}
inline int loadDefault(){
    HKEY k=NULL;if(RegOpenKeyExW(HKEY_CURRENT_USER,regKey(),0,KEY_READ,&k)!=ERROR_SUCCESS)return tvsTransAlign::Axis;
    DWORD v=0,size=sizeof(v),type=0;const bool ok=RegQueryValueExW(k,L"TransVert",NULL,&type,reinterpret_cast<LPBYTE>(&v),&size)==ERROR_SUCCESS&&type==REG_DWORD;
    RegCloseKey(k);return ok&&v<=2?static_cast<int>(v):tvsTransAlign::Axis;
}
inline int& defaultAlign(){static int a=loadDefault();return a;}
inline void setDefault(int a){
    if(a<0||a>2)return;defaultAlign()=a;
    HKEY k=NULL;if(RegCreateKeyExW(HKEY_CURRENT_USER,regKey(),0,NULL,0,KEY_WRITE,NULL,&k,NULL)!=ERROR_SUCCESS)return;
    const DWORD v=static_cast<DWORD>(a);RegSetValueExW(k,L"TransVert",0,REG_DWORD,reinterpret_cast<const BYTE*>(&v),sizeof(v));RegCloseKey(k);
}
// Plan type of new transitions (TransType): 1 simetricno, 2 ravno uz desnu, 3 ravno uz lijevu
// ivicu (u smjeru crtanja). Per user, as the height alignment.
inline int loadPlan(){
    HKEY k=NULL;if(RegOpenKeyExW(HKEY_CURRENT_USER,regKey(),0,KEY_READ,&k)!=ERROR_SUCCESS)return 1;
    DWORD v=0,size=sizeof(v),type=0;const bool ok=RegQueryValueExW(k,L"TransPlan",NULL,&type,reinterpret_cast<LPBYTE>(&v),&size)==ERROR_SUCCESS&&type==REG_DWORD;
    RegCloseKey(k);return ok&&v>=1&&v<=3?static_cast<int>(v):1;
}
inline void storePlan(int t){
    if(t<1||t>3)return;HKEY k=NULL;if(RegCreateKeyExW(HKEY_CURRENT_USER,regKey(),0,NULL,0,KEY_WRITE,NULL,&k,NULL)!=ERROR_SUCCESS)return;
    const DWORD v=static_cast<DWORD>(t);RegSetValueExW(k,L"TransPlan",0,REG_DWORD,reinterpret_cast<const BYTE*>(&v),sizeof(v));RegCloseKey(k);
}
inline double endOffset(const TVS_TRANS* t,int align){return tvsTransAlign::endOffset(align,t->SizeAp1,t->SizeBp1,t->SizeAp2,t->SizeBp2);}
inline double endOffset(const TVS_TRANS* t){return endOffset(t,tvsTransAlign::read(t));}
// State before a change: the end offset it had.
struct Pending {AcDbObjectId id;double before;};
inline std::vector<Pending>& pending(){static std::vector<Pending> p;return p;}
inline void queue(const AcDbObjectId& id,double offsetBefore){
    for(size_t i=0;i<pending().size();++i)if(pending()[i].id==id)return;   // first state counts
    Pending p={id,offsetBefore};pending().push_back(p);
}
// From Properties / double click (no command running): ask in the command line.
inline void post(){
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;
    if(doc)acDocManager->sendStringToExecute(doc,L"_TVS_PRELAZ_KOTA\n",false,true,false);
}
inline void settle(){
    std::vector<Pending> work;work.swap(pending());
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=2.0/f;
    for(size_t i=0;i<work.size();++i){
        double delta=0;
        {AcDbObjectPointer<TVS_TRANS> t(work[i].id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)continue;delta=endOffset(t.object())-work[i].before;}
        if(std::fabs(delta)<=1e-9)continue;
        wchar_t prompt[200];
        swprintf_s(prompt,L"\nPrelaz: kota ose na kraju se mijenja za %+.0f mm. Koja strana ostaje [Pocetak/Kraj] <Pocetak>: ",delta*f);
        acedInitGet(0,L"Pocetak Kraj");ACHAR kw[32]=L"";const int rc=acedGetKword(prompt,kw);
        if(rc!=RTNORM&&rc!=RTNONE){acutPrintf(L"\nMreza nije pomjerena; kote uz prelaz provjerite rucno.");continue;}
        const bool keepEnd=rc==RTNORM&&_wcsicmp(kw,L"Kraj")==0;
        std::vector<AcDbObjectId> ids;bool loop=false;
        if(!tvsElevationShift::side(db,work[i].id,keepEnd?0:1,tol,ids,loop))continue;
        if(loop){acutPrintf(L"\nPrelaz je u zatvorenom prstenu mreze: strana se ne moze pomjeriti sama. Kote provjerite rucno.");continue;}
        int n=0;
        if(keepEnd){   // the start side and the transition go by -delta, the end stays
            AcDbObjectPointer<TVS_TRANS> t(work[i].id,AcDb::kForWrite);
            if(t.openStatus()==Acad::eOk&&t->put_Elevation(t->Elev-delta)==Acad::eOk){t->recordGraphicsModified();++n;}
            t.close();n+=tvsElevationShift::shift(ids,-delta,work[i].id);
        }else n=tvsElevationShift::shift(ids,delta,work[i].id);
        acutPrintf(L"\nPomjereno %d elemenata za %+.0f mm; postojece promjene kote su sacuvane.",n,(keepEnd?-delta:delta)*f);
    }
}
// Sets the alignment of an open transition and queues the elevation question.
inline bool apply(TVS_TRANS* t,int align){
    if(!t||align<0||align>2||tvsTransAlign::read(t)==align)return false;
    const double before=endOffset(t);
    if(tvsTransAlign::write(t,align)!=Acad::eOk)return false;
    t->recordGraphicsModified();queue(t->objectId(),before);return true;
}
// Svojstva dialog: common alignment of the selected transitions (-1 = mixed or none).
struct Gather {bool any;int align;bool mixed;Gather():any(false),align(-1),mixed(false){}
    void add(const TVS_TRANS* t){const int a=tvsTransAlign::read(t);if(!any){any=true;align=a;}else if(a!=align)mixed=true;}
    int common()const{return any&&!mixed?align:-1;}};
// After the dialog changed a transition (open for write): alignment from the dialog (-1 = keep),
// and the elevation question when its end axis moved (alignment or heights).
inline void afterEdit(TVS_TRANS* t,double offsetBefore,int align){
    if(!t)return;
    if(align>=0&&align<=2&&tvsTransAlign::read(t)!=align&&tvsTransAlign::write(t,align)==Acad::eOk)t->recordGraphicsModified();
    if(std::fabs(endOffset(t)-offsetBefore)>1e-9){const bool wasEmpty=pending().empty();queue(t->objectId(),offsetBefore);if(wasEmpty)post();}
}
// TVS_PRELAZ_KOTA: queued changes, or pick a transition and choose its alignment.
inline void run(){
    if(!pending().empty()){settle();return;}
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite TVS prelaz: ",name,pick)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    {AcDbObjectPointer<TVS_TRANS> t(id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk){acutPrintf(L"\nIzabrani objekat nije TVS prelaz.");return;}
     wchar_t prompt[160];swprintf_s(prompt,L"\nPoravnanje po visini [Osa/Gore/Dolje] <%ls>: ",tvsTransAlign::name(tvsTransAlign::read(t.object())));
     acedInitGet(0,L"Osa Gore Dolje");ACHAR kw[32]=L"";const int rc=acedGetKword(prompt,kw);if(rc!=RTNORM)return;
     const int a=!_wcsicmp(kw,L"Gore")?tvsTransAlign::Top:!_wcsicmp(kw,L"Dolje")?tvsTransAlign::Bottom:tvsTransAlign::Axis;
     apply(t.object(),a);}
    settle();
}
}
