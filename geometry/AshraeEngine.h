#pragma once
// ASHRAE Duct Fitting Database engine: a C++ port of DuctPressDrop services/ashrae/engine.py
// (the user's program, itself a port of the ASHRAE calculator). Pure C++14, no AutoCAD.
//   Data (NOT in this repository): Constants.csv, Equation.csv, Output.csv (semicolon CSV) and
//   tables/<key>.TBL, read through a Reader callback (the folder chosen in TVS_SETTINGS).
//   calculate(key, inputs): Constants + inputs, then the Equation.csv lines of the key in Order;
//   Excel/BASIC syntax (^, [ ], SQR, ATN, IFLE0, fcn(x, y, z, table) ...). A line that fails
//   (unknown name, division by zero, sqrt of a negative) is skipped, like Python eval there.
//   fcn(): .TBL 1D / 2D / 3D linear interpolation as scipy RegularGridInterpolator / interp1d,
//   "clamp" (input outside the table -> edge value + warning) and "pv" (first Q/Qc interval of a
//   junction whose C is referenced to the branch Pv: interpolate C*x^2), the engine.py defaults.
//   loss(key, inputs, main): C and dP picked as engine.py (branch: Co Cb Cc Cs C / Po Pob Pos Poc;
//   main: Cs Cc Co / Pos Poc Po). SI inputs: mm, L/s, kg/m3. tests/test_ashrae_engine.cpp.
// No min / max / near / far / small identifiers (Windows macros).
#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
namespace tvsAshrae {
typedef std::function<bool(const std::string& file,std::string& content)> Reader;   // "Equation.csv", "tables/SR5-13.TBL"
inline double notNumber(){return std::numeric_limits<double>::quiet_NaN();}
inline bool word(char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||static_cast<unsigned char>(c)>=0x80;}
inline bool digit(char c){return c>='0'&&c<='9';}
inline bool space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\f'||c=='\v';}
inline std::string trim(const std::string& s){size_t a=0,b=s.size();while(a<b&&space(s[a]))++a;while(b>a&&space(s[b-1]))--b;return s.substr(a,b-a);}
// Python float(): [+-] digits [. digits] [e [+-] digits] (at least one digit), inf / nan
inline bool number(const std::string& t,double& v){
    std::string s=trim(t);if(s.empty())return false;
    size_t i=0;if(s[i]=='+'||s[i]=='-')++i;
    std::string rest=s.substr(i);for(size_t k=0;k<rest.size();++k)if(rest[k]>='A'&&rest[k]<='Z')rest[k]=static_cast<char>(rest[k]-'A'+'a');
    if(rest=="inf"||rest=="infinity"){v=s[0]=='-'?-HUGE_VAL:HUGE_VAL;return true;}
    if(rest=="nan"){v=notNumber();return true;}
    size_t d=0;while(i<s.size()&&digit(s[i])){++i;++d;}
    if(i<s.size()&&s[i]=='.'){++i;while(i<s.size()&&digit(s[i])){++i;++d;}}
    if(!d)return false;
    if(i<s.size()&&(s[i]=='e'||s[i]=='E')){++i;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;size_t e=0;while(i<s.size()&&digit(s[i])){++i;++e;}if(!e)return false;}
    if(i!=s.size())return false;
    std::istringstream in(s);in.imbue(std::locale::classic());in>>v;return !in.fail();
}
// ---- CSV (semicolon, header row, optional UTF-8 BOM, "quoted" fields) ----
typedef std::map<std::string,std::string> Row;
inline std::vector<std::string> fields(const std::string& line){
    std::vector<std::string> f;std::string cur;bool q=false;
    for(size_t i=0;i<line.size();++i){const char c=line[i];
        if(q){if(c=='"'){if(i+1<line.size()&&line[i+1]=='"'){cur+='"';++i;}else q=false;}else cur+=c;}
        else if(c=='"')q=true;else if(c==';'){f.push_back(cur);cur.clear();}else cur+=c;}
    f.push_back(cur);return f;
}
inline std::vector<std::string> lines(const std::string& text){
    std::vector<std::string> out;std::string cur;
    for(size_t i=0;i<text.size();++i){const char c=text[i];if(c=='\n'){if(!cur.empty()&&cur[cur.size()-1]=='\r')cur.erase(cur.size()-1);out.push_back(cur);cur.clear();}else cur+=c;}
    if(!cur.empty()){if(cur[cur.size()-1]=='\r')cur.erase(cur.size()-1);out.push_back(cur);}
    return out;
}
inline std::vector<Row> csv(std::string text){
    if(text.size()>=3&&static_cast<unsigned char>(text[0])==0xEF&&static_cast<unsigned char>(text[1])==0xBB&&static_cast<unsigned char>(text[2])==0xBF)text.erase(0,3);
    std::vector<Row> rows;const std::vector<std::string> l=lines(text);if(l.empty())return rows;
    const std::vector<std::string> head=fields(l[0]);
    for(size_t i=1;i<l.size();++i){if(l[i].empty())continue;const std::vector<std::string> f=fields(l[i]);Row r;
        for(size_t k=0;k<head.size()&&k<f.size();++k)r[head[k]]=f[k];rows.push_back(r);}
    return rows;
}
inline std::string field(const Row& r,const char* name){Row::const_iterator i=r.find(name);return i==r.end()?std::string():i->second;}
// ---- expressions (Python grammar subset: + - * / ** unary, calls, names, numbers) ----
enum Kind {Num,Var,Plus,Minus,Add,Sub,Mul,Div,Pow,Call};
struct Node {Kind kind;double value;std::string name;std::vector<int> args;Node():kind(Num),value(0){}};
struct Expr {std::vector<Node> nodes;int root;Expr():root(-1){}};
class Parser {
public:
    explicit Parser(const std::string& s):s_(s),i_(0){}
    bool parse(Expr& e){e=Expr();e_=&e;ok_=true;const int r=sum();ws();if(!ok_||i_!=s_.size()||r<0)return false;e.root=r;return true;}
private:
    std::string s_;size_t i_;Expr* e_;bool ok_;
    void ws(){while(i_<s_.size()&&space(s_[i_]))++i_;}
    bool at(const char* t){ws();size_t n=0;while(t[n])++n;if(s_.compare(i_,n,t)!=0)return false;return true;}
    int add(Node n){e_->nodes.push_back(n);return static_cast<int>(e_->nodes.size())-1;}
    int bin(Kind k,int a,int b){Node n;n.kind=k;n.args.push_back(a);n.args.push_back(b);return add(n);}
    int sum(){int a=product();
        while(ok_){if(at("+")){++i_;a=bin(Add,a,product());}else if(at("-")){++i_;a=bin(Sub,a,product());}else break;}
        return a;}
    int product(){int a=unary();
        while(ok_){if(at("**"))break;if(at("*")){++i_;a=bin(Mul,a,unary());}else if(at("/")){++i_;a=bin(Div,a,unary());}else break;}
        return a;}
    int unary(){if(at("+")){++i_;Node n;n.kind=Plus;n.args.push_back(unary());return add(n);}
        if(at("-")){++i_;Node n;n.kind=Minus;n.args.push_back(unary());return add(n);}
        return power();}
    int power(){const int a=atom();if(ok_&&at("**")){i_+=2;return bin(Pow,a,unary());}return a;}   // right: -x**2 = -(x**2), 2**-1
    int atom(){ws();if(i_>=s_.size()){ok_=false;return -1;}
        const char c=s_[i_];
        if(c=='('){++i_;const int a=sum();if(!at(")")){ok_=false;return -1;}++i_;return a;}
        if(digit(c)||(c=='.'&&i_+1<s_.size()&&digit(s_[i_+1]))){const size_t b=i_;
            while(i_<s_.size()&&digit(s_[i_]))++i_;if(i_<s_.size()&&s_[i_]=='.'){++i_;while(i_<s_.size()&&digit(s_[i_]))++i_;}
            if(i_<s_.size()&&(s_[i_]=='e'||s_[i_]=='E')){size_t j=i_+1;if(j<s_.size()&&(s_[j]=='+'||s_[j]=='-'))++j;if(j<s_.size()&&digit(s_[j])){i_=j;while(i_<s_.size()&&digit(s_[i_]))++i_;}}
            if(i_<s_.size()&&word(s_[i_])){ok_=false;return -1;}   // 1a: invalid literal
            Node n;n.kind=Num;if(!number(s_.substr(b,i_-b),n.value)){ok_=false;return -1;}return add(n);}
        if(word(c)&&!digit(c)){const size_t b=i_;while(i_<s_.size()&&word(s_[i_]))++i_;Node n;n.name=s_.substr(b,i_-b);
            if(at("(")){++i_;n.kind=Call;
                if(!at(")"))for(;;){n.args.push_back(sum());if(!ok_)return -1;if(at(",")){++i_;if(at(")"))break;continue;}break;}
                if(!at(")")){ok_=false;return -1;}++i_;return add(n);}
            n.kind=Var;return add(n);}
        ok_=false;return -1;}
};
// "Pvb=(Vb/VPC)^2" -> name, expression ('^' -> '**', '[ ]' -> '( )'); false: no '=' or no parse
inline bool statement(const std::string& raw,std::string& name,Expr& e){
    std::string q;for(size_t i=0;i<raw.size();++i){const char c=raw[i];if(c=='^')q+="**";else if(c=='[')q+='(';else if(c==']')q+=')';else q+=c;}
    const size_t eq=q.find('=');if(eq==std::string::npos)return false;
    name=trim(q.substr(0,eq));if(name.empty())return false;
    return Parser(q.substr(eq+1)).parse(e);
}
// DuctPressDrop _friction: Darcy-Weisbach (Swamee-Jain)
inline double friction(double e,double d,double re){
    if(re<=0||d<=0)return 0.02;if(re<2300)return 64.0/re;
    const double l=std::log10(e/(3.7*d)+5.74/std::pow(re,0.9));
    if(!std::isfinite(l)||l==0)return 0.02;return 0.25/(l*l);
}
// ---- .TBL ----
typedef std::vector<std::vector<double> > Rows;
inline bool pyNumberParen(const std::string& s,size_t p,size_t& groupEnd,size_t& end){
    // (-?\d+\.?\d*(?:[Ee][+-]?\d+)?)\s*\(\s*-?\d+\.?\d*\s*\)
    size_t i=p;if(i<s.size()&&s[i]=='-')++i;
    const size_t d0=i;while(i<s.size()&&digit(s[i]))++i;if(i==d0)return false;
    if(i<s.size()&&s[i]=='.')++i;while(i<s.size()&&digit(s[i]))++i;
    if(i<s.size()&&(s[i]=='e'||s[i]=='E')){size_t j=i+1;if(j<s.size()&&(s[j]=='+'||s[j]=='-'))++j;const size_t e0=j;while(j<s.size()&&digit(s[j]))++j;if(j>e0)i=j;}
    groupEnd=i;while(i<s.size()&&space(s[i]))++i;if(i>=s.size()||s[i]!='(')return false;++i;
    while(i<s.size()&&space(s[i]))++i;if(i<s.size()&&s[i]=='-')++i;
    const size_t n0=i;while(i<s.size()&&digit(s[i]))++i;if(i==n0)return false;
    if(i<s.size()&&s[i]=='.')++i;while(i<s.size()&&digit(s[i]))++i;
    while(i<s.size()&&space(s[i]))++i;if(i>=s.size()||s[i]!=')')return false;
    end=i+1;return true;
}
// the clean-ups of engine.py _parse_tbl_sections, in its order
inline std::string cleanLine(const std::string& s0){
    std::string s=s0,o;
    for(size_t p=0;p<s.size();){size_t g,e;if(pyNumberParen(s,p,g,e)){o+=s.substr(p,g-p);p=e;}else o+=s[p++];}   // 1.5(38) -> 1.5
    s.swap(o);o.clear();
    for(size_t p=0;p<s.size();){   // (?<!\w)-\s+(\d) -> -\1
        if(s[p]=='-'&&(p==0||!word(s[p-1]))){size_t j=p+1;while(j<s.size()&&space(s[j]))++j;if(j>p+1&&j<s.size()&&digit(s[j])){o+='-';o+=s[j];p=j+1;continue;}}
        o+=s[p++];}
    s.swap(o);o.clear();
    for(size_t p=0;p<s.size();){if(p+2<s.size()&&digit(s[p])&&s[p+1]=='-'&&digit(s[p+2])){o+=s[p];o+=" -";o+=s[p+2];p+=3;}else o+=s[p++];}   // (\d)(-\d)
    s.swap(o);o.clear();
    for(size_t p=0;p<s.size();){if(s[p]=='O'&&p+1<s.size()&&s[p+1]=='.'&&(p==0||!word(s[p-1]))){o+="0.";p+=2;}else o+=s[p++];}   // \bO\.
    s.swap(o);o.clear();
    for(size_t p=0;p<s.size();){if(s[p]=='.'&&p+1<s.size()&&s[p+1]=='O'&&(p+2>=s.size()||!word(s[p+2]))){o+=".0";p+=2;}else o+=s[p++];}   // \.O\b
    return o;
}
inline std::vector<Rows> sections(const std::string& text){
    std::vector<Rows> out;Rows cur;const std::vector<std::string> l=lines(text);
    for(size_t k=0;k<l.size();++k){const std::string s=trim(l[k]);
        if(s.empty()||s[0]=='\'')continue;
        bool eqLine=true,hashLine=true;
        for(size_t i=0;i<s.size();++i){if(s[i]!='='&&!space(s[i]))eqLine=false;if(s[i]!='#'&&!space(s[i]))hashLine=false;}
        if(eqLine){if(!cur.empty()){out.push_back(cur);cur.clear();}continue;}
        if(hashLine)continue;
        const std::string c=cleanLine(s);
        std::vector<double> nums;size_t tokens=0;std::istringstream in(c);std::string t;
        while(in>>t){++tokens;double v;if(number(t,v))nums.push_back(v);}
        if(nums.size()>=2&&static_cast<double>(nums.size())>=tokens*0.5)cur.push_back(nums);
    }
    if(!cur.empty())out.push_back(cur);
    if(out.empty())out.push_back(Rows());
    return out;
}
// scipy: strictly ascending or descending axis; index of the interval (searchsorted - 1, clipped)
inline bool monotonic(const std::vector<double>& g){
    if(g.size()<2)return false;const bool up=g[1]>g[0];
    for(size_t i=1;i<g.size();++i){if(up?!(g[i]>g[i-1]):!(g[i]<g[i-1]))return false;}
    return true;
}
struct Axis {std::vector<double> g;std::vector<size_t> order;};   // ascending copy + original index
inline bool axis(const std::vector<double>& raw,Axis& a){   // one point: that point only (scipy)
    a.g.clear();a.order.clear();if(raw.size()==1){a.g=raw;a.order.push_back(0);return true;}
    if(!monotonic(raw))return false;const bool up=raw[1]>raw[0];
    for(size_t i=0;i<raw.size();++i){const size_t k=up?i:raw.size()-1-i;a.g.push_back(raw[k]);a.order.push_back(k);}
    return true;
}
inline size_t interval(const std::vector<double>& g,double x){   // np.searchsorted(g, x) - 1 in [0, n-2]
    size_t lo=0,hi=g.size();while(lo<hi){const size_t m=(lo+hi)/2;if(g[m]<x)lo=m+1;else hi=m;}
    if(lo==0)return 0;size_t i=lo-1;if(i>g.size()-2)i=g.size()-2;return i;
}
// RegularGridInterpolator((y, x), z, linear, fill_value=None): bilinear, linear outside
inline bool grid2(const std::vector<double>& ys,const std::vector<double>& xs,const Rows& z,double ty,double tx,double& out){
    Axis ay,ax;if(!axis(ys,ay)||!axis(xs,ax))return false;
    const bool oy=ay.g.size()==1,ox=ax.g.size()==1;
    const size_t i=oy?0:interval(ay.g,ty),j=ox?0:interval(ax.g,tx),i1=oy?0:i+1,j1=ox?0:j+1;
    const double ny=oy?0:(ty-ay.g[i])/(ay.g[i1]-ay.g[i]),nx=ox?0:(tx-ax.g[j])/(ax.g[j1]-ax.g[j]);
    const double z00=z[ay.order[i]][ax.order[j]],z01=z[ay.order[i]][ax.order[j1]],z10=z[ay.order[i1]][ax.order[j]],z11=z[ay.order[i1]][ax.order[j1]];
    out=z00*(1-ny)*(1-nx)+z01*(1-ny)*nx+z10*ny*(1-nx)+z11*ny*nx;return true;
}
// interp1d(x, y, linear, fill_value="extrapolate"), x sorted first (stable)
inline bool line1(std::vector<double> x,std::vector<double> y,double t,double& out){
    if(x.size()<2||x.size()!=y.size())return false;
    for(size_t i=1;i<x.size();++i){const double xi=x[i],yi=y[i];size_t j=i;while(j>0&&x[j-1]>xi){x[j]=x[j-1];y[j]=y[j-1];--j;}x[j]=xi;y[j]=yi;}
    size_t lo=0,hi=x.size();while(lo<hi){const size_t m=(lo+hi)/2;if(x[m]<t)lo=m+1;else hi=m;}
    size_t k=lo;if(k<1)k=1;if(k>x.size()-1)k=x.size()-1;
    const double slope=(y[k]-y[k-1])/(x[k]-x[k-1]);out=slope*(t-x[k-1])+y[k-1];return true;
}
inline double largest(const std::vector<double>& v){double r=v[0];for(size_t i=1;i<v.size();++i)if(v[i]>r)r=v[i];return r;}
inline double smallest(const std::vector<double>& v){double r=v[0];for(size_t i=1;i<v.size();++i)if(v[i]<r)r=v[i];return r;}
// engine.py _singularno: C on the first x interval grows like 1/x^2
inline bool singular(const std::vector<double>& x,const Rows& z){
    if(x.size()<2||x[0]<=0||x[1]<=x[0])return false;
    std::vector<double> c0,c1;
    for(size_t r=0;r<z.size();++r){if(z[r].size()>1&&std::isfinite(z[r][0]))c0.push_back(std::fabs(z[r][0]));if(z[r].size()>1&&std::isfinite(z[r][1]))c1.push_back(std::fabs(z[r][1]));}
    if(c0.empty()||c1.empty())return false;
    double m1=largest(c1);if(m1<1e-9)m1=1e-9;
    return largest(c0)/m1*x[0]/x[1]>=3.0;
}
struct Param {std::string var,unit;double def,lo,hi;Param():def(0),lo(0),hi(0){}};   // Input.csv (SI)
inline bool isFlow(const Param& p){return p.unit.find("L/s")!=std::string::npos||(!p.var.empty()&&(p.var[0]=='Q'||p.var[0]=='q'));}
class Engine {
public:
    bool clamp,pv;   // engine.py EKSTRAPOLACIJA "clamp", INTERPOLACIJA "pv"
    Engine():clamp(true),pv(true),loaded_(false){}
    // false: Constants / Equation / Output missing
    bool load(const Reader& reader){
        reader_=reader;loaded_=false;constants_.clear();equations_.clear();outputs_.clear();params_.clear();tables_.clear();pvTables_.clear();
        std::string t;
        if(!reader_("Constants.csv",t))return false;
        {const std::vector<Row> rows=csv(t);for(size_t i=0;i<rows.size();++i){const std::string v=trim(field(rows[i],"Variable"));double x;if(!v.empty()&&number(field(rows[i],"SIValue"),x))constants_[v]=x;}}
        if(!reader_("Equation.csv",t))return false;
        {const std::vector<Row> rows=csv(t);std::map<std::string,std::vector<std::pair<double,std::string> > > by;
            for(size_t i=0;i<rows.size();++i){const std::string k=trim(field(rows[i],"Key"));if(k.empty())continue;double o;if(!number(field(rows[i],"Order"),o))o=0;by[k].push_back(std::make_pair(o,field(rows[i],"Equation")));}
            for(auto& kv:by){std::vector<std::pair<double,std::string> >& v=kv.second;
                for(size_t i=1;i<v.size();++i){std::pair<double,std::string> x=v[i];size_t j=i;while(j>0&&v[j-1].first>x.first){v[j]=v[j-1];--j;}v[j]=x;}   // stable, by Order
                equations_[kv.first]=v;}}
        if(!reader_("Output.csv",t))return false;
        {const std::vector<Row> rows=csv(t);std::map<std::string,std::vector<std::pair<double,std::string> > > by;
            for(size_t i=0;i<rows.size();++i){const std::string k=trim(field(rows[i],"Key"));if(k.empty())continue;double o;if(!number(field(rows[i],"Order"),o))o=0;by[k].push_back(std::make_pair(o,trim(field(rows[i],"Variable"))));}
            for(auto& kv:by){std::vector<std::pair<double,std::string> >& v=kv.second;
                for(size_t i=1;i<v.size();++i){std::pair<double,std::string> x=v[i];size_t j=i;while(j>0&&v[j-1].first>x.first){v[j]=v[j-1];--j;}v[j]=x;}
                std::vector<std::string>& o=outputs_[kv.first];for(size_t i=0;i<v.size();++i)if(!v[i].second.empty())o.push_back(v[i].second);}}
        if(reader_("Input.csv",t)){const std::vector<Row> rows=csv(t);std::map<std::string,std::vector<std::pair<double,Param> > > by;   // optional: defaults
            for(size_t i=0;i<rows.size();++i){const std::string k=trim(field(rows[i],"Key"));Param p;p.var=trim(field(rows[i],"Variable"));if(k.empty()||p.var.empty())continue;
                p.unit=field(rows[i],"SIUnitID");if(!number(field(rows[i],"SIDefault"),p.def))p.def=0;if(!number(field(rows[i],"SIMinimum"),p.lo))p.lo=0;if(!number(field(rows[i],"SIMaximum"),p.hi))p.hi=0;
                double o;if(!number(field(rows[i],"Order"),o))o=0;by[k].push_back(std::make_pair(o,p));}
            for(auto& kv:by){std::vector<std::pair<double,Param> >& v=kv.second;
                for(size_t i=1;i<v.size();++i){std::pair<double,Param> x=v[i];size_t j=i;while(j>0&&v[j-1].first>x.first){v[j]=v[j-1];--j;}v[j]=x;}
                std::vector<Param>& o=params_[kv.first];for(size_t i=0;i<v.size();++i)o.push_back(v[i].second);}}
        loaded_=true;return true;
    }
    const std::vector<Param>& params(const std::string& key)const{static const std::vector<Param> none;auto i=params_.find(key);return i==params_.end()?none:i->second;}
    bool loaded()const{return loaded_;}
    bool has(const std::string& key)const{return equations_.count(key)!=0;}
    const std::vector<std::string>& resultVars(const std::string& key)const{static const std::vector<std::string> none;auto i=outputs_.find(key);return i==outputs_.end()?none:i->second;}
    // engine.py get_z_from_tbl
    double table(const std::string& key,const std::vector<double>& lookup,int tableNum,std::vector<std::string>* warnings){
        const std::vector<Rows>* sec=tableOf(key);if(!sec)return 0.2;
        int idx=tableNum-1;if(idx>static_cast<int>(sec->size())-1)idx=static_cast<int>(sec->size())-1;if(idx<0)idx=0;
        const Rows& rows=(*sec)[idx];if(rows.empty())return 0.2;
        const bool pvOn=pv&&pvTable(key,tableNum);
        double tx=lookup.empty()?0.0:lookup[0];
        const bool hy=lookup.size()>1,hz=lookup.size()>2;double ty=hy?lookup[1]:0,tz=hz?lookup[2]:0;
        const std::vector<double>& xRow=rows[0];
        if(rows.size()<2)return xRow.empty()?0.2:xRow[0];
        const size_t nx=xRow.size();
        tx=check(key,tx,xRow,"x",warnings);
        double out=0;
        if(rows[1].size()>=nx+2&&hz){   // 3D: [z, y, data...] then [y, data...]
            std::map<double,std::map<double,std::vector<double> > > groups;bool haveZ=false;double curZ=0;
            for(size_t k=1;k<rows.size();++k){const std::vector<double>& r=rows[k];double y;std::vector<double> co;
                if(r.size()>=nx+2){curZ=r[0];haveZ=true;y=r[1];co.assign(r.begin()+2,r.begin()+2+nx);}
                else if(r.size()>=nx+1&&haveZ){y=r[0];co.assign(r.begin()+1,r.begin()+1+nx);}
                else continue;
                groups[curZ][y]=co;}
            if(groups.empty())return 0.2;
            std::vector<double> zs;for(auto& g:groups)zs.push_back(g.first);
            tz=check(key,tz,zs,"z",warnings);
            std::vector<double> vals;
            for(auto& g:groups){std::vector<double> ys;Rows m;for(auto& y:g.second){ys.push_back(y.first);m.push_back(y.second);}
                const double yt=hy?check(key,ty,ys,"y",warnings):ys[0];
                double div=1;Rows zm=m;pvScale(pvOn,xRow,zm,m,tx,div);
                double v;if(!grid2(ys,xRow,zm,yt,tx,v))return 0.15;vals.push_back(v/div);}
            if(!line1(zs,vals,tz,out))return 0.15;
            return out;
        }
        if(nx<rows[1].size()){   // 2D: [y, data...]
            std::vector<double> ys;Rows z;
            for(size_t k=1;k<rows.size();++k){const std::vector<double>& r=rows[k];if(r.size()<nx+1)return 0.15;ys.push_back(r[0]);z.push_back(std::vector<double>(r.begin()+1,r.begin()+1+nx));}
            double div=1;Rows zm=z;
            if(!hy){ty=ys[0];pvScale(pvOn,xRow,zm,Rows(1,z[0]),tx,div);}
            else{ty=check(key,ty,ys,"y",warnings);pvScale(pvOn,xRow,zm,z,tx,div);}
            if(!grid2(ys,xRow,zm,ty,tx,out))return 0.15;
            return out/div;
        }
        // 1D: the first data row
        const std::vector<double>& zr=rows[1];const size_t n=nx<zr.size()?nx:zr.size();
        std::vector<double> xs(xRow.begin(),xRow.begin()+n);Rows zm(1,std::vector<double>(zr.begin(),zr.begin()+n));
        double div=1;pvScale(pvOn,xs,zm,Rows(1,zm[0]),tx,div);
        if(!line1(xs,zm[0],tx,out))return 0.15;
        return out/div;
    }
    // engine.py calculate: every variable (constants, inputs, results)
    std::map<std::string,double> calculate(const std::string& key,const std::map<std::string,double>& inputs,std::vector<std::string>* warnings){
        std::map<std::string,double> cv=constants_;
        for(auto& kv:inputs)cv[kv.first]=kv.second;
        if(cv.count("Q1")&&cv.count("A1")&&cv["A1"]!=0)cv["V1"]=cv["Q1"]/(3600*cv["A1"]);
        auto it=equations_.find(key);if(it==equations_.end())return cv;
        for(size_t i=0;i<it->second.size();++i){std::string name;Expr e;
            if(!statement(it->second[i].second,name,e))continue;
            double v;if(eval(key,e,e.root,cv,warnings,v))cv[name]=v;}
        return cv;
    }
    struct Loss {double c,dp;Loss():c(0),dp(0){}};
    // engine.py loss: main = the straight run (Cs Cc Co), else branch / single path (Co Cb Cc Cs C)
    Loss loss(const std::string& key,const std::map<std::string,double>& inputs,bool main,std::vector<std::string>* warnings){
        std::vector<std::string> w;const std::map<std::string,double> cv=calculate(key,inputs,&w);
        Loss r;
        if(main){r.c=pick(key,cv,{"Cs","Cc","Co"},"C");r.dp=pick(key,cv,{"Pos","Poc","Po"},"Po");}
        else{r.c=pick(key,cv,{"Co","Cb","Cc","Cs","C"},"C");r.dp=pick(key,cv,{"Po","Pob","Pos","Poc"},"Po");}
        const std::string x=xiWarning(key,cv,main);if(!x.empty())w.push_back(x);
        if(warnings)warnings->insert(warnings->end(),w.begin(),w.end());
        return r;
    }
private:
    Reader reader_;bool loaded_;
    std::map<std::string,double> constants_;
    std::map<std::string,std::vector<std::pair<double,std::string> > > equations_;
    std::map<std::string,std::vector<std::string> > outputs_;
    std::map<std::string,std::vector<Param> > params_;
    std::map<std::string,std::vector<Rows> > tables_;std::set<std::string> missing_;
    std::map<std::string,std::set<int> > pvTables_;
    const std::vector<Rows>* tableOf(const std::string& key){
        auto i=tables_.find(key);if(i!=tables_.end())return &i->second;if(missing_.count(key))return nullptr;
        std::string t;if(!reader_("tables/"+key+".TBL",t)&&!reader_("tables/"+key+".tbl",t)){missing_.insert(key);return nullptr;}
        return &(tables_[key]=sections(t));
    }
    static std::string fmt(double v){std::ostringstream o;o.imbue(std::locale::classic());o.precision(6);o<<v;return o.str();}
    double check(const std::string& key,double val,const std::vector<double>& arr,const char* axisName,std::vector<std::string>* warnings)const{
        if(arr.empty())return val;
        const double lo=smallest(arr),hi=largest(arr);double span=std::fabs(hi-lo);if(span<1)span=1;const double tol=1e-6*span;
        if(val<lo-tol||val>hi+tol){if(warnings){const std::string head=key+": "+axisName+"=";bool seen=false;
                for(size_t i=0;i<warnings->size();++i)if((*warnings)[i].compare(0,head.size(),head)==0)seen=true;
                if(!seen)warnings->push_back(head+fmt(val)+" van tabli\xC4\x8Dnog opsega ["+fmt(lo)+", "+fmt(hi)+"]"+(clamp?" \xE2\x80\x93 uzeta grani\xC4\x8Dna vrijednost":" \xE2\x80\x93 ekstrapolacija"));}}
        if(clamp){if(val<lo)return lo;if(val>hi)return hi;}
        return val;
    }
    // engine.py _pv: C*x^2 on the first x interval of a branch-Pv junction table
    void pvScale(bool on,const std::vector<double>& x,Rows& zm,const Rows& chk,double tx,double& div)const{
        div=1;if(!on||x.size()<2||!(x[0]>0)||!(x[0]<tx&&tx<x[1])||!singular(x,chk))return;
        for(size_t r=0;r<zm.size();++r)for(size_t c=0;c<zm[r].size()&&c<x.size();++c)zm[r][c]*=x[c]*x[c];
        div=tx*tx;
    }
    // engine.py _pv_tabele, for one key
    bool pvTable(const std::string& key,int tno){
        auto f=pvTables_.find(key);if(f==pvTables_.end()){f=pvTables_.insert(std::make_pair(key,pvSet(key))).first;}
        return f->second.count(tno)!=0;
    }
    std::set<int> pvSet(const std::string& key)const{
        std::set<int> res;auto it=equations_.find(key);if(it==equations_.end())return res;
        std::vector<std::pair<std::string,Expr> > st;
        for(size_t i=0;i<it->second.size();++i){std::string n;Expr e;if(statement(it->second[i].second,n,e))st.push_back(std::make_pair(n,e));}
        auto nameOf=[](const Expr& e,int k)->std::string{return k>=0&&e.nodes[k].kind==Var?e.nodes[k].name:std::string();};
        std::map<std::string,std::string> alias,refs;
        for(size_t s=0;s<st.size();++s){const Expr& e=st[s].second;const Node& n=e.nodes[e.root];
            if(n.kind==Div){const std::string a=nameOf(e,n.args[0]),b=nameOf(e,n.args[1]);if(!a.empty()&&b=="Qc"&&a[0]=='Q')alias[st[s].first]=a;}}
        for(size_t s=0;s<st.size();++s){const Expr& e=st[s].second;const Node& n=e.nodes[e.root];const std::string& name=st[s].first;
            if(name.compare(0,2,"Po")==0&&n.kind==Mul){const std::string a=nameOf(e,n.args[0]),b=nameOf(e,n.args[1]);
                if(!a.empty()&&!b.empty()&&a[0]=='C'&&b.compare(0,2,"Pv")==0)refs[a]=b.substr(2);}}
        std::map<int,int> uses,good;
        for(size_t s=0;s<st.size();++s){const Expr& e=st[s].second;const std::string& name=st[s].first;
            for(size_t k=0;k<e.nodes.size();++k){const Node& n=e.nodes[k];
                if(n.kind!=Call||n.name!="fcn"||n.args.size()<2)continue;
                const Node& last=e.nodes[n.args[n.args.size()-1]];if(last.kind!=Num)continue;
                const int tno=static_cast<int>(std::nearbyint(last.value));++uses[tno];
                std::string q;const Node& a=e.nodes[n.args[0]];
                if(a.kind==Div){const std::string l=nameOf(e,a.args[0]),r=nameOf(e,a.args[1]);if(!l.empty()&&r=="Qc")q=l;}
                else if(a.kind==Var&&alias.count(a.name))q=alias[a.name];
                if(!q.empty()&&q[0]=='Q'&&!name.empty()&&name[0]=='C'){auto rf=refs.find(name);if(rf!=refs.end()&&rf->second==q.substr(1))++good[tno];}}}
        for(auto& g:good)if(g.second==uses[g.first])res.insert(g.first);
        return res;
    }
    // NaN = a Python complex (negative ** fraction): kept by arithmetic, refused by functions
    bool eval(const std::string& key,const Expr& e,int k,const std::map<std::string,double>& cv,std::vector<std::string>* warnings,double& out){
        const Node& n=e.nodes[k];double a=0,b=0;
        switch(n.kind){
        case Num:out=n.value;return true;
        case Var:{auto i=cv.find(n.name);if(i!=cv.end()){out=i->second;return true;}
            if(n.name=="PI"||n.name=="pi"){out=3.14159265358979323846;return true;}return false;}
        case Plus:return eval(key,e,n.args[0],cv,warnings,out);
        case Minus:if(!eval(key,e,n.args[0],cv,warnings,a))return false;out=-a;return true;
        case Add:case Sub:case Mul:case Div:case Pow:
            if(!eval(key,e,n.args[0],cv,warnings,a)||!eval(key,e,n.args[1],cv,warnings,b))return false;
            if(n.kind==Add)out=a+b;else if(n.kind==Sub)out=a-b;else if(n.kind==Mul)out=a*b;
            else if(n.kind==Div){if(b==0)return false;out=a/b;}
            else{if(a==0&&b<0)return false;
                if(a<0&&b!=std::floor(b)){out=notNumber();return true;}
                out=std::pow(a,b);if(std::isinf(out)&&std::isfinite(a)&&std::isfinite(b))return false;}
            return true;
        case Call:{std::vector<double> v;
            for(size_t i=0;i<n.args.size();++i){double x;if(!eval(key,e,n.args[i],cv,warnings,x))return false;v.push_back(x);}
            const std::string& f=n.name;
            if(f=="fcn"){if(v.empty())return false;for(size_t i=0;i<v.size();++i)if(std::isnan(v[i]))return false;
                const int tno=static_cast<int>(std::nearbyint(v[v.size()-1]));
                out=table(key,std::vector<double>(v.begin(),v.end()-1),tno,warnings);return true;}
            if(f=="IFIP"){if(v.size()!=2)return false;out=v[1];return true;}
            if(f=="IFLE0"||f=="IFLT0"){if(v.size()!=3||std::isnan(v[0]))return false;out=(f=="IFLE0"?v[0]<=0:v[0]<0)?v[1]:v[2];return true;}
            if(f=="MAX"||f=="MIN"){if(v.size()<2)return false;for(size_t i=0;i<v.size();++i)if(std::isnan(v[i]))return false;out=f=="MAX"?largest(v):smallest(v);return true;}
            if(f=="ABS"||f=="Abs"){if(v.size()!=1)return false;out=std::fabs(v[0]);return true;}
            if(f=="FRICTION"){if(v.size()!=3)return false;for(size_t i=0;i<3;++i)if(std::isnan(v[i]))return false;out=friction(v[0],v[1],v[2]);return true;}
            if(v.size()!=1||std::isnan(v[0]))return false;const double x=v[0];
            if(f=="ATN"||f=="Atn"){out=std::atan(x);return true;}
            if(f=="TAN"||f=="Tan"||f=="tan"){if(std::isinf(x))return false;out=std::tan(x);return true;}
            if(f=="SIN"||f=="Sin"){if(std::isinf(x))return false;out=std::sin(x);return true;}
            if(f=="COS"||f=="Cos"){if(std::isinf(x))return false;out=std::cos(x);return true;}
            if(f=="SQR"||f=="sqr"||f=="SQRT"){if(x<0)return false;out=std::sqrt(x);return true;}
            if(f=="LOG"){if(x<=0)return false;out=std::log10(x);return true;}
            if(f=="LN"){if(x<=0)return false;out=std::log(x);return true;}
            if(f=="EXP"){out=std::exp(x);return std::isfinite(out)||std::isinf(x);}
            if(f=="INT"){if(!std::isfinite(x))return false;out=std::trunc(x);return true;}
            return false;}
        }
        return false;
    }
    double pick(const std::string& key,const std::map<std::string,double>& cv,std::initializer_list<const char*> prefs,const char* prefix)const{
        for(const char* p:prefs){auto i=cv.find(p);if(i!=cv.end()&&!std::isnan(i->second))return i->second;}
        const std::vector<std::string>& o=resultVars(key);const std::string pre(prefix);
        for(size_t k=0;k<o.size();++k)if(o[k].compare(0,pre.size(),pre)==0){auto i=cv.find(o[k]);if(i!=cv.end()&&!std::isnan(i->second))return i->second;}
        return 0;
    }
    // engine.py _xi_upozorenje: elbows, transitions, junctions with C outside [-1, 50]
    std::string xiWarning(const std::string& key,const std::map<std::string,double>& cv,bool main)const{
        if(key.size()<4)return std::string();
        const char a=key[0],b=key[1];if(!(a=='S'||a=='C'||a=='E')||!(b=='D'||b=='R'||b=='F'))return std::string();
        size_t i=2;while(i<key.size()&&digit(key[i]))++i;if(i==2||i>=key.size()||key[i]!='-')return std::string();
        const std::string g=key.substr(2,i-2);if(g!="3"&&g!="4"&&g!="5")return std::string();
        const std::vector<std::string> names=main?std::vector<std::string>{"Ccs","Cs","Cc","Co"}:std::vector<std::string>{"Co","Ccb","Cb","Cc","C"};
        for(size_t k=0;k<names.size();++k){auto f=cv.find(names[k]);if(f==cv.end()||!std::isfinite(f->second))continue;
            if(f->second<-1.0||f->second>50.0)return key+": "+names[k]+" = "+fmt(f->second)+" van razumnog opsega [-1, 50] \xE2\x80\x93 sumnjiv koeficijent, provjeri ulaze";
            return std::string();}
        return std::string();
    }
};
}
