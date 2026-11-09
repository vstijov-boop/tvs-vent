#pragma once
// TVS_PRESJEK dialog (IDD_SECTION), after the mockup "Podešavanje presjeka": Poprečni / Uzdužni,
// Radnja (novi / osvježi), Oznaka, Kote etaže (pod, gornja etaža, spušteni plafon od vrha
// prostorije), Razmjera (text height 0.7 mm x scale), Dubina pogleda and Zaklonjeni elementi
// (poprečni), preview of the real cut / route (geometry/SectionSketch.h). Umetni -> insertion point.
// The section line / route is picked before the dialog; switching the tab closes it (Switch) and
// the command picks the other kind.
#include "SectionCommand.h"
#include "../geometry/SectionSketch.h"
#include "SketchRender.h"
#include "adui.h"
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>
namespace tvsSectionDialog {
enum {Switch=3001};
struct Setup {
    int mode;          // 0 poprečni, 1 uzdužni
    int action;        // 0 novi presjek, 1 osvježi postojeći
    std::wstring letter;double floorM,ceilingM,spM,scale,depthM;bool spOn,table;int hidden;
    struct Row {double s,bottom;std::wstring size;};std::vector<Row> rows;   // uzduzni: table preview (drawing units)
    double f;double lengthDU;int count;   // section line length (poprečni) / developed length (uzdužni), cut ducts / route elements
    std::vector<tvsSectionSketch::Box> boxes;std::vector<std::vector<tvsTeeSketch::Pt> > bands;   // absolute z (drawing units)
    AcDbDatabase* db;
    Setup():mode(0),action(0),floorM(0),ceilingM(3),spM(0.5),scale(50),depthM(0),spOn(false),table(true),hidden(0),f(1),lengthDU(0),count(0),db(NULL){}
};
class Dialog : public CAdUiBaseDialog {
public:
    enum {IdTabCross=1401,IdTabLong=1402,IdInfo=1403,IdAction=1404,IdLetter=1405,IdFloor=1406,IdCeiling=1407,IdSpOn=1408,IdSp=1409,
          IdScale=1410,IdDepth=1411,IdHidden=1412,IdSketch=1413,IdCardTitle=1414,IdCardScale=1415,IdNote=1416,IdStatus=1417,IdDepthLabel=1418,IdHiddenLabel=1419,IdTable=1420};
    Setup s;bool valid,loading;CString problem;CFont bold,title;CBrush back,white,band,info;
    Dialog(const Setup& setup,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),s(setup),valid(false),loading(false){}

    static bool number(CWnd* w,double& v,bool allowZero){
        if(!w)return false;CString t;w->GetWindowText(t);t.Trim();t.Replace(L',',L'.');if(t.IsEmpty())return false;
        wchar_t* end=NULL;v=wcstod(t,&end);return end&&!*end&&(allowZero?v>=0:v>0)&&v<1e6;}
    static bool level(CWnd* w,double& v){
        if(!w)return false;CString t;w->GetWindowText(t);t.Trim();t.Replace(L',',L'.');if(t.IsEmpty())return false;
        wchar_t* end=NULL;v=wcstod(t,&end);return end&&!*end&&std::fabs(v)<1e5;}
    void setValue(int id,double v,int dec){if(CWnd* w=GetDlgItem(id))if(::GetFocus()==w->m_hWnd)return;
        CString t;t.Format(dec==0?L"%.0f":dec==1?L"%.1f":L"%.2f",v);SetDlgItemText(id,t);}
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    void read(){
        double v=0;CString t;
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdAction)))s.action=c->GetCurSel()==1?1:0;
        GetDlgItemText(IdLetter,t);t.Trim();s.letter=static_cast<const wchar_t*>(t);
        if(level(GetDlgItem(IdFloor),v))s.floorM=v;if(level(GetDlgItem(IdCeiling),v))s.ceilingM=v;
        s.spOn=IsDlgButtonChecked(IdSpOn)==BST_CHECKED;if(number(GetDlgItem(IdSp),v,false))s.spM=v;
        if(number(GetDlgItem(IdScale),v,false))s.scale=v;
        if(s.mode==0&&number(GetDlgItem(IdDepth),v,true))s.depthM=v;
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdHidden)))s.hidden=c->GetCurSel()==1?1:0;
        if(s.mode==1)s.table=IsDlgButtonChecked(IdTable)==BST_CHECKED;
    }
    bool exists(const std::wstring& letter)const{
        if(!s.db||letter.empty())return false;
        return tvsSectionCmd::blockExists(s.db,s.mode==0?tvsSectionCmd::blockName(letter):tvsProfileCmd::blockName(letter));}
    bool check(CString& why)const{
        if(s.action==1)return true;
        if(s.letter.empty()){why=L"Upišite oznaku presjeka.";return false;}
        if(exists(s.letter)){why.Format(L"Presjek %ls već postoji - druga oznaka ili Osvježi postojeći.",s.letter.c_str());return false;}
        if(!(s.ceilingM>s.floorM)){why=L"Gornja etaža mora biti iznad poda.";return false;}
        if(s.spOn&&!(s.spM>0&&s.spM<s.ceilingM-s.floorM)){why=L"Spušteni plafon mora biti između poda i gornje etaže.";return false;}
        if(!(s.scale>0)){why=L"Unesite razmjeru.";return false;}
        if(s.mode==1&&s.count==0){why=L"Trasa nije pronađena.";return false;}
        return true;
    }
    void refresh(){
        valid=check(problem);const bool fresh=s.action==0;
        for(int id:{IdLetter,IdFloor,IdCeiling,IdSpOn,IdScale})enable(id,fresh);
        enable(IdSp,fresh&&s.spOn);enable(IdDepth,fresh&&s.mode==0);enable(IdHidden,fresh&&s.mode==0&&s.depthM>0);enable(IdTable,fresh);
        CString t;
        if(s.mode==0)t.Format(L"Linija presjeka  ·  L = %.2f m  ·  presječeno kanala: %d",s.lengthDU*s.f/1000,s.count);
        else t.Format(L"Trasa u crtežu  ·  %d elemenata  ·  razvijena dužina %.2f m",s.count,s.lengthDU*s.f/1000);
        SetDlgItemText(IdInfo,t);
        if(s.mode==0)t.Format(L"Presjek %ls\u2013%ls",s.letter.c_str(),s.letter.c_str());else t.Format(L"Uzdužni profil %ls",s.letter.c_str());SetDlgItemText(IdCardTitle,t);
        t.Format(L"1 : %.0f",s.scale);SetDlgItemText(IdCardScale,t);
        CString note;
        if(s.mode==0){if(s.depthM>0)note.Format(L"Dubina %.1f m  ·  zaklonjeni %ls",s.depthM,s.hidden?L"isprekidano":L"izostavljeni");else note=L"Samo presječeni elementi";}
        else note=s.table?L"Prikazana tabela stacionaža, kota i dimenzija":L"Bez tabele stacionaža, kota i dimenzija";
        CString th;th.Format(L"\nVisina teksta %.0f mm (0,7 mm × 1:%.0f)",0.7*s.scale,s.scale);note+=th;
        SetDlgItemText(IdNote,note);
        SetDlgItemText(IdStatus,!valid?problem:s.action==1?CString(L"Osvježava se postojeći presjek (izbor u crtežu)"):CString(L"Pregled prije izbora tačke umetanja"));
        SetDlgItemText(IDOK,s.action==1?L"Osvježi":L"Umetni");
        enable(IDOK,valid);if(CWnd* w=GetDlgItem(IDOK))w->Invalidate();
        if(CWnd* w=GetDlgItem(IdSketch))w->Invalidate(FALSE);
    }
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        LOGFONT lf={};if(CFont* f=GetFont())f->GetLogFont(&lf);
        lf.lfWeight=FW_SEMIBOLD;bold.CreateFontIndirect(&lf);lf.lfHeight=lf.lfHeight*13/10;title.CreateFontIndirect(&lf);
        back.CreateSolidBrush(RGB(244,245,247));white.CreateSolidBrush(RGB(255,255,255));band.CreateSolidBrush(RGB(231,236,242));info.CreateSolidBrush(RGB(234,244,253));
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdAction))){c->AddString(L"Novi presjek");c->AddString(L"Osvježi postojeći");c->SetCurSel(s.action);}
        if(CComboBox* c=static_cast<CComboBox*>(GetDlgItem(IdHidden))){c->AddString(L"Izostavi");c->AddString(L"Isprekidano");c->SetCurSel(s.hidden);}
        loading=true;
        SetDlgItemText(IdLetter,s.letter.c_str());setValue(IdFloor,s.floorM,2);setValue(IdCeiling,s.ceilingM,2);
        CheckDlgButton(IdSpOn,s.spOn?BST_CHECKED:BST_UNCHECKED);CheckDlgButton(IdTable,s.table?BST_CHECKED:BST_UNCHECKED);
        for(int id:{IdDepth,IdDepthLabel,IdHidden,IdHiddenLabel})if(CWnd* w=GetDlgItem(id))w->ShowWindow(s.mode==0?SW_SHOW:SW_HIDE);   // poprecni only
        if(CWnd* w=GetDlgItem(IdTable))w->ShowWindow(s.mode==1?SW_SHOW:SW_HIDE);                                                 // uzduzni onlysetValue(IdSp,s.spM,2);setValue(IdScale,s.scale,0);setValue(IdDepth,s.depthM,1);
        loading=false;refresh();return TRUE;
    }
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        const int id=LOWORD(wp),code=HIWORD(wp);
        if(!loading){
            if((id==IdTabCross||id==IdTabLong)&&code==BN_CLICKED){if((id==IdTabLong)!=(s.mode==1)){read();EndDialog(Switch);}return TRUE;}
            if(code==EN_CHANGE||code==CBN_SELCHANGE||((id==IdSpOn||id==IdTable)&&code==BN_CLICKED)){read();refresh();return TRUE;}
            if(code==EN_KILLFOCUS){read();loading=true;
                if(id==IdFloor)setValue(id,s.floorM,2);else if(id==IdCeiling)setValue(id,s.ceilingM,2);else if(id==IdSp)setValue(id,s.spM,2);
                else if(id==IdScale)setValue(id,s.scale,0);else if(id==IdDepth)setValue(id,s.depthM,1);
                loading=false;refresh();return TRUE;}
        }
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    virtual void OnOK(){read();refresh();if(!valid)return;CAdUiBaseDialog::OnOK();}
    CRect du(int x,int y,int w,int h){CRect r(x,y,x+w,y+h);MapDialogRect(&r);return r;}
    void frame(CDC* dc,const CRect& r,COLORREF c){CPen p(PS_SOLID,1,c);CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));dc->Rectangle(r);dc->SelectObject(op);dc->SelectObject(ob);}
    void paintBack(CDC* dc){
        CRect all;GetClientRect(&all);dc->FillSolidRect(all,RGB(244,245,247));
        const CRect head=du(0,0,400,24),foot=du(0,226,400,30);
        dc->FillSolidRect(CRect(all.left,all.top,all.right,head.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,head.bottom,all.Width(),1,RGB(173,186,203));
        dc->FillSolidRect(CRect(all.left,foot.top,all.right,all.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,foot.top,all.Width(),1,RGB(173,186,203));
        dc->SetBkMode(TRANSPARENT);
        {CFont* of=dc->SelectObject(&title);dc->SetTextColor(RGB(0,26,45));CRect t=du(10,0,200,24);dc->DrawText(L"Podešavanje presjeka",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(7,53,90));CRect t=du(200,0,190,24);dc->DrawText(L"TVS_PRESJEK",t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CRect r=du(10,51,220,15);dc->FillSolidRect(r,RGB(234,244,253));dc->FillSolidRect(r.left,r.top,3,r.Height(),RGB(38,117,174));}   // info bar
        {CRect r=du(10,101,220,60);frame(dc,r,RGB(213,223,236));CFont* of=dc->SelectObject(&bold);const wchar_t* t=L"Kote etaže";CSize sz=dc->GetTextExtent(t);
         dc->FillSolidRect(CRect(r.left+8,r.top-sz.cy/2,r.left+14+sz.cx,r.top+sz.cy/2+1),RGB(244,245,247));dc->SetTextColor(RGB(0,52,89));dc->TextOut(r.left+11,r.top-sz.cy/2,t);dc->SelectObject(of);}
        CRect card=du(238,30,152,188);dc->FillSolidRect(card,RGB(255,255,255));frame(dc,card,RGB(210,222,235));
        const CRect l1=du(238,47,152,0),l2=du(238,190,152,0);dc->FillSolidRect(card.left,l1.top,card.Width(),1,RGB(210,222,235));dc->FillSolidRect(card.left,l2.top,card.Width(),1,RGB(210,222,235));
    }
    void drawButton(LPDRAWITEMSTRUCT it){
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);const int id=static_cast<int>(it->CtlID);
        const bool disabled=(it->itemState&ODS_DISABLED)!=0,down=(it->itemState&ODS_SELECTED)!=0;
        COLORREF face=RGB(255,255,255),edge=RGB(173,187,207),text=RGB(6,71,119);
        if(id==IDOK){face=disabled?RGB(160,180,200):down?RGB(19,78,125):RGB(27,100,156);edge=face;text=RGB(255,255,255);}
        else if((id==IdTabCross&&s.mode==0)||(id==IdTabLong&&s.mode==1)){face=RGB(225,239,255);edge=RGB(0,102,178);text=RGB(0,91,164);}
        else if(down)face=RGB(234,244,253);
        dc->FillSolidRect(r,face);frame(dc,r,edge);
        CString t;GetDlgItem(id)->GetWindowText(t);CFont* of=dc->SelectObject(id==IDOK||(id==IdTabCross&&s.mode==0)||(id==IdTabLong&&s.mode==1)?&bold:GetFont());
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(text);dc->DrawText(t,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);
        if(it->itemState&ODS_FOCUS){CRect f(r);f.DeflateRect(3,3);dc->DrawFocusRect(f);}
    }
    void drawSketch(LPDRAWITEMSTRUCT it){
        tvsSectionSketch::Input in;const double fl=s.floorM*1000/s.f;
        in.L=s.lengthDU;in.H=(s.ceilingM-s.floorM)*1000/s.f;in.floorM=s.floorM;in.ceilingM=s.ceilingM;
        if(s.spOn){in.sp=s.spM*1000/s.f;in.spM=s.spM;}
        for(size_t i=0;i<s.boxes.size();++i){tvsSectionSketch::Box b=s.boxes[i];b.z-=fl;in.boxes.push_back(b);}
        for(size_t i=0;i<s.bands.size();++i){std::vector<tvsTeeSketch::Pt> q=s.bands[i];for(size_t j=0;j<q.size();++j)q[j].y-=fl;in.bands.push_back(q);}
        in.lengthM=s.lengthDU*s.f/1000;
        std::vector<tvsTeeSketch::Prim> p;tvsSectionSketch::build(in,p);
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);
        if(s.mode==1&&s.table){   // picture above, table (stacionaza / kota dna / dimenzija) below, as the mockup
            const int rowH=r.Height()/8,rowsShown=static_cast<int>(s.rows.size()>3?3:s.rows.size());
            CRect pic(r.left,r.top,r.right,r.bottom-rowH*(rowsShown+1));tvsSketchRender::draw(dc,pic,p,GetFont(),&bold,1e9,6);
            CRect tab(r.left,pic.bottom,r.right,r.bottom);dc->FillSolidRect(tab,RGB(255,255,255));
            const int c1=tab.left+tab.Width()*36/100,c2=tab.left+tab.Width()*66/100;
            CFont* of=dc->SelectObject(&bold);dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(0,26,45));
            auto cell=[&](int x0,int x1,int y,const wchar_t* t){CRect c(x0+6,y,x1,y+rowH);dc->DrawText(t,-1,c,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);};
            dc->FillSolidRect(tab.left,tab.top,tab.Width(),1,RGB(210,222,235));
            cell(tab.left,c1,tab.top,L"Stacionaža [m]");cell(c1,c2,tab.top,L"Kota dna [m]");cell(c2,tab.right,tab.top,L"Dimenzija [mm]");
            dc->SelectObject(GetFont());dc->SetTextColor(RGB(49,90,128));
            std::vector<size_t> pick;if(s.rows.size()<=3)for(size_t k=0;k<s.rows.size();++k)pick.push_back(k);else{pick.push_back(0);pick.push_back(s.rows.size()/2);pick.push_back(s.rows.size()-1);}
            for(size_t k=0;k<pick.size();++k){const Setup::Row& row=s.rows[pick[k]];const int y=tab.top+rowH*static_cast<int>(k+1);
                dc->FillSolidRect(tab.left,y,tab.Width(),1,RGB(226,233,241));
                CString a,b;a.Format(L"%.2f",row.s*s.f/1000);b.Format(L"%.2f",row.bottom*s.f/1000);a.Replace(L'.',L',');b.Replace(L'.',L',');
                cell(tab.left,c1,y,a);cell(c1,c2,y,b);cell(c2,tab.right,y,row.size.c_str());}
            dc->SelectObject(of);
        }else tvsSketchRender::draw(dc,r,p,GetFont(),&bold,1e9,8);
    }
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_ERASEBKGND){paintBack(CDC::FromHandle(reinterpret_cast<HDC>(wp)));return TRUE;}
        if(msg==WM_DRAWITEM){LPDRAWITEMSTRUCT it=reinterpret_cast<LPDRAWITEMSTRUCT>(lp);
            if(wp==IdSketch){drawSketch(it);return TRUE;}
            if(wp==IdTabCross||wp==IdTabLong||wp==IDOK){drawButton(it);return TRUE;}}
        if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORBTN){HWND h=reinterpret_cast<HWND>(lp);wchar_t cls[16]=L"";::GetClassNameW(h,cls,16);
            if(!_wcsicmp(cls,L"Static")||!_wcsicmp(cls,L"Button")){HDC dc=reinterpret_cast<HDC>(wp);const int id=::GetDlgCtrlID(h);
                const bool card=id==IdCardTitle||id==IdCardScale||id==IdNote,inf=id==IdInfo;::SetBkMode(dc,TRANSPARENT);
                ::SetTextColor(dc,card?(id==IdCardTitle?RGB(0,26,45):RGB(49,90,128)):inf?RGB(0,52,89):id==IdStatus?(valid?RGB(66,104,137):RGB(197,61,61)):RGB(49,90,128));
                if(id==IdCardTitle)::SelectObject(dc,bold.GetSafeHandle());
                return reinterpret_cast<LRESULT>((card?white:inf?info:id==IdStatus?band:back).GetSafeHandle());}}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }
};
// Returns IDOK, IDCANCEL or Switch; setup keeps what was typed.
inline int show(Setup& setup,UINT idd){
    Dialog dg(setup,idd);INT_PTR rc;
    {CAcModuleResourceOverride resources;rc=dg.DoModal();}
    setup=dg.s;return static_cast<int>(rc);
}
}
// ---------------------------------------------------------------- TVS_PRESJEK with the dialog
namespace tvsSectionDialog {
inline AcGePoint3d wcs(const ads_point p){resbuf from={};from.restype=RTSHORT;from.resval.rint=1;resbuf to={};to.restype=RTSHORT;to.resval.rint=0;
    ads_point w;acedTrans(p,&from,&to,0,w);return asPnt3d(w);}
inline void remember(const Setup& s){tvsSectionCmd::Remembered& m=tvsSectionCmd::remembered();
    m.floorM=s.floorM;m.ceilingM=s.ceilingM;m.scale=s.scale;m.spM=s.spM;m.spOn=s.spOn;if(s.mode==0){m.hidden=s.hidden;m.depthM=s.depthM;}else m.table=s.table;}
inline void fill(Setup& s,AcDbDatabase* db,int mode){tvsSectionCmd::Remembered& m=tvsSectionCmd::remembered();
    s.mode=mode;s.db=db;s.f=tvsUnits::factor(db);s.floorM=m.floorM;s.ceilingM=m.ceilingM;s.scale=m.scale;s.spM=m.spM;s.spOn=m.spOn;s.hidden=m.hidden;s.table=m.table;}
inline bool insertion(const wchar_t* prompt,AcGePoint3d& at){ads_point ins;if(acedGetPoint(NULL,prompt,ins)!=RTNORM)return false;at=wcs(ins);return true;}
inline void longitudinal(UINT idd);
inline void cross(UINT idd,const ads_point* first){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;const double f=tvsUnits::factor(db);
    ads_point a,b;
    if(first){a[0]=(*first)[0];a[1]=(*first)[1];a[2]=(*first)[2];}
    else if(acedGetPoint(NULL,L"\nPrva tačka linije presjeka: ",a)!=RTNORM)return;
    if(acedGetPoint(a,L"\nDruga tačka linije presjeka: ",b)!=RTNORM)return;
    tvsSectionCmd::Params p;p.p1=wcs(a);p.p2=wcs(b);p.p1.z=p.p2.z=0;
    if(AcGeVector3d(p.p2.x-p.p1.x,p.p2.y-p.p1.y,0).length()<1e-9){acutPrintf(L"\nLinija presjeka je prekratka.");return;}
    {   // side only (click on the side to look at); the depth is the dialog value (remembered), not dragged
        const double depth=tvsSectionCmd::remembered().depthM*1000/f;
        tvsSectionCmd::DepthJig jig(p.p1,p.p2,depth>0?depth:p.p1.distanceTo(p.p2)*0.05);
        jig.setDispPrompt(L"\nStrana gledanja presjeka (kliknite na stranu): ");
        if(jig.drag()!=AcEdJig::kNormal)return;p.flip=jig.flip;p.depth=depth;}
    Setup s;fill(s,db,0);s.letter=tvsSectionCmd::nextLetter(db);s.depthM=p.depth*f/1000;s.lengthDU=p.p1.distanceTo(p.p2);
    {std::vector<tvsSection::Member> net;tvsSectionCmd::members(db,net);
     const std::vector<tvsSection::Cut> cuts=tvsSection::cuts(tvsSection::P(p.p1.x,p.p1.y),tvsSection::P(p.p2.x,p.p2.y),net,p.flip,2.0/f);
     for(size_t i=0;i<cuts.size();++i){tvsSectionSketch::Box x;x.s=cuts[i].s;x.z=cuts[i].z;x.w=cuts[i].w;x.h=cuts[i].h;x.round=cuts[i].round;s.boxes.push_back(x);}
     s.count=static_cast<int>(cuts.size());}
    const int rc=show(s,idd);remember(s);
    if(rc==Switch){longitudinal(idd);return;}
    if(rc!=IDOK)return;
    if(s.action==1){tvsSectionCmd::refresh();return;}
    p.letter=s.letter;p.floor=s.floorM*1000/f;p.ceiling=s.ceilingM*1000/f;p.scale=s.scale;p.depth=s.depthM*1000/f;p.hidden=s.hidden;p.sp=s.spOn?s.spM*1000/f:0;
    AcGePoint3d position;if(!insertion(L"\nTačka umetanja presjeka (lijevi kraj, kota poda): ",position))return;
    AcDbObjectId btrId;
    {AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForWrite)!=Acad::eOk)return;
     AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
     AcDbBlockTableRecord* btr=new AcDbBlockTableRecord();btr->setName(tvsSectionCmd::blockName(p.letter).c_str());
     if(table->add(btrId,btr)!=Acad::eOk){delete btr;acutPrintf(L"\nBlok presjeka nije napravljen.");return;}
     const int n=tvsSectionCmd::build(btr,p);tvsSectionCmd::store(btr,p);btr->close();
     acutPrintf(L"\nPresjek %ls-%ls: presječenih elemenata %d.",p.letter.c_str(),p.letter.c_str(),n);}
    {AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk)return;
     tvsSectionCmd::append(space.object(),new AcDbBlockReference(position,btrId),tvsSectionCmd::layer(db));}
    tvsSectionCmd::planMarks(db,p);
}
inline void longitudinal(UINT idd){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;const double f=tvsUnits::factor(db),tol=2.0/f;
    tvsProfileCmd::Params p;
    if(!tvsProfileCmd::pick(L"\nPrvi element trase (početak profila): ",p.first))return;
    if(!tvsProfileCmd::pick(L"\nZadnji element trase (kraj profila): ",p.last))return;
    std::vector<tvsSection::Member> net;std::vector<int> tags;std::vector<AcDbObjectId> ids;
    if(!tvsProfileCmd::collect(db,p,net,tags,ids,tol))return;
    const std::vector<tvsProfile::Piece> pieces=tvsProfile::develop(net,tags,tol);
    if(pieces.empty()){acutPrintf(L"\nUzdužni presjek: trasa nema dužinu.");return;}
    tvsMove::StretchHighlight route;for(size_t k=0;k<tags.size();++k)route.add(ids[tags[k]-1]);
    Setup s;fill(s,db,1);s.letter=tvsProfileCmd::nextLetter(db);s.lengthDU=pieces.back().s1;s.count=static_cast<int>(tags.size());
    for(size_t i=0;i<pieces.size();++i){const tvsProfile::Piece& q=pieces[i];if(!(q.s1>q.s0))continue;
        std::vector<tvsTeeSketch::Pt> band;band.push_back(tvsTeeSketch::Pt(q.s0,q.z0+q.h0/2));band.push_back(tvsTeeSketch::Pt(q.s1,q.z1+q.h1/2));
        band.push_back(tvsTeeSketch::Pt(q.s1,q.z1-q.h1/2));band.push_back(tvsTeeSketch::Pt(q.s0,q.z0-q.h0/2));s.bands.push_back(band);}
    {const std::vector<double> st=tvsProfile::stations(pieces,0.7*tvsSectionCmd::remembered().scale*1.8/f);
     for(size_t k=0;k<st.size();++k){Setup::Row row;row.s=st[k];row.bottom=0;tvsProfile::bottomAt(pieces,st[k],tol,row.bottom);
        for(size_t i=0;i<pieces.size();++i){const tvsProfile::Piece& q=pieces[i];if(st[k]>=q.s0-tol&&st[k]<=q.s1+tol&&q.kind!=tvsProfile::Taper){
            wchar_t t[40];if(q.round)swprintf_s(t,L"\u00D8%.0f",q.A0*f);else swprintf_s(t,L"%.0f \u00D7 %.0f",q.A0*f,q.B0*f);row.size=t;break;}}
        s.rows.push_back(row);}}
    const int rc=show(s,idd);remember(s);
    if(rc==Switch){route.clear();cross(idd,NULL);return;}
    if(rc!=IDOK)return;
    route.clear();
    if(s.action==1){tvsSectionCmd::refresh();return;}
    p.letter=s.letter;p.floor=s.floorM*1000/f;p.ceiling=s.ceilingM*1000/f;p.scale=s.scale;p.sp=s.spOn?s.spM*1000/f:0;p.table=s.table?1:0;
    AcGePoint3d position;if(!insertion(L"\nTačka umetanja profila (početak trase, kota poda): ",position))return;
    AcDbObjectId btrId;
    {AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForWrite)!=Acad::eOk)return;
     AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
     AcDbBlockTableRecord* btr=new AcDbBlockTableRecord();btr->setName(tvsProfileCmd::blockName(p.letter).c_str());
     if(table->add(btrId,btr)!=Acad::eOk){delete btr;acutPrintf(L"\nBlok uzdužnog presjeka nije napravljen.");return;}
     const int n=tvsProfileCmd::build(btr,p);tvsProfileCmd::store(btr,p);btr->close();
     acutPrintf(L"\nUzdužni presjek %ls-%ls: elemenata trase %d.",p.letter.c_str(),p.letter.c_str(),n);}
    {AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk)return;
     tvsSectionCmd::append(space.object(),new AcDbBlockReference(position,btrId),tvsSectionCmd::layer(db));}
    tvsProfileCmd::planMarks(db,p,pieces);
}
inline void run(UINT idd){
    ads_point a;acedInitGet(0,L"Osvjezi Uzduzni");
    const int rc=acedGetPoint(NULL,L"\nPrva tačka linije presjeka ili [Osvjezi/Uzduzni]: ",a);
    if(rc==RTKWORD){ACHAR kw[32]=L"";acedGetInput(kw);if(_wcsicmp(kw,L"Uzduzni")==0)longitudinal(idd);else tvsSectionCmd::refresh();return;}
    if(rc!=RTNORM)return;
    cross(idd,&a);
}
}
