// geometry/DpdExport.h: the TVS_PAD network as a DuctPressDrop system file.
//   test_dpd_export [supply.json exhaust.json]: also writes the sample files (test_dpd_export.py
//   loads them with DuctPressDrop's own importer when it is available).
#include "../geometry/DpdExport.h"
#include <clocale>
#include <cstdio>
#include <fstream>
using namespace tvsPressure;
static int fails=0;
static void eq(double a,double b,const char* w,double tol=1e-9){if(!(std::fabs(a-b)<=tol)){std::printf("FAIL %s: %.9g vs %.9g\n",w,a,b);++fails;}}
static void yes(bool c,const char* w){if(!c){std::printf("FAIL %s\n",w);++fails;}}
static void same(const std::string& a,const std::string& b,const char* w){if(a!=b){std::printf("FAIL %s: [%s] vs [%s]\n",w,a.c_str(),b.c_str());++fails;}}
static double dpdLen(XY p,XY q){return std::hypot(q.x-p.x,q.y-p.y);}
static Part duct(double a,double b,bool round,XY p,XY q,bool flex=false){Part t;t.kind=Duct;t.flex=flex;t.length=dpdLen(p,q);t.sections.assign(2,Section(a,b,round));t.line.push_back(p);t.line.push_back(q);return t;}
static Element one(const Part& t,double flow,int parent,int entry,int via,std::vector<XY> at){Element e;e.parts.push_back(t);e.ports=t.sections;e.at=at;e.flow=flow;e.parent=parent;e.entry=entry;e.via=via;return e;}
// the network of test_pressure_drop.cpp, with the T komad drawn as TVS_PAD does (line through its
// base point): unit -> duct 600x300 -> elbow 90 -> duct -> TK-O (prolaz 500x300, ogranak 300x300)
//   prolaz: duct -> prelaz 500x300 -> 400x300 -> duct (terminal); ogranak: duct 300x300 -> flex
static std::vector<Element> network(){
    std::vector<Element> e;
    e.push_back(one(duct(600,300,false,XY(0,0),XY(5000,0)),3600,-1,0,-1,{XY(0,0),XY(5000,0)}));
    Part el;el.kind=Elbow;el.angle=90;el.radius=600;el.sections.assign(2,Section(600,300,false));el.line={XY(5000,0),XY(5600,0),XY(5600,600)};
    e.push_back(one(el,3600,0,0,1,{XY(5000,0),XY(5600,600)}));
    e.push_back(one(duct(600,300,false,XY(5600,600),XY(5600,4600)),3600,1,0,1,{XY(5600,600),XY(5600,4600)}));
    Part t;t.kind=Tee;t.tee=TeeBranch;t.sections={Section(600,300,false),Section(500,300,false),Section(300,300,false)};
    t.line={XY(5600,4600),XY(5600,5000),XY(5600,5400),XY(5600,5000),XY(5900,5000)};
    e.push_back(one(t,3600,2,0,1,{XY(5600,4600),XY(5600,5400),XY(5900,5000)}));
    e.push_back(one(duct(500,300,false,XY(5600,5400),XY(5600,8400)),2400,3,0,1,{XY(5600,5400),XY(5600,8400)}));
    Part tr;tr.kind=Transition;tr.length=300;tr.sections={Section(500,300,false),Section(400,300,false)};tr.line={XY(5600,8400),XY(5600,8700)};
    e.push_back(one(tr,2400,4,0,1,{XY(5600,8400),XY(5600,8700)}));
    e.push_back(one(duct(400,300,false,XY(5600,8700),XY(5600,10700)),2400,5,0,1,{XY(5600,8700),XY(5600,10700)}));
    e.push_back(one(duct(300,300,false,XY(5900,5000),XY(8400,5000)),1200,3,0,2,{XY(5900,5000),XY(8400,5000)}));
    e.push_back(one(duct(250,0,true,XY(8400,5000),XY(9900,5000),true),1200,7,0,1,{XY(8400,5000),XY(9900,5000)}));
    return e;
}
static bool has(const std::string& s,const std::string& w){return s.find(w)!=std::string::npos;}
static void write(const char* file,const std::string& text){std::ofstream f(file,std::ios::binary);f<<text;}
int main(int argc,char** argv){
    // numbers: '.', trailing zeros, -0, not finite, also under a decimal-comma locale
    same(tvsDpd::number(1.5,1),"1.5","1.5");same(tvsDpd::number(3600,1),"3600","3600");same(tvsDpd::number(-0.00001,4),"0","-0");
    same(tvsDpd::number(2.80004,4),"2.8","2.8");same(tvsDpd::number(-5.6,4),"-5.6","-5.6");same(tvsDpd::number(std::nan(""),2),"0","nan");
    if(std::setlocale(LC_NUMERIC,"de_DE.UTF-8")||std::setlocale(LC_NUMERIC,"de_DE")){same(tvsDpd::number(1.25,2),"1.25","comma locale");std::setlocale(LC_NUMERIC,"C");}
    // strings
    same(tvsDpd::escape("a\"b\\c\nd\x01"),"a\\\"b\\\\c\\nd\\u0001","escape");
    same(tvsDpd::utf8(L"žica č"),"\xc5\xbe" "ica \xc4\x8d","utf8 2 bytes");same(tvsDpd::utf8(L"–"),"\xe2\x80\x93","utf8 3 bytes");
    {std::wstring w;w+=static_cast<wchar_t>(0xD83D);w+=static_cast<wchar_t>(0xDE00);same(tvsDpd::utf8(w),"\xf0\x9f\x98\x80","utf8 surrogate pair");}
    // the network
    const std::vector<Element> net=network();
    Options o;Calculator calc(nullptr,o);const Result r=calc.run(net,0);
    yes(r.d.size()==5,"5 TVS dionice");
    std::vector<std::wstring> warn;
    const std::vector<tvsDpd::Segment> s=tvsDpd::segments(net,r,false,&warn);
    yes(warn.empty(),"no warnings");
    yes(s.size()==6,"6 pieces (the first dionica split at the elbow)");
    if(s.size()==6){
        same(s[0].id,"D-01","id");same(s[0].from,"J","J");same(s[0].to,"KO1","corner node");same(s[1].from,"KO1","corner");same(s[1].to,"T1","T1");
        eq(s[0].dx,5.6,"J-KO1 dx");eq(s[0].dy,0,"J-KO1 dy");eq(s[1].dx,0,"KO1-T1 dx");eq(s[1].dy,5.0,"KO1-T1 dy: T node on the run axis");
        same(s[0].opis,"TVS dionica 1 (1/2)","opis split");same(s[2].opis,"TVS dionica 2","opis");
        same(s[2].from,"T1","prolaz from T1");same(s[2].to,"P1","prolaz to P1");eq(s[2].dy,3.7,"prolaz up to the end of the prelaz");eq(s[2].q,2400,"Q prolaz");
        same(s[3].from,"T1","ogranak from T1");eq(s[3].dx,2.8,"ogranak from the run axis");eq(s[3].dy,0,"ogranak perpendicular");eq(s[3].q,1200,"Q ogranak");
        eq(s[3].w,300,"W");eq(s[3].h,300,"H");yes(!s[3].round&&!s[3].flex,"rigid rect");
        same(s[4].from,"P1","after prelaz");same(s[4].to,"K1","terminal");eq(s[4].w,400,"W after prelaz");eq(s[4].dy,2.0,"terminal length");
        yes(s[5].round&&s[5].flex,"flex round");eq(s[5].d,250,"D");eq(s[5].length(),1.5,"flex length");eq(s[0].q,3600,"Q at the unit");
        for(size_t k=0;k<s.size();++k)eq(s[k].dz,0,"plan only");}
    tvsDpd::System sys;sys.name="Dovod \xc5\xbe";sys.project="crte\xc5\xbe.dwg";sys.source="TVS_VENT TVS_DPD_IZVOZ";sys.segments=s;
    const std::string j=tvsDpd::json(sys);
    yes(has(j,"\"naziv\": \"Dovod \xc5\xbe\""),"naziv");yes(has(j,"\"sistem\": \"supply\""),"supply");yes(has(j,"\"fitinzi\": []"),"no fittings");
    yes(has(j,"\"smjer\": [5.6, 0, 0]"),"smjer");yes(has(j,"\"Q\": 3600, \"W\": 600, \"H\": 300, \"D\": 0, \"L\": 5.6"),"segment fields");
    yes(has(j,"\"tip\": \"K\"")&&has(j,"\"D\": 250"),"round");yes(has(j,"Pocinkovani \\u010deli\\u010dni lim")&&has(j,"Fleksibilni izolovani"),"materials");
    yes(!has(j,",\n  ]")&&!has(j,"},]"),"no trailing comma");
    // exhaust: same orientation (from the unit), other ids and kind
    tvsDpd::System ex=sys;ex.exhaust=true;ex.name="Odvod";ex.segments=tvsDpd::segments(net,r,true);
    const std::string k=tvsDpd::json(ex);yes(has(k,"\"id\": \"O-01\"")&&has(k,"\"sistem\": \"extract\"")&&!has(k,"supply"),"exhaust");
    // empty system is still valid JSON
    yes(has(tvsDpd::json(tvsDpd::System()),"\"dionice\": []"),"empty");
    // a dionica without length (flex of 0 mm between two ducts): dropped, its end node joined to its start
    {std::vector<Element> z;
     z.push_back(one(duct(300,200,false,XY(0,0),XY(1000,0)),500,-1,0,-1,{XY(0,0),XY(1000,0)}));
     z.push_back(one(duct(300,200,false,XY(1000,0),XY(1000,0),true),500,0,0,1,{XY(1000,0),XY(1000,0)}));
     z.push_back(one(duct(300,200,false,XY(1000,0),XY(3000,0)),500,1,0,1,{XY(1000,0),XY(3000,0)}));
     const Result rz=calc.run(z,0);std::vector<std::wstring> w;const std::vector<tvsDpd::Segment> sz=tvsDpd::segments(z,rz,false,&w);
     yes(rz.d.size()==3,"3 dionice (rigid / flex / rigid)");yes(sz.size()==2&&w.size()==1,"zero length dropped with a warning");
     if(sz.size()==2){same(sz[1].from,sz[0].to,"joined node");eq(sz[1].dx,2.0,"last piece");}}
    // a bend below 1 degree is straight on; a 45 degree elbow is a vertex
    {std::vector<Element> b;
     b.push_back(one(duct(300,200,false,XY(0,0),XY(1000,0)),500,-1,0,-1,{XY(0,0),XY(1000,0)}));
     Part el;el.kind=Elbow;el.angle=45;el.sections.assign(2,Section(300,200,false));el.line={XY(1000,0),XY(1200,0),XY(1341.42,141.42)};
     b.push_back(one(el,500,0,0,1,{XY(1000,0),XY(1341.42,141.42)}));
     b.push_back(one(duct(300,200,false,XY(1341.42,141.42),XY(2048.53,848.53)),500,1,0,1,{XY(1341.42,141.42),XY(2048.53,848.53)}));
     const Result rb=calc.run(b,0);const std::vector<tvsDpd::Segment> sb=tvsDpd::segments(b,rb,false);
     yes(sb.size()==2,"45 deg: two pieces");if(sb.size()==2){eq(sb[0].dx,1.2,"to the vertex");eq(sb[1].dx,0.84853,"45 deg dx",1e-6);eq(sb[1].dy,0.84853,"45 deg dy",1e-6);}}
    // oprema: unit duct 400x200 -> damper RP-01 (Nominal 30 Pa at 1800) -> duct -> elbow 90 -> duct ->
    // silencer PB-01 (FixedDp 25) -> duct -> diffuser D-01 (Zeta 2.5, terminal, qTerminal 900)
    std::string jo;
    {auto equip=[](int kind,int ports,int model,double v1,double v2,const wchar_t* tag,std::vector<XY> at){Part t;t.kind=Equipment;
         t.equipment.kind=kind;t.equipment.ports=ports;t.equipment.model=model;t.equipment.tag=tag;t.equipment.a=400;t.equipment.b=200;
         if(model==tvsEquipment::Nominal){t.equipment.dpNominal=v1;t.equipment.qNominal=v2;}else if(model==tvsEquipment::FixedDp)t.equipment.dp=v1;else t.equipment.zeta=v1;
         t.sections.assign(ports,Section(400,200,false));t.line=at;return t;};
     std::vector<Element> q;
     q.push_back(one(duct(400,200,false,XY(0,0),XY(1000,0)),900,-1,0,-1,{XY(0,0),XY(1000,0)}));
     q.push_back(one(equip(tvsEquipment::FlowDamper,2,tvsEquipment::Nominal,30,1800,L"RP-01",{XY(1000,0),XY(1400,0)}),900,0,0,1,{XY(1000,0),XY(1400,0)}));
     q.push_back(one(duct(400,200,false,XY(1400,0),XY(4000,0)),900,1,0,1,{XY(1400,0),XY(4000,0)}));
     Part el;el.kind=Elbow;el.angle=90;el.radius=400;el.sections.assign(2,Section(400,200,false));el.line={XY(4000,0),XY(4400,0),XY(4400,400)};
     q.push_back(one(el,900,2,0,1,{XY(4000,0),XY(4400,400)}));
     q.push_back(one(duct(400,200,false,XY(4400,400),XY(4400,2000)),900,3,0,1,{XY(4400,400),XY(4400,2000)}));
     q.push_back(one(equip(tvsEquipment::Silencer,2,tvsEquipment::FixedDp,25,0,L"PB-01",{XY(4400,2000),XY(4400,3000)}),900,4,0,1,{XY(4400,2000),XY(4400,3000)}));
     q.push_back(one(duct(400,200,false,XY(4400,3000),XY(4400,4400)),900,5,0,1,{XY(4400,3000),XY(4400,4400)}));
     Part df=equip(tvsEquipment::Diffuser,1,tvsEquipment::Zeta,2.5,0,L"D-01",{XY(4400,4400)});df.equipment.qTerminal=900;
     q.push_back(one(df,900,6,0,1,{XY(4400,4400)}));
     const Result rq=calc.run(q,0);std::vector<std::wstring> w;const std::vector<tvsDpd::Segment> sq=tvsDpd::segments(q,rq,false,&w);
     yes(rq.d.size()==1,"oprema: one TVS dionica");yes(sq.size()==2,"two pieces (elbow)");
     yes(w.size()==1&&w[0].find(L"RP-01")!=std::wstring::npos,"Nominal: warning (fixed in DuctPressDrop)");
     if(sq.size()==2){
        yes(sq[0].equipment.size()==1&&sq[1].equipment.size()==2,"oprema on its pieces");
        if(sq[0].equipment.size()==1){const tvsDpd::Equip& x=sq[0].equipment[0];same(x.tip,"zaluzina","damper tip");same(x.fn,"dp_fix","dp_fix");eq(x.value,7.5,"Nominal at Q");eq(x.pos,1200.0/4400,"damper pos",1e-12);yes(!x.terminal,"inline");same(x.oznaka,"RP-01","tag");}
        if(sq[1].equipment.size()==2){const tvsDpd::Equip& x=sq[1].equipment[0];const tvsDpd::Equip& y=sq[1].equipment[1];
            same(x.tip,"prigusivac","silencer");eq(x.value,25,"FixedDp");eq(x.pos,2500.0/4400,"silencer pos",1e-12);
            same(y.tip,"difuzor","diffuser");same(y.fn,"konst","zeta: catalogue entry");eq(y.value,2.5,"zeta");eq(y.pos,1,"terminal at the free end");yes(y.terminal,"terminal");}}
     tvsDpd::System so;so.name="Oprema";so.segments=sq;jo=tvsDpd::json(so);
     yes(has(jo,"\"fitinzi\": [{\"naziv\": \"Ru\\u010dni unos \\u0394P [Pa]\", \"fn\": \"dp_fix\", \"kolicina\": 1, \"param_vals\": [7.5], \"terminal\": false, \"oprema\": {\"tip\": \"zaluzina\", \"pos\": 0.2727, \"oznaka\": \"RP-01\"}}]"),"fitting JSON");
     // untagged: prefix-nn
     q[1].parts[0].equipment.tag.clear();const std::vector<tvsDpd::Segment> su=tvsDpd::segments(q,rq,false);
     if(su.size()==2&&su[0].equipment.size()==1)same(su[0].equipment[0].oznaka,"RP-01","generated tag");}
    // resetka na dnu: duct 400x200 -> tap junction (FlowCommand.h splitBottom) -> grille R-01 (neck 300x100,
    // FixedDp 15 Pa, 400 m3/h) as a short vertical branch; closed end beyond it (b) / the duct goes on to R-02 (c)
    std::string jb,jc;
    for(int on=0;on<2;++on){std::vector<Element> n;
        n.push_back(one(duct(400,200,false,XY(0,0),XY(2919.8,0)),on?700:400,-1,0,-1,{XY(0,0),XY(2919.8,0)}));
        Part tp;tp.kind=BottomTap;tp.sections={Section(400,200,false),Section(400,200,false),Section(300,100,false)};if(!on)tp.length=80;   // closed end 80 mm (DpdExportCommand.h)
        n.push_back(one(tp,on?700:400,0,0,1,{XY(2919.8,0),XY(2920.2,0),XY(2920,0)}));
        Part g;g.kind=Equipment;g.equipment.kind=tvsEquipment::Grille;g.equipment.ports=1;g.equipment.mount=tvsEquipment::Bottom;g.equipment.model=tvsEquipment::FixedDp;
        g.equipment.dp=15;g.equipment.tag=L"R-01";g.equipment.a=400;g.equipment.b=200;g.equipment.neckA=300;g.equipment.neckB=100;g.sections.assign(1,Section(300,100,false));
        n.push_back(one(g,400,1,0,2,{XY(2920,0)}));
        if(on){n.push_back(one(duct(400,200,false,XY(2920.2,0),XY(5000,0)),300,1,0,1,{XY(2920.2,0),XY(5000,0)}));
            Part h=g;h.equipment.mount=tvsEquipment::AtEnd;h.equipment.tag=L"R-02";h.equipment.dp=10;h.sections.assign(1,Section(400,200,false));
            n.push_back(one(h,300,3,0,1,{XY(5000,0)}));}
        const Result rn=calc.run(n,0);std::vector<std::wstring> w;const std::vector<tvsDpd::Segment> sn=tvsDpd::segments(n,rn,false,&w);
        yes(w.size()==(on?0u:1u),"bottom: warning for the closed end only");yes(sn.size()==3,"bottom: pieces");
        if(sn.size()<3)continue;
        same(sn[0].to,"T1","bottom: node on the duct");eq(sn[0].dx,2.92,"bottom: node at the tap point",1e-9);eq(sn[0].q,on?700:400,"bottom: Q before");
        const tvsDpd::Segment& b=sn[on?1:2];
        same(b.from,"T1","bottom: branch from the node");eq(b.dx,0,"branch dx");eq(b.dy,0,"branch dy");eq(b.dz,-0.1,"branch down 0.10 m");
        eq(b.w,300,"branch W = neck");eq(b.h,100,"branch H = neck");eq(b.q,400,"branch Q = grille");
        yes(b.equipment.size()==1&&b.equipment[0].terminal&&b.equipment[0].pos==1&&b.equipment[0].oznaka=="R-01"&&b.equipment[0].tip=="resetka"&&b.equipment[0].value==15,"bottom: grille at the free end");
        if(on){eq(sn[2].dx,2.08,"bottom on: the duct on",1e-9);same(sn[2].from,"T1","bottom on: from the node");eq(sn[2].q,300,"bottom on: Q");}
        else{const tvsDpd::Segment& c=sn[1];same(c.from,"T1","closed end from the node");same(c.to,"Z1","closed end node");eq(c.dx,0.08,"closed end as drawn",1e-12);eq(c.q,0,"closed end Q = 0");yes(c.equipment.empty()&&c.w==400&&c.h==200,"closed end: the duct");}
        tvsDpd::System sb;sb.name=on?"Resetka na dnu - nastavak":"Resetka na dnu";sb.segments=sn;(on?jc:jb)=tvsDpd::json(sb);}
    yes(has(jb,"\"smjer\": [0, 0, -0.1]"),"bottom: smjer JSON");
    if(argc>=3){write(argv[1],j);write(argv[2],k);}
    if(argc>=4)write(argv[3],jo);
    if(argc>=6){write(argv[4],jb);write(argv[5],jc);}
    if(fails){std::printf("%d FAIL\n",fails);return 1;}
    std::printf("OK\n");return 0;
}
