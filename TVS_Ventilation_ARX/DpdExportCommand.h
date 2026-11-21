#pragma once
// TVS_DPD_IZVOZ: the supply / exhaust network as a DuctPressDrop system file (JSON, UTF-8 without
// BOM), imported there with Datoteka -> Uvezi sistem... (Ctrl+I) as a new system. The network is
// read as in TVS_PAD (PressureCommand.h: the end at the unit, flows summed from the terminals,
// dionice), then geometry/DpdExport.h writes one DuctPressDrop dionica per straight piece (Q m3/h,
// W / H / D mm, smjer m, plan only; linked oprema as DuctPressDrop equipment). No other fittings: DuctPressDrop derives elbows, tees and
// transitions from the geometry. Nothing is written into the drawing.
//   A preselected element (one) with exactly one free end is taken as the end at the unit.
#include "PressureCommand.h"
#include "../geometry/DpdExport.h"
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
namespace tvsDpdExport {
inline bool& exhaust(){static bool e=false;return e;}
// drawing path without the extension ("" for an unsaved drawing) and its file name
inline void drawingName(AcDbDatabase* db,std::wstring& stem,std::wstring& file){
    stem.clear();file.clear();const ACHAR* name=NULL;
    if(db->getFilename(name)!=Acad::eOk||!name)return;
    const std::wstring n(name);const size_t slash=n.find_last_of(L"\\/");const size_t dot=n.find_last_of(L'.');
    file=slash==std::wstring::npos?n:n.substr(slash+1);
    stem=(dot!=std::wstring::npos&&(slash==std::wstring::npos||dot>slash))?n.substr(0,dot):n;
}
inline bool writeFile(const std::wstring& path,const std::string& text){
    FILE* fp=NULL;if(_wfopen_s(&fp,path.c_str(),L"wb")!=0||!fp)return false;
    const size_t written=fwrite(text.data(),1,text.size(),fp);
    const bool closed=fclose(fp)==0;return closed&&written==text.size();
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=0.1/f;
    tvsFlow::Net net;
    if(!tvsFlow::read(db,tol,net)){acutPrintf(L"\nTVS_DPD_IZVOZ: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    std::vector<tvsFlowNetwork::Element> el;tvsFlow::elements(net,el);
    // the end at the unit: a preselected element with one free end, else picked
    int root=-1,port=-1;
    {ads_name pre;if(acedSSGet(L"_I",NULL,NULL,NULL,pre)==RTNORM){Adesk::Int32 n=0;acedSSLength(pre,&n);
        if(n==1){ads_name en;AcDbObjectId id;if(acedSSName(pre,0,en)==RTNORM&&acdbGetObjectId(id,en)==Acad::eOk)
            for(size_t i=0;i<net.nodes.size();++i)if(net.nodes[i].id==id)root=net.owner[i];}
        acedSSFree(pre);acedSSSetFirst(NULL,NULL);}}
    if(root>=0&&root<static_cast<int>(el.size())){
        std::vector<std::vector<std::vector<std::pair<int,int> > > > c;tvsFlowNetwork::contacts(el,tol,c);
        int openEnds=0,last=-1;for(size_t p=0;p<c[root].size();++p)if(c[root][p].empty()){++openEnds;last=static_cast<int>(p);}
        if(openEnds==1)port=last;else root=-1;}
    else root=-1;
    if(root<0){
        ads_name picked;ads_point at;
        if(acedEntSel(L"\nTVS_DPD_IZVOZ: izaberite kraj mreže kod jedinice (kliknite blizu slobodnog kraja kanala): ",picked,at)!=RTNORM)return;
        AcDbObjectId pickedId;if(acdbGetObjectId(pickedId,picked)!=Acad::eOk)return;
        for(size_t i=0;i<net.nodes.size();++i)if(net.nodes[i].id==pickedId)root=net.owner[i];
        if(root<0||net.bodies[root].ports.empty()){acutPrintf(L"\nTVS_DPD_IZVOZ: izabrani objekat nije TVS kanal ili fiting mreže.");return;}
        ads_point w;if(tvsCoordinates::transformPoint(at,1,0,w)!=RTNORM)return;
        double nearest=0;
        for(size_t p=0;p<net.bodies[root].ports.size();++p){const double d=std::hypot(net.bodies[root].ports[p].x-w[0],net.bodies[root].ports[p].y-w[1]);if(port<0||d<nearest){port=static_cast<int>(p);nearest=d;}}}
    // koljeno gore / dolje: its ports carry the elevation in z, the duct ends often do not (as TVS_PAD)
    for(size_t i=0;i<net.bodies.size()&&i<el.size();++i){
        int only=-1,count=0;for(size_t j=0;j<net.nodes.size();++j)if(net.owner[j]==static_cast<int>(i)){only=static_cast<int>(j);++count;}
        if(count!=1)continue;
        {AcDbObjectPointer<TVS_TAP> t(net.nodes[only].id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk||t->get_Form()==Form_Direct)continue;}
        for(size_t p=0;p<el[i].ports.size();++p){bool done=false;
            for(size_t k=0;k<el.size()&&!done;++k){if(k==i)continue;
                for(size_t q=0;q<el[k].ports.size();++q){if(std::hypot(el[k].ports[q].x-el[i].ports[p].x,el[k].ports[q].y-el[i].ports[p].y)<=tol&&std::fabs(el[k].ports[q].z-el[i].ports[p].z)>tol){el[i].ports[p].z=el[k].ports[q].z;done=true;break;}}}}}
    tvsFlowNetwork::Result r=tvsFlowNetwork::solve(el,root,port,tol);
    tvsFlow::equipmentFlows(net,el,r,root,port,tol,L"TVS_DPD_IZVOZ");   // oprema: terminal flows from the records
    if(r.error!=tvsFlowNetwork::Ok){tvsFlow::problem(net,r,f);return;}
    int missing=0;for(size_t k=0;k<r.terminals.size();++k)if(!(el[r.terminals[k]].flow>0))++missing;
    if(missing)acutPrintf(L"\nTVS_DPD_IZVOZ: krajnjih elemenata bez protoka: %d (TVS_PROTOK ih unosi); njihove dionice imaju Q = 0.",missing);
    // system
    {acedInitGet(0,L"Dovod Odvod");ACHAR kw[16]=L"";wchar_t prompt[160];
     swprintf_s(prompt,160,L"\nSistem [Dovod/Odvod] <%ls>: ",exhaust()?L"Odvod":L"Dovod");
     const int k=acedGetKword(prompt,kw,16);if(k==RTCAN)return;
     if(k==RTNORM)exhaust()=_wcsicmp(kw,L"Odvod")==0;}
    // elements and dionice as TVS_PAD, without the ASHRAE data
    std::vector<tvsPressure::Element> pe(net.bodies.size());
    for(size_t i=0;i<net.bodies.size();++i){if(!r.reached[i])continue;tvsPressure::Element& x=pe[i];
        x.flow=r.flow[i];x.parent=r.parent[i];x.entry=r.entry[i];x.via=r.via[i];
        for(size_t p=0;p<net.bodies[i].ports.size();++p)x.at.push_back(tvsPad::xy(net.bodies[i].ports[p],f));
        for(size_t j=0;j<net.nodes.size();++j){if(net.owner[j]!=static_cast<int>(i))continue;tvsPressure::Part t;if(!tvsPad::part(net.nodes[j],f,t))continue;
            if(t.kind==tvsPressure::Elbow){   // koljeno gore / dolje: no bend in plan
                AcDbObjectPointer<TVS_TAP> tap(net.nodes[j].id,AcDb::kForRead);
                if(tap.openStatus()==Acad::eOk&&tap->get_Form()!=Form_Direct){t.line.clear();for(size_t k=0;k<net.nodes[j].ports.size();++k)t.line.push_back(tvsPad::xy(net.nodes[j].ports[k],f));}}
            x.parts.push_back(t);}
        if(net.bodies[i].sections.size()==net.bodies[i].ports.size())for(size_t p=0;p<net.bodies[i].sections.size();++p)x.ports.push_back(tvsPad::section(net.bodies[i].sections[p],f));
        else if(x.parts.size()==1)x.ports=x.parts[0].sections;
        else if(!x.parts.empty()&&!x.parts[0].sections.empty())x.ports.assign(x.at.size(),x.parts[0].sections[0]);}
    tvsPressure::Options o;o.exhaust=exhaust();
    tvsPressure::Calculator calc(nullptr,o);
    const tvsPressure::Result res=calc.run(pe,root);
    std::vector<std::wstring> warnings;
    tvsDpd::System sys;sys.exhaust=exhaust();sys.segments=tvsDpd::segments(pe,res,sys.exhaust,&warnings);
    if(sys.segments.empty()){acutPrintf(L"\nTVS_DPD_IZVOZ: mreža nema nijednu dionicu za izvoz.");return;}
    std::wstring stem,file;drawingName(db,stem,file);
    const std::wstring kind=exhaust()?L"Odvod":L"Dovod";
    std::wstring base=file;{const size_t dot=base.find_last_of(L'.');if(dot!=std::wstring::npos)base=base.substr(0,dot);}
    sys.name=tvsDpd::utf8(base.empty()?kind:kind+L" "+base);sys.project=tvsDpd::utf8(file);sys.source="TVS_VENT TVS_DPD_IZVOZ";
    // file
    const std::wstring def=(stem.empty()?std::wstring(L"mreza"):stem)+L"_"+kind;
    resbuf* rb=acutNewRb(RTSTR);
    if(acedGetFileD(L"TVS_DPD_IZVOZ: sistem za DuctPressDrop",def.c_str(),L"json",1,rb)!=RTNORM||!rb->resval.rstring){acutRelRb(rb);return;}
    const std::wstring path(rb->resval.rstring);acutRelRb(rb);
    if(!writeFile(path,tvsDpd::json(sys))){acutPrintf(L"\nTVS_DPD_IZVOZ: fajl %ls nije upisan (otvoren u drugom programu / nema prava?).",path.c_str());return;}
    for(size_t k=0;k<warnings.size();++k)acutPrintf(L"\n  %ls",warnings[k].c_str());
    acutPrintf(L"\nTVS_DPD_IZVOZ: %ls, Q = %.0f m3/h, TVS dionica %d -> DuctPressDrop dionica %d (podijeljene na koljenima). Upisano: %ls",
        kind.c_str(),res.d.empty()?0.0:res.d[0].flow,static_cast<int>(res.d.size()),static_cast<int>(sys.segments.size()),path.c_str());
    acutPrintf(L"\nTVS_DPD_IZVOZ: u DuctPressDrop-u Datoteka -> Uvezi sistem... (Ctrl+I). Koljena, T komadi i prelazi se tamo izvode iz geometrije; povezana oprema (TVS_OPREMA) ide kao oprema na dionici; kote se ne prenose.");
}
}
