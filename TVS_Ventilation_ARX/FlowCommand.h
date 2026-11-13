#pragma once
// TVS_PROTOK: air flows from the terminal elements summed to the unit (geometry/FlowNetwork.h).
// Pick the end of the network at the unit; every element on the way gets the sum of the
// terminals behind it. Terminals keep their own flow; those without one can be entered here
// (zoom on each). An etaz / obilazak counts as one element (TVS_MOVE bodies), flex too.
// Nothing is written before the confirmation; TVS labels of changed elements are refreshed.
// Then the velocities: dionice (chains between branchings) above a maximum velocity, listed with
// the size proposals (geometry/DuctSizing.h), then the dialog "Dimenzije dionice" for each
// (SizingDialog.h: m/s, Pa/m, own size) with a zoom between them. "Zadrži ručno" marks the
// dionica (xdata TVS_RUCNO on its ducts, with the chosen size). "Primijeni" gives the size to
// the fastest duct of the dionica and lets TVS_DIM_PRATI follow, exactly as after a manual
// change (run, elbows, T komad dialogs, prelazi). Then the network is read again.
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "../TVS_Ventilation_DBX/DuctCaption.h"
#include "../geometry/FlowNetwork.h"
#include "../geometry/DuctSizing.h"
#include "SizingDialog.h"
#include "SizeFollow.h"
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
inline int& sizingMode(){static int m=0;return m;}   // dialog: 0 height kept, 1 width : height <= 4
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
struct Dionica {std::vector<AcDbObjectId> ducts,all;int worst;double flow,v;bool round;Manual manual;double a,b;tvsDuctSizing::Size width,both;};
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
// Dionice of the network above vmax (manual ones too, marked), fastest first.
struct ApplyRequest {std::vector<AcDbObjectId> group;AcDbObjectId worst;double a,b;bool round;int index;ApplyRequest():a(0),b(0),round(false),index(0){}};
// true: "Primijeni" asked (request filled); the caller applies, reads the network again and calls again.
inline bool velocities(const Net& net,const tvsFlowNetwork::Result& r,const std::vector<tvsFlowNetwork::Element>& el,double f,UINT sizingDialog,bool ask,ApplyRequest& request,int startAt=0){
    if(ask){acedInitGet(RSG_NOZERO|RSG_NONEG,L"Ne");double v=maximumVelocity();wchar_t prompt[120];
     swprintf_s(prompt,120,L"\nProvjera brzina – najveća brzina [m/s] ili [Ne] <%.1f>: ",maximumVelocity());
     const int g=acedGetReal(prompt,&v);
     if(g==RTCAN||g==RTKWORD)return false;if(g==RTNORM)maximumVelocity()=v;}
    const double vmax=maximumVelocity();
    std::vector<int> group;const int count=tvsDuctSizing::dionice(r.parent,r.reached,group);
    std::vector<Dionica> all(count);for(int g=0;g<count;++g){all[g].worst=-1;all[g].v=0;}
    for(size_t i=0;i<net.nodes.size();++i){const int o=net.owner[i];if(o>=0&&group[o]>=0)all[group[o]].all.push_back(net.nodes[i].id);}
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
    if(fast.empty()){acutPrintf(L"\nTVS_PROTOK: sve dionice su do %.1f m/s.",vmax);return false;}
    int manual=0;for(size_t k=0;k<fast.size();++k)if(fast[k].manual.on)++manual;
    acutPrintf(L"\nTVS_PROTOK: brzina iznad %.1f m/s u %d dionica (ručno: %d):",vmax,static_cast<int>(fast.size()),manual);
    for(size_t k=0;k<fast.size()&&k<50;++k)describe(fast[k],static_cast<int>(k)+1,static_cast<int>(fast.size()),vmax);
    if(fast.size()>50)acutPrintf(L"\n  ... još %d",static_cast<int>(fast.size())-50);
    // dialog per dionica (SizingDialog.h); between two the drawing zooms on the next one
    int at=startAt>=0&&startAt<static_cast<int>(fast.size())?startAt:0;
    for(;;){Dionica& d=fast[at];
        const tvsFlowNetwork::Element& e=el[d.worst];tvsMoveGeometry::Point c(0,0,0);
        for(size_t p=0;p<e.ports.size();++p){c.x+=e.ports[p].x/e.ports.size();c.y+=e.ports[p].y/e.ports.size();}
        tvsCheck::zoom(c.x,c.y,3000.0/f);tvsCheck::select(d.ducts);acedUpdateDisplay();
        tvsSizingDialog::Input in;in.index=at+1;in.count=static_cast<int>(fast.size());in.flow=d.flow;in.a=d.a;in.b=d.b;in.round=d.round;
        in.manual=d.manual.on;in.manualA=d.manual.a;in.manualB=d.manual.b;in.ducts=static_cast<int>(d.ducts.size());
        tvsSizingDialog::Result def;def.vmax=maximumVelocity();def.mode=sizingMode();
        const tvsSizingDialog::Result res=tvsSizingDialog::show(in,def,sizingDialog);
        if(res.vmax>0)maximumVelocity()=res.vmax;if(!d.round)sizingMode()=res.mode;
        if(res.action==tvsSizingDialog::Close)break;
        if(res.action==tvsSizingDialog::Apply){
            request.group=d.all;request.worst=net.nodes[d.worst].id;request.a=res.size.a;request.b=res.size.b;request.round=!(res.size.b>0);request.index=at;
            acedSSSetFirst(NULL,NULL);return true;}
        if(res.action==tvsSizingDialog::Keep||res.action==tvsSizingDialog::Remove){
            Manual m;m.on=res.action==tvsSizingDialog::Keep;if(m.on){m.a=res.size.a;m.b=res.size.b;}
            int done=0;for(size_t k=0;k<d.ducts.size();++k)if(setManual(d.ducts[k],m))++done;
            if(done<static_cast<int>(d.ducts.size()))acutPrintf(L"\nOznaka nije upisana u %d kanala (zaključan sloj?).",static_cast<int>(d.ducts.size())-done);
            d.manual=m;describe(d,at+1,static_cast<int>(fast.size()),maximumVelocity());
            if(res.action==tvsSizingDialog::Remove)continue;   // stays on the dionica, now with proposals
            if(fast.size()==1)break;
        }
        if(res.action==tvsSizingDialog::Previous){at=at>0?at-1:static_cast<int>(fast.size())-1;continue;}
        at=(at+1)%static_cast<int>(fast.size());
    }
    acedSSSetFirst(NULL,NULL);return false;
}
// "Primijeni": the new size (mm) on the fastest duct of the dionica, then the same follow as after
// a manual change of that duct (SizeFollow.h settle = TVS_DIM_PRATI: the run, elbows, T komadi
// with their dialogs, prelazi). false: nothing changed (reason printed; one UNDO step).
inline bool applySize(AcDbDatabase* db,const Net&,double f,double,const ApplyRequest& q,std::set<AcDbObjectId>& written){
    const double A=q.a/f,B=q.round?0:q.b/f;
    tvsSizeFollow::Sizes old;
    {AcDbObjectPointer<TVS_Pipe> p(q.worst,AcDb::kForRead);
     if(p.openStatus()!=Acad::eOk||!tvsSizeFollow::plainDuct(p.object())){acutPrintf(L"\nPrimijeni: kanal dionice se ne može pročitati.");return false;}
     old.a[0]=old.a[1]=p->SizeA;old.b[0]=old.b[1]=p->ThisRound?0:p->SizeB;old.round[0]=old.round[1]=p->ThisRound;}
    if(q.round==old.round[0]&&std::fabs(A-old.a[0])<1e-6&&(q.round||std::fabs(B-old.b[0])<1e-6)){acutPrintf(L"\nPrimijeni: dionica već ima tu dimenziju.");return false;}
    {AcDbObjectPointer<TVS_Pipe> p(q.worst,AcDb::kForWrite);
     if(p.openStatus()!=Acad::eOk){acutPrintf(L"\nPrimijeni: kanal se ne može mijenjati (%ls). Ništa nije promijenjeno.",acadErrorStatusText(p.openStatus()));return false;}
     if(p->ThisRound!=q.round)p->put_ThisRound(q.round);p->put_SizeA(A);p->put_SizeB(B);p->recordGraphicsModified();}
    written.insert(q.worst);
    if(manualOf(q.worst).on)setManual(q.worst,Manual());   // the size is in the drawing now
    wchar_t from[32],to[32];sizeText(from,32,old.round[0],old.a[0]*f,old.b[0]*f);sizeText(to,32,q.round,q.a,q.b);
    acutPrintf(L"\nPrimijeni: %ls → %ls; ostatak dionice prati kao kod ručne izmjene (TVS_DIM_PRATI).",from,to);
    tvsSizeFollow::previous()[q.worst]=old;std::map<AcDbObjectId,int> work;work[q.worst]=3;
    tvsSizeFollow::settle(db,work);
    tvsSizeFollow::previous().erase(q.worst);
    tvsSizeFollow::refreshLabels(db);tvsSizeFollow::touched().clear();
    return true;
}
// Velocities, with "Primijeni" applied and the network read again until the dialog is closed.
inline void sizes(AcDbDatabase* db,Net& net,tvsFlowNetwork::Result& r,std::vector<tvsFlowNetwork::Element>& el,double f,double tol,UINT sizingDialog,
                  const AcDbObjectId& rootId,const tvsFlowNetwork::P rootPoint){
    ApplyRequest q;bool ask=true;int startAt=0;
    while(velocities(net,r,el,f,sizingDialog,ask,q,startAt)){ask=false;startAt=q.index;
        std::set<AcDbObjectId> written;applySize(db,net,f,tol,q,written);
        if(!written.empty()){MleaderController controller;int labels=0;
            for(int k=0;k<net.labels.length();++k){AcDbObjectId linked;
                if(controller.checkIfMLead(net.labels[k])&&controller.getObjectId(net.labels[k],linked)&&written.count(linked)&&controller.recalculateMleader(net.labels[k]))++labels;}
            if(labels)acutPrintf(L"\nAžurirano oznaka: %d.",labels);}
        Net again;if(!read(db,tol,again)){acutPrintf(L"\nTVS_PROTOK: crtež se ne može ponovo pročitati.");return;}
        net=again;int root=-1;for(size_t i=0;i<net.nodes.size();++i)if(net.nodes[i].id==rootId)root=net.owner[i];
        if(root<0){acutPrintf(L"\nTVS_PROTOK: početni element mreže više ne postoji.");return;}
        int port=-1;double nearest=0;
        for(size_t p=0;p<net.bodies[root].ports.size();++p){const tvsMoveGeometry::Point& x=net.bodies[root].ports[p];
            const double d=std::hypot(x.x-rootPoint.x,x.y-rootPoint.y);if(port<0||d<nearest){port=static_cast<int>(p);nearest=d;}}
        elements(net,el);r=tvsFlowNetwork::solve(el,root,port,tol);
        if(r.error!=tvsFlowNetwork::Ok){problem(net,r,f);return;}
    }
}
inline void execute(UINT sizingDialog){
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
    if(change.empty()){acutPrintf(L"\nTVS_PROTOK: protoci u crtežu su već tačni.");sizes(db,net,r,el,f,tol,sizingDialog,net.nodes[root].id,el[root].ports[port]);return;}
    {acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";wchar_t prompt[120];
     swprintf_s(prompt,120,L"\nUpisati protoke u %d elemenata? [Da/Ne] <Da>: ",static_cast<int>(change.size()));
     const int k=acedGetKword(prompt,kw,8);
     if(k==RTCAN)return;
     if(k==RTNORM&&_wcsicmp(kw,L"Ne")==0){acutPrintf(L"\nProtoci nisu upisani; brzine po izračunatim protocima.");sizes(db,net,r,el,f,tol,sizingDialog,net.nodes[root].id,el[root].ports[port]);return;}}
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
    sizes(db,net,r,el,f,tol,sizingDialog,net.nodes[root].id,el[root].ports[port]);
}
}
