#pragma once
// TVS_TK_TIP dialog (IDD_TEETYPE): type of the T komad (TK-O, TK-G, stucna, TK0), Okreni, the ports
// B / C with their sizes on the T komad and the duct that continues there, plan sketch.
// Rule TK-O / TK-G: every port has the inlet height B (locked); the T komad keeps its sizes and a
// duct of another size or shape on a port gets a prelaz right at the port (TeePort.h); the prelaz
// length: spiro by catalogue, rectangular from the field (default TVS_SETTINGS "Prelaz L").
// Stucna and TK0 keep free branch sizes. U redu = one command, one UNDO step.
#include "TeePort.h"
#include "TeeCommand.h"
#include "../TVS_Ventilation_DBX/SpiroCatalog.h"
#include "adui.h"
#include <cmath>
#include <utility>
#include <vector>
namespace tvsTeeDialog {
using tvsTeePort::Section;
enum Choice {Branch=0,Pants=1,Tap=2,Plain=3};
inline int teeType(int c){return c==Branch?tvsTee::Branch:c==Pants?tvsTee::Pants:c==Tap?tvsTee::Tap:tvsTee::Straight;}
inline int choiceOf(int t){return t==tvsTee::Branch?Branch:t==tvsTee::Pants?Pants:t==tvsTee::Tap?Tap:Plain;}
// The T komad as read before the dialog (drawing units).
struct Source {
    AcDbObjectId id;double f,Apr,Bpr,Aotv,Botv,second,radius2;bool roundPr,roundOtv;int type;AcGePoint3d port[3];
    Source():f(1),Apr(0),Bpr(0),Aotv(0),Botv(0),second(0),radius2(0),roundPr(false),roundOtv(false),type(0){}
};
// One port shown in the dialog (B or C): TVS index k, size on the T komad, what continues there.
struct Side {
    bool used;int k;Section tee;bool lockB,editShape;
    int neighbour;AcDbObjectId ductId;Section duct,ductWas;double lengthMm;
    Side():used(false),k(2),lockB(false),editShape(false),neighbour(0),lengthMm(0){}
};
inline bool readSource(const AcDbObjectId& id,Source& s){
    AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return false;
    s.id=id;s.f=tvsUnits::factor(w.object());s.Apr=w->SizeApr;s.Bpr=w->SizeBpr;s.Aotv=w->SizeAotv;s.Botv=w->SizeBotv;
    s.roundPr=w->ThisRoundpr;s.roundOtv=w->ThisRoundotv;
    tvsTeeData::Record r;if(tvsTeeData::read(w.object(),r)){s.type=r.type;s.second=r.second;s.radius2=r.radius2;}
    const tvsTeeFit::Ports ps=tvsTeeFit::ports(w.object());if(!ps.ok)return false;
    for(int k=0;k<3;++k)s.port[k]=ps.p[k];
    return true;
}

class Dialog : public CAdUiBaseDialog {
public:
    enum {IdType=1301,IdFlip=1302,IdInlet=1303,IdTabB=1304,IdTabC=1305,IdTeeShape=1306,IdTeeA=1307,IdTeeB=1308,
          IdDuctNote=1309,IdDuctShape=1310,IdDuctALabel=1311,IdDuctA=1312,IdDuctBLabel=1313,IdDuctB=1314,IdLength=1315,
          IdLengthNote=1316,IdSketch=1317,IdSummary=1318,IdTeeALabel=1319};
    Source src;int choice;bool flipped;Side side[2];int current;bool loading,valid;
    explicit Dialog(const Source& s,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),src(s),choice(choiceOf(s.type)),flipped(false),current(0),loading(false),valid(false){}

    // ---- model -----------------------------------------------------------------------------
    bool rectTee()const{return !src.roundPr&&src.Bpr>0;}
    Section inlet()const{Section s;
        if(choice==Pants){s.a=src.Aotv;s.round=src.roundOtv;s.b=s.round?0:src.Botv;}
        else{s.a=src.Apr;s.round=src.roundPr;s.b=s.round?0:src.Bpr;}return s;}
    int place(int k)const{return flipped&&k<2?1-k:k;}   // where the TVS port k is after Okreni (by place)
    void neighbourOf(Side& d)const{
        d.neighbour=0;d.ductId=AcDbObjectId::kNull;
        AcDbObjectPointer<TVS_WYE> w(src.id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return;
        const tvsTeePort::Neighbour n=tvsTeePort::at(w.object(),src.port[place(d.k)],2.0/src.f);
        if(n.kind==1){d.neighbour=1;d.ductId=n.id;d.duct=d.ductWas=n.s;return;}
        if(n.kind==2){d.neighbour=2;d.ductId=n.id;   // prelaz: its other side
            w.close();AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForRead);if(t.openStatus()!=Acad::eOk)return;
            const int e=1-n.end;d.duct.a=e==0?t->SizeAp1:t->SizeAp2;d.duct.round=e==0?t->ThisRoundp1:t->ThisRoundp2;
            d.duct.b=d.duct.round?0:(e==0?t->SizeBp1:t->SizeBp2);d.ductWas=d.duct;return;}
        d.neighbour=n.kind==0?0:3;
    }
    void build(){
        const Section in=inlet();
        for(int i=0;i<2;++i)side[i]=Side();
        Side& b=side[0];Side& c=side[1];
        if(choice==Branch){
            b.used=true;b.k=2;b.tee.a=src.Aotv;b.tee.b=in.b;b.lockB=true;
            c.used=true;c.k=1;c.tee.a=src.type==tvsTee::Branch&&src.second>0?src.second:src.Apr;c.tee.b=in.b;c.lockB=true;
        }else if(choice==Pants){
            b.used=true;b.k=1;b.tee.a=src.Apr;b.tee.b=in.b;b.lockB=true;
            c.used=true;c.k=0;c.tee.a=src.type==tvsTee::Pants&&src.second>0?src.second:src.Apr;c.tee.b=in.b;c.lockB=true;
        }else{
            b.used=true;b.k=2;b.tee.a=src.Aotv;b.tee.round=src.roundOtv;b.tee.b=src.roundOtv?0:src.Botv;
        }
        for(int i=0;i<2;++i)if(side[i].used){neighbourOf(side[i]);side[i].lengthMm=tvsTeeFit::transitionMm();}
        if(current>0&&!side[1].used)current=0;
    }
    bool needsPrelaz(const Side& d)const{return d.neighbour>=1&&d.neighbour<=2&&!tvsTeePort::same(d.tee,d.duct,1e-6);}
    bool spiro(const Side& d)const{return d.tee.round&&d.duct.round&&std::fabs(d.tee.a-d.duct.a)>1e-9&&tvsSpiro::read(NULL).catalog;}
    double prelazMm(const Side& d)const{
        if(spiro(d))return tvsSpiro::drawnMm(d.tee.a*src.f,d.duct.a*src.f,tvsSpiro::read(NULL));
        return d.lengthMm>0?d.lengthMm:tvsTeeFit::transitionMm();
    }

    // ---- fields ------------------------------------------------------------------------------
    static bool number(CWnd* w,double& v){
        if(!w)return false;CString t;w->GetWindowText(t);t.Trim();t.Replace(L',',L'.');if(t.IsEmpty())return false;
        wchar_t* end=NULL;v=wcstod(t,&end);return end&&!*end&&v>0&&v<1e7;
    }
    void setNumber(int id,double mm){CString t;t.Format(L"%.0f",mm);SetDlgItemText(id,t);}
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    void show(int id,bool on){if(CWnd* w=GetDlgItem(id))w->ShowWindow(on?SW_SHOW:SW_HIDE);}
    void load(){   // side[current] -> fields
        loading=true;const Side& d=side[current];const double f=src.f;
        CString t;
        const Section in=inlet();
        if(in.round)t.Format(L"Ulaz: Ø%.0f mm (iz T komada)",in.a*f);else t.Format(L"Ulaz: %.0f x %.0f mm (iz T komada)",in.a*f,in.b*f);
        SetDlgItemText(IdInlet,t);
        SetDlgItemText(IdTabB,L"Ogranak B");SetDlgItemText(IdTabC,choice==Pants?L"Ogranak C":L"Prolaz C");
        show(IdTabC,side[1].used);CheckRadioButton(IdTabB,IdTabC,current==0?IdTabB:IdTabC);
        SetDlgItemText(IdTeeShape,d.tee.round?L"Okrugli":L"Pravougaoni");
        SetDlgItemText(IdTeeALabel,d.tee.round?L"Ø [mm]":L"A [mm]");
        setNumber(IdTeeA,d.tee.a*f);
        if(d.tee.round)SetDlgItemText(IdTeeB,L"");else setNumber(IdTeeB,d.tee.b*f);
        enable(IdTeeB,!d.tee.round&&!d.lockB);
        const bool duct=d.neighbour==1;
        SetDlgItemText(IdDuctNote,d.neighbour==0?L"Nema priključenog kanala.":d.neighbour==1?L"Priključeni kanal (dimenzija se može promijeniti):"
            :d.neighbour==2?L"Na priključku je prelaz; kanal iza njega:":L"Na priključku nije kanal (koljeno, više elemenata) - uskladite ručno.");
        CComboBox* shape=static_cast<CComboBox*>(GetDlgItem(IdDuctShape));if(shape)shape->SetCurSel(d.duct.round?1:0);
        enable(IdDuctShape,duct);enable(IdDuctA,duct);
        SetDlgItemText(IdDuctALabel,d.duct.round?L"Ø [mm]":L"A [mm]");
        if(d.neighbour==1||d.neighbour==2){setNumber(IdDuctA,d.duct.a*f);if(d.duct.round)SetDlgItemText(IdDuctB,L"");else setNumber(IdDuctB,d.duct.b*f);}
        else{SetDlgItemText(IdDuctA,L"");SetDlgItemText(IdDuctB,L"");}
        enable(IdDuctB,duct&&!d.duct.round);
        loading=false;refresh();
    }
    void readFields(){   // fields -> side[current]
        Side& d=side[current];const double f=src.f;double v=0;
        if(number(GetDlgItem(IdTeeA),v))d.tee.a=v/f;
        if(!d.tee.round&&!d.lockB&&number(GetDlgItem(IdTeeB),v))d.tee.b=v/f;
        if(d.neighbour==1){
            CComboBox* shape=static_cast<CComboBox*>(GetDlgItem(IdDuctShape));
            const bool round=shape&&shape->GetCurSel()==1;
            if(round!=d.duct.round){d.duct.round=round;d.duct.b=round?0:(d.ductWas.b>0?d.ductWas.b:d.tee.b);
                loading=true;SetDlgItemText(IdDuctALabel,round?L"Ø [mm]":L"A [mm]");
                if(round)SetDlgItemText(IdDuctB,L"");else setNumber(IdDuctB,d.duct.b*f);enable(IdDuctB,!round);loading=false;}
            if(number(GetDlgItem(IdDuctA),v))d.duct.a=v/f;
            if(!d.duct.round&&number(GetDlgItem(IdDuctB),v))d.duct.b=v/f;
        }
        if(needsPrelaz(d)&&!spiro(d)&&number(GetDlgItem(IdLength),v))d.lengthMm=v;
    }
    void refresh(){   // validation, prelaz length field, summary, sketch
        const double f=src.f;CString why;valid=true;
        if(!rectTee()&&choice!=Plain){valid=false;why=L"TK-O, TK-G i štucna su za pravougaoni glavni kanal.";}
        else if(src.roundOtv&&(choice==Branch||choice==Pants)){valid=false;why=L"Okrugli ogranak: izaberite štucnu ili TK0.";}
        for(int i=0;i<2&&valid;++i){const Side& d=side[i];if(!d.used)continue;
            if(!(d.tee.a>0)||(!d.tee.round&&!(d.tee.b>0))||(d.neighbour==1&&(!(d.duct.a>0)||(!d.duct.round&&!(d.duct.b>0))))){valid=false;why=L"Unesite pozitivne dimenzije.";}}
        if(valid&&choice==Pants&&!tvsTee::pantsFeasible(src.Aotv,side[0].tee.a,side[1].tee.a,tvsTeeData::collarMm()/f)){
            valid=false;why.Format(L"Gaćasti T komad nije moguć: ogranci su preuski za ulaz %.0f mm.",src.Aotv*f);}
        const Side& d=side[current];
        const bool prelaz=needsPrelaz(d);
        loading=true;
        if(prelaz){setNumber(IdLength,prelazMm(d));}else SetDlgItemText(IdLength,L"");
        enable(IdLength,prelaz&&!spiro(d));
        SetDlgItemText(IdLengthNote,prelaz?(spiro(d)?L"spiro, katalog SOKO":L""):L"nije potreban");
        loading=false;
        CString text;
        if(!valid)text=why;
        else{
            wchar_t a[32],b[32];tvsTeePort::sizeText(a,32,d.tee,f);
            text.Format(L"%ls %ls na T komadu",current==0?L"Ogranak B":(choice==Pants?L"Ogranak C":L"Prolaz C"),a);
            if(d.neighbour==1||d.neighbour==2){tvsTeePort::sizeText(b,32,d.duct,f);
                CString more;if(prelaz)more.Format(L" -> prelaz L = %.0f mm -> kanal %ls mm",prelazMm(d),b);else more.Format(L" -> kanal %ls mm (bez prelaza)",b);text+=more;}
            if(choice==Branch||choice==Pants)text+=L".  Svi priključci imaju visinu ulaza.";
            if(flipped)text+=L"  Okrenut.";
        }
        SetDlgItemText(IdSummary,text);enable(IDOK,valid);
        if(CWnd* w=GetDlgItem(IdSketch))w->Invalidate(FALSE);
    }

    // ---- MFC ------------------------------------------------------------------------------------
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdType))){
            c->AddString(L"TK-O  ·  T komad sa ogrankom");c->AddString(L"TK-G  ·  gaćasti, dva ogranka");
            c->AddString(L"Štucna  ·  bočni priključak");c->AddString(L"TK0  ·  obični T komad");c->SetCurSel(choice);}
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdDuctShape))){c->AddString(L"Pravougaoni");c->AddString(L"Okrugli");}
        build();load();return TRUE;
    }
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        const int id=LOWORD(wp),code=HIWORD(wp);
        if(!loading){
            if(id==IdType&&code==CBN_SELCHANGE){CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdType));
                const int sel=c?c->GetCurSel():choice;if(sel>=0&&sel<=3&&sel!=choice){choice=sel;current=0;build();load();}return TRUE;}
            if(id==IdFlip&&code==BN_CLICKED){readFields();flipped=!flipped;for(int i=0;i<2;++i)if(side[i].used){
                    const Side keep=side[i];neighbourOf(side[i]);side[i].tee=keep.tee;side[i].lengthMm=keep.lengthMm;}
                load();return TRUE;}
            if((id==IdTabB||id==IdTabC)&&code==BN_CLICKED){const int want=id==IdTabB?0:1;
                if(want!=current&&side[want].used){readFields();current=want;load();}return TRUE;}
            if(((id==IdTeeA||id==IdTeeB||id==IdDuctA||id==IdDuctB||id==IdLength)&&code==EN_CHANGE)||(id==IdDuctShape&&code==CBN_SELCHANGE)){
                readFields();refresh();return TRUE;}
        }
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    virtual void OnOK(){readFields();refresh();if(!valid)return;CAdUiBaseDialog::OnOK();}
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_DRAWITEM&&wp==IdSketch){draw(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));return TRUE;}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }

    // ---- sketch (plan, schematic) ----------------------------------------------------------------
    void draw(LPDRAWITEMSTRUCT item){
        if(!item)return;CDC* dc=CDC::FromHandle(item->hDC);CRect r(item->rcItem);
        dc->FillSolidRect(r,RGB(255,255,255));
        const double W=r.Width(),H=r.Height(),cx=r.left+W/2,cy=r.top+H*0.58,s=(W<H?W:H)/6.0;   // s: drawn half-width of the inlet
        const double in=inlet().a>0?inlet().a:1;
        auto half=[&](double a){double v=s*a/in;return v<3?3:v>s*1.6?s*1.6:v;};
        const int mir=flipped?-1:1;
        auto X=[&](double x){return static_cast<int>(cx+mir*x);};auto Y=[&](double y){return static_cast<int>(cy-y);};
        CPen body(PS_SOLID,1,RGB(8,99,169)),pick(PS_SOLID,2,RGB(0,104,174)),axis(PS_DOT,1,RGB(132,155,179)),grey(PS_SOLID,1,RGB(165,180,198));
        CBrush fill(RGB(227,240,252)),light(RGB(231,236,242));
        auto poly=[&](const std::vector<CPoint>& p,CPen& pen,CBrush& br){CPen* op=dc->SelectObject(&pen);CBrush* ob=dc->SelectObject(&br);
            dc->Polygon(&p[0],static_cast<int>(p.size()));dc->SelectObject(op);dc->SelectObject(ob);};
        auto rect=[&](double x0,double y0,double x1,double y1,CPen& pen,CBrush& br){std::vector<CPoint> p;
            p.push_back(CPoint(X(x0),Y(y0)));p.push_back(CPoint(X(x1),Y(y0)));p.push_back(CPoint(X(x1),Y(y1)));p.push_back(CPoint(X(x0),Y(y1)));poly(p,pen,br);};
        // continuation of a port: from the port (x,y) along (dx,dy): prelaz (if any) then duct
        auto cont=[&](const Side& d,double x,double y,double dx,double dy,bool chosen){
            if(d.neighbour!=1&&d.neighbour!=2)return;
            const double h0=half(d.tee.a),h1=half(d.duct.a),L=needsPrelaz(d)?s*1.0:0,D=s*1.3;const double nx=-dy,ny=dx;
            CPen& pen=chosen?pick:body;
            if(L>0){std::vector<CPoint> p;
                p.push_back(CPoint(X(x+nx*h0),Y(y+ny*h0)));p.push_back(CPoint(X(x+dx*L+nx*h1),Y(y+dy*L+ny*h1)));
                p.push_back(CPoint(X(x+dx*L-nx*h1),Y(y+dy*L-ny*h1)));p.push_back(CPoint(X(x-nx*h0),Y(y-ny*h0)));poly(p,pen,fill);}
            const double x0=x+dx*L,y0=y+dy*L;std::vector<CPoint> p;
            p.push_back(CPoint(X(x0+nx*h1),Y(y0+ny*h1)));p.push_back(CPoint(X(x0+dx*D+nx*h1),Y(y0+dy*D+ny*h1)));
            p.push_back(CPoint(X(x0+dx*D-nx*h1),Y(y0+dy*D-ny*h1)));p.push_back(CPoint(X(x0-nx*h1),Y(y0-ny*h1)));poly(p,chosen?pick:grey,light);
        };
        const Side& b=side[0];const Side& c=side[1];
        const double hi=half(inlet().a);
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(39,84,118));
        if(choice==Pants){   // inlet from below, branches left (C) and right (B)
            const double hb=half(b.tee.a),hc=half(c.tee.a),top=hi*0.2;
            rect(-hi,-s*2.2,hi,-hi*0.2,grey,light);
            std::vector<CPoint> p;p.push_back(CPoint(X(-hi),Y(-hi*0.2)));p.push_back(CPoint(X(hi),Y(-hi*0.2)));
            p.push_back(CPoint(X(hi+s*0.9),Y(top+hb-hb)));p.push_back(CPoint(X(hi+s*0.9),Y(top+2*hb)));p.push_back(CPoint(X(0),Y(top+2*std::max(hb,hc))));
            p.push_back(CPoint(X(-hi-s*0.9),Y(top+2*hc)));p.push_back(CPoint(X(-hi-s*0.9),Y(top)));poly(p,body,fill);
            cont(b,hi+s*0.9,top+hb,1,0,current==0);cont(c,-(hi+s*0.9),top+hc,-1,0,current==1);
            dc->TextOut(X(0)-12,Y(-s*2.2)+2,L"Ulaz");dc->TextOut(X(hi+s*0.9)-4,Y(top+2*hb)-16,L"B");dc->TextOut(X(-hi-s*0.9)-4,Y(top+2*hc)-16,L"C");
        }else{   // run left (inlet) -> right (C), branch B up
            const double hb=b.tee.round?half(b.tee.a):half(b.tee.a),hcr=c.used?half(c.tee.a):hi,len=s*1.1+hb;
            rect(-s*3.0,-hi,-len,hi,grey,light);
            std::vector<CPoint> p;p.push_back(CPoint(X(-len),Y(-hi)));p.push_back(CPoint(X(len),Y(-hcr)));p.push_back(CPoint(X(len),Y(hcr)));
            if(choice==Branch){p.push_back(CPoint(X(hb*2.2),Y(hi)));p.push_back(CPoint(X(hb),Y(hi+s*0.9)));}
            else if(choice==Tap){p.push_back(CPoint(X(hb),Y(hi)));p.push_back(CPoint(X(hb),Y(hi+s*0.9)));}
            else{p.push_back(CPoint(X(hb),Y(hi)));p.push_back(CPoint(X(hb),Y(hi+s*0.9)));}
            p.push_back(CPoint(X(-hb),Y(hi+s*0.9)));
            if(choice==Tap)p.push_back(CPoint(X(-hb-s*0.6),Y(hi)));else p.push_back(CPoint(X(-hb),Y(hi)));
            p.push_back(CPoint(X(-len),Y(hi)));poly(p,body,fill);
            CPen* op=dc->SelectObject(&axis);dc->MoveTo(X(-s*3.0),Y(0));dc->LineTo(X(len),Y(0));dc->MoveTo(X(0),Y(0));dc->LineTo(X(0),Y(hi+s*0.9));dc->SelectObject(op);
            cont(b,0,hi+s*0.9,0,1,current==0);if(c.used)cont(c,len,0,1,0,current==1);
            dc->TextOut(X(-s*3.0)+2,Y(hi)-16,L"Ulaz");dc->TextOut(X(hb)+4,Y(hi+s*0.9)+2,L"B");if(c.used)dc->TextOut(X(len)-6,Y(-hcr)+2,L"C");
        }
    }
};

// Applies the dialog to the T komad (open nothing before): sizes and type, Okreni, the ducts on
// the ports follow the ports by place, changed duct sizes, then a prelaz on every port that needs one.
inline void apply(const Dialog& d){
    const Source& s=d.src;const double f=s.f;tvsTeeFit::Ports before,after;
    {AcDbObjectPointer<TVS_WYE> w(s.id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk){acutPrintf(L"\nT komad nije dostupan (zaključan sloj?).");return;}
     before=tvsTeeFit::ports(w.object());
     w->assertWriteEnabled();
     tvsTeeData::Record r;r.type=teeType(d.choice);r.l=tvsTeeData::collarMm()/f;
     const Side& b=d.side[0];const Side& c=d.side[1];
     if(d.choice==Branch){
        w->SizeAotv=b.tee.a;w->SizeBotv=w->SizeBpr;r.second=c.tee.a;
        w->LengthPl=tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w->SizeAotv),w->SizeAotv);
     }else if(d.choice==Pants){
        w->SizeApr=b.tee.a;w->SizeBpr=w->SizeBotv;r.second=c.tee.a;r.radius2=tvsTee::defaultRadius(r.second);
        w->LengthPl=tvsTee::pantsLengthPl(r.l,w->SizeApr,r.second);
     }else{
        w->SizeAotv=b.tee.a;w->SizeBotv=b.tee.round?0:b.tee.b;
        if(d.choice==Tap){r.l=tvsTeeCommand::tapLength(w->SizeAotv,f);w->LengthPl=r.l;}
        else r.type=0;
     }
     if(d.flipped)w->Vectpr=-w->Vectpr;
     const Acad::ErrorStatus es=tvsTeeData::write(w.object(),r);
     if(es!=Acad::eOk){acutPrintf(L"\nTip T komada nije upisan (%ls).",acadErrorStatusText(es));return;}
     w->recordGraphicsModified();
     after=tvsTeeFit::ports(w.object());
     if(d.flipped&&after.ok){std::swap(after.p[0],after.p[1]);std::swap(after.w[0],after.w[1]);}
     tvsTeeCommand::lastType()=r.type;}
    // the elements on the ports follow them by place (duct end, or a prelaz with the duct behind it)
    const double tol=2.0/f;
    if(before.ok&&after.ok)for(int k=0;k<3;++k){
        AcGeVector3d shift=after.p[k]-before.p[k];shift.z=0;if(shift.length()<=1e-9)continue;
        AcDbObjectPointer<TVS_WYE> w(s.id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)break;
        const tvsTeePort::Neighbour n=tvsTeePort::at(w.object(),before.p[k],tol);
        if(n.kind==1){w.close();AcDbObjectPointer<TVS_Pipe> p(n.id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
            AcGePoint3d to=(n.end==0?p->FirstPoint:p->LastPoint)+shift;if(n.end==0)p->put_FirstPoint(to);else p->put_Lastpoint(to);p->recordGraphicsModified();}
        else if(n.kind==2){
            tvsMove::Node tn;AcGePoint3d beyond;bool ok=false;
            {AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForRead);if(t.openStatus()==Acad::eOk&&tvsMove::describe(t.object(),tn)&&tn.ports.size()==2){beyond=tvsMove::acad(tn.ports[1-n.end]);ok=true;}}
            if(!ok)continue;const tvsTeePort::Neighbour q=tvsTeePort::at(w.object(),beyond,tol);w.close();
            {AcDbObjectPointer<TVS_TRANS> t(n.id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
             AcGeMatrix3d m;m.setToTranslation(shift);t->transformBy(m);t->recordGraphicsModified();}
            if(q.kind==1){AcDbObjectPointer<TVS_Pipe> p(q.id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
                AcGePoint3d to=(q.end==0?p->FirstPoint:p->LastPoint)+shift;if(q.end==0)p->put_FirstPoint(to);else p->put_Lastpoint(to);p->recordGraphicsModified();}
        }else if(n.kind!=0)acutPrintf(L"\nT komad: element na pomjerenom priključku nije kanal ni prelaz; prilagodite ručno.");
    }
    // changed duct sizes ("Kanal u nastavku")
    for(int i=0;i<2;++i){const Side& x=d.side[i];if(!x.used||x.neighbour!=1||tvsTeePort::same(x.duct,x.ductWas,1e-9))continue;
        bool first=false;
        {AcDbObjectPointer<TVS_Pipe> p(x.ductId,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
         p->assertWriteEnabled();p->ThisRound=x.duct.round;p->put_SizeA(x.duct.a);p->put_SizeB(x.duct.round?0:x.duct.b);p->recordGraphicsModified();
         first=tvsTeeFit::planDistance(p->FirstPoint,after.ok?after.p[d.place(x.k)]:before.p[d.place(x.k)])<=tol;}
        wchar_t t[32];tvsTeePort::sizeText(t,32,x.duct,f);acutPrintf(L"\nKanal na priključku je sada %ls mm.",t);
        tvsTeeFit::farEnd(s.id,x.ductId,first,tol,f);
    }
    // a prelaz on every port that needs one (the T komad keeps its sizes)
    for(int k=0;k<3;++k){double lengthMm=0;for(int i=0;i<2;++i)if(d.side[i].used&&d.side[i].k==k)lengthMm=d.side[i].lengthMm;
        tvsTeePort::ensure(s.id,k,lengthMm);}
    acutPrintf(L"\nT komad: %ls.",d.choice==Branch?L"TK-O sa ogrankom":d.choice==Pants?L"TK-G gaćasti":d.choice==Tap?L"štucna":L"TK0 obični");
}
inline void run(UINT idd){
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite TVS T-komad: ",name,pick)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    Source s;if(!readSource(id,s)){acutPrintf(L"\nIzabrani objekat nije TVS T-komad.");return;}
    Dialog dg(s,idd);
    {CAcModuleResourceOverride resources;if(dg.DoModal()!=IDOK)return;}
    apply(dg);
}
}
