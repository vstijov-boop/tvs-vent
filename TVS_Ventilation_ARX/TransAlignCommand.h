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
// State before a change: the end axis offset and the outlet (port 1) it had.
struct Before {double offset;AcGePoint3d outlet;bool hasOutlet;Before():offset(0),hasOutlet(false){}};
inline Before before(TVS_TRANS* t){
    Before b;if(!t)return b;b.offset=endOffset(t);
    tvsMove::Node n;if(tvsMove::describe(t,n)&&n.ports.size()==2){b.outlet=tvsMove::acad(n.ports[1]);b.hasOutlet=true;}
    return b;
}
struct Pending {AcDbObjectId id;Before was;};
inline std::vector<Pending>& pending(){static std::vector<Pending> p;return p;}
inline void queue(const AcDbObjectId& id,const Before& was){
    for(size_t i=0;i<pending().size();++i)if(pending()[i].id==id)return;   // first state counts
    Pending p;p.id=id;p.was=was;pending().push_back(p);
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
        double dz=0;AcGeVector3d dxy(0,0,0);
        {AcDbObjectPointer<TVS_TRANS> t(work[i].id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)continue;
         const Before now=before(t.object());dz=now.offset-work[i].was.offset;
         if(now.hasOutlet&&work[i].was.hasOutlet){dxy=now.outlet-work[i].was.outlet;dxy.z=0;}}
        const bool level=std::fabs(dz)>1e-9,plan=dxy.length()>1e-9;
        if(!level&&!plan)continue;
        wchar_t what[160];
        if(level&&plan)swprintf_s(what,L"kota ose na kraju %+.0f mm i izlaz u tlocrtu %.0f mm",dz*f,dxy.length()*f);
        else if(level)swprintf_s(what,L"kota ose na kraju %+.0f mm",dz*f);
        else swprintf_s(what,L"izlaz u tlocrtu %.0f mm",dxy.length()*f);
        wchar_t prompt[260];swprintf_s(prompt,L"\nPrelaz: mijenja se %ls. Koja strana ostaje [Pocetak/Kraj] <Pocetak>: ",what);
        acedInitGet(0,L"Pocetak Kraj");ACHAR kw[32]=L"";const int rc=acedGetKword(prompt,kw);
        if(rc!=RTNORM&&rc!=RTNONE){acutPrintf(L"\nMreza nije pomjerena; spojeve uz prelaz provjerite rucno.");continue;}
        const bool keepEnd=rc==RTNORM&&_wcsicmp(kw,L"Kraj")==0;
        std::vector<AcDbObjectId> ids;bool loop=false;
        const AcGePoint3d* seed=(!keepEnd&&work[i].was.hasOutlet)?&work[i].was.outlet:NULL;   // the end side touched the old outlet
        if(!tvsElevationShift::side(db,work[i].id,keepEnd?0:1,tol,ids,loop,seed))continue;
        if(loop){acutPrintf(L"\nPrelaz je u zatvorenom prstenu mreze: strana se ne moze pomjeriti sama. Spojeve provjerite rucno.");continue;}
        int n=0;
        if(keepEnd){   // the transition and its start side go back by the change, the end side stays
            std::vector<AcDbObjectId> withSelf(ids);withSelf.push_back(work[i].id);
            n=tvsElevationShift::shift(withSelf,-dz);tvsElevationShift::move(withSelf,-dxy);
        }else{n=tvsElevationShift::shift(ids,dz,work[i].id);n=(std::max)(n,tvsElevationShift::move(ids,dxy,work[i].id));}
        acutPrintf(L"\nPrilagodjeno %d elemenata; postojece promjene kote su sacuvane.",n);
    }
}
// Sets the alignment of an open transition and queues the elevation question.
inline bool apply(TVS_TRANS* t,int align){
    if(!t||align<0||align>2||tvsTransAlign::read(t)==align)return false;
    const Before was=before(t);
    if(tvsTransAlign::write(t,align)!=Acad::eOk)return false;
    t->recordGraphicsModified();queue(t->objectId(),was);return true;
}
// Svojstva dialog: common alignment / plan type of the selected transitions (-1 = mixed or none).
struct Gather {bool any;int align,plan;bool mixedAlign,mixedPlan;Gather():any(false),align(-1),plan(-1),mixedAlign(false),mixedPlan(false){}
    void add(const TVS_TRANS* t){const int a=tvsTransAlign::read(t),p=t->TransType==2||t->TransType==3?t->TransType:1;
        if(!any){any=true;align=a;plan=p;}else{if(a!=align)mixedAlign=true;if(p!=plan)mixedPlan=true;}}
    int common()const{return any&&!mixedAlign?align:-1;}
    int commonPlan()const{return any&&!mixedPlan?plan:-1;}};
// After the dialog changed a transition (open for write): alignment by height and plan type from
// the dialog (-1 = keep), then the question when its end axis or outlet moved.
inline void afterEdit(TVS_TRANS* t,const Before& was,int align,int plan){
    if(!t)return;
    if(align>=0&&align<=2&&tvsTransAlign::read(t)!=align&&tvsTransAlign::write(t,align)==Acad::eOk)
        acutPrintf(L"\nPrelaz: poravnanje po visini %ls.",tvsTransAlign::name(align));
    const int type=t->TransType==2||t->TransType==3?t->TransType:1;
    if(plan>=1&&plan<=3&&plan!=type){t->put_Type(plan);
        acutPrintf(L"\nPrelaz: u tlocrtu %ls.",plan==2?L"ravno uz desnu ivicu":plan==3?L"ravno uz lijevu ivicu":L"simetricno");}
    t->recordGraphicsModified();
    const Before now=before(t);
    const bool moved=std::fabs(now.offset-was.offset)>1e-9||(now.hasOutlet&&was.hasOutlet&&now.outlet.distanceTo(was.outlet)>1e-9);
    if(moved){const bool wasEmpty=pending().empty();queue(t->objectId(),was);if(wasEmpty)post();}
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
