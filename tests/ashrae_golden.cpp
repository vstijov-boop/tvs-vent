// Golden comparison of geometry/AshraeEngine.h with the DuctPressDrop engine (tools/ashrae_golden.py).
//   ashrae_golden <ashrae_data folder> <golden.txt>      (the ASHRAE data is not in this repository)
#include "../geometry/AshraeEngine.h"
#include <cstdio>
#include <fstream>
#include <iostream>
static std::vector<std::string> split(const std::string& s,char c){std::vector<std::string> v;std::string cur;for(size_t i=0;i<s.size();++i){if(s[i]==c){v.push_back(cur);cur.clear();}else cur+=s[i];}v.push_back(cur);return v;}
static bool same(double a,double b){if(std::isnan(a)&&std::isnan(b))return true;if(std::isinf(a)||std::isinf(b))return a==b;double m=std::fabs(a)>std::fabs(b)?std::fabs(a):std::fabs(b);return std::fabs(a-b)<=1e-9+1e-8*m;}
int main(int argc,char** argv){
    if(argc<3){std::cerr<<"usage: ashrae_golden data golden\n";return 2;}
    const std::string dir=argv[1];
    tvsAshrae::Reader rd=[dir](const std::string& f,std::string& t){std::ifstream in((dir+"/"+f).c_str(),std::ios::binary);if(!in)return false;std::ostringstream o;o<<in.rdbuf();t=o.str();return true;};
    tvsAshrae::Engine eng[2];for(int m=0;m<2;++m){if(!eng[m].load(rd)){std::cerr<<"no data\n";return 2;}}
    eng[1].clamp=false;eng[1].pv=false;
    std::ifstream g(argv[2]);std::string line;int n=0,bad=0;std::map<std::string,int> badKeys;
    while(std::getline(g,line)){if(line.empty())continue;const std::vector<std::string> f=split(line,'\t');if(f.size()<7)continue;++n;
        tvsAshrae::Engine& e=eng[f[0]=="clamp"?0:1];
        std::map<std::string,double> in;if(!f[3].empty())for(const std::string& kv:split(f[3],',')){const size_t q=kv.find('=');in[kv.substr(0,q)]=std::atof(kv.substr(q+1).c_str());}
        const tvsAshrae::Engine::Loss l=e.loss(f[1],in,f[2]=="main",nullptr);
        bool ok=same(l.c,std::atof(f[4].c_str()))&&same(l.dp,std::atof(f[5].c_str()));std::string why;
        if(!ok)why="C "+std::to_string(l.c)+" vs "+f[4]+" dP "+std::to_string(l.dp)+" vs "+f[5];
        const std::map<std::string,double> cv=e.calculate(f[1],in,nullptr);
        if(!f[6].empty())for(const std::string& kv:split(f[6],',')){const size_t q=kv.find('=');const std::string k=kv.substr(0,q);const double v=std::atof(kv.substr(q+1).c_str());
            auto i=cv.find(k);if(i==cv.end()||!same(i->second,v)){ok=false;if(why.size()<300)why+=" "+k+"="+(i==cv.end()?std::string("missing"):std::to_string(i->second))+" vs "+kv.substr(q+1);}}
        if(!ok){++bad;if(++badKeys[f[0]+" "+f[1]]<=1)std::cout<<"DIFF "<<f[0]<<" "<<f[1]<<" "<<f[2]<<" "<<f[3]<<" : "<<why<<"\n";}
    }
    std::cout<<n<<" cases, "<<bad<<" different, keys "<<badKeys.size()<<"\n";
    if(!bad)std::cout<<"OK\n";
    return bad?1:0;
}
