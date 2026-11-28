#pragma once
// TVS_PAD: pad pritiska po dionicama u Excel. The network from the end at the unit (as TVS_PROTOK;
// flows summed from the terminals, nothing is written into the drawing), split into dionice with
// friction and the ASHRAE fittings (geometry/PressureDrop.h; linked oprema: the loss of its record,
// terminals in "Δp kraj. elem."; rešetka na dnu: a zero-length branch with the Idelchik T), then the workbook
// (geometry/PressureSheet.h) is built in Excel (ExcelWriter.h): Dionice, Fitinzi, Sema.
//   ASHRAE data: the user's DuctPressDrop folder ashrae_data (Constants / Equation / Output /
//   Input.csv, tables\*.TBL), chosen once by picking its Equation.csv (registry Software\TVS_Vent,
//   AshraeData); not in the TVS_VENT repository. Without it: friction only, zeta typed in Excel.
#include "Warnings.h"
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
    if(n.kind==tvsEquipmentFlow::tapKind()){   // rešetka na dnu: the tap junction (FlowCommand.h splitBottom), not in the drawing
        for(size_t k=0;k<n.sections.size();++k)t.sections.push_back(section(n.sections[k],f));
        t.kind=tvsPressure::BottomTap;return true;}
    AcDbObjectPointer<AcDbEntity> e(n.id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)return false;
    for(size_t k=0;k<n.sections.size();++k)t.sections.push_back(section(n.sections[k],f));
    for(size_t k=0;k<n.ports.size();++k)t.line.push_back(xy(n.ports[k],f));
    if(TVS_Pipe* p=TVS_Pipe::cast(e.object())){t.kind=tvsPressure::Duct;t.flex=p->DuctType==DuctTypeFlex;
        t.length=n.ports.size()==2?(n.ports[1]-n.ports[0]).length()*f:0;return true;}
    if(TVS_TAP* p=TVS_TAP::cast(e.object())){
        // koljeno gore / dolje (Form_Up / Form_Down): the bend is in the vertical plane, so the side in
        // it (ASHRAE W) is the height SizeB; get_radius() already takes that side (TVS_TAP.cpp)
        const bool vertical=!p->ThisRound&&(p->get_Form()==Form_Up||p->get_Form()==Form_Down);
        const double bend=vertical?p->SizeB:p->SizeA,across=vertical?p->SizeA:p->SizeB;
        t.radius=(p->get_radius()+bend/2)*f;t.angle=std::fabs(p->Swectangle)*180.0/3.14159265358979323846;
        t.length=t.radius*std::fabs(p->Swectangle);
        t.sections.assign(2,tvsPressure::Section(bend*f,p->ThisRound?0:across*f,p->ThisRound!=0));
        if(n.ports.size()==2){t.line.clear();t.line.push_back(xy(n.ports[0],f));t.line.push_back(tvsPressure::XY(p->MiddlePoint.x*f,p->MiddlePoint.y*f));t.line.push_back(xy(n.ports[1],f));}
        if(p->DuctType==DuctTypeFlex){t.kind=tvsPressure::Duct;t.flex=true;}else t.kind=tvsPressure::Elbow;
        return true;}
    if(TVS_TRANS* p=TVS_TRANS::cast(e.object())){t.kind=tvsPressure::Transition;t.length=p->LengthTr*f;return true;}
    if(TVS_WYE* p=TVS_WYE::cast(e.object())){t.kind=tvsPressure::Tee;
        tvsTeeData::Record rec;t.tee=tvsPressure::TeePlain;
        if(tvsTeeData::read(p,rec)){t.cone=rec.type==tvsTee::Tap&&p->ThisRoundotv&&rec.neck<=0&&tvsTee::isCone(rec.second,p->SizeAotv,rec.l);
            t.lateral=(rec.type==tvsTee::Tap||rec.type==tvsTee::Pants)&&rec.neck>0&&p->ThisRoundpr&&p->ThisRoundotv;}   // kosa stucna / Y45: each leg as a wye 45 branch
        if(tvsTeeData::read(p,rec))t.tee=rec.type==tvsTee::Branch?tvsPressure::TeeBranch:rec.type==tvsTee::Pants?tvsPressure::TeePants:rec.type==tvsTee::Tap?tvsPressure::TeeTap:tvsPressure::TeePlain;
        if(n.ports.size()==3){const tvsPressure::XY m(p->Basepoint.x*f,p->Basepoint.y*f);t.line.clear();
            if(t.tee==tvsPressure::TeePants){t.line.push_back(xy(n.ports[2],f));t.line.push_back(m);t.line.push_back(xy(n.ports[0],f));t.line.push_back(m);t.line.push_back(xy(n.ports[1],f));}
            else{t.line.push_back(xy(n.ports[0],f));t.line.push_back(m);t.line.push_back(xy(n.ports[2],f));t.line.push_back(m);t.line.push_back(xy(n.ports[1],f));}}
        if(n.ports.size()==4){const tvsPressure::XY m(p->Basepoint.x*f,p->Basepoint.y*f);t.line.clear();   // krstasti komad / XK
            t.line.push_back(xy(n.ports[0],f));t.line.push_back(m);t.line.push_back(xy(n.ports[2],f));t.line.push_back(m);
            t.line.push_back(xy(n.ports[3],f));t.line.push_back(m);t.line.push_back(xy(n.ports[1],f));}
        return true;}
    if(const AcDbBlockReference* b=AcDbBlockReference::cast(e.object())){   // oprema (EquipmentNodes.h): loss from its record
        tvsEquipment::Record rec;if(tvsEquipmentStore::read(b,rec)){
            if(rec.twin&&rec.ports==2&&n.ports.size()==1){   // one connection of a linijski difuzor: its half (loss at the total flow)
                const double s=tvsEquipmentFlow::unitScale(rec.mmPerUnit,f);int k=0;double best=-1;
                for(int j=0;j<2;++j){const double d=std::hypot(rec.port[j].x*s-n.ports[0].x,rec.port[j].y*s-n.ports[0].y);if(best<0||d<best){best=d;k=j;}}
                rec=tvsEquipment::half(rec,k);}
            t.kind=tvsPressure::Equipment;t.equipment=rec;return true;}}
    t.kind=tvsPressure::Other;return true;
}
// One network from the element `root`, its free port `port` (the unit): flows, elements, the calculation.
// side: L"" or L" (usis)" / L" (potis)" in the messages. total: the flow at the root.
inline bool compute(const tvsFlow::Net& net,int root,int port,bool odvod,tvsAshrae::Engine* engine,double f,const wchar_t* side,tvsPressure::Result& res,double& total){
    const double tol=0.1/f;
    std::vector<tvsFlowNetwork::Element> el;tvsFlow::elements(net,el);
    tvsFlow::verticalJoints(net,el,tol);   // koljeno gore / dolje: joined with the duct in plan (its ports carry the elevation in z)
    tvsFlowNetwork::Result r=tvsFlowNetwork::solve(el,root,port,tol);
    tvsFlow::equipmentFlows(net,el,r,root,port,tol,L"TVS_PAD");   // oprema: terminal flows from the records
    if(r.error!=tvsFlowNetwork::Ok){tvsFlow::problem(net,r,f);return false;}
    total=r.total;
    if(tvsFlow::fanCut().port<0&&tvsFlow::fanIn(net,r.reached)>=0)
        tvsWarn::add(L"TVS_PAD: u mreži je ventilator – izaberite ventilator da se usis i potis računaju posebno i saberu.");
    int missing=0;for(size_t k=0;k<r.terminals.size();++k)if(!(el[r.terminals[k]].flow>0))++missing;
    if(missing)tvsWarn::add(L"TVS_PAD%ls: krajnjih elemenata bez protoka: %d (TVS_PROTOK ih unosi); njihove dionice imaju Q = 0.",side,missing);
    {   // flows written in the drawing (labels, Properties) vs the sums used here (user 2026-11-24): a reminder for TVS_PROTOK
        int stale=0;for(size_t i=0;i<net.real;++i){if(net.linked[i]&&!net.records[i].terminal())continue;
            const int o=tvsFlow::pieceOf(net,r,net.owner[i]);if(o>=0&&o<static_cast<int>(r.reached.size())&&r.reached[o]&&std::fabs(r.flow[o]-net.old[i])>0.5)++stale;}
        if(stale)tvsWarn::add(L"TVS_PAD%ls: protok upisan u crtežu se razlikuje od zbira krajnjih elemenata na %d elemenata – proračun koristi zbir; oznake i protoci u crtežu nisu ažurni (TVS_PROTOK ih upisuje).",side,stale);}
    std::vector<tvsPressure::Element> pe(net.bodies.size());
    for(size_t i=0;i<net.bodies.size();++i){if(!r.reached[i])continue;tvsPressure::Element& x=pe[i];
        x.flow=r.flow[i];x.parent=r.parent[i];x.entry=r.entry[i];x.via=r.via[i];
        for(size_t p=0;p<net.bodies[i].ports.size();++p)x.at.push_back(xy(net.bodies[i].ports[p],f));
        for(size_t j=0;j<net.nodes.size();++j){if(net.owner[j]!=static_cast<int>(i))continue;
            if(static_cast<int>(i)==root&&net.linked[j]&&tvsFan::isFan(net.records[j]))continue;   // the fan itself: no loss
            tvsPressure::Part t;if(part(net.nodes[j],f,t))x.parts.push_back(t);}
        if(net.bodies[i].sections.size()==net.bodies[i].ports.size())for(size_t p=0;p<net.bodies[i].sections.size();++p)x.ports.push_back(section(net.bodies[i].sections[p],f));
        else if(x.parts.size()==1)x.ports=x.parts[0].sections;
        else if(!x.parts.empty()&&!x.parts[0].sections.empty())x.ports.assign(x.at.size(),x.parts[0].sections[0]);}
    tvsPressure::Options o;o.exhaust=odvod;
    tvsPressure::Calculator calc(engine,o);
    res=calc.run(pe,root);
    return true;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    tvsWarn::Scope warnings(L"TVS_PAD – upozorenja");
    const double f=tvsUnits::factor(db),tol=0.1/f;
    ads_name picked;ads_point at;
    if(acedEntSel(L"\nTVS_PAD: izaberite kraj mreže kod jedinice (kliknite blizu slobodnog kraja kanala) ili ventilator u mreži: ",picked,at)!=RTNORM)return;
    AcDbObjectId pickedId;if(acdbGetObjectId(pickedId,picked)!=Acad::eOk)return;
    tvsFlow::Net net;
    if(!tvsFlow::read(db,tol,net)){acutPrintf(L"\nTVS_PAD: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    ads_point w;if(tvsCoordinates::transformPoint(at,1,0,w)!=RTNORM)return;
    int root=-1,port=-1;   // a split duct (rešetka na dnu): the piece with the nearest end
    if(!tvsFlow::rootOf(net,pickedId,w[0],w[1],root,port)){acutPrintf(L"\nTVS_PAD: izabrani objekat nije TVS kanal ili fiting mreže.");return;}
    int suction=-1;const bool fan=tvsFlow::fanAt(net,root,suction);
    if(fan&&suction<0){acutPrintf(L"\nTVS_PAD: ventilatoru nije određen usis.");if(!tvsFan::chooseSide(net.nodes[root].id))return;
        tvsFlow::Net again;if(!tvsFlow::read(db,tol,again))return;net=again;if(!tvsFlow::rootOf(net,pickedId,w[0],w[1],root,port)||!tvsFlow::fanAt(net,root,suction)||suction<0)return;}
    // system and data (ventilator: usis = odvod, potis = dovod, nothing to choose)
    std::wstring dir=folder();
    for(;;){
        if(fan){acedInitGet(0,L"Nastavi Podaci");ACHAR kw[16]=L"";
            const int k=acedGetKword(L"\nVentilator: usis = odvod, potis = dovod [Nastavi/Podaci] <Nastavi>: ",kw,16);if(k==RTCAN)return;
            if(k==RTNORM&&_wcsicmp(kw,L"Podaci")==0){std::wstring d;if(pickFolder(d)){dir=d;storeFolder(d);}continue;}
            break;}
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
    tvsSheet::Info info;
    {const ACHAR* name=NULL;if(db->getFilename(name)==Acad::eOk&&name){std::wstring n(name);const size_t s=n.find_last_of(L"\\/");info.project=s==std::wstring::npos?n:n.substr(s+1);}}
    {SYSTEMTIME t;GetLocalTime(&t);wchar_t d[16];swprintf_s(d,16,L"%02d.%02d.%04d",t.wDay,t.wMonth,t.wYear);info.date=d;}
    if(data)info.source=dir;
    auto report=[&](const tvsPressure::Result& res,const wchar_t* side){int computed=0,typed=0;for(size_t k=0;k<res.f.size();++k)(res.f[k].ok?computed:typed)++;
        acutPrintf(L"\nTVS_PAD%ls: dionica %d, fitinga %d (ASHRAE %d, za unos %d).",side,static_cast<int>(res.d.size()),static_cast<int>(res.f.size()),computed,typed);};
    tvsSheet::Workbook book;
    if(!fan){
        tvsPressure::Result res;double q=0;if(!compute(net,root,port,exhaust(),data?&engine:nullptr,f,L"",res,q))return;
        tvsPressure::Options o;o.exhaust=exhaust();
        info.system=o.exhaust?L"Odvod (od jedinice prema krajnjim elementima)":L"Dovod (od jedinice prema krajnjim elementima)";
        report(res,L"");book=tvsSheet::build(res,info,o);}
    else{   // ventilator u mreži: usis (odvod) and potis (dovod) on their own, the fan needs the sum
        tvsFan::Labels labels;labels.show(net.records[root],suction,f);
        const AcDbObjectId fanId=net.nodes[root].id;const std::wstring tag=net.records[root].tag;
        tvsPressure::Result res[2];double q[2]={0,0};const wchar_t* side[2]={L" (usis)",L" (potis)"};
        for(int k=0;k<2;++k){
            tvsFlow::FanScope scope(fanId,k==0?1-suction:suction);   // usis: the potis port is cut, and the other way
            tvsFlow::Net n;if(!tvsFlow::read(db,tol,n)){acutPrintf(L"\nTVS_PAD: crtež se ne može pročitati.");return;}
            int r0=-1;for(size_t i=0;i<n.nodes.size();++i)if(n.nodes[i].id==fanId&&n.owner[i]==static_cast<int>(i))r0=static_cast<int>(i);
            if(r0<0)return;
            if(!compute(n,r0,tvsFlow::fanCut().port,k==0,data?&engine:nullptr,f,side[k],res[k],q[k]))return;
            report(res[k],side[k]);}
        if(q[0]>0&&q[1]>0&&std::fabs(q[0]-q[1])>0.01*(q[0]>q[1]?q[0]:q[1]))
            tvsWarn::add(L"TVS_PAD: ventilator – usis %.0f m³/h, potis %.0f m³/h. Kroz ventilator prolazi jedan protok; provjerite protoke krajnjih elemenata (TVS_PROTOK).",q[0],q[1]);
        tvsPressure::Options ou;ou.exhaust=true;tvsPressure::Options op;op.exhaust=false;
        tvsSheet::Info iu=info,ip=info;iu.system=L"Usisna strana ventilatora – odvod (krajnji elementi → ventilator)";ip.system=L"Potisna strana ventilatora – dovod (ventilator → krajnji elementi)";
        book=tvsSheet::fanBook(tvsSheet::build(res[0],iu,ou,L"usis"),tvsSheet::build(res[1],ip,op,L"potis"),info,q[0],q[1],tag);}
    tvsWarn::show(L"TVS_PAD – upozorenja");   // before Excel comes to the front
    acutPrintf(L"\nTVS_PAD: pravim Excel tabelu...");
    if(!tvsExcel::write(book)){acutPrintf(L"\nTVS_PAD: Excel nije dostupan na ovom računaru.");return;}
    acutPrintf(fan?L"\nTVS_PAD: tabela je otvorena u Excelu (nije sačuvana) – list „Ventilator“ sabira usis i potis. Unesite Δp krajnjih elemenata (žute ćelije).":L"\nTVS_PAD: tabela je otvorena u Excelu (nije sačuvana). Unesite Δp krajnjih elemenata (žute ćelije).");
}
}
