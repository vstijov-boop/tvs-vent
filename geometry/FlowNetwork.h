#pragma once
// TVS_PROTOK: air flows summed from the terminal elements to the unit. Pure graph, drawing units;
// tests/test_flow_network.cpp.
//   Elements with their ports (3D, as TVS_MOVE sees them: an etaz / obilazak is one element).
//   Ports closer than tol are one joint; a port on more than one other port is ambiguous.
//   The root is a free port of one element (the end at the unit). Walking away from it, an
//   element with nothing connected further on is a terminal: its own flow is kept. Every other
//   element gets the sum of the terminals behind it. No system type: supply and exhaust alike.
//   A loop, an ambiguous joint or a joined root stops the calculation (nothing is written).
#include <cmath>
#include <map>
#include <utility>
#include <vector>
namespace tvsFlowNetwork {
struct P {double x,y,z;P():x(0),y(0),z(0){}P(double a,double b,double c):x(a),y(b),z(c){}};
struct Element {std::vector<P> ports;double flow;Element():flow(0){}};
enum Error {Ok=0,BadRoot=1,RootJoined=2,Ambiguous=3,Cycle=4};
struct Result {
    Error error;
    int problem;                  // element at the problem (Ambiguous / Cycle), -1
    P where;                      // problem point
    std::vector<double> flow;     // per element (unreached: its own flow)
    std::vector<char> reached;
    std::vector<int> parent;      // element towards the unit, -1 for the root / unreached
    std::vector<int> entry;       // port of the element towards the unit (root: rootPort), -1 unreached
    std::vector<int> via;         // port of the parent this element is joined to, -1 root / unreached
    std::vector<int> order;       // reached elements, walking order from the unit
    std::vector<int> terminals;   // reached elements with nothing further on
    std::vector<std::pair<int,int> > open;   // free ports (element, port) of elements that go on
    double total;
    Result():error(Ok),problem(-1),total(0){}
};
inline double distance(const P& a,const P& b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
// contacts[i][p]: the (element, port) pairs within tol of port p of element i (other elements)
inline void contacts(const std::vector<Element>& e,double tol,std::vector<std::vector<std::vector<std::pair<int,int> > > >& out){
    out.assign(e.size(),std::vector<std::vector<std::pair<int,int> > >());
    const double cell=tol>0?tol*4:1e-6;
    std::map<std::pair<long long,long long>,std::vector<std::pair<int,int> > > grid;
    auto key=[&](const P& p){return std::make_pair(static_cast<long long>(std::floor(p.x/cell)),static_cast<long long>(std::floor(p.y/cell)));};
    for(size_t i=0;i<e.size();++i){out[i].resize(e[i].ports.size());
        for(size_t p=0;p<e[i].ports.size();++p)grid[key(e[i].ports[p])].push_back(std::make_pair(static_cast<int>(i),static_cast<int>(p)));}
    for(size_t i=0;i<e.size();++i)for(size_t p=0;p<e[i].ports.size();++p){
        const std::pair<long long,long long> k=key(e[i].ports[p]);
        for(long long dx=-1;dx<=1;++dx)for(long long dy=-1;dy<=1;++dy){
            auto g=grid.find(std::make_pair(k.first+dx,k.second+dy));if(g==grid.end())continue;
            for(size_t n=0;n<g->second.size();++n){const int j=g->second[n].first,q=g->second[n].second;
                if(j==static_cast<int>(i))continue;
                if(distance(e[i].ports[p],e[j].ports[q])<=tol)out[i][p].push_back(g->second[n]);}}
    }
}
inline Result solve(const std::vector<Element>& e,int root,int rootPort,double tol){
    Result r;const size_t n=e.size();
    r.flow.resize(n);for(size_t i=0;i<n;++i)r.flow[i]=e[i].flow;
    r.reached.assign(n,0);r.parent.assign(n,-1);r.entry.assign(n,-1);r.via.assign(n,-1);
    if(root<0||root>=static_cast<int>(n)||rootPort<0||rootPort>=static_cast<int>(e[root].ports.size())){r.error=BadRoot;return r;}
    std::vector<std::vector<std::vector<std::pair<int,int> > > > c;contacts(e,tol,c);
    if(!c[root][rootPort].empty()){r.error=RootJoined;r.problem=root;r.where=e[root].ports[rootPort];return r;}
    std::vector<int> entry(n,-1),order;std::vector<std::vector<int> > children(n);
    r.reached[root]=1;entry[root]=rootPort;order.push_back(root);
    for(size_t k=0;k<order.size();++k){const int i=order[k];
        for(size_t p=0;p<e[i].ports.size();++p){if(static_cast<int>(p)==entry[i])continue;
            const std::vector<std::pair<int,int> >& at=c[i][p];
            if(at.empty())continue;
            if(at.size()>1){r.error=Ambiguous;r.problem=i;r.where=e[i].ports[p];return r;}
            const int j=at[0].first,q=at[0].second;
            if(c[j][q].size()>1){r.error=Ambiguous;r.problem=j;r.where=e[j].ports[q];return r;}
            if(r.reached[j]){r.error=Cycle;r.problem=j;r.where=e[i].ports[p];return r;}
            r.reached[j]=1;entry[j]=q;r.via[j]=static_cast<int>(p);r.parent[j]=i;children[i].push_back(j);order.push_back(j);
        }
    }
    for(size_t k=order.size();k-->0;){const int i=order[k];
        if(children[i].empty()){r.terminals.push_back(i);continue;}
        double sum=0;for(size_t m=0;m<children[i].size();++m)sum+=r.flow[children[i][m]];
        r.flow[i]=sum;
        for(size_t p=0;p<e[i].ports.size();++p)if(static_cast<int>(p)!=entry[i]&&c[i][p].empty())r.open.push_back(std::make_pair(i,static_cast<int>(p)));
    }
    // terminals in walking order (nearest to the unit first)
    std::vector<int> t;for(size_t k=0;k<order.size();++k)if(children[order[k]].empty())t.push_back(order[k]);
    r.terminals=t;r.total=r.flow[root];r.order=order;
    for(size_t k=0;k<order.size();++k)r.entry[order[k]]=entry[order[k]];
    return r;
}
}
