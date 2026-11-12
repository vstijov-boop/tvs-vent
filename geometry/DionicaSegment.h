#pragma once
// TVS_PROTOK "Primijeni": the part of a dionica that takes the new size. Pure, drawing units;
// tests/test_dionica_segment.cpp.
//   From the chosen duct through joined plain ducts and plan elbows of the same size and the
//   same dionica. Elbows at the ends are left out again (their ports would move with the new
//   radius onto a neighbour that does not follow), so the part always ends with duct ends.
//   Every end of the part: free, or the one element and port joined there (that neighbour
//   decides what happens: prelaz takes the size, other size / T komad: a prelaz is inserted).
//   A port on more than one other port is ambiguous (reported, nothing is changed).
#include "FlowNetwork.h"
#include <vector>
namespace tvsDionicaSegment {
enum Kind {Other=0,Duct=1,Elbow=2};
struct Item {std::vector<tvsFlowNetwork::P> ports;int kind;int group;double a,b;bool round;Item():kind(Other),group(-1),a(0),b(0),round(false){}};
struct End {int element,port,other,otherPort;End():element(-1),port(-1),other(-1),otherPort(-1){}};   // other -1: free
struct Result {bool ok,ambiguous;std::vector<int> members;std::vector<End> ends;Result():ok(false),ambiguous(false){}};
inline bool sameSize(const Item& x,const Item& y){
    return x.round==y.round&&std::fabs(x.a-y.a)<1e-6&&(x.round||std::fabs(x.b-y.b)<1e-6);
}
inline Result find(const std::vector<Item>& items,int start,double tol){
    Result r;const int n=static_cast<int>(items.size());
    if(start<0||start>=n||items[start].kind!=Duct)return r;
    std::vector<tvsFlowNetwork::Element> el(n);
    for(int i=0;i<n;++i)el[i].ports=items[i].ports;
    std::vector<std::vector<std::vector<std::pair<int,int> > > > c;tvsFlowNetwork::contacts(el,tol,c);
    auto fits=[&](int j){const Item& t=items[j];return (t.kind==Duct||t.kind==Elbow)&&t.ports.size()==2&&t.group==items[start].group&&sameSize(t,items[start]);};
    std::vector<char> in(n,0);std::vector<int> queue(1,start);in[start]=1;
    for(size_t k=0;k<queue.size();++k){const int i=queue[k];
        for(size_t p=0;p<items[i].ports.size();++p){const std::vector<std::pair<int,int> >& at=c[i][p];
            if(at.size()>1){r.ambiguous=true;return r;}
            if(at.empty())continue;
            const int j=at[0].first;
            if(!in[j]&&fits(j)){in[j]=1;queue.push_back(j);}}}
    // elbows at the ends out (repeat: two elbows in a row)
    for(bool changed=true;changed;){changed=false;
        for(int i=0;i<n;++i){if(!in[i]||items[i].kind!=Elbow)continue;
            bool end=false;for(size_t p=0;p<items[i].ports.size();++p){const std::vector<std::pair<int,int> >& at=c[i][p];
                if(at.empty()||!in[at[0].first])end=true;}
            if(end){in[i]=0;changed=true;}}}
    if(!in[start])return r;
    for(int i=0;i<n;++i)if(in[i])r.members.push_back(i);
    for(size_t m=0;m<r.members.size();++m){const int i=r.members[m];
        for(size_t p=0;p<items[i].ports.size();++p){const std::vector<std::pair<int,int> >& at=c[i][p];
            if(!at.empty()&&in[at[0].first])continue;
            End e;e.element=i;e.port=static_cast<int>(p);if(!at.empty()){e.other=at[0].first;e.otherPort=at[0].second;}
            r.ends.push_back(e);}}
    r.ok=true;return r;
}
}
