#pragma once
// TVS_PROVJERA: problems in a duct network that the network commands (TVS_MOVE / TVS_STRETCH /
// TVS_ALIGN / TVS_DIM_PRATI) refuse or that do not show in plan. Pure geometry, drawing units
// (f = mm per unit); tests/test_network_check.cpp.
//   Size      ports joined in plan (<= 0,1 mm), other size / shape (prelaz missing)
//   Level     ports joined in plan, same size, other axis level (drawn at another kota)
//   Miss      ports 0,1 .. 5 mm apart in plan (end that misses the port)
//   Middle    duct end on the middle of another duct (branch without T komad / split)
//   Angle     plan elbow a little off 15 / 22,5 / 30 / 45 / 60 / 75 / 90 deg (TVS_KOLJENA_UGAO)
//   Short     duct shorter than 10 mm
#include <cmath>
#include <map>
#include <utility>
#include <vector>
namespace tvsNetworkCheck {
enum Type {Size=1,Level=2,Miss=3,Middle=4,Angle=5,Short=6};
struct Section {double a,b,elevation;bool round,known;Section():a(0),b(0),elevation(0),round(false),known(false){}};
struct Item {
    int kind;                    // 1 duct, 2 elbow, 3 prelaz, 4 T komad, 0 other (flex ...)
    bool flex;
    std::vector<double> x,y;     // ports in plan
    std::vector<Section> s;      // section at each port (known = false: not compared)
    double angle;                // plan elbow: sweep in degrees, else < 0
    Item():kind(0),flex(false),angle(-1){}
};
struct Issue {int type,a,b;double x,y,value;};   // a, b: items (b = -1), value: mm or degrees
inline double standardAngle(double deg,double maxOff){
    static const double list[]={15,22.5,30,45,60,75,90};double best=-1,off=1e9;
    for(size_t i=0;i<sizeof(list)/sizeof(list[0]);++i){const double d=std::fabs(deg-list[i]);if(d<off){off=d;best=list[i];}}
    return off<=maxOff?best:-1;
}
inline void check(const std::vector<Item>& items,double f,std::vector<Issue>& out){
    out.clear();if(!(f>0))f=1;
    const double join=0.1/f,miss=5.0/f,cell=miss;
    // ports on a grid of 5 mm cells
    std::map<std::pair<long long,long long>,std::vector<std::pair<int,int> > > grid;
    auto key=[&](double x,double y){return std::make_pair(static_cast<long long>(std::floor(x/cell)),static_cast<long long>(std::floor(y/cell)));};
    for(size_t i=0;i<items.size();++i)for(size_t p=0;p<items[i].x.size();++p)grid[key(items[i].x[p],items[i].y[p])].push_back(std::make_pair(static_cast<int>(i),static_cast<int>(p)));
    std::vector<std::vector<char> > joined(items.size());for(size_t i=0;i<items.size();++i)joined[i].assign(items[i].x.size(),0);
    for(size_t i=0;i<items.size();++i)for(size_t p=0;p<items[i].x.size();++p){
        const double x=items[i].x[p],y=items[i].y[p];const std::pair<long long,long long> k=key(x,y);
        // other ports at this plan point (several: stacked ducts, not one joint)
        std::vector<std::pair<int,int> > at;std::vector<double> dist;
        for(long long dx=-1;dx<=1;++dx)for(long long dy=-1;dy<=1;++dy){
            auto g=grid.find(std::make_pair(k.first+dx,k.second+dy));if(g==grid.end())continue;
            for(size_t n=0;n<g->second.size();++n){const int j=g->second[n].first,q=g->second[n].second;if(j==static_cast<int>(i))continue;
                const double d=std::hypot(items[j].x[q]-x,items[j].y[q]-y);if(d<=miss){at.push_back(g->second[n]);dist.push_back(d);}}}
        int close=0;for(size_t n=0;n<at.size();++n)if(dist[n]<=join)++close;
        if(close)joined[i][p]=1;
        for(size_t n=0;n<at.size();++n){const int j=at[n].first,q=at[n].second;if(j<static_cast<int>(i))continue;   // each pair once
            const Section& a=items[i].s.size()>p?items[i].s[p]:Section();const Section& b=items[j].s.size()>static_cast<size_t>(q)?items[j].s[q]:Section();
            Issue e;e.a=static_cast<int>(i);e.b=j;e.x=x;e.y=y;
            if(dist[n]>join){if(close==0){e.type=Miss;e.value=dist[n]*f;out.push_back(e);}continue;}
            if(!a.known||!b.known||close>1)continue;   // one-to-one joints only
            const bool same=a.round==b.round&&std::fabs(a.a-b.a)<=join&&(a.round||std::fabs(a.b-b.b)<=join);
            if(!same){e.type=Size;e.value=0;out.push_back(e);}
            else if(std::fabs(a.elevation-b.elevation)>join){e.type=Level;e.value=(b.elevation-a.elevation)*f;out.push_back(e);}
        }
    }
    // free duct ends on the middle of another duct
    for(size_t i=0;i<items.size();++i){if(items[i].kind!=1&&!items[i].flex)continue;
        for(size_t p=0;p<items[i].x.size();++p){if(joined[i][p])continue;const double x=items[i].x[p],y=items[i].y[p];
            for(size_t j=0;j<items.size();++j){if(j==i||items[j].kind!=1||items[j].x.size()!=2)continue;
                const double ax=items[j].x[0],ay=items[j].y[0],bx=items[j].x[1],by=items[j].y[1],l2=(bx-ax)*(bx-ax)+(by-ay)*(by-ay);if(l2<1e-18)continue;
                const double t=((x-ax)*(bx-ax)+(y-ay)*(by-ay))/l2,l=std::sqrt(l2);if(t*l<miss||(1-t)*l<miss)continue;
                const double d=std::hypot(ax+(bx-ax)*t-x,ay+(by-ay)*t-y);
                const double half=items[j].s.empty()?0:items[j].s[0].a/2;
                if(d<=half+join){Issue e;e.type=Middle;e.a=static_cast<int>(i);e.b=static_cast<int>(j);e.x=x;e.y=y;e.value=0;out.push_back(e);break;}}}}
    for(size_t i=0;i<items.size();++i){const Item& it=items[i];
        if(it.kind==2&&it.angle>0){const double s=standardAngle(it.angle,0.5);
            if(s>0&&std::fabs(s-it.angle)>1e-6){Issue e;e.type=Angle;e.a=static_cast<int>(i);e.b=-1;e.x=it.x.empty()?0:it.x[0];e.y=it.y.empty()?0:it.y[0];e.value=it.angle;out.push_back(e);}}
        if(it.kind==1&&it.x.size()==2){const double l=std::hypot(it.x[1]-it.x[0],it.y[1]-it.y[0]);
            if(l*f<10&&!(it.s.size()==2&&it.s[0].known&&std::fabs(it.s[0].elevation-it.s[1].elevation)*f>=10)){Issue e;e.type=Short;e.a=static_cast<int>(i);e.b=-1;e.x=it.x[0];e.y=it.y[0];e.value=l*f;out.push_back(e);}}
    }
}
}
