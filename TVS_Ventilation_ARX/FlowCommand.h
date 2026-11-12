#pragma once
// TVS_PROTOK: air flows from the terminal elements summed to the unit (geometry/FlowNetwork.h).
// Pick the end of the network at the unit; every element on the way gets the sum of the
// terminals behind it. Terminals keep their own flow; those without one can be entered here
// (zoom on each). An etaz / obilazak counts as one element (TVS_MOVE bodies), flex too.
// Nothing is written before the confirmation; TVS labels of changed elements are refreshed.
// Then the velocities: dionice (chains between branchings) above a maximum velocity, with the
// size proposals (geometry/DuctSizing.h), walked through with a zoom. Nothing is resized; a
// dionica can be marked "ručno" (xdata TVS_RUCNO on its ducts): no proposal for it any more.
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "../TVS_Ventilation_DBX/DuctCaption.h"
#include "../geometry/FlowNetwork.h"
#include "../geometry/DuctSizing.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <vector>
namespace tvsFlow {
struct Net {
    std::vector<tvsMove::Node> nodes;
    std::vector<tvsMoveNetwork::Body> bodies;
    std::vector<std::vector<AcDbObjectId> > members;
    std::vector<int> owner;            // node -> element (body) it belongs to
    std::vector<double> old;           // flow in the drawing, per node
    AcDbObjectIdArray labels;
};
inline bool read(AcDbDatabase* db,double tol,Net& net){
    if(!tvsMove::snapshot(db,net.nodes,net.labels))return false;
    tvsMove::bodiesFor(net.nodes,net.bodies,net.members,tol);
    const size_t n=net.nodes.size();
    for(size_t i=0;i<n;++i)if(net.nodes[i].flex){net.bodies[i].ports=net.nodes[i].ports;}   // flex carries the flow too
    net.owner.resize(n);for(size_t i=0;i<n;++i)net.owner[i]=static_cast<int>(i);
    for(size_t i=0;i<n;++i)for(size_t k=0;k<net.members[i].size();++k)
        for(size_t j=0;j<n;++j)if(net.nodes[j].id==net.members[i][k])net.owner[j]=static_cast<int>(i);
    net.old.assign(n,0);
    for(size_t i=0;i<n;++i){AcDbObjectPointer<AcDbEntity> e(net.nodes[i].id,AcDb::kForRead);
        if(e.openStatus()!=Acad::eOk)return false;
        if(TVS_Entity* t=TVS_Entity::cast(e.object()))net.old[i]=t->get_Flow();}
    return true;
}
inline void elements(const Net& net,std::vector<tvsFlowNetwork::Element>& out){
    out.assign(net.bodies.size(),tvsFlowNetwork::Element());
    for(size_t i=0;i<net.bodies.size();++i){
        if(net.owner[i]!=static_cast<int>(i))continue;   // member of an etaz / obilazak: no ports of its own
        for(size_t p=0;p<net.bodies[i].ports.size();++p){const tvsMoveGeometry::Point& q=net.bodies[i].ports[p];out[i].ports.push_back(tvsFlowNetwork::P(q.x,q.y,q.z));}
        out[i].flow=net.old[i];
    }
}
inline const wchar_t* name(const tvsMove::Node& n,bool assembly){return assembly?L"etaž / obilazak":tvsCheck::kindName(n);}
inline tvsMoveGeometry::Point centre(const tvsFlowNetwork::Element& e){
    tvsMoveGeometry::Point c(0,0,0);if(e.ports.empty())return c;
    for(size_t p=0;p<e.ports.size();++p){c.x+=e.ports[p].x;c.y+=e.ports[p].y;}
    c.x/=e.ports.size();c.y/=e.ports.size();return c;
}
inline std::vector<AcDbObjectId> idsOf(const Net& net,int i){
    if(!net.members[i].empty())return net.members[i];
    return std::vector<AcDbObjectId>(1,net.nodes[i].id);
}
inline void problem(const Net& net,const tvsFlowNetwork::Result& r,double f){
    switch(r.error){
    case tvsFlowNetwork::RootJoined:acutPrintf(L"\nTVS_PROTOK: izabrani kraj je spojen sa drugim elementom. Izaberite slobodan kraj mreže – onaj koji ide na jedinicu.");break;
    case tvsFlowNetwork::Ambiguous:acutPrintf(L"\nTVS_PROTOK: na tački X=%.0f Y=%.0f sastaje se više od dva kraja – spoj nije jednoznačan (TVS_PROVJERA). Ništa nije upisano.",r.where.x*f,r.where.y*f);break;
    case tvsFlowNetwork::Cycle:acutPrintf(L"\nTVS_PROTOK: mreža je zatvorena u petlju kod X=%.0f Y=%.0f – protoci se ne mogu jednoznačno sabrati. Ništa nije upisano.",r.where.x*f,r.where.y*f);break;
    default:acutPrintf(L"\nTVS_PROTOK: kraj mreže nije prepoznat.");break;
    }
    if(r.problem>=0){std::vector<AcDbObjectId> ids=idsOf(net,r.problem);tvsCheck::zoom(r.where.x,r.where.y,3000.0/f);tvsCheck::select(ids);}
}
// highest velocity on a duct (m/s) of the network, for the summary
inline void fastest(const Net& net,const tvsFlowNetwork::Result& r,double f){
    double best=0;int at=-1;
    for(size_t i=0;i<net.nodes.size();++i){const int o=net.owner[i];if(!r.reached[o]||net.nodes[i].kind!=1)continue;
        const double v=tvsDuctCaption::velocity(r.flow[o],net.nodes[i].a*f,net.nodes[i].b*f,net.nodes[i].round);
        if(v>best){best=v;at=static_cast<int>(i);}}
    if(at<0)return;const tvsMove::Node& n=net.nodes[at];
    if(n.round)acutPrintf(L"\n  najveća brzina u kanalu: %.2f m/s (Ø%.0f, %.0f m³/h).",best,n.a*f,r.flow[net.owner[at]]);
    else acutPrintf(L"\n  najveća brzina u kanalu: %.2f m/s (%.0f×%.0f, %.0f m³/h).",best,n.a*f,n.b*f,r.flow[net.owner[at]]);
}
inline double& maximumVelocity(){static double v=5.0;return v;}
const wchar_t* const lockApp=L"TVS_RUCNO";
// "ručno" on a duct: xdata TVS_RUCNO (1070 = 1, 1040 width, 1040 height, 0 = round), the size
// the user chose for the dionica (mm). No proposal for it any more; its velocity is shown.
struct Manual {bool on;double a,b;Manual():on(false),a(0),b(0){}};
inline Manual manualOf(const AcDbObjectId& id){
    Manual m;AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)return m;
    resbuf* x=e->xData(lockApp);if(!x)return m;int reals=0;
    for(resbuf* r=x;r;r=r->rbnext){if(r->restype==1070&&r->resval.rint==1)m.on=true;
        if(r->restype==1040){if(reals==0)m.a=r->resval.rreal;else if(reals==1)m.b=r->resval.rreal;++reals;}}
    acutRelRb(x);return m;
}
inline bool setManual(const AcDbObjectId& id,const Manual& m){
    acdbRegApp(lockApp);
    AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForWrite);if(e.openStatus()!=Acad::eOk)return false;
    resbuf* x=m.on?acutBuildList(1001,lockApp,1070,1,1040,m.a,1040,m.b,0):acutBuildList(1001,lockApp,0);if(!x)return false;
    const Acad::ErrorStatus es=e->setXData(x);acutRelRb(x);return es==Acad::eOk;
}
struct Dionica {std::vector<AcDbObjectId> ducts;int worst;double flow,v;bool round;Manual manual;double a,b;tvsDuctSizing::Size width,both;};
inline void sizeText(wchar_t* out,size_t n,bool round,double a,double b){
    if(round)swprintf_s(out,n,L"Ø%.0f",a);else swprintf_s(out,n,L"%.0f×%.0f",a,b);
}
inline void describe(const Dionica& d,int k,int count,double vmax){
    wchar_t now[32],w[32],h[32];sizeText(now,32,d.round,d.a,d.b);
    acutPrintf(L"\n  %d/%d: %ls, %.0f m³/h, %.2f m/s",k,count,now,d.flow,d.v);
    if(d.manual.on){
        if(d.manual.a>0){const bool rd=!(d.manual.b>0);sizeText(w,32,rd,d.manual.a,d.manual.b);
            const double v=tvsDuctSizing::velocity(d.flow,d.manual.a,d.manual.b,rd);
            acutPrintf(L" – ručno: %ls, %.2f m/s%ls",w,v,v>vmax+1e-9?L" (i dalje iznad)":L"");}
        else acutPrintf(L" – ručno (zadržana sadašnja dimenzija)");
        return;}
    if(d.round){if(d.width.ok){sizeText(w,32,true,d.width.a,0);acutPrintf(L" → %ls (%.2f m/s)",w,d.width.v);}else acutPrintf(L" → nema standardnog prečnika");return;}
    if(d.width.ok){sizeText(w,32,false,d.width.a,d.width.b);acutPrintf(L" → širina: %ls (%.2f m/s)",w,d.width.v);}else acutPrintf(L" → širina: nema do 3000 mm");
    if(d.both.ok){sizeText(h,32,false,d.both.a,d.both.b);acutPrintf(L"; š×v: %ls (%.2f m/s)",h,d.both.v);}
}
// "Rucno" in the walk-through: the user's size for the dionica (or keep / remove the mark).
inline bool askManual(Dionica& d,double vmax){
    for(;;){wchar_t prompt[220],now[32];
        if(d.manual.on&&d.manual.a>0){sizeText(now,32,!(d.manual.b>0),d.manual.a,d.manual.b);
            swprintf_s(prompt,220,L"\nRučna dimenzija dionice (sada %ls), npr. 300x150 ili d200; U = ukloni oznaku; Enter = zadrži: ",now);}
        else if(d.manual.on)swprintf_s(prompt,220,L"\nRučna dimenzija dionice, npr. 300x150 ili d200; U = ukloni oznaku; Enter = zadrži: ");
        else swprintf_s(prompt,220,L"\nRučna dimenzija dionice, npr. 300x150 ili d200; Enter = sadašnja dimenzija: ");
        ACHAR text[64]=L"";const int g=acedGetString(1,prompt,text,64);
        if(g!=RTNORM)return false;
        Manual m;
        if(text[0]==0){if(d.manual.on)return false;m.on=true;}
        else if((text[0]==L'U'||text[0]==L'u')&&text[1]==0)m.on=false;
        else{tvsDuctSizing::Size s;if(!tvsDuctSizing::parse(text,d.round,s)){acutPrintf(L"\nNije dimenzija: „%ls“. Primjeri: 300x150, d200%ls.",text,d.round?L", 200":L"");continue;}
            m.on=true;m.a=s.a;m.b=s.b;}
        int done=0;for(size_t k=0;k<d.ducts.size();++k)if(setManual(d.ducts[k],m))++done;
        if(done<static_cast<int>(d.ducts.size()))acutPrintf(L"\nOznaka nije upisana u %d kanala (zaključan sloj?).",static_cast<int>(d.ducts.size())-done);
        d.manual=m;
        if(!m.on)acutPrintf(L"\nOznaka „ručno“ uklonjena; dionica ponovo dobija prijedlog.");
        else if(m.a>0){const bool rd=!(m.b>0);const double v=tvsDuctSizing::velocity(d.flow,m.a,m.b,rd);sizeText(now,32,rd,m.a,m.b);
            acutPrintf(L"\nRučno: %ls, %.2f m/s%ls. Dimenziju kanala promijenite u Svojstvima / TVS_DIM_PRATI.",now,v,v>vmax+1e-9?L" – i dalje iznad najveće brzine":L"");}
        else acutPrintf(L"\nDionica označena „ručno“ sa sadašnjom dimenzijom; nema više prijedloga.");
        return true;
    }
}
// Dionice of the network above vmax (manual ones too, marked), fastest first.
inline void velocities(const Net& net,const tvsFlowNetwork::Result& r,const std::vector<tvsFlowNetwork::Element>& el,double f){
    {acedInitGet(RSG_NOZERO|RSG_NONEG,L"Ne");double v=maximumVelocity();wchar_t prompt[120];
     swprintf_s(prompt,120,L"\nProvjera brzina – najveća brzina [m/s] ili [Ne] <%.1f>: ",maximumVelocity());
     const int g=acedGetReal(prompt,&v);
     if(g==RTCAN||g==RTKWORD)return;if(g==RTNORM)maximumVelocity()=v;}
    const double vmax=maximumVelocity();
    std::vector<int> group;const int count=tvsDuctSizing::dionice(r.parent,r.reached,group);
    std::vector<Dionica> all(count);for(int g=0;g<count;++g){all[g].worst=-1;all[g].v=0;}
    for(size_t i=0;i<net.nodes.size();++i){const tvsMove::Node& n=net.nodes[i];
        if(net.owner[i]!=static_cast<int>(i)||!net.members[i].empty()||n.kind!=1||n.flex||group[i]<0)continue;
        Dionica& d=all[group[i]];d.ducts.push_back(n.id);{const Manual m=manualOf(n.id);if(m.on&&!d.manual.on)d.manual=m;}
        const double v=tvsDuctSizing::velocity(r.flow[i],n.a*f,n.b*f,n.round);
        if(d.worst<0||v>d.v){d.worst=static_cast<int>(i);d.v=v;d.flow=r.flow[i];d.round=n.round;d.a=n.a*f;d.b=n.round?0:n.b*f;}}
    std::vector<Dionica> fast;
    for(int g=0;g<count;++g){Dionica& d=all[g];if(d.worst<0||d.v<=vmax+1e-9)continue;
        if(d.round)d.width=tvsDuctSizing::diameter(d.flow,vmax);
        else{d.width=tvsDuctSizing::widthOnly(d.flow,d.b,vmax);d.both=tvsDuctSizing::widthHeight(d.flow,d.b,vmax);}
        fast.push_back(d);}
    std::sort(fast.begin(),fast.end(),[](const Dionica& x,const Dionica& y){return x.v>y.v;});
    if(fast.empty()){acutPrintf(L"\nTVS_PROTOK: sve dionice su do %.1f m/s.",vmax);return;}
    int manual=0;for(size_t k=0;k<fast.size();++k)if(fast[k].manual.on)++manual;
    acutPrintf(L"\nTVS_PROTOK: brzina iznad %.1f m/s u %d dionica (ručno: %d):",vmax,static_cast<int>(fast.size()),manual);
    for(size_t k=0;k<fast.size()&&k<50;++k)describe(fast[k],static_cast<int>(k)+1,static_cast<int>(fast.size()),vmax);
    if(fast.size()>50)acutPrintf(L"\n  ... još %d",static_cast<int>(fast.size())-50);
    int at=0;
    for(;;){Dionica& d=fast[at];
        const tvsFlowNetwork::Element& e=el[d.worst];tvsMoveGeometry::Point c(0,0,0);
        for(size_t p=0;p<e.ports.size();++p){c.x+=e.ports[p].x/e.ports.size();c.y+=e.ports[p].y/e.ports.size();}
        tvsCheck::zoom(c.x,c.y,3000.0/f);tvsCheck::select(d.ducts);describe(d,at+1,static_cast<int>(fast.size()),vmax);
        const bool last=fast.size()==1;
        acedInitGet(0,last?L"Rucno Kraj":L"Sljedeci Prethodni Rucno Kraj");ACHAR kw[32]=L"";
        const int k=acedGetKword(last?L"\nRucno = svoja dimenzija dionice. [Rucno/Kraj] <Kraj>: ":L"\nRucno = svoja dimenzija dionice. [Sljedeci/Prethodni/Rucno/Kraj] <Sljedeci>: ",kw,32);
        if(k==RTCAN||(k==RTNORM&&_wcsicmp(kw,L"Kraj")==0)||(last&&k==RTNONE))break;
        if(k==RTNORM&&_wcsicmp(kw,L"Rucno")==0){askManual(d,vmax);continue;}
        if(k==RTNORM&&_wcsicmp(kw,L"Prethodni")==0){at=at>0?at-1:static_cast<int>(fast.size())-1;continue;}
        at=(at+1)%static_cast<int>(fast.size());
    }
    acedSSSetFirst(NULL,NULL);
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=0.1/f;
    ads_name picked;ads_point at;
    const int rc=acedEntSel(L"\nIzaberite kraj mreže kod jedinice (kliknite blizu slobodnog kraja kanala): ",picked,at);
    if(rc!=RTNORM)return;
    AcDbObjectId pickedId;if(acdbGetObjectId(pickedId,picked)!=Acad::eOk)return;
    Net net;
    if(!read(db,tol,net)){acutPrintf(L"\nTVS_PROTOK: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    int root=-1;for(size_t i=0;i<net.nodes.size();++i)if(net.nodes[i].id==pickedId)root=net.owner[i];
    if(root<0||net.bodies[root].ports.empty()){acutPrintf(L"\nTVS_PROTOK: izabrani objekat nije TVS kanal ili fiting mreže.");return;}
    ads_point w;if(tvsCoordinates::transformPoint(at,1,0,w)!=RTNORM)return;
    int port=-1;double nearest=0;
    for(size_t p=0;p<net.bodies[root].ports.size();++p){const double d=std::hypot(net.bodies[root].ports[p].x-w[0],net.bodies[root].ports[p].y-w[1]);
        if(port<0||d<nearest){port=static_cast<int>(p);nearest=d;}}
    std::vector<tvsFlowNetwork::Element> el;elements(net,el);
    tvsFlowNetwork::Result r=tvsFlowNetwork::solve(el,root,port,tol);
    if(r.error!=tvsFlowNetwork::Ok){problem(net,r,f);return;}
    // terminals without a flow
    std::vector<int> missing;for(size_t k=0;k<r.terminals.size();++k)if(!(el[r.terminals[k]].flow>0))missing.push_back(r.terminals[k]);
    int reached=0;for(size_t i=0;i<net.nodes.size();++i)if(r.reached[net.owner[i]])++reached;
    acutPrintf(L"\nTVS_PROTOK: mreža ima %d elemenata, krajnjih elemenata %d (bez protoka %d), ukupno %.0f m³/h.",
               reached,static_cast<int>(r.terminals.size()),static_cast<int>(missing.size()),r.total);
    if(!missing.empty()){
        acedInitGet(0,L"Unos Nastavi");ACHAR kw[16]=L"";
        const int k=acedGetKword(L"\nKrajnji elementi bez protoka [Unos/Nastavi] <Unos>: ",kw,16);
        if(k==RTCAN)return;
        if(k==RTNONE||(k==RTNORM&&_wcsicmp(kw,L"Unos")==0)){
            for(size_t m=0;m<missing.size();++m){const int i=missing[m];
                const tvsMoveGeometry::Point c=centre(el[i]);tvsCheck::zoom(c.x,c.y,3000.0/f);tvsCheck::select(idsOf(net,i));
                acedInitGet(RSG_NONEG,L"Kraj");double q=0;wchar_t prompt[160];
                swprintf_s(prompt,160,L"\n%d/%d %ls – protok [m³/h] ili [Kraj] <preskoči>: ",static_cast<int>(m)+1,static_cast<int>(missing.size()),name(net.nodes[i],!net.members[i].empty()));
                const int g=acedGetReal(prompt,&q);
                if(g==RTCAN)return;
                if(g==RTKWORD)break;
                if(g==RTNORM)el[i].flow=q;
            }
            r=tvsFlowNetwork::solve(el,root,port,tol);
            if(r.error!=tvsFlowNetwork::Ok){problem(net,r,f);return;}
            int still=0;for(size_t k2=0;k2<r.terminals.size();++k2)if(!(el[r.terminals[k2]].flow>0))++still;
            acutPrintf(L"\nTVS_PROTOK: ukupno %.0f m³/h; krajnjih elemenata bez protoka %d.",r.total,still);
        }
    }
    if(!r.open.empty()){
        acutPrintf(L"\n  slobodnih priključaka na fitinzima (ne doprinose protoku): %d",static_cast<int>(r.open.size()));
        for(size_t k=0;k<r.open.size()&&k<10;++k){const tvsFlowNetwork::P& q=el[r.open[k].first].ports[r.open[k].second];
            acutPrintf(L"\n    %ls X=%.0f Y=%.0f",name(net.nodes[r.open[k].first],!net.members[r.open[k].first].empty()),q.x*f,q.y*f);}
    }
    fastest(net,r,f);
    // what changes
    std::vector<int> change;
    for(size_t i=0;i<net.nodes.size();++i){const int o=net.owner[i];if(r.reached[o]&&std::fabs(r.flow[o]-net.old[i])>1e-6)change.push_back(static_cast<int>(i));}
    if(change.empty()){acutPrintf(L"\nTVS_PROTOK: protoci u crtežu su već tačni.");velocities(net,r,el,f);return;}
    {acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";wchar_t prompt[120];
     swprintf_s(prompt,120,L"\nUpisati protoke u %d elemenata? [Da/Ne] <Da>: ",static_cast<int>(change.size()));
     const int k=acedGetKword(prompt,kw,8);
     if(k==RTCAN)return;
     if(k==RTNORM&&_wcsicmp(kw,L"Ne")==0){acutPrintf(L"\nProtoci nisu upisani; brzine po izračunatim protocima.");velocities(net,r,el,f);return;}}
    std::set<AcDbObjectId> written;
    {tvsMove::Transaction tx;if(!tx.good()){acutPrintf(L"\nTVS_PROTOK: izmjena nije moguća.");return;}
     for(size_t k=0;k<change.size();++k){const int i=change[k];
        TVS_Entity* t=TVS_Entity::cast(tx.write(net.nodes[i].id));
        if(!t||t->put_Flow(r.flow[net.owner[i]])!=Acad::eOk){
            const tvsMoveGeometry::Point c=centre(el[net.owner[i]]);
            acutPrintf(L"\nTVS_PROTOK: element kod X=%.0f Y=%.0f se ne može mijenjati (zaključan sloj?). Ništa nije upisano.",c.x*f,c.y*f);return;}
        t->recordGraphicsModified();written.insert(net.nodes[i].id);}
     if(!tx.commit()){acutPrintf(L"\nTVS_PROTOK: izmjena nije sačuvana.");return;}}
    // TVS labels of the changed elements
    MleaderController controller;int labels=0;
    for(int k=0;k<net.labels.length();++k){AcDbObjectId linked;
        if(controller.checkIfMLead(net.labels[k])&&controller.getObjectId(net.labels[k],linked)&&written.count(linked)&&controller.recalculateMleader(net.labels[k]))++labels;}
    acutPrintf(L"\nTVS_PROTOK: upisan protok u %d elemenata; ažurirano oznaka %d.",static_cast<int>(written.size()),labels);
    velocities(net,r,el,f);
}
}
