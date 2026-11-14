#pragma once
// TVS_PAD: pad pritiska po dionicama u Excel. The network from the end at the unit (as TVS_PROTOK;
// flows summed from the terminals, nothing is written into the drawing), split into dionice with
// friction and the ASHRAE fittings (geometry/PressureDrop.h), then the workbook
// (geometry/PressureSheet.h) is built in Excel (ExcelWriter.h): Dionice, Fitinzi, Sema.
//   ASHRAE data: the user's DuctPressDrop folder ashrae_data (Constants / Equation / Output /
//   Input.csv, tables\*.TBL), chosen once by picking its Equation.csv (registry Software\TVS_Vent,
//   AshraeData); not in the TVS_VENT repository. Without it: friction only, zeta typed in Excel.
#include "FlowCommand.h"
#include "ExcelWriter.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "../geometry/PressureDrop.h"
#include "../geometry/PressureSheet.h"
#include <cstdio>
#include <string>
#include <vector>
namespace tvsPad {
inline const wchar_t* regKey(){return L"Software\\TVS_Vent";}
inline std::wstring folder(){
    HKEY k=NULL;if(RegOpenKeyExW(HKEY_CURRENT_USER,regKey(),0,KEY_READ,&k)!=ERROR_SUCCESS)return std::wstring();
    wchar_t v[1024]={0};DWORD size=sizeof(v)-sizeof(wchar_t),type=0;
    const bool ok=RegQueryValueExW(k,L"AshraeData",NULL,&type,reinterpret_cast<LPBYTE>(v),&size)==ERROR_SUCCESS&&type==REG_SZ;
    RegCloseKey(k);return ok?std::wstring(v):std::wstring();
}
inline void storeFolder(const std::wstring& path){
    HKEY k=NULL;if(RegCreateKeyExW(HKEY_CURRENT_USER,regKey(),0,NULL,0,KEY_WRITE,NULL,&k,NULL)!=ERROR_SUCCESS)return;
    RegSetValueExW(k,L"AshraeData",0,REG_SZ,reinterpret_cast<const BYTE*>(path.c_str()),static_cast<DWORD>((path.size()+1)*sizeof(wchar_t)));RegCloseKey(k);
}
inline bool readFile(const std::wstring& path,std::string& out){
    FILE* fp=NULL;if(_wfopen_s(&fp,path.c_str(),L"rb")!=0||!fp)return false;
    out.clear();char buf[65536];size_t n;while((n=fread(buf,1,sizeof(buf),fp))>0)out.append(buf,n);fclose(fp);return true;
}
inline tvsAshrae::Reader reader(const std::wstring& dir){
    return [dir](const std::string& file,std::string& text){std::wstring w(file.begin(),file.end());for(size_t i=0;i<w.size();++i)if(w[i]==L'/')w[i]=L'\\';return readFile(dir+L"\\"+w,text);};
}
// the folder of the picked Equation.csv
inline bool pickFolder(std::wstring& dir){
    resbuf* rb=acutNewRb(RTSTR);const std::wstring def=folder().empty()?std::wstring(L"Equation"):folder()+L"\\Equation";
    if(acedGetFileD(L"ASHRAE podaci: izaberite Equation.csv u folderu ashrae_data (DuctPressDrop)",def.c_str(),L"csv",0,rb)!=RTNORM||!rb->resval.rstring){acutRelRb(rb);return false;}
    std::wstring p(rb->resval.rstring);acutRelRb(rb);
    const size_t slash=p.find_last_of(L"\\/");if(slash==std::wstring::npos)return false;
    dir=p.substr(0,slash);return true;
}
inline bool& exhaust(){static bool e=false;return e;}
inline tvsPressure::Section section(const tvsMoveNetwork::Section& s,double f){return tvsPressure::Section(s.a*f,s.round?0:s.b*f,s.round);}
inline tvsPressure::XY xy(const tvsMoveGeometry::Point& p,double f){return tvsPressure::XY(p.x*f,p.y*f);}
// one drawing element as a part (sizes in mm)
inline bool part(const tvsMove::Node& n,double f,tvsPressure::Part& t){
    AcDbObjectPointer<AcDbEntity> e(n.id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)return false;
    for(size_t k=0;k<n.sections.size();++k)t.sections.push_back(section(n.sections[k],f));
    for(size_t k=0;k<n.ports.size();++k)t.line.push_back(xy(n.ports[k],f));
    if(TVS_Pipe* p=TVS_Pipe::cast(e.object())){t.kind=tvsPressure::Duct;t.flex=p->DuctType==DuctTypeFlex;
        t.length=n.ports.size()==2?(n.ports[1]-n.ports[0]).length()*f:0;return true;}
    if(TVS_TAP* p=TVS_TAP::cast(e.object())){
        double radius=p->Radius;const int type=static_cast<int>(p->ThisRound?p->RadiusTypeRound:p->RadiusTypeRect);
        if(p->ThisRound?type==RadiusTypeRound_RadiusVariable:type==RadiusTypeRect_RadiusVariable)radius=p->SizeA*p->RadiusVariableParameter;
        if(p->ThisRound?type==RadiusTypeRound_RadiusConstant:type==RadiusTypeRect_RadiusConstant)radius=p->RadiusConst;
        if(p->ThisRound?type==RadiusTypeRound_RadiusSpecific:type==RadiusTypeRect_RadiusSpecific)radius=tvsUnits::mm(p->SizeA,p)<355?p->SizeA:p->SizeA/2;
        t.radius=(radius+p->SizeA/2)*f;t.angle=std::fabs(p->Swectangle)*180.0/3.14159265358979323846;
        t.length=t.radius*std::fabs(p->Swectangle);
        t.sections.assign(2,tvsPressure::Section(p->SizeA*f,p->ThisRound?0:p->SizeB*f,p->ThisRound!=0));
        if(n.ports.size()==2){t.line.clear();t.line.push_back(xy(n.ports[0],f));t.line.push_back(tvsPressure::XY(p->MiddlePoint.x*f,p->MiddlePoint.y*f));t.line.push_back(xy(n.ports[1],f));}
        if(p->DuctType==DuctTypeFlex){t.kind=tvsPressure::Duct;t.flex=true;}else t.kind=tvsPressure::Elbow;
        return true;}
    if(TVS_TRANS* p=TVS_TRANS::cast(e.object())){t.kind=tvsPressure::Transition;t.length=p->LengthTr*f;return true;}
    if(TVS_WYE* p=TVS_WYE::cast(e.object())){t.kind=tvsPressure::Tee;
        tvsTeeData::Record rec;t.tee=tvsPressure::TeePlain;
        if(tvsTeeData::read(p,rec))t.tee=rec.type==tvsTee::Branch?tvsPressure::TeeBranch:rec.type==tvsTee::Pants?tvsPressure::TeePants:rec.type==tvsTee::Tap?tvsPressure::TeeTap:tvsPressure::TeePlain;
        if(n.ports.size()==3){const tvsPressure::XY m(p->Basepoint.x*f,p->Basepoint.y*f);t.line.clear();
            if(t.tee==tvsPressure::TeePants){t.line.push_back(xy(n.ports[2],f));t.line.push_back(m);t.line.push_back(xy(n.ports[0],f));t.line.push_back(m);t.line.push_back(xy(n.ports[1],f));}
            else{t.line.push_back(xy(n.ports[0],f));t.line.push_back(m);t.line.push_back(xy(n.ports[2],f));t.line.push_back(m);t.line.push_back(xy(n.ports[1],f));}}
        return true;}
    t.kind=tvsPressure::Other;return true;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=0.1/f;
    ads_name picked;ads_point at;
    if(acedEntSel(L"\nTVS_PAD: izaberite kraj mreže kod jedinice (kliknite blizu slobodnog kraja kanala): ",picked,at)!=RTNORM)return;
    AcDbObjectId pickedId;if(acdbGetObjectId(pickedId,picked)!=Acad::eOk)return;
    tvsFlow::Net net;
    if(!tvsFlow::read(db,tol,net)){acutPrintf(L"\nTVS_PAD: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    int root=-1;for(size_t i=0;i<net.nodes.size();++i)if(net.nodes[i].id==pickedId)root=net.owner[i];
    if(root<0||net.bodies[root].ports.empty()){acutPrintf(L"\nTVS_PAD: izabrani objekat nije TVS kanal ili fiting mreže.");return;}
    ads_point w;if(tvsCoordinates::transformPoint(at,1,0,w)!=RTNORM)return;
    int port=-1;double nearest=0;
    for(size_t p=0;p<net.bodies[root].ports.size();++p){const double d=std::hypot(net.bodies[root].ports[p].x-w[0],net.bodies[root].ports[p].y-w[1]);if(port<0||d<nearest){port=static_cast<int>(p);nearest=d;}}
    std::vector<tvsFlowNetwork::Element> el;tvsFlow::elements(net,el);
    const tvsFlowNetwork::Result r=tvsFlowNetwork::solve(el,root,port,tol);
    if(r.error!=tvsFlowNetwork::Ok){tvsFlow::problem(net,r,f);return;}
    int missing=0;for(size_t k=0;k<r.terminals.size();++k)if(!(el[r.terminals[k]].flow>0))++missing;
    if(missing)acutPrintf(L"\nTVS_PAD: krajnjih elemenata bez protoka: %d (TVS_PROTOK ih unosi); njihove dionice imaju Q = 0.",missing);
    // system and data
    std::wstring dir=folder();
    for(;;){
        acedInitGet(0,L"Dovod Odvod Podaci");ACHAR kw[16]=L"";wchar_t prompt[200];
        swprintf_s(prompt,200,L"\nSistem [Dovod/Odvod/Podaci] <%ls>: ",exhaust()?L"Odvod":L"Dovod");
        const int k=acedGetKword(prompt,kw,16);if(k==RTCAN)return;
        if(k==RTNORM&&_wcsicmp(kw,L"Podaci")==0){std::wstring d;if(pickFolder(d)){dir=d;storeFolder(d);}continue;}
        if(k==RTNORM)exhaust()=_wcsicmp(kw,L"Odvod")==0;
        break;}
    tvsAshrae::Engine engine;bool data=false;
    if(dir.empty()){acutPrintf(L"\nTVS_PAD: folder ASHRAE podataka nije izabran.");
        acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";const int k=acedGetKword(L"\nIzabrati ga sada (Equation.csv u ashrae_data)? [Da/Ne] <Da>: ",kw,8);if(k==RTCAN)return;
        if(k!=RTNORM||_wcsicmp(kw,L"Ne")!=0){std::wstring d;if(pickFolder(d)){dir=d;storeFolder(d);}}}
    if(!dir.empty()){data=engine.load(reader(dir));if(!data)acutPrintf(L"\nTVS_PAD: u folderu %ls nema Constants / Equation / Output.csv – samo trenje, ζ unosite u Excelu (opcija Podaci).",dir.c_str());}
    // elements
    std::vector<tvsPressure::Element> pe(net.bodies.size());
    for(size_t i=0;i<net.bodies.size();++i){if(!r.reached[i])continue;tvsPressure::Element& x=pe[i];
        x.flow=r.flow[i];x.parent=r.parent[i];x.entry=r.entry[i];x.via=r.via[i];
        for(size_t p=0;p<net.bodies[i].ports.size();++p)x.at.push_back(xy(net.bodies[i].ports[p],f));
        for(size_t j=0;j<net.nodes.size();++j){if(net.owner[j]!=static_cast<int>(i))continue;tvsPressure::Part t;if(part(net.nodes[j],f,t))x.parts.push_back(t);}
        if(net.bodies[i].sections.size()==net.bodies[i].ports.size())for(size_t p=0;p<net.bodies[i].sections.size();++p)x.ports.push_back(section(net.bodies[i].sections[p],f));
        else if(x.parts.size()==1)x.ports=x.parts[0].sections;
        else if(!x.parts.empty()&&!x.parts[0].sections.empty())x.ports.assign(x.at.size(),x.parts[0].sections[0]);}
    tvsPressure::Options o;o.exhaust=exhaust();
    tvsPressure::Calculator calc(data?&engine:nullptr,o);
    const tvsPressure::Result res=calc.run(pe,root);
    // workbook
    tvsSheet::Info info;info.system=o.exhaust?L"Odvod (od jedinice prema krajnjim elementima)":L"Dovod (od jedinice prema krajnjim elementima)";
    {const ACHAR* name=NULL;if(db->getFilename(name)==Acad::eOk&&name){std::wstring n(name);const size_t s=n.find_last_of(L"\\/");info.project=s==std::wstring::npos?n:n.substr(s+1);}}
    {SYSTEMTIME t;GetLocalTime(&t);wchar_t d[16];swprintf_s(d,16,L"%02d.%02d.%04d",t.wDay,t.wMonth,t.wYear);info.date=d;}
    if(data)info.source=dir;
    int computed=0,typed=0;for(size_t k=0;k<res.f.size();++k)(res.f[k].ok?computed:typed)++;
    acutPrintf(L"\nTVS_PAD: dionica %d, fitinga %d (ASHRAE %d, za unos %d). Pravim Excel tabelu...",static_cast<int>(res.d.size()),static_cast<int>(res.f.size()),computed,typed);
    if(!tvsExcel::write(tvsSheet::build(res,info,o))){acutPrintf(L"\nTVS_PAD: Excel nije dostupan na ovom računaru.");return;}
    acutPrintf(L"\nTVS_PAD: tabela je otvorena u Excelu (nije sačuvana). Unesite Δp krajnjih elemenata (žute ćelije).");
}
}
