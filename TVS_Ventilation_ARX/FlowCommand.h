#pragma once
// TVS_PROTOK: air flows from the terminal elements summed to the unit (geometry/FlowNetwork.h).
// Pick the end of the network at the unit; every element on the way gets the sum of the
// terminals behind it. Terminals keep their own flow; those without one can be entered here
// (zoom on each). An etaz / obilazak counts as one element (TVS_MOVE bodies), flex too.
// Nothing is written before the confirmation; TVS labels of changed elements are refreshed.
// Then the velocities: dionice (chains between branchings) above a maximum velocity, listed with
// the size proposals (geometry/DuctSizing.h), then the dialog "Dimenzije dionice" for each
// (SizingDialog.h: m/s, Pa/m, own size) with a zoom between them. "Zadrži ručno" marks the
// dionica (xdata TVS_RUCNO on its ducts, with the chosen size). "Primijeni": the plan of every
// edit (geometry/DionicaPlan.h), a preview with [Da/Ne], then the writes; T komad at the end of
// the dionica: inlet takes the size and a prelaz goes behind the prolaz (TK-O / TK-G: their
// dialog). Dionice are offered farthest from the unit first. Then the network is read again.
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "../TVS_Ventilation_DBX/DuctCaption.h"
#include "../geometry/FlowNetwork.h"
#include "../geometry/DuctSizing.h"
#include "SizingDialog.h"
#include "SizeFollow.h"
#include "../geometry/DionicaPlan.h"
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
struct Dionica {std::vector<AcDbObjectId> ducts,all;int worst,depth;double flow,v;bool round;Manual manual;double a,b;tvsDuctSizing::Size width,both;};
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
// Dionice of the network above vmax (manual ones too, marked), farthest from the unit first.
struct ApplyRequest {std::vector<int> groups;AcDbObjectId worst;double a,b;bool round;int index;ApplyRequest():a(0),b(0),round(false),index(0){}};
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
    // farthest from the unit first (then fastest): a dionica is changed before the one feeding it
    for(size_t k=0;k<fast.size();++k){int d=0;for(int i=fast[k].worst;i>=0&&d<100000;i=r.parent[i])++d;fast[k].depth=d;}
    std::sort(fast.begin(),fast.end(),[](const Dionica& x,const Dionica& y){return x.depth!=y.depth?x.depth>y.depth:x.v>y.v;});
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
            request.groups=group;request.worst=net.nodes[d.worst].id;request.a=res.size.a;request.b=res.size.b;request.round=!(res.size.b>0);request.index=at;
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
// "Primijeni" (design: Fable 5.2 review): geometry/DionicaPlan.h decides every edit from the
// snapshot; here they are shown, confirmed and executed with the proven writers (elbow / T komad
// fields as applyRun, tvsTeePort::ensure, prelazBehind, openArriving), then the joints are checked.
// v1: width / diameter only (height and shape changes are refused), the part of the fastest size.
namespace planUse {
inline AcDbDatabase*& db(){static AcDbDatabase* d=NULL;return d;}
inline double& factor(){static double f=1;return f;}
inline double prelazLength(const tvsDionicaPlan::Section& a,const tvsDionicaPlan::Section& b){
    tvsTeePort::Section x,y;x.a=a.a;x.b=a.b;x.round=a.round;y.a=b.a;y.b=b.b;y.round=b.round;
    return tvsTeePort::prelazLength(db(),x,y,factor());
}
inline tvsDionicaPlan::P pt(const AcGePoint3d& p){return tvsDionicaPlan::P(p.x,p.y,p.z);}
inline tvsDionicaPlan::P pt(const tvsMoveGeometry::Point& p){return tvsDionicaPlan::P(p.x,p.y,p.z);}
inline tvsDionicaPlan::P unit(const AcGeVector3d& v){AcGeVector3d u=v;u.z=0;if(u.length()<1e-12)return tvsDionicaPlan::P();u.normalize();return tvsDionicaPlan::P(u.x,u.y,0);}
}
inline void planItems(const Net& net,const std::vector<int>& groups,double f,std::vector<tvsDionicaPlan::Item>& items){
    using namespace tvsDionicaPlan;
    const size_t n=net.nodes.size();items.assign(n,Item());
    for(size_t i=0;i<n;++i){Item& it=items[i];const tvsMove::Node& nd=net.nodes[i];
        if(net.owner[i]!=static_cast<int>(i))continue;   // member of an etaz / obilazak: no ports of its own
        it.group=i<groups.size()?groups[i]:-1;
        for(size_t p=0;p<net.bodies[i].ports.size();++p)it.ports.push_back(planUse::pt(net.bodies[i].ports[p]));
        const Section own(nd.a,nd.round?0:nd.b,nd.round);
        it.s.assign(it.ports.size(),own);
        for(size_t p=0;p<nd.sections.size()&&p<it.s.size();++p)it.s[p]=Section(nd.sections[p].a,nd.sections[p].round?0:nd.sections[p].b,nd.sections[p].round);
        if(!net.members[i].empty()){it.kind=Assembly;continue;}   // etaz / obilazak: boundary in v1
        if(nd.flex)continue;
        AcDbObjectPointer<AcDbEntity> e(nd.id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)continue;
        if(nd.kind==1&&tvsSizeFollow::plainDuct(e.object())){it.kind=Duct;it.plain=it.ports.size()==2;}
        else if(const TVS_TAP* t=TVS_TAP::cast(e.object())){
            if(nd.kind!=2||!tvsSizeFollow::plainElbow(e.object())||it.ports.size()!=2)continue;
            it.kind=Elbow;it.middle=planUse::pt(t->MiddlePoint);it.sweep=t->Swectangle;
            const tvsSizeFollow::ElbowRule r=tvsSizeFollow::elbowRule(t,nd.round);
            if(nd.round){if(r.typeRound==RadiusTypeRound_RadiusVariable){it.rule=Variable;it.k=r.k;it.plain=true;}
                else if(r.typeRound==RadiusTypeRound_RadiusConstant){it.rule=Constant;it.k=r.constant;it.plain=true;}}
            else{if(r.typeRect==RadiusTypeRect_RadiusVariable){it.rule=Variable;it.k=r.k;it.plain=true;}
                else if(r.typeRect==RadiusTypeRect_RadiusConstant){it.rule=Constant;it.k=r.constant;it.plain=true;}
                else if(r.typeRect!=RadiusTypeRect_RadiusSpecific){it.rule=Fixed;it.plain=true;}}}
        else if(const TVS_TRANS* t=TVS_TRANS::cast(e.object())){
            if(it.ports.size()!=2)continue;it.kind=Prelaz;
            it.s[0]=Section(t->SizeAp1,t->ThisRoundp1?0:t->SizeBp1,t->ThisRoundp1);it.s[1]=Section(t->SizeAp2,t->ThisRoundp2?0:t->SizeBp2,t->ThisRoundp2);
            it.aligned=t->TransType==2||t->TransType==3;}   // the far end moves with a new width
        else if(const TVS_WYE* w=TVS_WYE::cast(e.object())){
            const tvsTeeFit::Ports ps=tvsTeeFit::ports(w);if(!ps.ok)continue;
            it.kind=Tee;it.ports.clear();it.s.clear();
            for(int k=0;k<3;++k){it.ports.push_back(planUse::pt(ps.p[k]));tvsTeePort::Port q;
                if(tvsTeePort::port(w,k,q))it.s.push_back(Section(q.s.a,q.s.b,q.s.round));else it.s.push_back(own);}
            const int type=tvsTeePort::type(w);it.tee=type==tvsTee::Branch?Branch:type==tvsTee::Pants?Pants:type==tvsTee::Tap?Tap:Plain;
            it.base=planUse::pt(w->Basepoint);it.u=planUse::unit(w->Vectpr);it.v=planUse::unit(w->Vectotv);it.lengthPl=w->LengthPl;it.tapMin=tvsTeeData::tapMinimumMm()/f;}
    }
}
inline const wchar_t* issueText(int code){
    using namespace tvsDionicaPlan;
    switch(code){
    case Ambiguous:return L"više od dva kraja u jednoj tački (TVS_PROVJERA)";
    case NotDuct:return L"izabrani element nije običan kanal";
    case SameAlready:return L"dionica već ima tu dimenziju";
    case WorstTooShort:return L"kanal je prekratak za prelaze koji trebaju";
    case ElbowKept:return L"koljeno ostaje staro (na njemu nije kanal dionice), prelaz ide u susjedni kanal";
    case DuctKept:return L"kratak kanal ostaje star (nema mjesta za prelaz)";
    case TeeKept:return L"T komad ostaje (iza njega nije kanal koji može da prati), prelaz ispred njega";
    case AlignedPrelaz:return L"prelaz poravnat uz jednu stranu ostaje, umeće se novi prelaz";
    case UnknownNeighbour:return L"susjed nepoznate dimenzije, provjerite spoj";
    case PartLeft:return L"dio dionice druge dimenzije ostaje kakav je";
    case DialogNoRoom:return L"za TK-O / TK-G možda nema mjesta za prelaz";
    default:return L"";}
}
// Joints of the resized ducts that were joined before and are not now (plan, tol).
inline int brokenJoints(AcDbDatabase* db,double tol,const std::vector<AcDbObjectId>& ids,const std::vector<int>& joinedBefore,tvsDionicaPlan::P& where){
    Net net;if(!read(db,tol,net))return -1;std::vector<tvsFlowNetwork::Element> el;elements(net,el);
    std::vector<std::vector<std::vector<std::pair<int,int> > > > c;tvsFlowNetwork::contacts(el,tol,c);
    int broken=0;
    for(size_t k=0;k<ids.size();++k){int i=-1;for(size_t j=0;j<net.nodes.size();++j)if(net.nodes[j].id==ids[k])i=static_cast<int>(j);if(i<0)continue;
        int joined=0;for(size_t p=0;p<c[i].size();++p)if(!c[i][p].empty())++joined;else where=el[i].ports[p];
        if(joined<joinedBefore[k])broken+=joinedBefore[k]-joined;}
    return broken;
}
inline bool applySize(AcDbDatabase* db,const Net& net,double f,double tol,const ApplyRequest& q,std::set<AcDbObjectId>& written){
    using namespace tvsDionicaPlan;
    std::vector<Item> items;planItems(net,q.groups,f,items);
    int worst=-1;for(size_t i=0;i<net.nodes.size();++i)if(net.nodes[i].id==q.worst)worst=static_cast<int>(i);
    if(worst<0||items[worst].kind!=Duct){acutPrintf(L"\nPrimijeni: kanal dionice nije prepoznat.");return false;}
    const Section was=items[worst].s[0],S(q.a/f,q.round?0:q.b/f,q.round);
    if(was.round!=S.round){acutPrintf(L"\nPrimijeni: promjena oblika (pravougaoni / okrugli) još nije podržana – Zadrži ručno i promijenite ručno.");return false;}
    if(!S.round&&std::fabs(S.b-was.b)>1e-6){acutPrintf(L"\nPrimijeni: promjena visine (vrh ostaje) dolazi u sljedećoj verziji – izaberite H = const ili ručno.");return false;}
    planUse::db()=db;planUse::factor()=f;
    Request rq;rq.worst=worst;rq.size=S;rq.scope=SameSize;rq.tol=tol;rq.minDuct=50.0/f;rq.prelazLength=&planUse::prelazLength;
    const Plan pl=plan(items,rq);
    // preview
    wchar_t from[32],to[32];sizeText(from,32,was.round,was.a*f,was.b*f);sizeText(to,32,S.round,S.a*f,S.b*f);
    acutPrintf(L"\nPrimijeni %ls → %ls:",from,to);
    for(size_t k=0;k<pl.issues.size();++k){const Issue& x=pl.issues[k];
        acutPrintf(L"\n  %ls %ls (X=%.0f Y=%.0f)",x.level==Block?L"NE MOŽE:":x.level==Warning?L"pažnja:":L"–",issueText(x.code),x.at.x*f,x.at.y*f);}
    if(!pl.ok){acutPrintf(L"\nNišta nije promijenjeno.");return false;}
    acutPrintf(L"\n  kanala %d, koljena %d, T komad ulaz/ogranak %d, prelaza: novih %d, preuzima dimenziju %d, uklanja se %d; dijalog TK-O / TK-G %d.",
               pl.count(DuctSize),pl.count(ElbowSize),pl.count(TeeMain)+pl.count(TeeBranch),pl.count(PrelazInsert),pl.count(PrelazEnd),pl.count(PrelazRemove),pl.count(TeeDialog));
    {acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";const int k=acedGetKword(L"\nPrimijeniti? [Da/Ne] <Da>: ",kw,8);
     if(k==RTCAN||(k==RTNORM&&_wcsicmp(kw,L"Ne")==0)){acutPrintf(L"\nNišta nije promijenjeno.");return false;}}
    // joints of the resized ducts now, for the check afterwards
    std::vector<AcDbObjectId> resizedIds;std::vector<int> joinedBefore;
    {std::vector<tvsFlowNetwork::Element> el;elements(net,el);std::vector<std::vector<std::vector<std::pair<int,int> > > > c;tvsFlowNetwork::contacts(el,tol,c);
     for(size_t k=0;k<pl.resized.size();++k){const int i=pl.resized[k];if(items[i].kind!=Duct)continue;resizedIds.push_back(net.nodes[i].id);
        int j=0;for(size_t p=0;p<c[i].size();++p)if(!c[i][p].empty())++j;joinedBefore.push_back(j);}}
    const bool wasApplying=tvsSizeFollow::applying();tvsSizeFollow::applying()=true;
    tvsTeeDialog::FollowHook hook=tvsTeeDialog::followHook();tvsTeeDialog::followHook()=NULL;   // no run dialog from a T komad dialog here
    std::vector<std::pair<tvsDionicaPlan::P,tvsDionicaPlan::P> > moves;   // ports of the rigid members: before -> after
    int failed=0;
    auto id=[&](int i){return net.nodes[i].id;};
    for(size_t k=0;k<pl.edits.size();++k){const Edit& x=pl.edits[k];
        switch(x.op){
        case ElbowSize:{AcDbObjectPointer<TVS_TAP> t(id(x.item),AcDb::kForWrite);if(t.openStatus()!=Acad::eOk){++failed;break;}
            t->put_SizeA(S.a);if(!S.round)t->put_SizeB(S.b);t->recordGraphicsModified();
            tvsMove::Node after;if(tvsMove::describe(t.object(),after)&&after.ports.size()==2)
                for(int m=0;m<2;++m)moves.push_back(std::make_pair(items[x.item].ports[m],planUse::pt(after.ports[m])));
            written.insert(id(x.item));break;}
        case TeeMain:case TeeBranch:{AcDbObjectPointer<TVS_WYE> w(id(x.item),AcDb::kForWrite);if(w.openStatus()!=Acad::eOk){++failed;break;}
            tvsTeeData::Record r;const bool typed=tvsTeeData::read(w.object(),r);
            w->assertWriteEnabled();
            if(x.op==TeeMain){w->SizeApr=S.a;w->SizeBpr=S.round?0:S.b;}
            else{w->SizeAotv=S.a;w->SizeBotv=S.round?0:S.b;if(typed&&r.type==tvsTee::Tap){r.l=tvsTeeCommand::tapLength(S.a,f);w->LengthPl=r.l;}}
            if(typed)tvsTeeData::write(w.object(),r);
            w->recordGraphicsModified();
            const tvsTeeFit::Ports ps=tvsTeeFit::ports(w.object());
            if(ps.ok)for(int m=0;m<3;++m)moves.push_back(std::make_pair(items[x.item].ports[m],planUse::pt(ps.p[m])));
            written.insert(id(x.item));break;}
        case DuctSize:{AcDbObjectPointer<TVS_Pipe> p(id(x.item),AcDb::kForWrite);if(p.openStatus()!=Acad::eOk){++failed;break;}
            p->put_SizeA(S.a);p->put_SizeB(S.round?0:S.b);p->recordGraphicsModified();written.insert(id(x.item));break;}
        case DuctEnd:{tvsDionicaPlan::P target=x.to;const tvsDionicaPlan::P& now=items[x.item].ports[x.port];
            for(size_t m=0;m<moves.size();++m)if(dist(moves[m].first,now)<=tol){target=moves[m].second;break;}   // the real port, not the plan's
            AcDbObjectPointer<TVS_Pipe> p(id(x.item),AcDb::kForWrite);if(p.openStatus()!=Acad::eOk){++failed;break;}
            AcGePoint3d e=x.port==0?p->FirstPoint:p->LastPoint;e.x=target.x;e.y=target.y;
            if((x.port==0?p->put_FirstPoint(e):p->put_Lastpoint(e))!=Acad::eOk)++failed;p->recordGraphicsModified();written.insert(id(x.item));break;}
        case PrelazEnd:{AcDbObjectPointer<TVS_TRANS> t(id(x.item),AcDb::kForWrite);if(t.openStatus()!=Acad::eOk){++failed;break;}
            t->assertWriteEnabled();
            if(x.port==0){t->SizeAp1=S.a;t->SizeBp1=S.round?0:S.b;}else{t->SizeAp2=S.a;t->SizeBp2=S.round?0:S.b;}
            t->recordGraphicsModified();written.insert(id(x.item));break;}
        case PrelazRemove:{
            {AcDbObjectPointer<TVS_TRANS> t(id(x.item),AcDb::kForWrite);if(t.openStatus()!=Acad::eOk||t->erase()!=Acad::eOk){++failed;break;}}
            AcDbObjectPointer<TVS_Pipe> p(id(x.other),AcDb::kForWrite);if(p.openStatus()!=Acad::eOk){++failed;break;}
            AcGePoint3d e=x.otherPort==0?p->FirstPoint:p->LastPoint;e.x=x.to.x;e.y=x.to.y;
            if((x.otherPort==0?p->put_FirstPoint(e):p->put_Lastpoint(e))!=Acad::eOk)++failed;p->recordGraphicsModified();written.insert(id(x.other));break;}
        case PrelazInsert:{
            if(x.other>=0){if(!tvsTeePort::ensure(id(x.other),x.otherPort))++failed;written.insert(id(x.item));break;}
            AcGePoint3d at;{AcDbObjectPointer<TVS_Pipe> p(id(x.item),AcDb::kForRead);if(p.openStatus()!=Acad::eOk){++failed;break;}at=x.port==0?p->FirstPoint:p->LastPoint;}
            if(!tvsSizeFollow::prelazBehind(id(x.item),x.port==0,at,x.from.a,x.from.b,f,L"Primijeni",x.from.round?1:0))++failed;
            written.insert(id(x.item));break;}
        default:break;}
    }
    // TK-O / TK-G of the dionica: their own dialog with the new size on the port (cancel: prelaz there)
    for(size_t k=0;k<pl.edits.size();++k){const Edit& x=pl.edits[k];if(x.op!=TeeDialog)continue;
        tvsTeePort::Section in;in.a=S.a;in.b=S.round?0:S.b;in.round=S.round;
        if(!tvsTeeDialog::openArriving(id(x.item),tvsSizeFollow::ids().tee,x.port,in)){
            acutPrintf(L"\nT komad nije mijenjan: na priključku se umeće prelaz.");if(!tvsTeePort::ensure(id(x.item),x.port))++failed;}
        written.insert(id(x.item));}
    tvsTeeDialog::followHook()=hook;tvsSizeFollow::applying()=wasApplying;
    for(size_t k=0;k<resizedIds.size();++k)if(manualOf(resizedIds[k]).on)setManual(resizedIds[k],Manual());   // the size is in the drawing now
    tvsDionicaPlan::P where;const int broken=brokenJoints(db,tol,resizedIds,joinedBefore,where);
    if(failed||broken>0){
        acutPrintf(L"\nPrimijeni: %d izmjena nije uspjela, razdvojenih spojeva %d%ls. Provjerite (TVS_PROVJERA) ili vratite sa U.",failed,broken>0?broken:0,L"");
        if(broken>0){acutPrintf(L" Spoj kod X=%.0f Y=%.0f.",where.x*f,where.y*f);tvsCheck::zoom(where.x,where.y,3000.0/f);}}
    else acutPrintf(L"\nPrimijenjeno %ls → %ls. U vraća cijelu komandu.",from,to);
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
