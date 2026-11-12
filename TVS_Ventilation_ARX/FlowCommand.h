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
// the part of the dionica around its fastest duct (geometry/DionicaSegment.h: plain ducts and
// plan elbows of that size; the elbows get the new radius as in TVS_DIM_PRATI) and puts the
// joints right at its ends: a prelaz there takes the new size, a duct of another size gets a
// prelaz in it, a TK-O / TK-G its prelaz on the port, anything else a prelaz in our duct.
// Then the network is read again and the dialog goes on.
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "../TVS_Ventilation_DBX/DuctCaption.h"
#include "../geometry/FlowNetwork.h"
#include "../geometry/DuctSizing.h"
#include "SizingDialog.h"
#include "SizeFollow.h"
#include "../geometry/DionicaSegment.h"
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
// "Primijeni": the new size (mm) on the part of the dionica around `worst`. false: nothing or
// not all changed (reason printed; the command is one UNDO step).
inline bool applySize(AcDbDatabase* db,const Net& net,double f,double tol,const ApplyRequest& q,std::set<AcDbObjectId>& written){
    std::set<AcDbObjectId> group(q.group.begin(),q.group.end());
    const size_t n=net.nodes.size();int start=-1;
    std::vector<tvsDionicaSegment::Item> items(n);
    for(size_t i=0;i<n;++i){tvsDionicaSegment::Item& it=items[i];const tvsMove::Node& nd=net.nodes[i];
        if(net.owner[i]!=static_cast<int>(i))continue;   // member of an etaz / obilazak: no ports of its own
        for(size_t p=0;p<net.bodies[i].ports.size();++p){const tvsMoveGeometry::Point& x=net.bodies[i].ports[p];it.ports.push_back(tvsFlowNetwork::P(x.x,x.y,x.z));}
        it.group=group.count(nd.id)?0:1;it.a=nd.a;it.b=nd.b;it.round=nd.round;
        if(net.members[i].empty()&&!nd.flex){AcDbObjectPointer<AcDbEntity> e(nd.id,AcDb::kForRead);
            if(e.openStatus()==Acad::eOk){if(nd.kind==1&&tvsSizeFollow::plainDuct(e.object()))it.kind=tvsDionicaSegment::Duct;
                else if(nd.kind==2&&tvsSizeFollow::plainElbow(e.object()))it.kind=tvsDionicaSegment::Elbow;}}
        if(nd.id==q.worst)start=static_cast<int>(i);}
    if(start<0){acutPrintf(L"\nPrimijeni: kanal dionice više nije u mreži.");return false;}
    const tvsDionicaSegment::Result seg=tvsDionicaSegment::find(items,start,tol);
    if(!seg.ok){acutPrintf(seg.ambiguous?L"\nPrimijeni: na dionici se sastaje više od dva kraja u jednoj tački (TVS_PROVJERA). Ništa nije promijenjeno.":L"\nPrimijeni: dionica nije prepoznata. Ništa nije promijenjeno.");return false;}
    const double A=q.a/f,B=q.round?0:q.b/f,oldA=net.nodes[start].a,oldB=net.nodes[start].round?0:net.nodes[start].b;const bool oldRound=net.nodes[start].round;
    if(q.round==oldRound&&std::fabs(A-oldA)<1e-6&&(q.round||std::fabs(B-oldB)<1e-6)){acutPrintf(L"\nPrimijeni: dionica već ima tu dimenziju.");return false;}
    // everything that changes must be writable before the first change
    {std::vector<AcDbObjectId> check;for(size_t m=0;m<seg.members.size();++m)check.push_back(net.nodes[seg.members[m]].id);
     for(size_t k=0;k<seg.ends.size();++k)if(seg.ends[k].other>=0)check.push_back(net.nodes[seg.ends[k].other].id);
     for(size_t k=0;k<check.size();++k){AcDbObjectPointer<AcDbEntity> e(check[k],AcDb::kForWrite);
        if(e.openStatus()!=Acad::eOk){acutPrintf(L"\nPrimijeni: element dionice ili susjed se ne može mijenjati (%ls). Ništa nije promijenjeno.",acadErrorStatusText(e.openStatus()));return false;}}}
    // elbows of the part must take the new radius (round elbow with a specific radius cannot)
    for(size_t m=0;m<seg.members.size();++m){const int i=seg.members[m];if(items[i].kind!=tvsDionicaSegment::Elbow)continue;
        AcDbObjectPointer<TVS_TAP> t(net.nodes[i].id,AcDb::kForRead);tvsMove::Point np[2];
        if(t.openStatus()!=Acad::eOk||!tvsSizeFollow::elbowPortsFor(t.object(),A,q.round,np)){acutPrintf(L"\nPrimijeni: koljeno dionice se ne može preračunati za novu dimenziju (poluprečnik u TVS_SETTINGS). Ništa nije promijenjeno.");return false;}}
    // the chosen duct first (applyRun reads the size from it), then the run with the elbows
    {AcDbObjectPointer<TVS_Pipe> p(net.nodes[start].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)return false;
     if(p->ThisRound!=q.round)p->put_ThisRound(q.round);p->put_SizeA(A);p->put_SizeB(B);p->recordGraphicsModified();}
    tvsSizeFollow::Run run;int ducts=1,elbows=0;
    for(size_t m=0;m<seg.members.size();++m){const int i=seg.members[m];if(i==start)continue;
        if(items[i].kind==tvsDionicaSegment::Elbow){run.elbows.push_back(i);++elbows;}else{run.ducts.push_back(i);++ducts;}}
    std::vector<AcDbObjectId> resized;tvsSizeFollow::Etazes none;
    if(!tvsSizeFollow::applyRun(net.nodes,start,run,none,tol,resized)){   // nothing of the run written: the chosen duct back
        AcDbObjectPointer<TVS_Pipe> p(net.nodes[start].id,AcDb::kForWrite);
        if(p.openStatus()==Acad::eOk){if(p->ThisRound!=oldRound)p->put_ThisRound(oldRound);p->put_SizeA(oldA);p->put_SizeB(oldB);p->recordGraphicsModified();}
        acutPrintf(L" Ništa nije promijenjeno.");return false;}
    written.insert(net.nodes[start].id);for(size_t k=0;k<resized.size();++k)written.insert(resized[k]);
    for(size_t m=0;m<seg.members.size();++m)written.insert(net.nodes[seg.members[m]].id);
    // the joints at the ends of the part
    int prelazi=0,adjusted=0,warnings=0;
    for(size_t k=0;k<seg.ends.size();++k){const tvsDionicaSegment::End& e=seg.ends[k];if(e.other<0)continue;
        const tvsFlowNetwork::P& x=items[e.element].ports[e.port];const AcGePoint3d at(x.x,x.y,x.z);
        const AcDbObjectId ours=net.nodes[e.element].id,theirs=net.nodes[e.other].id;
        const bool oursFirst=e.port==0;
        int what=0;   // 0 prelaz in our duct, 1 prelaz takes the size, 2 prelaz in their duct, 3 TK-O / TK-G port
        {AcDbObjectPointer<AcDbEntity> o(theirs,AcDb::kForRead);
         if(o.openStatus()==Acad::eOk&&net.members[e.other].empty()){
            if(const TVS_TRANS* t=TVS_TRANS::cast(o.object())){
                // a prelaz aligned to one side moves its other end with a new width: a prelaz of our own instead
                if(t->TransType==2||t->TransType==3)acutPrintf(L"\nNa kraju dionice: prelaz poravnat uz jednu stranu ostaje; umeće se novi prelaz u kanal dionice.");
                else what=1;}
            else if(tvsSizeFollow::plainDuct(o.object())&&!net.nodes[e.other].flex)what=2;
            else if(const TVS_WYE* w=TVS_WYE::cast(o.object()))what=tvsTeePort::keeps(w)?3:0;}}
        if(what==1){AcDbObjectPointer<TVS_TRANS> t(theirs,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk){++warnings;continue;}
            t->assertWriteEnabled();
            if(e.otherPort==0){t->SizeAp1=A;t->SizeBp1=B;t->ThisRoundp1=q.round;}else{t->SizeAp2=A;t->SizeBp2=B;t->ThisRoundp2=q.round;}
            t->recordGraphicsModified();tvsSizeFollow::spiroLength(t.object(),e.otherPort,ours,f);++adjusted;written.insert(theirs);continue;}
        if(what==2){bool theirFirst=true;
            {AcDbObjectPointer<TVS_Pipe> d(theirs,AcDb::kForRead);if(d.openStatus()!=Acad::eOk){++warnings;continue;}
             theirFirst=tvsTeeFit::planDistance(d->FirstPoint,at)<=tvsTeeFit::planDistance(d->LastPoint,at);}
            if(tvsSizeFollow::prelazBehind(theirs,theirFirst,at,A,B,f,L"Na kraju dionice",q.round?1:0))++prelazi;else ++warnings;continue;}
        if(what==3){int port=-1;double best=0;
            {AcDbObjectPointer<TVS_WYE> w(theirs,AcDb::kForRead);if(w.openStatus()!=Acad::eOk){++warnings;continue;}
             const tvsTeeFit::Ports ps=tvsTeeFit::ports(w.object());
             if(ps.ok)for(int j=0;j<3;++j){const double dd=tvsTeeFit::planDistance(ps.p[j],at);if(port<0||dd<best){port=j;best=dd;}}}
            if(port>=0&&tvsTeePort::ensure(theirs,port))++adjusted;else ++warnings;continue;}
        if(tvsSizeFollow::prelazBehind(ours,oursFirst,at,oldA,oldB,f,L"Na kraju dionice",oldRound?1:0))++prelazi;else ++warnings;
    }
    // the size is now in the drawing: the "ručno" mark of these ducts goes
    for(size_t m=0;m<seg.members.size();++m)if(items[seg.members[m]].kind==tvsDionicaSegment::Duct&&manualOf(net.nodes[seg.members[m]].id).on)setManual(net.nodes[seg.members[m]].id,Manual());
    wchar_t from[32],to[32];sizeText(from,32,oldRound,oldA*f,oldB*f);sizeText(to,32,q.round,q.a,q.b);
    acutPrintf(L"\nPrimijenjeno %ls → %ls: kanala %d, koljena %d; prelaza umetnuto %d, spojeva prilagođeno %d.%ls",from,to,ducts,elbows,prelazi,adjusted,
               warnings?L" Neki spojevi nisu usklađeni (poruke iznad) – provjerite ih (TVS_PROVJERA); U vraća izmjenu.":L"");
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
