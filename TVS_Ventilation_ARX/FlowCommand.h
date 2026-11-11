#pragma once
// TVS_PROTOK: air flows from the terminal elements summed to the unit (geometry/FlowNetwork.h).
// Pick the end of the network at the unit; every element on the way gets the sum of the
// terminals behind it. Terminals keep their own flow; those without one can be entered here
// (zoom on each). An etaz / obilazak counts as one element (TVS_MOVE bodies), flex too.
// Nothing is written before the confirmation; TVS labels of changed elements are refreshed.
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "../TVS_Ventilation_DBX/DuctCaption.h"
#include "../geometry/FlowNetwork.h"
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
    if(change.empty()){acutPrintf(L"\nTVS_PROTOK: protoci u crtežu su već tačni.");return;}
    {acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";wchar_t prompt[120];
     swprintf_s(prompt,120,L"\nUpisati protoke u %d elemenata? [Da/Ne] <Da>: ",static_cast<int>(change.size()));
     const int k=acedGetKword(prompt,kw,8);
     if(k==RTCAN||(k==RTNORM&&_wcsicmp(kw,L"Ne")==0))return;}
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
}
}
