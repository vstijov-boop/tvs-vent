#pragma once
// TVS_DIM_PRATI dialog "Promjena dimenzije dionice" (IDD_RUN), after TVS_DIM_PRATI_mockup.html:
// shown after a duct got a new size and the run of the old size goes on beyond it. Obuhvat
// (cijela dionica / samo izabrani kanal / jedna strana / kanal + prelaz na spoju) and the
// decisions on the run: T komad (nastavi; TK-O / TK-G open their own dialog afterwards / stani /
// prelaz iza), etaž and obilazak (preračunaj / stani). The preview is the real plan of the run
// (geometry/RunSketch.h): what changes is blue. The walk itself is SizeFollow.h (Survey callback).
#include "../geometry/RunSketch.h"
#include "SketchRender.h"
#include "adui.h"
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>
namespace tvsRunDialog {
enum Scope {All=0,Only=1,Side1=2,Side2=3,OnlyPrelaz=4};
struct Choices {int scope,tee;bool etaz,bypass;Choices():scope(All),tee(0),etaz(true),bypass(true){}};   // tee: 0 nastavi (TK-O / TK-G: dijalog), 1 stani, 2 prelaz iza
struct Preview {
    tvsRunSketch::Input sketch;int ducts,elbows,tees,etazes,bypasses,teeDialogs,metTee,metEtaz,metBypass,joints;std::vector<std::wstring> stops;
    Preview():ducts(0),elbows(0),tees(0),etazes(0),bypasses(0),teeDialogs(0),metTee(0),metEtaz(0),metBypass(0),joints(0){}
    int elements()const{return ducts+elbows+tees+etazes+bypasses;}
};
typedef std::function<void(const Choices&,Preview&)> Survey;
class Dialog : public CAdUiBaseDialog {
public:
    enum {IdInfo=1501,IdScope=1502,IdTee=1503,IdEtaz=1504,IdBypass=1505,IdSketch=1506,IdCaption=1507,IdFacts=1508,IdStatus=1509,
          IdTeeLabel=1510,IdEtazLabel=1511,IdBypassLabel=1512,IdCardTitle=1513,IdCardHint=1514};
    Choices c;Preview pv;Survey survey;CString banner;bool loading;CFont bold,title;CBrush back,white,band,info;
    Dialog(const Survey& s,const CString& text,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),survey(s),banner(text),loading(false){}
    int sel(int id){CComboBox* b=static_cast<CComboBox*>(GetDlgItem(id));const int k=b?b->GetCurSel():0;return k<0?0:k;}
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    void read(){c.scope=sel(IdScope);c.tee=sel(IdTee);c.etaz=sel(IdEtaz)==0;c.bypass=sel(IdBypass)==0;}
    void refresh(){
        pv=Preview();if(survey)survey(c,pv);
        const bool run=c.scope!=Only&&c.scope!=OnlyPrelaz;
        enable(IdTee,run&&pv.metTee>0);enable(IdTeeLabel,run&&pv.metTee>0);
        enable(IdEtaz,run&&pv.metEtaz>0);enable(IdEtazLabel,run&&pv.metEtaz>0);
        enable(IdBypass,run&&pv.metBypass>0);enable(IdBypassLabel,run&&pv.metBypass>0);
        CString t;
        if(run)t.Format(L"Uz izabrani kanal mijenja se elemenata: %d",pv.elements());
        else t=c.scope==Only?L"Mijenja se samo izabrani kanal":L"Izabrani kanal + prelaz na spoju";
        SetDlgItemText(IdCaption,t+L"\nTamno plavo: izabrani kanal  ·  plavo: izmjena");
        CString facts;
        if(run){
            facts.Format(L"Kanala %d  ·  koljena %d  ·  T komada %d  ·  etaža %d  ·  obilazaka %d",pv.ducts,pv.elbows,pv.tees,pv.etazes,pv.bypasses);
            if(!pv.stops.empty()){facts+=L"\nGranice: ";for(size_t k=0;k<pv.stops.size();++k){if(k)facts+=L", ";facts+=pv.stops[k].c_str();}}
            if(pv.teeDialogs){CString d;d.Format(L"\nPoslije primjene otvara se dijalog T komada (%d).",pv.teeDialogs);facts+=d;}
        }else if(pv.joints==0)facts=L"Izabrani kanal ne naliježe direktno na element stare dimenzije.";
        else if(c.scope==Only)facts.Format(L"Spojeva sa elementom stare dimenzije: %d.\nOstaju bez prelaza (upozorenje u komandnoj liniji).",pv.joints);
        else facts.Format(L"Spojeva sa elementom stare dimenzije: %d.\nNa svaki se umeće prelaz nova → stara dimenzija.",pv.joints);
        SetDlgItemText(IdFacts,facts);
        SetDlgItemText(IdStatus,L"Pregled prije izmjene crteža");
        if(CWnd* w=GetDlgItem(IdSketch))w->Invalidate(FALSE);
    }
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        LOGFONT lf={};if(CFont* f=GetFont())f->GetLogFont(&lf);
        lf.lfWeight=FW_SEMIBOLD;bold.CreateFontIndirect(&lf);lf.lfHeight=lf.lfHeight*13/10;title.CreateFontIndirect(&lf);
        back.CreateSolidBrush(RGB(244,245,247));white.CreateSolidBrush(RGB(255,255,255));band.CreateSolidBrush(RGB(231,236,242));info.CreateSolidBrush(RGB(234,244,253));
        loading=true;
        auto fill=[&](int id,std::initializer_list<const wchar_t*> items,int cur){if(CComboBox* b=static_cast<CComboBox*>(GetDlgItem(id))){for(const wchar_t* s:items)b->AddString(s);b->SetCurSel(cur);}};
        fill(IdScope,{L"Cijela dionica do granica",L"Samo izabrani kanal",L"Od izabranog kanala prema strani 1",L"Od izabranog kanala prema strani 2",L"Samo izabrani kanal + prelaz na spoju"},c.scope);
        fill(IdTee,{L"Nastavi (TK-O / TK-G: otvori njegov dijalog)",L"Stani na T komadu",L"Prelaz iza T komada (TK0 / štucna)"},c.tee);
        fill(IdEtaz,{L"Preračunaj etaž",L"Stani prije etaža"},c.etaz?0:1);
        fill(IdBypass,{L"Preračunaj obilazak (njegov dijalog)",L"Stani prije obilaska"},c.bypass?0:1);
        SetDlgItemText(IdInfo,banner);SetDlgItemText(IdCardTitle,L"Obuhvat u crtežu");SetDlgItemText(IdCardHint,L"Osnova trase");
        loading=false;refresh();return TRUE;
    }
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        if(!loading&&HIWORD(wp)==CBN_SELCHANGE){read();refresh();return TRUE;}
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    virtual void OnOK(){read();CAdUiBaseDialog::OnOK();}
    CRect du(int x,int y,int w,int h){CRect r(x,y,x+w,y+h);MapDialogRect(&r);return r;}
    void frame(CDC* dc,const CRect& r,COLORREF col){CPen p(PS_SOLID,1,col);CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));dc->Rectangle(r);dc->SelectObject(op);dc->SelectObject(ob);}
    void paintBack(CDC* dc){
        CRect all;GetClientRect(&all);dc->FillSolidRect(all,RGB(244,245,247));
        const CRect head=du(0,0,400,24),foot=du(0,226,400,30);
        dc->FillSolidRect(CRect(all.left,all.top,all.right,head.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,head.bottom,all.Width(),1,RGB(173,186,203));
        dc->FillSolidRect(CRect(all.left,foot.top,all.right,all.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,foot.top,all.Width(),1,RGB(173,186,203));
        dc->SetBkMode(TRANSPARENT);
        {CFont* of=dc->SelectObject(&title);dc->SetTextColor(RGB(0,26,45));CRect t=du(10,0,220,24);dc->DrawText(L"Promjena dimenzije dionice",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(7,53,90));CRect t=du(230,0,160,24);dc->DrawText(L"Svojstva kanala  ·  TVS_DIM_PRATI",t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CRect r=du(10,30,220,15);dc->FillSolidRect(r,RGB(234,244,253));dc->FillSolidRect(r.left,r.top,3,r.Height(),RGB(38,117,174));}   // banner
        {CRect r=du(10,86,220,96);frame(dc,r,RGB(213,223,236));CFont* of=dc->SelectObject(&bold);const wchar_t* t=L"Odluke na trasi";CSize sz=dc->GetTextExtent(t);
         dc->FillSolidRect(CRect(r.left+8,r.top-sz.cy/2,r.left+14+sz.cx,r.top+sz.cy/2+1),RGB(244,245,247));dc->SetTextColor(RGB(0,52,89));dc->TextOut(r.left+11,r.top-sz.cy/2,t);dc->SelectObject(of);}
        CRect card=du(238,30,152,188);dc->FillSolidRect(card,RGB(255,255,255));frame(dc,card,RGB(210,222,235));
        const CRect l1=du(238,47,152,0),l2=du(238,190,152,0);dc->FillSolidRect(card.left,l1.top,card.Width(),1,RGB(210,222,235));dc->FillSolidRect(card.left,l2.top,card.Width(),1,RGB(210,222,235));
    }
    void drawButton(LPDRAWITEMSTRUCT it){
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);const bool down=(it->itemState&ODS_SELECTED)!=0;
        const COLORREF face=down?RGB(19,78,125):RGB(27,100,156);dc->FillSolidRect(r,face);frame(dc,r,face);
        CString t;GetDlgItem(IDOK)->GetWindowText(t);CFont* of=dc->SelectObject(&bold);
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(255,255,255));dc->DrawText(t,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);
        if(it->itemState&ODS_FOCUS){CRect f(r);f.DeflateRect(3,3);dc->DrawFocusRect(f);}
    }
    void drawSketch(LPDRAWITEMSTRUCT it){
        std::vector<tvsTeeSketch::Prim> p;tvsRunSketch::build(pv.sketch,p);
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);tvsSketchRender::draw(dc,r,p,GetFont(),&bold,1e9,8);
    }
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_ERASEBKGND){paintBack(CDC::FromHandle(reinterpret_cast<HDC>(wp)));return TRUE;}
        if(msg==WM_DRAWITEM){LPDRAWITEMSTRUCT it=reinterpret_cast<LPDRAWITEMSTRUCT>(lp);
            if(wp==IdSketch){drawSketch(it);return TRUE;}
            if(wp==IDOK){drawButton(it);return TRUE;}}
        if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORBTN){HWND h=reinterpret_cast<HWND>(lp);wchar_t cls[16]=L"";::GetClassNameW(h,cls,16);
            if(!_wcsicmp(cls,L"Static")||!_wcsicmp(cls,L"Button")){HDC dc=reinterpret_cast<HDC>(wp);const int id=::GetDlgCtrlID(h);
                const bool card=id==IdCardTitle||id==IdCardHint||id==IdCaption,inf=id==IdInfo;::SetBkMode(dc,TRANSPARENT);
                ::SetTextColor(dc,id==IdCardTitle?RGB(0,26,45):inf?RGB(0,30,53):id==IdStatus?RGB(0,100,107):RGB(49,90,128));
                if(id==IdCardTitle||inf)::SelectObject(dc,bold.GetSafeHandle());
                return reinterpret_cast<LRESULT>((card?white:inf?info:id==IdStatus?band:back).GetSafeHandle());}}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }
};
// true = Primijeni (choices as set), false = Otkaži.
inline bool show(const Survey& survey,const CString& banner,Choices& c,UINT idd){
    Dialog dg(survey,banner,idd);dg.c=c;INT_PTR rc;
    {CAcModuleResourceOverride resources;rc=dg.DoModal();}
    if(rc!=IDOK)return false;c=dg.c;return true;
}
}
