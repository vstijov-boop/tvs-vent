#pragma once
// TVS_PROTOK "Primijeni": pure planner (design: Fable 5.2 review, 13.11.2026). Decides every edit
// from a snapshot; the ARX layer only executes the edits and verifies the result.
// Drawing units. Depends only on geometry/FlowNetwork.h (contacts). tests/test_dionica_plan.cpp.
//   Resized set R: from the fastest duct through plain ducts, plan elbows, resizable etazi and
//   removable (centred) prelazi of the same dionica and shape (scope SameSize: same size only).
//   Shrinking to a fixed point: an elbow / etaz needs a duct of R (or nothing) on every port,
//   because its ports move with the new size and only a duct can take that up; a duct must keep
//   minDuct after the port moves and the prelazi inserted in it. What leaves R keeps its size.
//   So every boundary of R is a duct end, and there:
//     prelaz              its end takes the size (removed when no step is left); an aligned
//                         one is left alone and gets a centred prelaz in front of it
//     T komad of this dionica (the dionica arrives on it)
//        TK-O / TK-G      its dialog after the write (TeeDialog); if cancelled: prelaz at the port
//        plain / stucna   main inlet: main takes the size, prelaz behind the prolaz, the branch
//                         duct follows the port (dA/2); branch inlet: branch takes the size, the
//                         main ducts follow. Not possible (no duct / prelaz to take it): the
//                         T komad stays and the prelaz goes in front of it (warning)
//     T komad of another dionica (the dionica starts on its outlet): prelaz at the port, in our duct
//     anything else of another size: prelaz in our duct
#include "FlowNetwork.h"
#include <cmath>
#include <set>
#include <vector>
namespace tvsDionicaPlan {
typedef tvsFlowNetwork::P P;
struct Section {double a,b;bool round;Section():a(0),b(0),round(false){}Section(double x,double y,bool r=false):a(x),b(r?0:y),round(r){}};
inline bool same(const Section& x,const Section& y){return x.round==y.round&&std::fabs(x.a-y.a)<1e-6&&(x.round||std::fabs(x.b-y.b)<1e-6);}
enum Kind {Other=0,Duct=1,Elbow=2,Prelaz=3,Tee=4,Assembly=5};
enum TeeType {Plain=0,Branch=1,Pants=2,Tap=3};
enum Radius {Fixed=0,Variable=1,Constant=2};   // inner radius: as drawn / k x A / k
struct Item {
    int kind,group;                 // group: dionica (tvsDuctSizing::dionice), -1 outside the network
    std::vector<P> ports;std::vector<Section> s;   // section at each port
    bool plain;                     // duct: rigid TVS duct; elbow: plan elbow that can be recomputed; etaz: can be rebuilt
    P middle;double sweep;int rule;double k;       // elbow: tangent intersection, rule resolved for the target shape
    bool aligned;                   // prelaz: a port would move with the requested change (side / height alignment)
    int tee;P base,u,v;double lengthPl,tapMin;     // T komad: type, TVS_WYE frame (unit u, v)
    std::vector<P> after;           // etaz: outer ports with the requested size (computed by the caller)
    Item():kind(Other),group(-1),plain(false),sweep(0),rule(Fixed),k(0),aligned(false),tee(Plain),lengthPl(0),tapMin(0){}
};
enum Scope {Whole=0,SameSize=1};
struct Request {int worst;Section size;int scope;double tol,minDuct;double (*prelazLength)(const Section&,const Section&);
    Request():worst(-1),scope(Whole),tol(0.1),minDuct(50),prelazLength(0){}};
enum Op {DuctSize=1,ElbowSize,AssemblySize,DuctEnd,TeeMain,TeeBranch,PrelazEnd,PrelazInsert,PrelazRemove,TeeDialog};
// DuctSize / ElbowSize / AssemblySize: item takes s.          DuctEnd: end `port` of duct `item` moves to `to`.
// TeeMain / TeeBranch: T komad `item` takes s on the main / branch.
// PrelazEnd: end `port` of prelaz `item` takes s.              PrelazRemove: prelaz `item` goes, duct `other` end `otherPort` -> `to`.
// PrelazInsert: in duct `item` at end `port`, from -> s, `length`; other >= 0: at port otherPort of T komad `other` (tvsTeePort::ensure).
// TeeDialog: T komad `item`, the dionica arrived on `port` with s; fallback: PrelazInsert at that port.
struct Edit {int op,item,port,other,otherPort;Section s,from;P to;double length;Edit():op(0),item(-1),port(-1),other(-1),otherPort(-1),length(0){}};
enum Level {Info=0,Warning=1,Block=2};
enum Code {Ambiguous=1,NotDuct,SameAlready,WorstTooShort,ElbowKept,DuctKept,TeeKept,AlignedPrelaz,UnknownNeighbour,PartLeft,DialogNoRoom};
struct Issue {int level,code,item;P at;};
struct Plan {bool ok;std::vector<int> resized;std::vector<Edit> edits;std::vector<Issue> issues;Plan():ok(false){}
    int count(int op)const{int n=0;for(size_t i=0;i<edits.size();++i)if(edits[i].op==op)++n;return n;}
    bool has(int code)const{for(size_t i=0;i<issues.size();++i)if(issues[i].code==code)return true;return false;}};
inline P add(const P& a,const P& d,double t){return P(a.x+d.x*t,a.y+d.y*t,a.z+d.z*t);}
inline double dist(const P& a,const P& b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));}
inline double axisRadius(const Item& e,double A,double now){return e.rule==Variable?e.k*A+A/2:e.rule==Constant?e.k+A/2:now+(A-e.s[0].a)/2;}
// Ports of a plan elbow with width A: on its legs, axisRadius x tan(sweep / 2) from the middle.
inline void elbowPorts(const Item& e,double A,P out[2]){
    const double t=std::tan(e.sweep/2),now=t>1e-12?dist(e.ports[0],e.middle)/t:0,want=axisRadius(e,A,now)*t;
    for(int k=0;k<2;++k){const double l=dist(e.ports[k],e.middle);out[k]=l>1e-12?add(e.middle,P((e.ports[k].x-e.middle.x)/l,(e.ports[k].y-e.middle.y)/l,0),want):e.ports[k];out[k].z=e.ports[k].z;}
}
// Ports of a plain T komad / stucna with main width Apr and branch width Aotv (TVS_WYE port model).
inline void teePorts(const Item& t,double Apr,double Aotv,P out[3]){
    const double l=t.tee==Tap?(0.25*Aotv>t.tapMin?0.25*Aotv:t.tapMin):t.lengthPl,half=l+Aotv/2;
    out[0]=add(t.base,t.u,-half);out[1]=add(t.base,t.u,half);out[2]=add(t.base,t.v,Apr/2+l);
    for(int k=0;k<3;++k)out[k].z=t.ports[k].z;
}
inline Plan plan(const std::vector<Item>& items,const Request& rq){
    Plan out;const int n=static_cast<int>(items.size());
    auto issue=[&](int level,int code,int item,const P& at){Issue e;e.level=level;e.code=code;e.item=item;e.at=at;out.issues.push_back(e);};
    if(rq.worst<0||rq.worst>=n||items[rq.worst].kind!=Duct||!items[rq.worst].plain||items[rq.worst].ports.size()!=2){issue(Block,NotDuct,rq.worst,P());return out;}
    const Item& W=items[rq.worst];const int g=W.group;const Section was=W.s[0],S=rq.size;
    if(same(was,S)){issue(Info,SameAlready,rq.worst,W.ports[0]);return out;}
    const bool reshape=was.round!=S.round;
    std::vector<tvsFlowNetwork::Element> el(n);for(int i=0;i<n;++i)el[i].ports=items[i].ports;
    std::vector<std::vector<std::vector<std::pair<int,int> > > > c;tvsFlowNetwork::contacts(el,rq.tol,c);
    for(int i=0;i<n;++i)if(items[i].group==g)for(size_t p=0;p<c[i].size();++p)if(c[i][p].size()>1){issue(Block,Ambiguous,i,items[i].ports[p]);return out;}
    auto next=[&](int i,int p,int& q){if(c[i][p].empty())return -1;q=c[i][p][0].second;return c[i][p][0].first;};
    auto L=[&](const Section& a,const Section& b){return rq.prelazLength?rq.prelazLength(a,b):300.0;};
    // candidates and the part connected with the fastest duct
    auto fits=[&](int i){const Item& t=items[i];if(t.group!=g||t.ports.size()!=2||t.s.size()<2)return false;
        if(t.kind==Prelaz)return rq.scope==Whole&&!reshape&&!t.aligned&&t.s[0].round==was.round&&t.s[1].round==was.round;
        if(!t.plain||(t.kind!=Duct&&t.kind!=Elbow&&t.kind!=Assembly))return false;
        if(t.kind==Assembly&&(reshape||t.after.size()!=2))return false;
        return rq.scope==SameSize?same(t.s[0],was):t.s[0].round==was.round;};
    std::vector<char> in(n,0);{std::vector<int> queue(1,rq.worst);in[rq.worst]=1;
        for(size_t k=0;k<queue.size();++k){const int i=queue[k];for(size_t p=0;p<items[i].ports.size();++p){int q=0;const int j=next(i,static_cast<int>(p),q);
            if(j>=0&&!in[j]&&fits(j)){in[j]=1;queue.push_back(j);}}}}
    std::set<int> keptTee;   // plain T komadi of this dionica that cannot follow
    for(int guard=0;guard<=3*n+3;++guard){
        std::vector<Edit> edits;std::vector<Issue> notes;std::vector<int> drop;
        auto note=[&](int level,int code,int item,const P& at){Issue e;e.level=level;e.code=code;e.item=item;e.at=at;notes.push_back(e);};
        auto ductIn=[&](int j){return j>=0&&in[j]&&items[j].kind==Duct;};
        // rigid members: every port on a duct of R or free
        for(int i=0;i<n;++i){if(!in[i]||items[i].kind==Duct)continue;
            bool ok=true;for(int p=0;p<2;++p){int q=0;const int j=next(i,p,q);if(j>=0&&!ductIn(j))ok=false;}
            if(!ok)drop.push_back(i);}
        if(!drop.empty()){for(size_t k=0;k<drop.size();++k){in[drop[k]]=0;if(items[drop[k]].kind==Elbow)issue(Info,ElbowKept,drop[k],items[drop[k]].middle);}continue;}
        // ducts: new ends, boundaries, room
        std::vector<Edit> ends,tees;std::set<int> teeDone;bool again=false;
        for(int i=0;i<n&&!again;++i){if(!ductIn(i))continue;const Item& d=items[i];
            P e[2]={d.ports[0],d.ports[1]};double need=0;std::vector<Edit> mine;
            for(int p=0;p<2;++p){int q=0;const int j=next(i,p,q);if(j<0)continue;const Item& t=items[j];
                auto insert=[&](const Section& from,int tee,int teePort){if(same(from,S))return;
                    Edit x;x.op=PrelazInsert;x.item=i;x.port=p;x.s=S;x.from=from;x.length=L(from,S);x.other=tee;x.otherPort=teePort;mine.push_back(x);need+=x.length;};
                if(in[j]){
                    if(t.kind==Elbow){P np[2];elbowPorts(t,S.a,np);e[p]=np[q];e[p].z=d.ports[p].z;}
                    else if(t.kind==Assembly){e[p]=t.after[q];e[p].z=d.ports[p].z;}
                    else if(t.kind==Prelaz){int q2=0;const int beyond=next(j,1-q,q2);   // removable: the duct with the lower index grows over it
                        if(beyond<0||i<beyond){e[p]=t.ports[1-q];e[p].z=d.ports[p].z;Edit x;x.op=PrelazRemove;x.item=j;x.other=i;x.otherPort=p;x.to=e[p];mine.push_back(x);}}
                    continue;}
                if(t.kind==Prelaz&&t.ports.size()==2&&t.s.size()>=2){
                    if(t.aligned){insert(t.s[q],-1,-1);note(Warning,AlignedPrelaz,j,d.ports[p]);continue;}
                    if(same(t.s[1-q],S)){e[p]=t.ports[1-q];e[p].z=d.ports[p].z;Edit x;x.op=PrelazRemove;x.item=j;x.other=i;x.otherPort=p;x.to=e[p];mine.push_back(x);continue;}
                    if(!same(t.s[q],S)){Edit x;x.op=PrelazEnd;x.item=j;x.port=q;x.s=S;mine.push_back(x);}
                    continue;}
                if(t.kind==Tee&&t.ports.size()==3&&t.s.size()>=3){
                    const bool own=t.group==g;
                    if(!own||reshape||keptTee.count(j)){insert(t.s[q],j,q);if(own&&keptTee.count(j))note(Warning,TeeKept,j,t.ports[q]);continue;}
                    if(t.tee==Branch||t.tee==Pants){Edit x;x.op=TeeDialog;x.item=j;x.port=q;x.s=S;x.other=i;x.otherPort=p;x.length=L(t.s[q],S);mine.push_back(x);
                        if(dist(d.ports[0],d.ports[1])<x.length+rq.minDuct)note(Warning,DialogNoRoom,j,t.ports[q]);
                        continue;}
                    // plain / stucna: it follows when every port that changes has a duct (or a centred prelaz behind the prolaz)
                    const bool main=q<2;P np[3];teePorts(t,main?S.a:t.s[0].a,main?t.s[2].a:S.a,np);
                    bool can=true;std::vector<Edit> follow;
                    for(int k=0;k<3&&can;++k){if(k==q)continue;int q2=0;const int m=next(j,k,q2);const bool moves=dist(np[k],t.ports[k])>1e-9;
                        const bool resize=main&&k<2;   // the prolaz takes the inlet size
                        if(m<0)continue;
                        const Item& o=items[m];
                        if(o.kind==Duct&&o.plain&&o.ports.size()==2&&!in[m]){
                            const double step=resize&&!same(o.s[q2],S)?L(S,o.s[q2]):0;
                            const P farEnd=o.ports[1-q2];const double len=dist(np[k],farEnd);
                            const double dot=(farEnd.x-np[k].x)*(farEnd.x-o.ports[q2].x)+(farEnd.y-np[k].y)*(farEnd.y-o.ports[q2].y);
                            if(len<step+rq.minDuct||dot<=0){can=false;break;}
                            if(moves){Edit x;x.op=DuctEnd;x.item=m;x.port=q2;x.to=np[k];x.to.z=o.ports[q2].z;follow.push_back(x);}
                            if(step>0){Edit x;x.op=PrelazInsert;x.item=m;x.port=q2;x.from=S;x.s=o.s[q2];x.length=step;x.other=j;x.otherPort=k;follow.push_back(x);}
                        }else if(o.kind==Prelaz&&!o.aligned&&!moves&&resize&&o.s.size()>=2){
                            if(!same(o.s[q2],S)){Edit x;x.op=PrelazEnd;x.item=m;x.port=q2;x.s=S;follow.push_back(x);}
                        }else if(moves||(resize&&!same(o.s[q2],S)))can=false;}
                    if(!can){keptTee.insert(j);again=true;break;}
                    Edit x;x.op=main?TeeMain:TeeBranch;x.item=j;x.port=q;x.s=S;mine.push_back(x);
                    for(size_t k=0;k<follow.size();++k)mine.push_back(follow[k]);
                    if(dist(np[q],t.ports[q])>1e-9){e[p]=np[q];e[p].z=d.ports[p].z;}
                    continue;}
                if(t.s.size()<=static_cast<size_t>(q)||!(t.s[q].a>0)){note(Warning,UnknownNeighbour,j,d.ports[p]);continue;}
                insert(t.s[q],-1,-1);
            }
            if(again)break;
            const double len=dist(e[0],e[1]);
            const double dot=(e[1].x-e[0].x)*(d.ports[1].x-d.ports[0].x)+(e[1].y-e[0].y)*(d.ports[1].y-d.ports[0].y);
            if(len<need+rq.minDuct||dot<=0){
                if(i==rq.worst){issue(Block,WorstTooShort,i,d.ports[0]);return out;}
                in[i]=0;issue(Warning,DuctKept,i,d.ports[0]);again=true;break;}
            Edit x;x.op=DuctSize;x.item=i;x.s=S;edits.push_back(x);
            for(int p=0;p<2;++p)if(dist(e[p],d.ports[p])>1e-9){bool removal=false;for(size_t k=0;k<mine.size();++k)if(mine[k].op==PrelazRemove&&mine[k].otherPort==p)removal=true;
                if(!removal){Edit y;y.op=DuctEnd;y.item=i;y.port=p;y.to=e[p];edits.push_back(y);}}
            for(size_t k=0;k<mine.size();++k)edits.push_back(mine[k]);
        }
        if(again)continue;
        for(int i=0;i<n;++i){if(!in[i]||items[i].kind==Duct||items[i].kind==Prelaz)continue;
            Edit x;x.op=items[i].kind==Elbow?ElbowSize:AssemblySize;x.item=i;x.s=S;edits.push_back(x);}
        // order for the executor: sizes of rigid members, T komadi, ducts and their ends, prelaz ends, removals, insertions, dialogs
        static const int order[]={ElbowSize,AssemblySize,TeeMain,TeeBranch,DuctSize,DuctEnd,PrelazEnd,PrelazRemove,PrelazInsert,TeeDialog};
        for(size_t o=0;o<sizeof(order)/sizeof(order[0]);++o)for(size_t k=0;k<edits.size();++k)if(edits[k].op==order[o])out.edits.push_back(edits[k]);
        for(size_t k=0;k<notes.size();++k)out.issues.push_back(notes[k]);
        for(int i=0;i<n;++i)if(in[i]&&items[i].kind!=Prelaz)out.resized.push_back(i);
        for(int i=0;i<n;++i)if(!in[i]&&items[i].group==g&&items[i].kind==Duct&&items[i].plain&&!same(items[i].s[0],S)){issue(Warning,PartLeft,i,items[i].ports[0]);break;}
        out.ok=true;return out;
    }
    issue(Block,WorstTooShort,rq.worst,W.ports[0]);return out;
}
}
