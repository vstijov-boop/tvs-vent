#pragma once
// TVS_PROVJERA: network check of the current space (geometry/NetworkCheck.h). Lists the problems
// in the command line and walks through them: zoom to the joint, the elements selected (grips).
// A selection limits the report to problems that touch it (Enter = whole drawing).
#include "TVSMoveCommand.h"
#include "../geometry/NetworkCheck.h"
#include <cmath>
#include <set>
#include <vector>
namespace tvsCheck {
inline const wchar_t* kindName(const tvsMove::Node& n){
    if(n.flex)return L"flex";switch(n.kind){case 1:return L"kanal";case 2:return L"koljeno";case 3:return L"prelaz";case 4:return L"T komad";default:return L"element";}
}
inline void section(wchar_t* out,size_t len,const tvsNetworkCheck::Section& s,double f){
    if(s.round)swprintf_s(out,len,L"Ø%.0f",s.a*f);else swprintf_s(out,len,L"%.0f×%.0f",s.a*f,s.b*f);
}
inline void describeIssue(const tvsNetworkCheck::Issue& e,const std::vector<tvsMove::Node>& nodes,const std::vector<tvsNetworkCheck::Item>& items,int k,double f){
    const wchar_t* a=kindName(nodes[e.a]);const wchar_t* b=e.b>=0?kindName(nodes[e.b]):L"";
    acutPrintf(L"\n%3d. X=%.0f Y=%.0f: ",k,e.x*f,e.y*f);
    switch(e.type){
    case tvsNetworkCheck::Size:{
        wchar_t s1[40]=L"",s2[40]=L"";
        for(size_t p=0;p<items[e.a].x.size();++p)if(std::hypot(items[e.a].x[p]-e.x,items[e.a].y[p]-e.y)<1e-9)section(s1,40,items[e.a].s[p],f);
        for(size_t p=0;p<items[e.b].x.size();++p)if(std::hypot(items[e.b].x[p]-e.x,items[e.b].y[p]-e.y)<=0.1/f)section(s2,40,items[e.b].s[p],f);
        acutPrintf(L"%ls %ls ↔ %ls %ls – različite dimenzije bez prelaza.",a,s1,b,s2);break;}
    case tvsNetworkCheck::Level:acutPrintf(L"%ls ↔ %ls – ista dimenzija, osa se razlikuje za %.1f mm (kota).",a,b,e.value);break;
    case tvsNetworkCheck::Miss:acutPrintf(L"%ls ↔ %ls – kraj promašuje priključak za %.1f mm.",a,b,e.value);break;
    case tvsNetworkCheck::Middle:acutPrintf(L"kraj (%ls) naliježe na sredinu kanala – nema T komada / kanal nije podijeljen.",a);break;
    case tvsNetworkCheck::Angle:acutPrintf(L"koljeno %.3f° – malo odstupa od standardnog ugla (TVS_KOLJENA_UGAO).",e.value);break;
    case tvsNetworkCheck::Short:acutPrintf(L"kanal dužine %.1f mm.",e.value);break;
    }
}
inline void zoom(double x,double y,double height){
    ads_point w={x,y,0},u;if(tvsCoordinates::transformPoint(w,0,1,u)!=RTNORM)return;
    acedCommandS(RTSTR,L"_.ZOOM",RTSTR,L"_C",RTPOINT,u,RTREAL,height,RTNONE);
}
inline void select(const std::vector<AcDbObjectId>& ids){
    ads_name ss;if(acedSSAdd(NULL,NULL,ss)!=RTNORM)return;
    for(size_t i=0;i<ids.size();++i){ads_name e;if(acdbGetAdsName(e,ids[i])==Acad::eOk)acedSSAdd(e,ss,ss);}
    acedSSSetFirst(ss,NULL);acedSSFree(ss);
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db);
    std::set<AcDbObjectId> limit;
    {ads_name ss;acutPrintf(L"\nIzaberite dio mreže za provjeru (Enter = cijeli crtež): ");
     const int rc=acedSSGet(NULL,NULL,NULL,NULL,ss);if(rc==RTCAN)return;
     if(rc==RTNORM){Int32 n=0;acedSSLength(ss,&n);for(Int32 i=0;i<n;++i){ads_name e;AcDbObjectId id;if(acedSSName(ss,i,e)==RTNORM&&acdbGetObjectId(id,e)==Acad::eOk)limit.insert(id);}acedSSFree(ss);}}
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_PROVJERA: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    std::vector<tvsNetworkCheck::Item> items(nodes.size());
    for(size_t i=0;i<nodes.size();++i){const tvsMove::Node& n=nodes[i];tvsNetworkCheck::Item& it=items[i];
        it.kind=n.kind;it.flex=n.flex;
        for(size_t p=0;p<n.ports.size();++p){it.x.push_back(n.ports[p].x);it.y.push_back(n.ports[p].y);
            tvsNetworkCheck::Section s;if(p<n.sections.size()){s.a=n.sections[p].a;s.b=n.sections[p].b;s.round=n.sections[p].round;s.elevation=n.sections[p].elevation;s.known=!n.flex;}
            it.s.push_back(s);}
        if(n.kind==2){AcDbObjectPointer<TVS_TAP> t(n.id,AcDb::kForRead);
            if(t.openStatus()==Acad::eOk&&t->Form==Form_Direct&&std::fabs(std::fabs(t->Normvector.z)-1)<1e-9)it.angle=t->Swectangle*180/M_PI;}
    }
    std::vector<tvsNetworkCheck::Issue> all,issues;tvsNetworkCheck::check(items,f,all);
    for(size_t k=0;k<all.size();++k)if(limit.empty()||limit.count(nodes[all[k].a].id)||(all[k].b>=0&&limit.count(nodes[all[k].b].id)))issues.push_back(all[k]);
    int count[7]={0};for(size_t k=0;k<issues.size();++k)++count[issues[k].type];
    acutPrintf(L"\nTVS_PROVJERA: elemenata %d, problema %d.",static_cast<int>(limit.empty()?nodes.size():limit.size()),static_cast<int>(issues.size()));
    if(issues.empty())return;
    acutPrintf(L"\n  dimenzije bez prelaza: %d, kota: %d, promašen priključak: %d, kraj usred kanala: %d, ugao koljena: %d, kratki kanali: %d",
               count[1],count[2],count[3],count[4],count[5],count[6]);
    for(size_t k=0;k<issues.size()&&k<200;++k)describeIssue(issues[k],nodes,items,static_cast<int>(k)+1,f);
    if(issues.size()>200)acutPrintf(L"\n... i još %d.",static_cast<int>(issues.size())-200);
    if(count[5])acutPrintf(L"\nUglove koljena popravlja TVS_KOLJENA_UGAO; kotu i prelaz na T komadu nude TVS_STRETCH / TVS_MOVE.");
    // walk through them
    int k=0;const double height=3000.0/f;
    for(;;){
        const tvsNetworkCheck::Issue& e=issues[k];
        std::vector<AcDbObjectId> ids;ids.push_back(nodes[e.a].id);if(e.b>=0)ids.push_back(nodes[e.b].id);
        zoom(e.x,e.y,height);select(ids);describeIssue(e,nodes,items,k+1,f);
        acedInitGet(0,L"Sljedeci Prethodni Kraj");ACHAR kw[16]=L"";
        const int rc=acedGetKword(L"\n[Sljedeci/Prethodni/Kraj] <Sljedeci>: ",kw,16);
        if(rc==RTCAN||(rc==RTNORM&&_wcsicmp(kw,L"Kraj")==0))break;
        if(rc==RTNORM&&_wcsicmp(kw,L"Prethodni")==0)k=k>0?k-1:static_cast<int>(issues.size())-1;
        else{++k;if(k>=static_cast<int>(issues.size())){acutPrintf(L"\nKraj liste.");break;}}
    }
}
}
