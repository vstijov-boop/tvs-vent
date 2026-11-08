#pragma once
// TVS_TK_TIP dialog (IDD_TEETYPE), layout and sketch after TVS_TK_TIP_mockup.html: type of the T komad
// (TK-O, TK-G, stucna, TK0) and Okreni; per port (Ogranak B / Prolaz C, TK-G Ogranak C):
//   Na T komadu          shape (stucna / TK0 branch), A, B (TK-O / TK-G: B = inlet height, locked);
//   Odmah iza prikljucka elbow 30 / 45 / 60 / 90 deg, Desno / Lijevo (plan) or Gore / Dolje (vertical);
//   Kanal u nastavku     shape and size of the connected duct, order elbow -> prelaz or prelaz -> elbow,
//                        prelaz length (spiro: catalogue, read only).
// TK-O / TK-G keep their sizes; a duct of another size or shape on a port gets a prelaz at the port
// (TeePort.h). An elbow needs a free port or a duct whose far end is free (that duct is carried
// round the elbow); Gore / Dolje needs a free port. U redu = one command, one UNDO step.
// Also opened by Svojstva (TVS_CHANGE / double click) on one T komad.
#include "TeePort.h"
#include "TeeCommand.h"
#include "../TVS_Ventilation_DBX/SpiroCatalog.h"
#include "../TVS_Ventilation_DBX/TVS_TAP.h"
#include "../geometry/RoundElbow.h"
#include "ElbowDirection.h"
#include "Settings.h"
#include "adui.h"
#include <cmath>
#include <initializer_list>
#include <utility>
#include <vector>
namespace tvsTeeDialog {
using tvsTeePort::Section;
enum Choice {Branch=0,Pants=1,Tap=2,Plain=3};
enum Turn {Right=0,Left=1,Up=2,Down=3};
inline int teeType(int c){return c==Branch?tvsTee::Branch:c==Pants?tvsTee::Pants:c==Tap?tvsTee::Tap:tvsTee::Straight;}
inline int choiceOf(int t){return t==tvsTee::Branch?Branch:t==tvsTee::Pants?Pants:t==tvsTee::Tap?Tap:Plain;}
inline double pi(){return 3.14159265358979323846;}
struct Source {
    AcDbObjectId id;double f,Apr,Bpr,Aotv,Botv,second;bool roundPr,roundOtv;int type;AcGePoint3d port[3];
    Source():f(1),Apr(0),Bpr(0),Aotv(0),Botv(0),second(0),roundPr(false),roundOtv(false),type(0){}
};
struct Side {
    bool used;int k;Section tee;bool lockB,shapeFree;
    int neighbour;AcDbObjectId ductId;Section duct,ductWas;bool farFree;double ductLength;
    int elbow,turn,order;double lengthMm;   // elbow deg (0 none), Turn, order 0 elbow->prelaz / 1 prelaz->elbow
    Side():used(false),k(2),lockB(false),shapeFree(false),neighbour(0),farFree(false),ductLength(0),elbow(0),turn(Right),order(0),lengthMm(0){}
};
inline bool readSource(const AcDbObjectId& id,Source& s){
    AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return false;
    s.id=id;s.f=tvsUnits::factor(w.object());s.Apr=w->SizeApr;s.Bpr=w->SizeBpr;s.Aotv=w->SizeAotv;s.Botv=w->SizeBotv;
    s.roundPr=w->ThisRoundpr;s.roundOtv=w->ThisRoundotv;
    tvsTeeData::Record r;if(tvsTeeData::read(w.object(),r)){s.type=r.type;s.second=r.second;}
    const tvsTeeFit::Ports ps=tvsTeeFit::ports(w.object());if(!ps.ok)return false;
    for(int k=0;k<3;++k)s.port[k]=ps.p[k];
    return true;
}
// Far end of a duct is free: no other TVS port there.
inline bool farEndFree(const TVS_WYE* w,const AcDbObjectId& ductId,int nearEnd,double tol,double& length){
    AcGePoint3d farPt;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return false;
     farPt=nearEnd==0?d->LastPoint:d->FirstPoint;AcGeVector3d v=d->LastPoint-d->FirstPoint;v.z=0;length=v.length();}
    const tvsTeePort::Neighbour n=tvsTeePort::at(w,farPt,tol);
    return n.kind==1&&n.id==ductId;   // only the duct itself
}

class Dialog : public CAdUiBaseDialog {
public:
    enum {IdType=1301,IdFlip=1302,IdTabB=1304,IdTabC=1305,IdTeeShape=1306,IdTeeA=1307,IdTeeB=1308,IdTeeALabel=1319,IdTeeBLabel=1320,
          IdElbow=1321,IdTurn=1322,IdDuctShape=1310,IdDuctALabel=1311,IdDuctA=1312,IdDuctBLabel=1313,IdDuctB=1314,
          IdOrder=1323,IdLength=1315,IdSketch=1317,IdDescription=1318,IdSizes=1324,IdStatus=1309};
    Source src;int choice;bool flipped;Side side[2];int current;bool loading,valid;CString problem;
    Dialog(const Source& s,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),src(s),choice(choiceOf(s.type)),flipped(false),current(0),loading(false),valid(false){}

    // ---------------------------------------------------------------- model
    bool rectTee()const{return !src.roundPr&&src.Bpr>0;}
    Section inlet()const{Section s;
        if(choice==Pants){s.a=src.Aotv;s.round=src.roundOtv;s.b=s.round?0:src.Botv;}
        else{s.a=src.Apr;s.round=src.roundPr;s.b=s.round?0:src.Bpr;}return s;}
    int place(int k)const{return flipped&&k<2?1-k:k;}
    void neighbourOf(Side& d)const{
        d.neighbour=0;d.ductId=AcDbObjectId::kNull;d.farFree=false;d.ductLength=0;
        AcDbObjectPointer<TVS_WYE> w(src.id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return;
        const double tol=2.0/src.f;const tvsTeePort::Neighbour n=tvsTeePort::at(w.object(),src.port[place(d.k)],tol);
        if(n.kind==1){d.neighbour=1;d.ductId=n.id;d.duct=d.ductWas=n.s;d.farFree=farEndFree(w.object(),n.id,n.end,tol,d.ductLength);return;}
        if(n.kind==2){d.neighbour=2;d.ductId=n.id;w.close();
            AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)return;
            const int e=1-n.end;d.duct.a=e==0?t->SizeAp1:t->SizeAp2;d.duct.round=e==0?t->ThisRoundp1:t->ThisRoundp2;
            d.duct.b=d.duct.round?0:(e==0?t->SizeBp1:t->SizeBp2);d.ductWas=d.duct;return;}
        d.neighbour=n.kind==0?0:3;
    }
    void build(){
        const Section in=inlet();Side keep[2]={side[0],side[1]};
        for(int i=0;i<2;++i)side[i]=Side();
        Side& b=side[0];Side& c=side[1];
        if(choice==Branch){
            b.used=true;b.k=2;b.tee.a=src.Aotv;b.tee.b=in.b;b.lockB=true;
            c.used=true;c.k=1;c.tee.a=src.type==tvsTee::Branch&&src.second>0?src.second:src.Apr;c.tee.b=in.b;c.lockB=true;
        }else if(choice==Pants){
            b.used=true;b.k=1;b.tee.a=src.Apr;b.tee.b=in.b;b.lockB=true;
            c.used=true;c.k=0;c.tee.a=src.type==tvsTee::Pants&&src.second>0?src.second:src.Apr;c.tee.b=in.b;c.lockB=true;
        }else{
            b.used=true;b.k=2;b.tee.a=src.Aotv;b.tee.round=src.roundOtv;b.tee.b=src.roundOtv?0:src.Botv;b.shapeFree=true;
        }
        for(int i=0;i<2;++i)if(side[i].used){neighbourOf(side[i]);side[i].lengthMm=tvsTeeFit::transitionMm();
            side[i].elbow=keep[i].elbow;side[i].turn=keep[i].turn;side[i].order=keep[i].order;}
        if(current>0&&!side[1].used)current=0;
    }
    bool hasDuct(const Side& d)const{return d.neighbour==1||d.neighbour==2;}
    bool needsPrelaz(const Side& d)const{return hasDuct(d)&&!tvsTeePort::same(d.tee,d.duct,1e-6);}
    bool spiro(const Side& d)const{return d.tee.round&&d.duct.round&&std::fabs(d.tee.a-d.duct.a)>1e-9&&tvsSpiro::read(NULL).catalog;}
    double prelazMm(const Side& d)const{
        if(spiro(d))return tvsSpiro::drawnMm(d.tee.a*src.f,d.duct.a*src.f,tvsSpiro::read(NULL));
        return d.lengthMm>0?d.lengthMm:tvsTeeFit::transitionMm();}
    const wchar_t* tabName(int i)const{return i==0?L"Ogranak B":choice==Pants?L"Ogranak C":L"Prolaz C";}

    // ---------------------------------------------------------------- fields
    static bool number(CWnd* w,double& v){
        if(!w)return false;CString t;w->GetWindowText(t);t.Trim();t.Replace(L',',L'.');if(t.IsEmpty())return false;
        wchar_t* end=NULL;v=wcstod(t,&end);return end&&!*end&&v>0&&v<1e7;}
    void setNumber(int id,double mm){CString t;t.Format(L"%.0f",mm);SetDlgItemText(id,t);}
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    void show(int id,bool on){if(CWnd* w=GetDlgItem(id))w->ShowWindow(on?SW_SHOW:SW_HIDE);}
    int sel(int id)const{const CComboBox* c=static_cast<const CComboBox*>(GetDlgItem(id));return c?c->GetCurSel():-1;}
    void setSel(int id,int i){if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(id)))c->SetCurSel(i);}
    static int elbowIndex(int deg){return deg==30?1:deg==45?2:deg==60?3:deg==90?4:0;}
    static int elbowDeg(int index){static const int d[5]={0,30,45,60,90};return index>=0&&index<5?d[index]:0;}
    void load(){
        loading=true;const Side& d=side[current];const double f=src.f;
        SetDlgItemText(IdTabC,tabName(1));show(IdTabC,side[1].used);CheckRadioButton(IdTabB,IdTabC,current==0?IdTabB:IdTabC);
        setSel(IdTeeShape,d.tee.round?1:0);enable(IdTeeShape,d.shapeFree&&rectTee());
        SetDlgItemText(IdTeeALabel,d.tee.round?L"Ø [mm]":L"A [mm]");setNumber(IdTeeA,d.tee.a*f);
        if(d.tee.round)SetDlgItemText(IdTeeB,L"");else setNumber(IdTeeB,d.tee.b*f);
        enable(IdTeeB,!d.tee.round&&!d.lockB);
        setSel(IdElbow,elbowIndex(d.elbow));setSel(IdTurn,d.turn);
        const bool duct=d.neighbour==1;
        setSel(IdDuctShape,d.duct.round?1:0);enable(IdDuctShape,duct);enable(IdDuctA,duct);
        SetDlgItemText(IdDuctALabel,d.duct.round?L"Ø [mm]":L"A [mm]");
        if(hasDuct(d)){setNumber(IdDuctA,d.duct.a*f);if(d.duct.round)SetDlgItemText(IdDuctB,L"");else setNumber(IdDuctB,d.duct.b*f);}
        else{SetDlgItemText(IdDuctA,L"");SetDlgItemText(IdDuctB,L"");}
        enable(IdDuctB,duct&&!d.duct.round);
        setSel(IdOrder,d.order);
        loading=false;refresh();
    }
    void readFields(){
        Side& d=side[current];const double f=src.f;double v=0;
        if(d.shapeFree&&rectTee()){const bool round=sel(IdTeeShape)==1;
            if(round!=d.tee.round){d.tee.round=round;d.tee.b=round?0:(src.Botv>0?src.Botv:d.tee.a);
                loading=true;SetDlgItemText(IdTeeALabel,round?L"Ø [mm]":L"A [mm]");if(round)SetDlgItemText(IdTeeB,L"");else setNumber(IdTeeB,d.tee.b*f);
                enable(IdTeeB,!round&&!d.lockB);loading=false;}}
        if(number(GetDlgItem(IdTeeA),v))d.tee.a=v/f;
        if(!d.tee.round&&!d.lockB&&number(GetDlgItem(IdTeeB),v))d.tee.b=v/f;
        d.elbow=elbowDeg(sel(IdElbow));if(sel(IdTurn)>=0)d.turn=sel(IdTurn);if(sel(IdOrder)>=0)d.order=sel(IdOrder);
        if(d.neighbour==1){
            const bool round=sel(IdDuctShape)==1;
            if(round!=d.duct.round){d.duct.round=round;d.duct.b=round?0:(d.ductWas.b>0?d.ductWas.b:d.tee.b);
                loading=true;SetDlgItemText(IdDuctALabel,round?L"Ø [mm]":L"A [mm]");
                if(round)SetDlgItemText(IdDuctB,L"");else setNumber(IdDuctB,d.duct.b*f);enable(IdDuctB,!round);loading=false;}
            if(number(GetDlgItem(IdDuctA),v))d.duct.a=v/f;
            if(!d.duct.round&&number(GetDlgItem(IdDuctB),v))d.duct.b=v/f;
        }
        if(needsPrelaz(d)&&!spiro(d)&&number(GetDlgItem(IdLength),v))d.lengthMm=v;
    }
    bool check(CString& why)const{
        const double f=src.f;
        if(!rectTee()&&choice!=Plain){why=L"TK-O, TK-G i štucna su za pravougaoni glavni kanal.";return false;}
        if(src.roundOtv&&(choice==Branch||choice==Pants)){why=L"Okrugli ogranak: izaberite štucnu ili TK0.";return false;}
        for(int i=0;i<2;++i){const Side& d=side[i];if(!d.used)continue;
            if(!(d.tee.a>0)||(!d.tee.round&&!(d.tee.b>0))||(d.neighbour==1&&(!(d.duct.a>0)||(!d.duct.round&&!(d.duct.b>0))))){why=L"Unesite pozitivne dimenzije.";return false;}
            if(d.elbow){
                if(d.neighbour==2||d.neighbour==3){why.Format(L"%ls: na priključku je prelaz ili drugi element - koljeno nije moguće.",tabName(i));return false;}
                if(d.neighbour==1&&!d.farFree){why.Format(L"%ls: iza kanala je mreža - koljeno bi pomjerilo cijelu mrežu.",tabName(i));return false;}
                if((d.turn==Up||d.turn==Down)&&d.neighbour!=0){why.Format(L"%ls: koljeno gore / dolje traži slobodan priključak.",tabName(i));return false;}
            }}
        if(choice==Pants&&!tvsTee::pantsFeasible(src.Aotv,side[0].tee.a,side[1].tee.a,tvsTeeData::collarMm()/f)){
            why.Format(L"Gaćasti T komad nije moguć: ogranci su preuski za ulaz %.0f mm.",src.Aotv*f);return false;}
        return true;
    }
    void refresh(){
        const double f=src.f;valid=check(problem);
        const Side& d=side[current];const bool prelaz=needsPrelaz(d);
        loading=true;
        if(prelaz)setNumber(IdLength,prelazMm(d));else SetDlgItemText(IdLength,L"");
        enable(IdLength,prelaz&&!spiro(d));
        enable(IdTurn,d.elbow!=0);enable(IdOrder,prelaz&&d.elbow!=0);
        loading=false;
        // description like the mockup: "Ogranak B -> Koljeno 90 -> Prelaz -> Kanal 200"
        CString text=tabName(current);text+=L" → ";
        wchar_t a[32],b[32];tvsTeePort::sizeText(a,32,d.tee,f);
        CString elbow;if(d.elbow){static const wchar_t* turns[4]={L"desno",L"lijevo",L"gore",L"dolje"};elbow.Format(L"Koljeno %d° %ls",d.elbow,turns[d.turn]);}
        std::vector<CString> parts;
        if(prelaz&&d.order==1)parts.push_back(L"Prelaz");
        if(d.elbow)parts.push_back(elbow);
        if(prelaz&&d.order==0)parts.push_back(L"Prelaz");
        if(hasDuct(d)){tvsTeePort::sizeText(b,32,d.duct,f);CString k;k.Format(L"Kanal %ls",b);parts.push_back(k);}
        else if(!d.elbow)parts.push_back(L"slobodan priključak");
        for(size_t i=0;i<parts.size();++i){if(i)text+=L" → ";text+=parts[i];}
        SetDlgItemText(IdDescription,text);
        CString sizes=a;if(prelaz){sizes+=L" → ";sizes+=b;}sizes+=L" mm";
        if(prelaz){CString l;l.Format(L"  ·  L = %.0f mm%ls",prelazMm(d),spiro(d)?L" (SOKO)":L"");sizes+=l;}
        SetDlgItemText(IdSizes,sizes);
        CString status;
        if(!valid)status=problem;
        else if(d.neighbour==3)status=L"Na priključku nije kanal ni prelaz - uskladite ga ručno.";
        else status=L"Pregled prije upisa u DWG";
        SetDlgItemText(IdStatus,status);enable(IDOK,valid);
        if(CWnd* w=GetDlgItem(IdSketch))w->Invalidate(FALSE);
    }

    // ---------------------------------------------------------------- MFC
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        auto fill=[&](int id,std::initializer_list<const wchar_t*> items){if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(id)))for(const wchar_t* s:items)c->AddString(s);};
        fill(IdType,{L"TK-O  ·  T komad sa ogrankom",L"TK-G  ·  T komad sa dva ogranka",L"Štucna  ·  bočni priključak",L"TK0  ·  T komad"});
        fill(IdTeeShape,{L"Pravougaoni",L"Okrugli"});fill(IdDuctShape,{L"Pravougaoni",L"Okrugli"});
        fill(IdElbow,{L"Bez koljena",L"30°",L"45°",L"60°",L"90°"});fill(IdTurn,{L"Desno",L"Lijevo",L"Gore",L"Dolje"});
        fill(IdOrder,{L"Koljeno → prelaz",L"Prelaz → koljeno"});
        setSel(IdType,choice);build();load();return TRUE;
    }
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        const int id=LOWORD(wp),code=HIWORD(wp);
        if(!loading){
            if(id==IdType&&code==CBN_SELCHANGE){const int s=sel(IdType);if(s>=0&&s<=3&&s!=choice){readFields();choice=s;current=0;build();load();}return TRUE;}
            if(id==IdFlip&&code==BN_CLICKED){readFields();flipped=!flipped;
                for(int i=0;i<2;++i)if(side[i].used){const Side keep=side[i];neighbourOf(side[i]);
                    side[i].tee=keep.tee;side[i].lengthMm=keep.lengthMm;side[i].elbow=keep.elbow;side[i].turn=keep.turn;side[i].order=keep.order;}
                load();return TRUE;}
            if((id==IdTabB||id==IdTabC)&&code==BN_CLICKED){const int want=id==IdTabB?0:1;
                if(want!=current&&side[want].used){readFields();current=want;load();}return TRUE;}
            if(((id==IdTeeA||id==IdTeeB||id==IdDuctA||id==IdDuctB||id==IdLength)&&code==EN_CHANGE)
                ||((id==IdTeeShape||id==IdDuctShape||id==IdElbow||id==IdTurn||id==IdOrder)&&code==CBN_SELCHANGE)){readFields();refresh();return TRUE;}
        }
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    virtual void OnOK(){readFields();refresh();if(!valid)return;CAdUiBaseDialog::OnOK();}
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_DRAWITEM&&wp==IdSketch){draw(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));return TRUE;}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }

    // ---------------------------------------------------------------- sketch (mockup, viewBox 300 x 209)
    void draw(LPDRAWITEMSTRUCT item){
        if(!item)return;CDC* dc=CDC::FromHandle(item->hDC);CRect r(item->rcItem);
        dc->FillSolidRect(r,RGB(255,255,255));
        const double sc=std::fmin(r.Width()/300.0,r.Height()/209.0),ox=r.left+(r.Width()-300*sc)/2,oy=r.top+(r.Height()-209*sc)/2;
        auto P=[&](double x,double y){if(flipped)x=300-x;return CPoint(static_cast<int>(ox+x*sc+0.5),static_cast<int>(oy+y*sc+0.5));};
        auto quad=[&](std::vector<CPoint>& out,double x0,double y0,double cx,double cy,double x1,double y1){
            for(int i=1;i<=12;++i){const double t=i/12.0,u=1-t;out.push_back(P(u*u*x0+2*u*t*cx+t*t*x1,u*u*y0+2*u*t*cy+t*t*y1));}};
        auto pen=[&](int w,COLORREF c,int style){LOGBRUSH lb={BS_SOLID,c,0};return ExtCreatePen(PS_GEOMETRIC|style|PS_ENDCAP_FLAT,w<1?1:w,&lb,0,NULL);};
        const int thin=static_cast<int>(sc+0.5)<1?1:static_cast<int>(sc+0.5);
        HGDIOBJ oldPen=dc->SelectObject(GetStockObject(NULL_PEN)),oldBrush=dc->SelectObject(GetStockObject(NULL_BRUSH));
        // T komad body (grey), as in the mockup
        {std::vector<CPoint> b;b.push_back(P(96,96));b.push_back(P(126,96));quad(b,126,96,140,96,140,79);b.push_back(P(160,79));b.push_back(P(160,86));
         quad(b,160,86,160,96,178,96);b.push_back(P(178,120));b.push_back(P(96,120));
         HPEN p=pen(thin,RGB(165,180,198),PS_SOLID);HBRUSH f=CreateSolidBrush(RGB(231,236,242));dc->SelectObject(p);dc->SelectObject(f);
         dc->Polygon(&b[0],static_cast<int>(b.size()));dc->SelectObject(GetStockObject(NULL_PEN));dc->SelectObject(GetStockObject(NULL_BRUSH));DeleteObject(p);DeleteObject(f);}
        {HPEN p=pen(thin,RGB(132,155,179),PS_DASH);dc->SelectObject(p);
         CPoint a=P(96,108),b=P(178,108),c=P(150,108),e=P(150,79);dc->MoveTo(a);dc->LineTo(b);dc->MoveTo(c);dc->LineTo(e);
         dc->SelectObject(GetStockObject(NULL_PEN));DeleteObject(p);}
        {HPEN p=pen(thin,RGB(182,196,210),PS_SOLID);dc->SelectObject(p);dc->MoveTo(P(121,96));dc->LineTo(P(121,120));dc->SelectObject(GetStockObject(NULL_PEN));DeleteObject(p);}
        if(choice==Pants){std::vector<CPoint> q;q.push_back(P(178,108));quad(q,178,108,202,108,202,79);
            HPEN p=pen(static_cast<int>(20*sc),RGB(189,202,215),PS_SOLID);dc->SelectObject(p);dc->Polyline(&q[0],static_cast<int>(q.size()));dc->SelectObject(GetStockObject(NULL_PEN));DeleteObject(p);}
        // selected port: elbow, prelaz, duct (mockup draw())
        const Side& s=side[current];const bool isB=current==0,tr=needsPrelaz(s);
        const double angle=(s.turn==Up||s.turn==Down)?0:s.elbow*pi()/180,sign=s.turn==Left?-1:1;
        const double org[2]={isB?150.0:178.0,isB?79.0:108.0},v[2]={isB?0.0:1.0,isB?-1.0:0.0},n[2]={-v[1],v[0]};
        std::vector<std::pair<double,double> > pts;pts.push_back(std::make_pair(0.0,0.0));
        const double pre=tr&&s.order==1?30:0,radius=25;
        if(pre>0)pts.push_back(std::make_pair(pre,0.0));
        if(angle>0)for(int i=1;i<=20;++i){const double t=angle*i/20;pts.push_back(std::make_pair(pre+radius*std::sin(t),sign*radius*(1-std::cos(t))));}
        const double dir[2]={std::cos(angle),sign*std::sin(angle)};
        const size_t coneStart=pts.size()-1;
        if(tr&&s.order==0){const std::pair<double,double> e=pts.back();pts.push_back(std::make_pair(e.first+dir[0]*30,e.second+dir[1]*30));}
        const bool vertical=s.elbow&&(s.turn==Up||s.turn==Down);
        if(hasDuct(s)||!vertical){const std::pair<double,double> e=pts.back();pts.push_back(std::make_pair(e.first+dir[0]*(hasDuct(s)?30:14),e.second+dir[1]*(hasDuct(s)?30:14)));}
        auto map=[&](double x,double y){return P(org[0]+v[0]*x+n[0]*y,org[1]+v[1]*x+n[1]*y);};
        auto width=[&](size_t i){if(!tr)return 10.0;if(s.order==1)return i==0?10.0:6.0;return i<=coneStart?10.0:6.0;};
        std::vector<CPoint> left,right,axis;
        for(size_t i=0;i<pts.size();++i){
            const std::pair<double,double> a=pts[i>0?i-1:0],b=pts[i+1<pts.size()?i+1:i];
            double dx=b.first-a.first,dy=b.second-a.second,l=std::sqrt(dx*dx+dy*dy);if(l<=0)l=1;
            const double w=width(i);
            left.push_back(map(pts[i].first-dy/l*w,pts[i].second+dx/l*w));right.push_back(map(pts[i].first+dy/l*w,pts[i].second-dx/l*w));
            axis.push_back(map(pts[i].first,pts[i].second));
        }
        if(pts.size()>1){
            std::vector<CPoint> poly(left);for(size_t i=right.size();i-->0;)poly.push_back(right[i]);
            HPEN p=pen(thin,RGB(8,99,169),PS_SOLID);HBRUSH f=CreateSolidBrush(RGB(227,240,252));dc->SelectObject(p);dc->SelectObject(f);
            dc->Polygon(&poly[0],static_cast<int>(poly.size()));dc->SelectObject(GetStockObject(NULL_PEN));dc->SelectObject(GetStockObject(NULL_BRUSH));DeleteObject(p);DeleteObject(f);
            HPEN d=pen(thin,RGB(8,99,169),PS_DASH);dc->SelectObject(d);dc->Polyline(&axis[0],static_cast<int>(axis.size()));dc->SelectObject(GetStockObject(NULL_PEN));DeleteObject(d);
            HPEN c=pen(thin,RGB(71,136,185),PS_SOLID);dc->SelectObject(c);
            const size_t marks[3]={1,coneStart,pts.size()-2};
            for(int m=0;m<3;++m){const size_t i=marks[m];if(i>0&&i<left.size()){dc->MoveTo(left[i]);dc->LineTo(right[i]);}}
            dc->SelectObject(GetStockObject(NULL_PEN));DeleteObject(c);
        }
        if(vertical){   // gore: circle with a dot, dolje: circle with a cross, at the end of the chain
            const CPoint e=axis.back();const int rr=static_cast<int>(9*sc);
            HPEN p=pen(thin,RGB(8,99,169),PS_SOLID);HBRUSH f=CreateSolidBrush(RGB(227,240,252));dc->SelectObject(p);dc->SelectObject(f);
            dc->Ellipse(e.x-rr,e.y-rr,e.x+rr,e.y+rr);
            if(s.turn==Up){HBRUSH k=CreateSolidBrush(RGB(8,99,169));dc->SelectObject(k);const int q=rr/3+1;dc->Ellipse(e.x-q,e.y-q,e.x+q,e.y+q);dc->SelectObject(f);DeleteObject(k);}
            else{const int q=static_cast<int>(rr*0.7);dc->MoveTo(e.x-q,e.y-q);dc->LineTo(e.x+q,e.y+q);dc->MoveTo(e.x-q,e.y+q);dc->LineTo(e.x+q,e.y-q);}
            dc->SelectObject(GetStockObject(NULL_PEN));dc->SelectObject(GetStockObject(NULL_BRUSH));DeleteObject(p);DeleteObject(f);
        }
        // port circles
        {HPEN p=pen(thin,RGB(143,167,189),PS_SOLID);dc->SelectObject(p);
         HBRUSH on=CreateSolidBrush(RGB(0,104,174)),off=CreateSolidBrush(RGB(231,236,242));
         const CPoint b=P(150,79),c=P(178,108);const int r3=static_cast<int>(3*sc)+1,r4=static_cast<int>(4*sc)+1;
         dc->SelectObject(isB?on:off);dc->Ellipse(b.x-r3,b.y-r3,b.x+r3,b.y+r3);
         if(side[1].used){dc->SelectObject(!isB?on:off);dc->Ellipse(c.x-r4,c.y-r4,c.x+r4,c.y+r4);}
         dc->SelectObject(GetStockObject(NULL_PEN));dc->SelectObject(GetStockObject(NULL_BRUSH));DeleteObject(p);DeleteObject(on);DeleteObject(off);}
        // labels
        CFont* font=GetFont();CFont* oldFont=font?dc->SelectObject(font):NULL;
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(39,84,118));
        const CPoint lu=P(flipped?183+30:85,86),lb=P(flipped?133+8:162,76),lc=P(flipped?119+8:175,121);
        dc->TextOut(flipped?lu.x-30:lu.x-6,lu.y,L"Ulaz");dc->TextOut(lb.x,lb.y,L"B");if(side[1].used)dc->TextOut(lc.x,lc.y,L"C");
        if(oldFont)dc->SelectObject(oldFont);
        dc->SelectObject(oldPen);dc->SelectObject(oldBrush);
    }
};

// ---------------------------------------------------------------- writing
inline AcGeVector3d turned(const AcGeVector3d& d,double a){return AcGeVector3d(d.x*std::cos(a)-d.y*std::sin(a),d.x*std::sin(a)+d.y*std::cos(a),0);}
inline bool append(AcDbDatabase* db,AcDbEntity* e,AcDbObjectId& id){
    AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk){delete e;return false;}
    if(space->appendAcDbEntity(id,e)!=Acad::eOk){delete e;return false;}e->close();return true;
}
inline bool prelaz(AcDbDatabase* db,const AcDbObjectId& like,AcGePoint3d at,const AcGeVector3d& dir,const Section& a,const Section& b,double L,double elev){
    double a1=a.a,b1=a.b,a2=b.a,b2=b.b,len=L;AcGeVector3d v=dir*L;
    TVS_TRANS* t=TVS_TRANS::add_new(a1,b1,a2,b2,len,v,at,a.round,b.round,1,false);if(!t||t->objectId().isNull())return false;
    AcDbObjectPointer<TVS_TRANS> o(t->objectId(),AcDb::kForWrite);if(o.openStatus()!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbEntity> src(like,AcDb::kForRead);if(src.openStatus()==Acad::eOk)o->setPropertiesFrom(src.object());
    o->put_Elevation(elev);o->recordGraphicsModified();(void)db;return true;
}
// Plan elbow at `at` (port, outward `dir`), turning by `angle` (+ = left). Returns the exit point and direction.
inline bool planElbow(const AcDbObjectId& like,const AcGePoint3d& at,const AcGeVector3d& dir,double angle,const Section& s,double elev,double f,AcGePoint3d& exit,AcGeVector3d& out){
    out=turned(dir,angle);const double K=10*(s.a>0?s.a:1);
    const AcGePoint3d t1=at,t2=at+dir*K,t3=t2+out*K;
    double sweep=0,sx=0,sy=0;if(!tvsGeometry::elbowDirection(t1.x,t1.y,t2.x,t2.y,t3.x,t3.y,sweep,sx,sy))return false;
    double A=s.a,B=s.round?0:s.b,R=globalTapRadiusConst;AcGeVector3d norm(0,0,1),start(sx,sy,0);AcGePoint3d mid=t2;
    TVS_TAP* e=TVS_TAP::add_new(A,B,R,norm,start,mid,sweep,false,s.round);if(!e||e->objectId().isNull())return false;
    AcDbObjectPointer<TVS_TAP> p(e->objectId(),AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)return false;
    p->RadiusTypeRound=globalRadiusTypeRound;p->RadiusTypeRect=globalRadiusTypeRect;p->TypeRoundTap=globalTypeRoundTap;
    p->RadiusVariableParameter=globalTapRadiusVariableParameter;p->RadiusConst=globalTapRadiusConst;p->Form=Form_Direct;
    // axis ports as TVS_MOVE reads them (tangent intersection = MiddlePoint)
    double radius=p->Radius;const int type=static_cast<int>(p->ThisRound?p->RadiusTypeRound:p->RadiusTypeRect);
    if(type==(p->ThisRound?RadiusTypeRound_RadiusVariable:RadiusTypeRect_RadiusVariable))radius=p->SizeA*p->RadiusVariableParameter;
    else if(type==(p->ThisRound?RadiusTypeRound_RadiusConstant:RadiusTypeRect_RadiusConstant))radius=p->RadiusConst;
    else if(type==(p->ThisRound?RadiusTypeRound_RadiusSpecific:RadiusTypeRect_RadiusSpecific))radius=p->SizeA*f<355?p->SizeA:p->SizeA/2;
    tvsMoveGeometry::Point ports[2];
    if(!tvsMoveGeometry::elbowPorts(tvsMove::point(p->MiddlePoint),tvsMoveGeometry::Point(sx,sy,0),sweep,radius+p->SizeA/2,ports)){p->erase();return false;}
    const int nearPort=tvsTeeFit::planDistance(tvsMove::acad(ports[0]),t1)<=tvsTeeFit::planDistance(tvsMove::acad(ports[1]),t1)?0:1;
    AcGeVector3d shift=at-tvsMove::acad(ports[nearPort]);shift.z=0;
    AcGeMatrix3d m;m.setToTranslation(shift);p->transformBy(m);
    exit=tvsMove::acad(ports[1-nearPort])+shift;exit.z=at.z;
    AcDbObjectPointer<AcDbEntity> src(like,AcDb::kForRead);if(src.openStatus()==Acad::eOk)p->setPropertiesFrom(src.object());
    p->put_Elevation(elev);p->recordGraphicsModified();return true;
}
// Vertical elbow (Gore / Dolje) on a free port, as the etaz elbows (OffsetService::makeElbow).
inline bool verticalElbow(AcDbDatabase* db,const AcDbObjectId& like,const AcGePoint3d& at,const AcGeVector3d& dir,bool up,double angle,const Section& s,double elev){
    TVS_TAP* tap=new TVS_TAP();tap->setDatabaseDefaults(db);
    {AcDbObjectPointer<AcDbEntity> src(like,AcDb::kForRead);if(src.openStatus()==Acad::eOk)tap->setPropertiesFrom(src.object());}
    const double inner=globalTapRadiusConst>0?globalTapRadiusConst:s.a;
    tap->SizeA=s.a;tap->SizeB=s.round?0:s.b;tap->ThisRound=s.round;tap->DuctType=DuctTypeStill;
    tap->TypeRoundTap=s.round?globalTypeRoundTap:TypeRoundTap_TapSection;tap->RadiusTypeRect=RadiusTypeRect_RadiusConstant;
    tap->RadiusTypeRound=RadiusTypeRound_RadiusConstant;tap->RadiusVariableParameter=.5;tap->RadiusConst=inner;
    tap->Swectangle=angle;tap->Startvector=AcGeVector3d(dir.y,-dir.x,0);tap->Normvector=AcGeVector3d(0,0,1);
    const double rc=inner+(s.round?s.a:s.b)/2;
    const AcGePoint3d mc=at+dir*(rc*std::sin(angle));tap->MiddlePoint=mc;
    if(s.round){const tvsRoundElbow::P m=tvsRoundElbow::middleFromMc(tvsRoundElbow::P(mc.x,mc.y),dir.y,-dir.x,angle,inner+s.a/2);tap->MiddlePoint=AcGePoint3d(m.x,m.y,mc.z);}
    tap->CenterPoint=tap->MiddlePoint;
    if(tap->put_Form(up?Form_Up:Form_Down)!=Acad::eOk||tap->put_Elevation(elev)!=Acad::eOk){delete tap;return false;}
    AcDbObjectId id;return append(db,tap,id);
}
// Elbow (and prelaz) chain on side `d` of the T komad, after the T komad got its new sizes.
inline void chain(const Dialog& dg,const Side& d,double tol){
    const Source& s=dg.src;const double f=s.f;AcDbDatabase* db=NULL;tvsTeePort::Port P;double elev=0;tvsTeePort::Neighbour n;
    {AcDbObjectPointer<TVS_WYE> w(s.id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk||!tvsTeePort::port(w.object(),d.k,P))return;
     db=w->database();elev=w->Elev;n=tvsTeePort::at(w.object(),P.p,tol);}
    const bool vertical=d.turn==Up||d.turn==Down;
    if(n.kind!=0&&n.kind!=1){acutPrintf(L"\nKoljeno nije umetnuto: na priključku nije kanal.");return;}
    double Ld=0;bool first=true;
    if(n.kind==1){AcDbObjectPointer<TVS_Pipe> p(n.id,AcDb::kForRead);if(p.openStatus()!=Acad::eOk)return;
        AcGeVector3d v=p->LastPoint-p->FirstPoint;v.z=0;Ld=v.length();first=n.end==0;}
    if(vertical&&n.kind==1){acutPrintf(L"\nKoljeno gore / dolje: priključak mora biti slobodan.");return;}
    const Section ds=n.kind==1?d.duct:P.s;const bool mismatch=n.kind==1&&!tvsTeePort::same(P.s,ds,1e-6);
    const double L=mismatch?tvsTeePort::prelazLength(db,P.s,ds,f,d.lengthMm):0;
    AcGePoint3d cursor=P.p;AcGeVector3d dir=P.out;Section sec=P.s;
    if(mismatch&&d.order==1){if(!prelaz(db,s.id,cursor,dir,sec,ds,L,elev))return;cursor+=dir*L;sec=ds;}
    const double angle=d.elbow*pi()/180;
    if(vertical){if(!verticalElbow(db,s.id,cursor,dir,d.turn==Up,angle,sec,elev))acutPrintf(L"\nKoljeno gore / dolje nije napravljeno.");
        else acutPrintf(L"\nUmetnuto koljeno %d° %ls.",d.elbow,d.turn==Up?L"gore":L"dolje");return;}
    AcGePoint3d exit;AcGeVector3d out;
    if(!planElbow(s.id,cursor,dir,d.turn==Left?angle:-angle,sec,elev,f,exit,out)){acutPrintf(L"\nKoljeno nije napravljeno.");return;}
    cursor=exit;dir=out;
    if(mismatch&&d.order==0){if(!prelaz(db,s.id,cursor,dir,sec,ds,L,elev))return;cursor+=dir*L;sec=ds;}
    if(n.kind==1){AcDbObjectPointer<TVS_Pipe> p(n.id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)return;
        AcGePoint3d a=cursor,b=cursor+dir*Ld;a.z=b.z=(first?p->FirstPoint:p->LastPoint).z;
        if(first){p->put_FirstPoint(a);p->put_Lastpoint(b);}else{p->put_Lastpoint(a);p->put_FirstPoint(b);}
        p->assertWriteEnabled();p->ThisRound=ds.round;p->put_SizeA(ds.a);p->put_SizeB(ds.round?0:ds.b);p->recordGraphicsModified();}
    acutPrintf(L"\nUmetnuto koljeno %d° %ls%ls.",d.elbow,d.turn==Left?L"lijevo":L"desno",mismatch?L" i prelaz":L"");
}
inline void apply(const Dialog& d){
    const Source& s=d.src;const double f=s.f;tvsTeeFit::Ports before,after;
    {AcDbObjectPointer<TVS_WYE> w(s.id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk){acutPrintf(L"\nT komad nije dostupan (zaključan sloj?).");return;}
     before=tvsTeeFit::ports(w.object());w->assertWriteEnabled();
     tvsTeeData::Record r;r.type=teeType(d.choice);r.l=tvsTeeData::collarMm()/f;
     const Side& b=d.side[0];const Side& c=d.side[1];
     if(d.choice==Branch){w->SizeAotv=b.tee.a;w->SizeBotv=w->SizeBpr;r.second=c.tee.a;
        w->LengthPl=tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w->SizeAotv),w->SizeAotv);}
     else if(d.choice==Pants){w->SizeApr=b.tee.a;w->SizeBpr=w->SizeBotv;r.second=c.tee.a;r.radius2=tvsTee::defaultRadius(r.second);
        w->LengthPl=tvsTee::pantsLengthPl(r.l,w->SizeApr,r.second);}
     else{w->ThisRoundotv=b.tee.round;w->SizeAotv=b.tee.a;w->SizeBotv=b.tee.round?0:b.tee.b;
        if(d.choice==Tap){r.l=tvsTeeCommand::tapLength(w->SizeAotv,f);w->LengthPl=r.l;}else r.type=0;}
     if(d.flipped)w->Vectpr=-w->Vectpr;
     const Acad::ErrorStatus es=tvsTeeData::write(w.object(),r);
     if(es!=Acad::eOk){acutPrintf(L"\nTip T komada nije upisan (%ls).",acadErrorStatusText(es));return;}
     w->recordGraphicsModified();after=tvsTeeFit::ports(w.object());
     if(d.flipped&&after.ok){std::swap(after.p[0],after.p[1]);std::swap(after.w[0],after.w[1]);}
     tvsTeeCommand::lastType()=r.type;}
    const double tol=2.0/f;
    // the elements on the ports follow them by place (duct end, or a prelaz with the duct behind it)
    if(before.ok&&after.ok)for(int k=0;k<3;++k){
        AcGeVector3d shift=after.p[k]-before.p[k];shift.z=0;if(shift.length()<=1e-9)continue;
        AcDbObjectPointer<TVS_WYE> w(s.id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)break;
        const tvsTeePort::Neighbour n=tvsTeePort::at(w.object(),before.p[k],tol);
        if(n.kind==1){w.close();AcDbObjectPointer<TVS_Pipe> p(n.id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
            AcGePoint3d to=(n.end==0?p->FirstPoint:p->LastPoint)+shift;if(n.end==0)p->put_FirstPoint(to);else p->put_Lastpoint(to);p->recordGraphicsModified();}
        else if(n.kind==2){tvsMove::Node tn;AcGePoint3d beyond;bool ok=false;
            {AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForRead);if(t.openStatus()==Acad::eOk&&tvsMove::describe(t.object(),tn)&&tn.ports.size()==2){beyond=tvsMove::acad(tn.ports[1-n.end]);ok=true;}}
            if(!ok)continue;const tvsTeePort::Neighbour q=tvsTeePort::at(w.object(),beyond,tol);w.close();
            {AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;AcGeMatrix3d m;m.setToTranslation(shift);t->transformBy(m);t->recordGraphicsModified();}
            if(q.kind==1){AcDbObjectPointer<TVS_Pipe> p(q.id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
                AcGePoint3d to=(q.end==0?p->FirstPoint:p->LastPoint)+shift;if(q.end==0)p->put_FirstPoint(to);else p->put_Lastpoint(to);p->recordGraphicsModified();}}
        else if(n.kind!=0)acutPrintf(L"\nT komad: element na pomjerenom priključku nije kanal ni prelaz; prilagodite ručno.");
    }
    // per side: elbow chain, or the duct size and the prelaz at the port
    for(int i=0;i<2;++i){const Side& x=d.side[i];if(!x.used)continue;
        if(x.elbow){chain(d,x,tol);continue;}
        if(x.neighbour==1&&!tvsTeePort::same(x.duct,x.ductWas,1e-9)){bool first=false;
            {AcDbObjectPointer<TVS_Pipe> p(x.ductId,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
             p->assertWriteEnabled();p->ThisRound=x.duct.round;p->put_SizeA(x.duct.a);p->put_SizeB(x.duct.round?0:x.duct.b);p->recordGraphicsModified();
             first=tvsTeeFit::planDistance(p->FirstPoint,after.ok?after.p[d.place(x.k)]:before.p[d.place(x.k)])<=tol;}
            wchar_t t[32];tvsTeePort::sizeText(t,32,x.duct,f);acutPrintf(L"\nKanal na priključku je sada %ls mm.",t);
            tvsTeeFit::farEnd(s.id,x.ductId,first,tol,f);}
        tvsTeePort::ensure(s.id,x.k,x.lengthMm);
    }
    {const int inletPort=d.choice==Pants?2:0;tvsTeePort::ensure(s.id,inletPort);}
    acutPrintf(L"\nT komad: %ls.",d.choice==Branch?L"TK-O sa ogrankom":d.choice==Pants?L"TK-G sa dva ogranka":d.choice==Tap?L"štucna":L"TK0");
}
inline bool open(const AcDbObjectId& id,UINT idd){
    Source s;if(!readSource(id,s)){acutPrintf(L"\nIzabrani objekat nije TVS T-komad.");return false;}
    Dialog dg(s,idd);
    {CAcModuleResourceOverride resources;if(dg.DoModal()!=IDOK)return true;}
    apply(dg);return true;
}
inline void run(UINT idd){
    {ads_name pre;if(acedSSGet(L"_I",NULL,NULL,NULL,pre)==RTNORM){   // double click / preselected T komad
        Adesk::Int32 n=0;acedSSLength(pre,&n);ads_name e;AcDbObjectId id;
        const bool one=n==1&&acedSSName(pre,0,e)==RTNORM&&acdbGetObjectId(id,e)==Acad::eOk&&id.objectClass()==TVS_WYE::desc();
        acedSSFree(pre);acedSSSetFirst(NULL,NULL);if(one){open(id,idd);return;}}}
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite TVS T-komad: ",name,pick)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    open(id,idd);
}
// Svojstva (TVS_CHANGE) with exactly one T komad selected: this dialog instead of the general one.
inline bool forSelection(const ads_name sset,UINT idd){
    Adesk::Int32 n=0;if(acedSSLength(sset,&n)!=RTNORM||n!=1)return false;
    ads_name e;if(acedSSName(sset,0,e)!=RTNORM)return false;AcDbObjectId id;if(acdbGetObjectId(id,e)!=Acad::eOk)return false;
    if(id.objectClass()!=TVS_WYE::desc())return false;
    return open(id,idd);
}
}
