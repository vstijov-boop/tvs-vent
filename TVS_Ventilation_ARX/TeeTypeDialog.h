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
#include "../geometry/TeeSketch.h"
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
          IdElbow=1321,IdTurn=1322,IdTurnLabel=1325,IdDuctShape=1310,IdDuctALabel=1311,IdDuctA=1312,IdDuctBLabel=1313,IdDuctB=1314,
          IdOrder=1323,IdLength=1315,IdSketch=1317,IdDescription=1318,IdSizes=1324,IdStatus=1309};
    Source src;int choice;bool flipped;Side side[2];int current;bool loading,valid;CString problem;
    bool newInlet;Section inletNew;int inletChoice;   // TVS_DIM_PRATI: the run arrived on the inlet with this section (openArriving)
    int arriveK;Section arrive;                       // ... or on port arriveK (ogranak / prolaz): that port starts with its width
    CFont bold,title;CBrush back,white,band;
    Dialog(const Source& s,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),src(s),choice(choiceOf(s.type)),flipped(false),current(0),loading(false),valid(false),newInlet(false),inletChoice(choiceOf(s.type)),arriveK(-1){}

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
        for(int i=0;i<2;++i)if(side[i].used&&side[i].k==arriveK){Side& x=side[i];   // the run arrived here: the port takes its size
            if(x.shapeFree){x.tee=arrive;}else if(arrive.round==x.tee.round){x.tee.a=arrive.a;if(!x.lockB)x.tee.b=arrive.round?0:arrive.b;}}
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
    bool focused(int id)const{const CWnd* w=GetDlgItem(id);return w&&::GetFocus()==w->m_hWnd;}
    void setNumber(int id,double mm){if(focused(id))return;CString t;t.Format(L"%.0f",mm);SetDlgItemText(id,t);}   // never under the cursor
    void setText(int id,const wchar_t* t){if(focused(id))return;SetDlgItemText(id,t);}
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    void show(int id,bool on){if(CWnd* w=GetDlgItem(id))w->ShowWindow(on?SW_SHOW:SW_HIDE);}
    int sel(int id)const{const CComboBox* c=static_cast<const CComboBox*>(GetDlgItem(id));return c?c->GetCurSel():-1;}
    void setSel(int id,int i){if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(id)))if(c->GetCurSel()!=i)c->SetCurSel(i);}
    static int elbowIndex(int deg){return deg==30?1:deg==45?2:deg==60?3:deg==90?4:0;}
    static int elbowDeg(int index){static const int d[5]={0,30,45,60,90};return index>=0&&index<5?d[index]:0;}
    // shape-dependent fields: B only for rectangular, Ø label for round (as the mockup)
    void shapes(){
        const Side& d=side[current];
        SetDlgItemText(IdTeeALabel,d.tee.round?L"Ø [mm]":L"A [mm]");show(IdTeeB,!d.tee.round);show(IdTeeBLabel,!d.tee.round);
        enable(IdTeeB,!d.tee.round&&!d.lockB);
        SetDlgItemText(IdDuctALabel,d.duct.round?L"Ø [mm]":L"A [mm]");show(IdDuctB,!d.duct.round);show(IdDuctBLabel,!d.duct.round);
        enable(IdDuctB,d.neighbour==1&&!d.duct.round);
    }
    void load(){   // the whole side into the fields (tab, type, Okreni)
        loading=true;const Side& d=side[current];const double f=src.f;
        SetDlgItemText(IdTabC,tabName(1));show(IdTabC,side[1].used);
        if(CWnd* t=GetDlgItem(IdTabB))t->Invalidate();if(CWnd* t=GetDlgItem(IdTabC))t->Invalidate();
        setSel(IdTeeShape,d.tee.round?1:0);enable(IdTeeShape,d.shapeFree&&rectTee());
        SetDlgItemText(IdTeeA,L"");setNumber(IdTeeA,d.tee.a*f);
        if(!d.tee.round)setNumber(IdTeeB,d.tee.b*f);
        setSel(IdElbow,elbowIndex(d.elbow));setSel(IdTurn,d.turn);
        const bool duct=d.neighbour==1;
        setSel(IdDuctShape,d.duct.round?1:0);enable(IdDuctShape,duct);enable(IdDuctA,duct);
        if(hasDuct(d)){setNumber(IdDuctA,d.duct.a*f);if(!d.duct.round)setNumber(IdDuctB,d.duct.b*f);}
        else{SetDlgItemText(IdDuctA,L"");SetDlgItemText(IdDuctB,L"");}
        setSel(IdOrder,d.order);
        if(needsPrelaz(d))setNumber(IdLength,prelazMm(d));else SetDlgItemText(IdLength,L"");
        shapes();loading=false;refresh();
    }
    void readFields(){   // fields -> side[current]; nothing is written back into the field being typed
        Side& d=side[current];const double f=src.f;double v=0;
        if(d.shapeFree&&rectTee()){const bool round=sel(IdTeeShape)==1;
            if(round!=d.tee.round){d.tee.round=round;d.tee.b=round?0:(src.Botv>0?src.Botv:d.tee.a);
                loading=true;if(!round)setNumber(IdTeeB,d.tee.b*f);shapes();loading=false;}}
        if(number(GetDlgItem(IdTeeA),v))d.tee.a=v/f;
        if(!d.tee.round&&!d.lockB&&number(GetDlgItem(IdTeeB),v))d.tee.b=v/f;
        d.elbow=elbowDeg(sel(IdElbow));if(sel(IdTurn)>=0)d.turn=sel(IdTurn);if(sel(IdOrder)>=0)d.order=sel(IdOrder);
        if(d.neighbour==1){
            const bool round=sel(IdDuctShape)==1;
            if(round!=d.duct.round){d.duct.round=round;d.duct.b=round?0:(d.ductWas.b>0?d.ductWas.b:d.tee.b);
                loading=true;if(!round)setNumber(IdDuctB,d.duct.b*f);shapes();loading=false;}
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
    void refresh(){   // derived values, enable state, description, status, sketch
        const double f=src.f;valid=check(problem);
        const Side& d=side[current];const bool prelaz=needsPrelaz(d);
        loading=true;
        if(prelaz){if(spiro(d)||!focused(IdLength))setNumber(IdLength,prelazMm(d));}else setText(IdLength,L"");
        enable(IdLength,prelaz&&!spiro(d));enable(IdTurn,d.elbow!=0);enable(IdOrder,prelaz&&d.elbow!=0);
        SetDlgItemText(IdTurnLabel,d.turn>=Up?L"Smjer (vertikalno)":L"Smjer u osnovi");
        loading=false;
        CString text=tabName(current);
        wchar_t a[32],b[32]=L"";tvsTeePort::sizeText(a,32,d.tee,f);
        CString elbow;if(d.elbow){static const wchar_t* turns[4]={L"desno",L"lijevo",L"gore",L"dolje"};elbow.Format(L"Koljeno %d° %ls",d.elbow,turns[d.turn]);}
        std::vector<CString> parts;
        if(prelaz&&d.order==1)parts.push_back(L"Prelaz");
        if(d.elbow)parts.push_back(elbow);
        if(prelaz&&d.order==0)parts.push_back(L"Prelaz");
        if(hasDuct(d)){tvsTeePort::sizeText(b,32,d.duct,f);CString k;k.Format(L"Kanal %ls",b);parts.push_back(k);}
        else if(!d.elbow)parts.push_back(L"slobodan priključak");
        for(size_t i=0;i<parts.size();++i){text+=L" → ";text+=parts[i];}
        SetDlgItemText(IdDescription,text);
        CString sizes;CString ins;{wchar_t t[32];tvsTeePort::sizeText(t,32,inlet(),f);ins=t;}
        sizes.Format(L"%ls%ls%ls mm",a,prelaz?L" → ":L"",prelaz?b:L"");
        if(prelaz){CString l;l.Format(L"  ·  L = %.0f mm%ls",prelazMm(d),spiro(d)?L" (SOKO)":L"");sizes+=l;}
        sizes+=L"\nUlaz "+ins+L" mm";if(choice==Branch||choice==Pants)sizes+=L" (visina svih)";
        SetDlgItemText(IdSizes,sizes);
        CString status=!valid?problem:d.neighbour==3?CString(L"Na priključku nije kanal ni prelaz - uskladite ga ručno."):CString(L"Pregled prije upisa u DWG");
        SetDlgItemText(IdStatus,status);enable(IDOK,valid);if(CWnd* w=GetDlgItem(IDOK))w->Invalidate();
        if(CWnd* w=GetDlgItem(IdSketch))w->Invalidate(FALSE);
    }

    // ---------------------------------------------------------------- MFC
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        LOGFONT lf={};if(CFont* f=GetFont())f->GetLogFont(&lf);
        lf.lfWeight=FW_SEMIBOLD;bold.CreateFontIndirect(&lf);lf.lfHeight=lf.lfHeight*13/10;title.CreateFontIndirect(&lf);
        back.CreateSolidBrush(RGB(244,245,247));white.CreateSolidBrush(RGB(255,255,255));band.CreateSolidBrush(RGB(231,236,242));
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
            if((id==IdTeeA||id==IdTeeB||id==IdDuctA||id==IdDuctB||id==IdLength)&&code==EN_CHANGE){readFields();refresh();return TRUE;}
            if((id==IdTeeA||id==IdTeeB||id==IdDuctA||id==IdDuctB||id==IdLength)&&code==EN_KILLFOCUS){readFields();
                const Side& d=side[current];const double f=src.f;loading=true;   // tidy the value once the field is left
                if(id==IdTeeA)setNumber(id,d.tee.a*f);else if(id==IdTeeB&&!d.tee.round)setNumber(id,d.tee.b*f);
                else if(id==IdDuctA&&hasDuct(d))setNumber(id,d.duct.a*f);else if(id==IdDuctB&&hasDuct(d)&&!d.duct.round)setNumber(id,d.duct.b*f);
                loading=false;refresh();return TRUE;}
            if((id==IdTeeShape||id==IdDuctShape||id==IdElbow||id==IdTurn||id==IdOrder)&&code==CBN_SELCHANGE){readFields();refresh();return TRUE;}
        }
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    virtual void OnOK(){readFields();refresh();if(!valid)return;CAdUiBaseDialog::OnOK();}
    // dialog units -> pixels
    CRect du(int x,int y,int w,int h){CRect r(x,y,x+w,y+h);MapDialogRect(&r);return r;}
    void legendBox(CDC* dc,int x,int y,int w,int h,const wchar_t* text){   // fieldset of the mockup
        CRect r=du(x,y,w,h);CPen p(PS_SOLID,1,RGB(213,223,236));CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));
        dc->Rectangle(r);dc->SelectObject(op);dc->SelectObject(ob);
        CFont* of=dc->SelectObject(&bold);CSize s=dc->GetTextExtent(text);const CRect t(r.left+8,r.top-s.cy/2,r.left+14+s.cx,r.top+s.cy/2+1);
        dc->FillSolidRect(t,RGB(244,245,247));dc->SetTextColor(RGB(0,52,89));dc->SetBkMode(TRANSPARENT);dc->TextOut(t.left+3,t.top,text);dc->SelectObject(of);
    }
    void hline(CDC* dc,int x,int y,int w,COLORREF c){CRect r=du(x,y,w,0);dc->FillSolidRect(r.left,r.top,r.Width(),1,c);}
    void paintBack(CDC* dc){
        CRect all;GetClientRect(&all);dc->FillSolidRect(all,RGB(244,245,247));
        const CRect head=du(0,0,400,24),foot=du(0,266,400,30);
        dc->FillSolidRect(CRect(all.left,all.top,all.right,head.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,head.bottom,all.Width(),1,RGB(173,186,203));
        dc->FillSolidRect(CRect(all.left,foot.top,all.right,all.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,foot.top,all.Width(),1,RGB(173,186,203));
        dc->SetBkMode(TRANSPARENT);
        {CFont* of=dc->SelectObject(&title);dc->SetTextColor(RGB(0,26,45));CRect t=du(10,0,200,24);dc->DrawText(L"Tip T komada",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(7,53,90));CRect t=du(200,0,190,24);dc->DrawText(L"TVS_TK_TIP",t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        legendBox(dc,10,84,218,46,L"Na T komadu");legendBox(dc,10,134,218,46,L"Odmah iza priključka");legendBox(dc,10,184,218,78,L"Kanal u nastavku");
        hline(dc,17,224,204,RGB(213,223,236));
        // preview card
        CRect card=du(236,62,154,200);dc->FillSolidRect(card,RGB(255,255,255));
        {CPen p(PS_SOLID,1,RGB(210,222,235));CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));dc->Rectangle(card);dc->SelectObject(op);dc->SelectObject(ob);}
        {CFont* of=dc->SelectObject(&bold);dc->SetTextColor(RGB(0,26,45));CRect t=du(242,62,100,18);dc->DrawText(L"Pregled u osnovi",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(49,90,128));CRect t=du(330,62,54,18);dc->DrawText(L"Skica",t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
         dc->SetTextColor(RGB(38,117,174));CRect k=du(242,207,146,12);dc->DrawText(L"● Izabrani priključak     – –  Osa kanala",k,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        hline(dc,236,80,154,RGB(210,222,235));hline(dc,236,206,154,RGB(210,222,235));hline(dc,236,220,154,RGB(210,222,235));
    }
    void drawButton(LPDRAWITEMSTRUCT it){   // tabs (selected = blue) and the primary U redu, as the mockup
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);const int id=static_cast<int>(it->CtlID);
        const bool disabled=(it->itemState&ODS_DISABLED)!=0,down=(it->itemState&ODS_SELECTED)!=0;
        COLORREF face=RGB(255,255,255),edge=RGB(173,187,207),text=RGB(6,71,119);
        if(id==IDOK){face=disabled?RGB(160,180,200):down?RGB(19,78,125):RGB(27,100,156);edge=face;text=RGB(255,255,255);}
        else if((id==IdTabB&&current==0)||(id==IdTabC&&current==1)){face=RGB(225,239,255);edge=RGB(0,102,178);text=RGB(0,91,164);}
        else if(down)face=RGB(234,244,253);
        dc->FillSolidRect(r,face);CPen p(PS_SOLID,1,edge);CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));
        dc->Rectangle(r);dc->SelectObject(op);dc->SelectObject(ob);
        CString t;GetDlgItem(id)->GetWindowText(t);CFont* of=dc->SelectObject(id==IDOK?&bold:GetFont());
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(text);dc->DrawText(t,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);
        if(it->itemState&ODS_FOCUS){CRect f(r);f.DeflateRect(3,3);dc->DrawFocusRect(f);}
    }
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_ERASEBKGND){paintBack(CDC::FromHandle(reinterpret_cast<HDC>(wp)));return TRUE;}
        if(msg==WM_DRAWITEM){LPDRAWITEMSTRUCT it=reinterpret_cast<LPDRAWITEMSTRUCT>(lp);
            if(wp==IdSketch){draw(it);return TRUE;}
            if(wp==IdTabB||wp==IdTabC||wp==IDOK){drawButton(it);return TRUE;}}
        if(msg==WM_CTLCOLORSTATIC){HWND h=reinterpret_cast<HWND>(lp);wchar_t cls[16]=L"";::GetClassNameW(h,cls,16);
            if(!_wcsicmp(cls,L"Static")){HDC dc=reinterpret_cast<HDC>(wp);const int id=::GetDlgCtrlID(h);
                const bool card=id==IdDescription||id==IdSizes;::SetBkMode(dc,TRANSPARENT);
                ::SetTextColor(dc,card?RGB(0,26,45):id==IdStatus?(valid?RGB(66,104,137):RGB(197,61,61)):RGB(49,90,128));
                return reinterpret_cast<LRESULT>((card?white:id==IdStatus?band:back).GetSafeHandle());}}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }

    // ---------------------------------------------------------------- sketch (geometry/TeeSketch.h)
    void draw(LPDRAWITEMSTRUCT item){
        if(!item)return;CDC* dc=CDC::FromHandle(item->hDC);CRect r(item->rcItem);
        dc->FillSolidRect(r,RGB(255,255,255));
        tvsTeeSketch::Input in;in.type=choice;in.flipped=flipped;in.selected=current;
        for(int i=0;i<2;++i){const Side& s=side[i];tvsTeeSketch::Side& t=in.side[i];
            t.used=s.used;t.hasDuct=hasDuct(s);t.prelaz=needsPrelaz(s);t.elbow=s.elbow;t.turn=s.turn;t.order=s.order;}
        std::vector<tvsTeeSketch::Prim> p;tvsTeeSketch::build(in,p);double x0,y0,x1,y1;tvsTeeSketch::bounds(p,x0,y0,x1,y1);
        const double m=10,fit=std::fmin((r.Width()-2*m)/(x1-x0),(r.Height()-2*m)/(y1-y0)),sc=fit>2.2?2.2:fit;
        const double ox=r.left+(r.Width()-(x1-x0)*sc)/2-x0*sc,oy=r.top+(r.Height()-(y1-y0)*sc)/2-y0*sc;
        auto P=[&](const tvsTeeSketch::Pt& q){return CPoint(static_cast<int>(ox+q.x*sc+0.5),static_cast<int>(oy+q.y*sc+0.5));};
        auto rgb=[](unsigned c){return RGB((c>>16)&255,(c>>8)&255,c&255);};
        CFont* of=dc->SelectObject(GetFont());dc->SetBkMode(TRANSPARENT);
        for(size_t i=0;i<p.size();++i){const tvsTeeSketch::Prim& q=p[i];
            if(q.kind==tvsTeeSketch::Text){CFont* f=dc->SelectObject(q.bold?&bold:GetFont());dc->SetTextColor(rgb(q.fill));
                const CPoint a=P(q.pts[0]);CSize s=dc->GetTextExtent(q.text.c_str());
                dc->TextOut(q.align==2?a.x-s.cx:q.align==1?a.x-s.cx/2:a.x,a.y-s.cy*3/4,q.text.c_str());dc->SelectObject(f);continue;}
            const int w=static_cast<int>(q.width*(sc>1?sc:1)+0.5);
            LOGBRUSH lb={BS_SOLID,rgb(q.stroke),0};DWORD dash[2]={static_cast<DWORD>(4*sc+1),static_cast<DWORD>(4*sc+1)};
            HPEN pen=q.stroke==tvsTeeSketch::none?static_cast<HPEN>(GetStockObject(NULL_PEN))
                :ExtCreatePen(PS_GEOMETRIC|(q.dash?PS_USERSTYLE:PS_SOLID)|PS_ENDCAP_FLAT|PS_JOIN_ROUND,w<1?1:w,&lb,q.dash?2:0,q.dash?dash:NULL);
            HBRUSH brush=q.fill==tvsTeeSketch::none||q.kind==tvsTeeSketch::Polyline?static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)):CreateSolidBrush(rgb(q.fill));
            HGDIOBJ op=dc->SelectObject(pen),ob=dc->SelectObject(brush);
            std::vector<CPoint> pts;for(size_t j=0;j<q.pts.size();++j)pts.push_back(P(q.pts[j]));
            if(q.kind==tvsTeeSketch::Polygon&&pts.size()>2)dc->Polygon(&pts[0],static_cast<int>(pts.size()));
            else if(q.kind==tvsTeeSketch::Polyline&&pts.size()>1)dc->Polyline(&pts[0],static_cast<int>(pts.size()));
            else if(q.kind==tvsTeeSketch::Circle&&!pts.empty()){const int rr=static_cast<int>(q.r*(sc>1.4?1.4:sc)+0.5);dc->Ellipse(pts[0].x-rr,pts[0].y-rr,pts[0].x+rr+1,pts[0].y+rr+1);}
            dc->SelectObject(op);dc->SelectObject(ob);
            if(q.stroke!=tvsTeeSketch::none)DeleteObject(pen);if(!(q.fill==tvsTeeSketch::none||q.kind==tvsTeeSketch::Polyline))DeleteObject(brush);
        }
        dc->SelectObject(of);
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
// Duct on a port that got another size or shape in the dialog: TVS_DIM_PRATI offers the run
// beyond it (SizeFollow.h sets the hook). false / no hook: prelaz at its far end as before.
typedef bool (*FollowHook)(const AcDbObjectId& duct,const Section& was,const AcDbObjectId& tee);
inline FollowHook& followHook(){static FollowHook h=NULL;return h;}
inline void apply(const Dialog& d){
    const Source& s=d.src;const double f=s.f;tvsTeeFit::Ports before,after;
    {AcDbObjectPointer<TVS_WYE> w(s.id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk){acutPrintf(L"\nT komad nije dostupan (zaključan sloj?).");return;}
     before=tvsTeeFit::ports(w.object());w->assertWriteEnabled();
     if(d.newInlet&&(d.choice==Pants)==(d.inletChoice==Pants)){   // inlet takes the size of the run that arrived (TVS_DIM_PRATI)
        if(d.choice==Pants){w->SizeAotv=d.inletNew.a;w->SizeBotv=d.inletNew.round?0:d.inletNew.b;}
        else{w->SizeApr=d.inletNew.a;w->SizeBpr=d.inletNew.round?0:d.inletNew.b;}}
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
    struct Follow {AcDbObjectId duct;Section was;bool first;};std::vector<Follow> follow;
    for(int i=0;i<2;++i){const Side& x=d.side[i];if(!x.used)continue;
        if(x.elbow){chain(d,x,tol);continue;}
        if(x.neighbour==1&&!tvsTeePort::same(x.duct,x.ductWas,1e-9)){bool first=false;
            {AcDbObjectPointer<TVS_Pipe> p(x.ductId,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
             p->assertWriteEnabled();p->ThisRound=x.duct.round;p->put_SizeA(x.duct.a);p->put_SizeB(x.duct.round?0:x.duct.b);p->recordGraphicsModified();
             first=tvsTeeFit::planDistance(p->FirstPoint,after.ok?after.p[d.place(x.k)]:before.p[d.place(x.k)])<=tol;}
            wchar_t t[32];tvsTeePort::sizeText(t,32,x.duct,f);acutPrintf(L"\nKanal na priključku je sada %ls mm.",t);
                        Follow fw={x.ductId,x.ductWas,first};
            if(followHook())follow.push_back(fw);else tvsTeeFit::farEnd(s.id,x.ductId,first,tol,f);}
        tvsTeePort::ensure(s.id,x.k,x.lengthMm);
    }
    // the run beyond the resized ducts, after the T komad and its prelazi are written
    for(size_t k=0;k<follow.size();++k)if(!followHook()(follow[k].duct,follow[k].was,s.id))tvsTeeFit::farEnd(s.id,follow[k].duct,follow[k].first,tol,f);
    {const int inletPort=d.choice==Pants?2:0;tvsTeePort::ensure(s.id,inletPort);}
    acutPrintf(L"\nT komad: %ls.",d.choice==Branch?L"TK-O sa ogrankom":d.choice==Pants?L"TK-G sa dva ogranka":d.choice==Tap?L"štucna":L"TK0");
}
inline bool open(const AcDbObjectId& id,UINT idd){
    Source s;if(!readSource(id,s)){acutPrintf(L"\nIzabrani objekat nije TVS T-komad.");return false;}
    Dialog dg(s,idd);
    {CAcModuleResourceOverride resources;if(dg.DoModal()!=IDOK)return true;}
    apply(dg);return true;
}
// TVS_DIM_PRATI: a resized run arrived on port k of a TK-O / TK-G with section `in`. On the
// inlet (TK-O port 0, TK-G port 2) the T komad takes that section (heights of all ports with
// it; the other widths stay) and the dialog decides the rest; on another port the dialog
// opens on that side, where the duct already shows the new size. false: dialog cancelled or
// not possible (the caller leaves the T komad as it is: prelaz on the port).
inline bool openArriving(const AcDbObjectId& id,UINT idd,int k,const Section& in){
    Source s;if(!idd||!readSource(id,s))return false;
    const int choice=choiceOf(s.type);if(choice!=Branch&&choice!=Pants)return false;
    const bool inlet=choice==Pants?k==2:k==0;int tab=0;
    if(inlet){
        if((choice==Pants?s.roundOtv:s.roundPr)!=in.round){acutPrintf(L"\nT komad: ulaz je drugog oblika od kanala; T komad nije mijenjan (prelaz na priključku).");return false;}
        if(choice==Pants){s.Aotv=in.a;s.Botv=in.round?0:in.b;}
        else{if(!(s.second>0))s.second=s.Apr;s.Apr=in.a;s.Bpr=in.round?0:in.b;}   // prolaz C keeps its width
    }else tab=choice==Branch?(k==1?1:0):(k==0?1:0);
        Dialog dg(s,idd);dg.newInlet=inlet;dg.inletNew=in;dg.current=tab;if(!inlet){dg.arriveK=k;dg.arrive=in;}
    {CAcModuleResourceOverride resources;if(dg.DoModal()!=IDOK)return false;}
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
