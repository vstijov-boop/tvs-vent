#pragma once
// TVS_PROTOK, dimenzije dionice (IDD_SIZING): one dionica above the maximum velocity. Shows the
// present size with velocity and friction, the limit (editable), the criterion (height kept /
// width : height <= 4:1; round ducts: diameters), the standard sizes around the limit with m/s
// and Pa/m (too fast ones grey; a click copies the size into "Ručni unos") and the manual size
// with its velocity while typing. Modal: the drawing zooms between two dionice, so the dialog
// closes with an action and FlowCommand.h opens it again on the next one. "Zadrži ručno" stores
// the size as the "ručno" mark of the dionica (xdata TVS_RUCNO); "Primijeni" gives the size to
// the dionica (FlowCommand.h applySize).
#include "../geometry/DuctSizing.h"
#include "adui.h"
#include <afxcmn.h>
#include <string>
#include <vector>
namespace tvsSizingDialog {
enum Action {Close=0,Previous=1,Next=2,Keep=3,Remove=4,Apply=5};
struct Input {
    int index,count;double flow,a,b;bool round;   // present size (mm); b = 0 round
    bool manual;double manualA,manualB;int ducts;
    Input():index(1),count(1),flow(0),a(0),b(0),round(false),manual(false),manualA(0),manualB(0),ducts(0){}
};
struct Result {Action action;double vmax;int mode;tvsDuctSizing::Size size;Result():action(Close),vmax(5),mode(0){}};
inline CString sizeText(bool round,double a,double b){CString t;if(round)t.Format(L"Ø%.0f",a);else t.Format(L"%.0f×%.0f",a,b);return t;}
inline CString number(double v,int digits){CString t;t.Format(digits==1?L"%.1f":L"%.2f",v);t.Replace(L'.',L',');return t;}
class Dialog : public CAdUiBaseDialog {
public:
    enum {IdNow=1601,IdLimit=1602,IdHeight=1603,IdRatio=1604,IdList=1605,IdManual=1606,IdManualInfo=1607,IdMark=1608,
          IdPrevious=1609,IdNext=1610,IdRemove=1611,IdLimitLabel=1612,IdListLabel=1613,IdManualLabel=1614,IdRoundNote=1615,IdApply=1616};
    Input in;Result out;std::vector<tvsDuctSizing::Size> list;CListCtrl grid;bool loading,nowFast,manualFast;CFont bold,title;CBrush back,band,white;
    static CPoint& place(){static CPoint p(-1,-1);return p;}   // the dialog opens where it was left
    Dialog(const Input& i,const Result& r,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),in(i),out(r),loading(false),nowFast(false),manualFast(false){}
    double limit(){CString t;GetDlgItemText(IdLimit,t);t.Replace(L',',L'.');wchar_t* e=NULL;const double v=wcstod(t,&e);return v>0.05&&v<100?v:out.vmax;}
    void refresh(){
        out.vmax=limit();out.mode=in.round?0:(IsDlgButtonChecked(IdRatio)==BST_CHECKED?1:0);
        const double v=tvsDuctSizing::velocity(in.flow,in.a,in.b,in.round),r=tvsDuctSizing::friction(in.flow,in.a,in.b,in.round);nowFast=v>out.vmax+1e-9;
        CString t;t.Format(L"Protok %.0f m³/h  ·  kanala %d\nSada %ls:  %ls m/s,  %ls Pa/m%ls",in.flow,in.ducts,sizeText(in.round,in.a,in.b).GetString(),number(v,2).GetString(),number(r,2).GetString(),nowFast?L"  – iznad granice":L"");
        SetDlgItemText(IdNow,t);
        if(in.manual){CString m=in.manualA>0?L"Oznaka: ručno "+sizeText(!(in.manualB>0),in.manualA,in.manualB):CString(L"Oznaka: ručno (sadašnja dimenzija)");SetDlgItemText(IdMark,m);}
        else SetDlgItemText(IdMark,L"Bez oznake „ručno“");
        GetDlgItem(IdRemove)->EnableWindow(in.manual?TRUE:FALSE);
        tvsDuctSizing::candidates(in.flow,in.round,in.b,out.mode,out.vmax,list);
        loading=true;grid.DeleteAllItems();
        for(size_t k=0;k<list.size();++k){const int row=grid.InsertItem(static_cast<int>(k),sizeText(in.round,list[k].a,list[k].b));
            grid.SetItemText(row,1,number(list[k].v,2));grid.SetItemText(row,2,number(list[k].r,2));
            grid.SetItemText(row,3,list[k].v>out.vmax+1e-9?L"iznad":L"");}
        loading=false;
        GetDlgItem(IdNow)->Invalidate();manualInfo();
    }
    void manualInfo(){
        CString t;GetDlgItemText(IdManual,t);tvsDuctSizing::Size s;manualFast=false;
        if(t.Trim().IsEmpty())SetDlgItemText(IdManualInfo,L"npr. 300x150, d200");
        else if(!tvsDuctSizing::parse(std::wstring(t.GetString()),in.round,s))SetDlgItemText(IdManualInfo,L"nije dimenzija");
        else{const bool rd=!(s.b>0);const double v=tvsDuctSizing::velocity(in.flow,s.a,s.b,rd),r=tvsDuctSizing::friction(in.flow,s.a,s.b,rd);manualFast=v>out.vmax+1e-9;
            CString i;i.Format(L"%ls m/s,  %ls Pa/m",number(v,2).GetString(),number(r,2).GetString());SetDlgItemText(IdManualInfo,i);}
        GetDlgItem(IdManualInfo)->Invalidate();
    }
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        LOGFONT lf={};if(CFont* f=GetFont())f->GetLogFont(&lf);
        lf.lfWeight=FW_SEMIBOLD;bold.CreateFontIndirect(&lf);lf.lfHeight=lf.lfHeight*13/10;title.CreateFontIndirect(&lf);
        back.CreateSolidBrush(RGB(244,245,247));band.CreateSolidBrush(RGB(231,236,242));white.CreateSolidBrush(RGB(255,255,255));
        CString head;head.Format(L"TVS_PROTOK  ·  dionica %d / %d",in.index,in.count);SetWindowText(L"TVS_VENT – dimenzije dionice");
        SetDlgItemText(IdListLabel,L"Prijedlog dimenzija (klik = u ručni unos)");SetDlgItemText(IdMark,L"");
        grid.SubclassDlgItem(IdList,this);
        grid.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
        CRect gr;grid.GetClientRect(&gr);const int w=gr.Width();
        grid.InsertColumn(0,L"Dimenzija",LVCFMT_LEFT,w*36/100);grid.InsertColumn(1,L"m/s",LVCFMT_RIGHT,w*20/100);
        grid.InsertColumn(2,L"Pa/m",LVCFMT_RIGHT,w*20/100);grid.InsertColumn(3,L"",LVCFMT_LEFT,w-w*76/100-2);
        SetDlgItemText(IdLimit,number(out.vmax,1));
        CheckRadioButton(IdHeight,IdRatio,out.mode==1?IdRatio:IdHeight);
        GetDlgItem(IdHeight)->ShowWindow(in.round?SW_HIDE:SW_SHOW);GetDlgItem(IdRatio)->ShowWindow(in.round?SW_HIDE:SW_SHOW);
        GetDlgItem(IdRoundNote)->ShowWindow(in.round?SW_SHOW:SW_HIDE);SetDlgItemText(IdRoundNote,L"Okrugli kanal: standardni prečnici");
        if(in.manual&&in.manualA>0)SetDlgItemText(IdManual,sizeText(!(in.manualB>0),in.manualA,in.manualB));
        GetDlgItem(IdPrevious)->EnableWindow(in.count>1?TRUE:FALSE);GetDlgItem(IdNext)->EnableWindow(in.count>1?TRUE:FALSE);
        SetDlgItemText(IdManualLabel,L"Ručni unos");SetDlgItemText(IdLimitLabel,L"Najveća brzina, m/s");
        headText=head;
        if(place().x>=0){SetWindowPos(NULL,place().x,place().y,0,0,SWP_NOSIZE|SWP_NOZORDER);}
        refresh();
        GetDlgItem(IdManual)->SetFocus();return FALSE;
    }
    CString headText;
    void remember(){CRect r;GetWindowRect(&r);place()=r.TopLeft();}
    void finish(Action a){remember();out.action=a;out.vmax=limit();EndDialog(a==Close?IDCANCEL:IDOK);}
    virtual void OnOK(){   // Zadrži ručno
        CString t;GetDlgItemText(IdManual,t);t.Trim();tvsDuctSizing::Size s;
        if(t.IsEmpty()){s.ok=true;s.a=0;s.b=0;}   // ručno with the present size
        else if(!tvsDuctSizing::parse(std::wstring(t.GetString()),in.round,s)){SetDlgItemText(IdManualInfo,L"nije dimenzija – npr. 300x150, d200");GetDlgItem(IdManual)->SetFocus();return;}
        out.size=s;finish(Keep);
    }
    void apply(){
        CString t;GetDlgItemText(IdManual,t);t.Trim();tvsDuctSizing::Size s;
        if(t.IsEmpty()||!tvsDuctSizing::parse(std::wstring(t.GetString()),in.round,s)){
            SetDlgItemText(IdManualInfo,L"izaberite iz liste ili upišite dimenziju");GetDlgItem(IdManual)->SetFocus();return;}
        out.size=s;finish(Apply);
    }
    virtual void OnCancel(){finish(Close);}
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        const int id=LOWORD(wp),code=HIWORD(wp);
        if(id==IdPrevious&&code==BN_CLICKED){finish(Previous);return TRUE;}
        if(id==IdNext&&code==BN_CLICKED){finish(Next);return TRUE;}
        if(id==IdRemove&&code==BN_CLICKED){finish(Remove);return TRUE;}
        if(id==IdApply&&code==BN_CLICKED){apply();return TRUE;}
        if((id==IdHeight||id==IdRatio)&&code==BN_CLICKED){refresh();return TRUE;}
        if(id==IdLimit&&code==EN_CHANGE&&!loading){refresh();return TRUE;}
        if(id==IdManual&&code==EN_CHANGE&&!loading){manualInfo();return TRUE;}
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    virtual BOOL OnNotify(WPARAM wp,LPARAM lp,LRESULT* res){
        NMHDR* h=reinterpret_cast<NMHDR*>(lp);
        if(h->idFrom==IdList&&h->code==LVN_ITEMCHANGED&&!loading){NMLISTVIEW* n=reinterpret_cast<NMLISTVIEW*>(lp);
            if((n->uNewState&LVIS_SELECTED)&&n->iItem>=0&&n->iItem<static_cast<int>(list.size())){
                loading=true;SetDlgItemText(IdManual,sizeText(in.round,list[n->iItem].a,list[n->iItem].b));loading=false;manualInfo();}
            return TRUE;}
        if(h->idFrom==IdList&&h->code==NM_CUSTOMDRAW){NMLVCUSTOMDRAW* d=reinterpret_cast<NMLVCUSTOMDRAW*>(lp);
            if(d->nmcd.dwDrawStage==CDDS_PREPAINT){*res=CDRF_NOTIFYITEMDRAW;return TRUE;}
            if(d->nmcd.dwDrawStage==CDDS_ITEMPREPAINT){const size_t k=static_cast<size_t>(d->nmcd.dwItemSpec);
                if(k<list.size()&&list[k].v>out.vmax+1e-9)d->clrText=RGB(150,150,150);
                else d->clrText=RGB(0,40,70);
                *res=CDRF_DODEFAULT;return TRUE;}}
        return CAdUiBaseDialog::OnNotify(wp,lp,res);
    }
    CRect du(int x,int y,int w,int h){CRect r(x,y,x+w,y+h);MapDialogRect(&r);return r;}
    void paintBack(CDC* dc){
        CRect all;GetClientRect(&all);dc->FillSolidRect(all,RGB(244,245,247));
        const CRect head=du(0,0,320,22),foot=du(0,236,320,40);
        dc->FillSolidRect(CRect(all.left,all.top,all.right,head.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,head.bottom,all.Width(),1,RGB(173,186,203));
        dc->FillSolidRect(CRect(all.left,foot.top,all.right,all.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,foot.top,all.Width(),1,RGB(173,186,203));
        dc->SetBkMode(TRANSPARENT);
        {CFont* of=dc->SelectObject(&title);dc->SetTextColor(RGB(0,26,45));CRect t=du(10,0,160,22);dc->DrawText(L"Dimenzije dionice",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(7,53,90));CRect t=du(170,0,140,22);dc->DrawText(headText,t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CRect r=du(10,28,300,24);dc->FillSolidRect(r,RGB(255,255,255));dc->FillSolidRect(r.left,r.top,3,r.Height(),nowFast?RGB(200,60,50):RGB(38,117,174));}
    }
    void drawButton(LPDRAWITEMSTRUCT it){
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);const bool down=(it->itemState&ODS_SELECTED)!=0;
        const COLORREF face=down?RGB(19,78,125):RGB(27,100,156);dc->FillSolidRect(r,face);
        CString t;GetDlgItem(IdApply)->GetWindowText(t);CFont* of=dc->SelectObject(&bold);
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(255,255,255));dc->DrawText(t,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);
        if(it->itemState&ODS_FOCUS){CRect f(r);f.DeflateRect(3,3);dc->DrawFocusRect(f);}
    }
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_ERASEBKGND){paintBack(CDC::FromHandle(reinterpret_cast<HDC>(wp)));return TRUE;}
        if(msg==WM_DRAWITEM&&wp==IdApply){drawButton(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));return TRUE;}
        if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORBTN){HWND h=reinterpret_cast<HWND>(lp);wchar_t cls[16]=L"";::GetClassNameW(h,cls,16);
            if(!_wcsicmp(cls,L"Static")||!_wcsicmp(cls,L"Button")){HDC dc=reinterpret_cast<HDC>(wp);const int id=::GetDlgCtrlID(h);::SetBkMode(dc,TRANSPARENT);
                COLORREF c=RGB(49,90,128);
                if(id==IdNow)c=nowFast?RGB(170,40,30):RGB(0,30,53);
                if(id==IdManualInfo)c=manualFast?RGB(170,40,30):RGB(0,100,60);
                if(id==IdMark)c=RGB(0,100,107);
                ::SetTextColor(dc,c);
                if(id==IdNow||id==IdListLabel||id==IdManualLabel)::SelectObject(dc,bold.GetSafeHandle());
                return reinterpret_cast<LRESULT>((id==IdNow?white:back).GetSafeHandle());}}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }
};
inline Result show(const Input& in,const Result& defaults,UINT idd){
    Dialog dg(in,defaults,idd);
    {CAcModuleResourceOverride resources;dg.DoModal();}
    return dg.out;
}
}
