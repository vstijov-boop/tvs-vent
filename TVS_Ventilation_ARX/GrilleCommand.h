#pragma once
// TVS_RESETKA (priključenje rešetke, korisnik 2026-11-23): crta priključak rešetke koji se danas
// crta ručno i povezuje blok rešetke u mrežu.
//  1. blok rešetke (jedan; izbor prije komande važi), 2. kanal (podrazumijevano: kanal čija je osa
//     najbliža tački umetanja bloka u osnovi), 3. dijalog IDD_GRILLE: strana (Lijevo / Desno / Dno),
//     način (ravni T komad TK0 / štucna / konusna štucna / direktno na stranicu), dužina ravnog dijela
//     ogranka (pamti se u HKCU\Software\TVS_Vent, ResetkaOgranak), dimenzija ogranka.
//  - Lijevo / Desno: T komad na kanalu u projekciji tačke umetanja (kanal se dijeli: original do
//    ulaza T komada, novi komad od izlaza), ravni kanal ogranka zadate dužine, blok pomjeren i
//    okrenut tako da mu je bazna tačka na slobodnom kraju ogranka (lokalna -y osa bloka gleda
//    napolje, geometry/GrilleConnect.h); zapis: 1 priključak na kraju ogranka (mount AtEnd).
//  - Dno + direktno: ništa se ne crta; zapis mount Bottom, priključak = projekcija sredine bloka na
//    osu kanala, dimenzija priključka = dimenzija iz dijaloga.
// Geometrija i provjera: geometry/GrilleConnect.h (test tests/test_grille_connect.cpp). Sve u jednoj
// transakciji (tvsAction::Transaction) poslije provjere upisivosti (tvsAction::writable): jedan UNDO
// korak, sve ili ništa. Vrsta / model / opis iz ranijeg zapisa bloka ili iz podrazumijevanog zapisa
// za naziv bloka (tvsEquipmentStore::readDefault), inače Rešetka.
#include "EquipmentLinkCommand.h"
#include "TeeNeighbours.h"
#include "ActionTransaction.h"
#include "EquipmentStore.h"
#include "DecimalText.h"
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../geometry/GrilleConnect.h"
#include "../geometry/NumericField.h"
#include "adui.h"
#include <dbents.h>
#include <windows.h>
#include <cmath>
#include <string>
#include <vector>
namespace tvsGrilleCmd {
namespace gc=tvsGrille;
// ---- the stub length, remembered for the user (HKCU\Software\TVS_Vent, ResetkaOgranak, mm) ----
inline const wchar_t* regKey(){return L"Software\\TVS_Vent";}
inline const wchar_t* regValue(){return L"ResetkaOgranak";}
inline double stubDefaultMm(){return 100;}
inline double storedStubMm(){
    HKEY k=NULL;if(RegOpenKeyExW(HKEY_CURRENT_USER,regKey(),0,KEY_READ,&k)!=ERROR_SUCCESS)return stubDefaultMm();
    wchar_t v[64]={0};DWORD size=sizeof(v)-sizeof(wchar_t),type=0;
    const bool ok=RegQueryValueExW(k,regValue(),NULL,&type,reinterpret_cast<LPBYTE>(v),&size)==ERROR_SUCCESS&&type==REG_SZ;
    RegCloseKey(k);double mm=0;
    return ok&&tvsNumeric::parse(v,mm)&&mm>0&&mm<100000?mm:stubDefaultMm();
}
inline void storeStubMm(double mm){
    HKEY k=NULL;if(RegCreateKeyExW(HKEY_CURRENT_USER,regKey(),0,NULL,0,KEY_WRITE,NULL,&k,NULL)!=ERROR_SUCCESS)return;
    const std::wstring s=tvsNumeric::formatG(mm,10);
    RegSetValueExW(k,regValue(),0,REG_SZ,reinterpret_cast<const BYTE*>(s.c_str()),static_cast<DWORD>((s.size()+1)*sizeof(wchar_t)));RegCloseKey(k);
}
inline double roundMm(double v){return std::floor(v+0.5);}

class Dialog : public CAdUiBaseDialog {
public:
    enum {IdInfo=1801,IdLeft=1802,IdRight=1803,IdBottom=1804,IdTee=1805,IdTap=1806,IdCone=1807,IdDirect=1808,
          IdNote=1809,IdLengthLabel=1810,IdLength=1811,IdALabel=1812,IdA=1813,IdX=1814,IdBLabel=1815,IdB=1816,
          IdRound=1817,IdFacts=1818,IdStatus=1819,IdEnd=1820};
    // in / out
    int side,method;double lengthMm,aMm,bMm;bool round;
    int end;double stubMm,endLengthMm;int shownSide;   // End: which free end; stored stub / current end duct length (mm)
    // context of the check (drawing units)
    gc::Duct duct;gc::Place atInsert,atCentre;gc::Options options;double f;bool mainRound;
    CString banner,facts;bool loading;CFont bold,title;CBrush back,band,white;
    explicit Dialog(UINT idd):CAdUiBaseDialog(idd,NULL,NULL),side(gc::Left),method(gc::Tee),lengthMm(100),aMm(0),bMm(0),round(false),
        end(-1),stubMm(100),endLengthMm(0),shownSide(-1),f(1),mainRound(false),loading(false){}
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    static int sideId(int k){return k==gc::End?IdEnd:IdLeft+k;}
    int sideSel(){for(int k=gc::Left;k<=gc::End;++k)if(IsDlgButtonChecked(sideId(k))==BST_CHECKED)return k;return gc::Left;}
    void checkSide(int s){for(int k=gc::Left;k<=gc::End;++k)CheckDlgButton(sideId(k),k==s?BST_CHECKED:BST_UNCHECKED);}
    int methodSel(){for(int k=gc::Tee;k<=gc::Direct;++k)if(IsDlgButtonChecked(IdTee+k)==BST_CHECKED)return k;return gc::Tee;}
    bool roundSel(){return IsDlgButtonChecked(IdRound)==BST_CHECKED;}
    // Methods that fit the side and the shapes; the chosen one moves to a valid one.
    void refresh(){
        const int s=sideSel();const bool bottom=s==gc::Bottom,atEnd=s==gc::End,r=roundSel();
        enable(IdEnd,end>=0);
        // the one length field: stub (Lijevo / Desno) or the end duct length (Kraj)
        if(s!=shownSide){
            if(shownSide>=0&&!loading){CString t;GetDlgItemText(IdLength,t);double v=0;if(tvsNumeric::parse(std::wstring(t.GetString()),v)){if(shownSide==gc::End)endLengthMm=v;else stubMm=v;}}
            CString t;t.Format(L"%.0f",atEnd?endLengthMm:stubMm);SetDlgItemText(IdLength,t);shownSide=s;}
        SetDlgItemText(IdLengthLabel,atEnd?L"Dužina kanala do rešetke [mm]":L"Dužina ravnog dijela ogranka [mm]");
        enable(IdTee,!bottom&&!atEnd);enable(IdTap,!bottom&&!atEnd&&!mainRound);enable(IdCone,!bottom&&!atEnd&&!mainRound&&r);enable(IdDirect,bottom||atEnd);
        int m=methodSel();
        if(bottom||atEnd)m=gc::Direct;
        else{if(m==gc::Direct)m=gc::Tee;if(m==gc::Cone&&!r)m=gc::Tap;if((m==gc::Tap||m==gc::Cone)&&mainRound)m=gc::Tee;}
        CheckRadioButton(IdTee,IdDirect,IdTee+m);
        enable(IdLength,!bottom);enable(IdLengthLabel,!bottom);
        enable(IdB,!r);enable(IdBLabel,!r);enable(IdX,!r);
        SetDlgItemText(IdALabel,r?L"D [mm]":L"A [mm]");
        SetDlgItemText(IdNote,atEnd?L"Kraj kanala: rešetka na slobodnom kraju, bez T komada. Kanal do rešetke se postavlja na zadatu dužinu od drugog kraja (npr. 100 mm regulator rešetke, 50 mm bez regulatora); blok se postavlja na kraj, okrenut prema van."
                           :bottom?L"Dno: ogranak naniže (T komad / štucna na dnu kanala) za sada nije podržan – mreža je horizontalna. Rešetka se povezuje direktno na stranicu kanala, ništa se ne crta."
                           :mainRound?L"Okrugli kanal: samo ravni T komad (štucna je za pravougaoni kanal)."
                           :L"Štucna: L = 0,25 × širina ogranka (najmanje 75 mm), kosi ulaz uzvodno (u smjeru crtanja kanala). Konusna: L = 100 mm, samo okrugli ogranak.");
        SetDlgItemText(IdStatus,atEnd?L"Kanal do rešetke + blok na kraju, jedan UNDO korak":bottom?L"Upisuje se u blok (rešetka na dnu kanala), jedan UNDO korak"
                                      :L"T komad + ogranak + blok na kraju ogranka, jedan UNDO korak");
    }
    virtual void DoDataExchange(CDataExchange* dx){
        CAdUiBaseDialog::DoDataExchange(dx);
        tvsDecimal::DDX_Real(dx,IdLength,lengthMm);tvsDecimal::DDX_Real(dx,IdA,aMm);tvsDecimal::DDX_Real(dx,IdB,bMm);
    }
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        LOGFONT lf={};if(CFont* fn=GetFont())fn->GetLogFont(&lf);
        lf.lfWeight=FW_SEMIBOLD;bold.CreateFontIndirect(&lf);lf.lfHeight=lf.lfHeight*13/10;title.CreateFontIndirect(&lf);
        back.CreateSolidBrush(RGB(244,245,247));band.CreateSolidBrush(RGB(231,236,242));white.CreateSolidBrush(RGB(255,255,255));
        SetWindowText(L"TVS_VENT – priključenje rešetke");
        loading=true;
        checkSide(side);CheckRadioButton(IdTee,IdDirect,IdTee+method);
        CheckDlgButton(IdRound,round?BST_CHECKED:BST_UNCHECKED);
        SetDlgItemText(IdInfo,banner);SetDlgItemText(IdFacts,facts);
        loading=false;refresh();
        GetDlgItem(sideId(side))->SetFocus();return FALSE;
    }
    virtual void OnOK(){
        if(!UpdateData(TRUE))return;
        gc::Choice c;c.side=sideSel();c.method=methodSel();c.round=roundSel();
        c.stub=lengthMm/f;c.a=aMm/f;c.b=c.round?0:bMm/f;c.end=end;
        const gc::Check k=gc::validate(duct,c.side==gc::Bottom?atCentre:atInsert,c,options);
        if(k.error!=gc::Ok){
            CString m;m.Format(L"Priključenje nije moguće: %ls",gc::errorText(k.error));
            if(k.detail>0){CString d;
                if(k.error==gc::TooTall)d.Format(L" (stranica kanala %.0f mm).",k.detail*f);
                else if(k.error==gc::TooWide)d.Format(L" (širina kanala %.0f mm).",k.detail*f);
                else if(k.error==gc::NearEnd)d.Format(L" (potrebno najmanje %.0f mm do kraja kanala).",k.detail*f);
                else if(k.error==gc::Outside)d.Format(L" (%.0f mm iza kraja kanala).",k.detail*f);
                m+=d;}
            else m+=L".";
            AfxMessageBox(m,MB_ICONEXCLAMATION);return;
        }
        side=c.side;method=c.method;round=c.round;
        EndDialog(IDOK);
    }
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        const int id=LOWORD(wp),code=HIWORD(wp);
        if(!loading&&code==BN_CLICKED&&((id>=IdLeft&&id<=IdDirect)||id==IdRound||id==IdEnd)){refresh();}
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    CRect du(int x,int y,int w,int h){CRect r(x,y,x+w,y+h);MapDialogRect(&r);return r;}
    void frame(CDC* dc,const CRect& r,COLORREF col,const wchar_t* label){
        CPen p(PS_SOLID,1,col);CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));dc->Rectangle(r);dc->SelectObject(op);dc->SelectObject(ob);
        CFont* of=dc->SelectObject(&bold);CSize sz=dc->GetTextExtent(label);
        dc->FillSolidRect(CRect(r.left+8,r.top-sz.cy/2,r.left+14+sz.cx,r.top+sz.cy/2+1),RGB(244,245,247));dc->SetTextColor(RGB(0,52,89));dc->TextOut(r.left+11,r.top-sz.cy/2,label);dc->SelectObject(of);
    }
    void paintBack(CDC* dc){
        CRect all;GetClientRect(&all);dc->FillSolidRect(all,RGB(244,245,247));
        const CRect head=du(0,0,320,22),foot=du(0,226,320,30);
        dc->FillSolidRect(CRect(all.left,all.top,all.right,head.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,head.bottom,all.Width(),1,RGB(173,186,203));
        dc->FillSolidRect(CRect(all.left,foot.top,all.right,all.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,foot.top,all.Width(),1,RGB(173,186,203));
        dc->SetBkMode(TRANSPARENT);
        {CFont* of=dc->SelectObject(&title);dc->SetTextColor(RGB(0,26,45));CRect t=du(10,0,160,22);dc->DrawText(L"Priključenje rešetke",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(7,53,90));CRect t=du(170,0,140,22);dc->DrawText(L"Oprema na mreži  ·  TVS_RESETKA",t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CRect r=du(10,28,300,15);dc->FillSolidRect(r,RGB(255,255,255));dc->FillSolidRect(r.left,r.top,3,r.Height(),RGB(38,117,174));}   // banner
        frame(dc,du(10,54,300,22),RGB(213,223,236),L"Strana");
        frame(dc,du(10,86,300,62),RGB(213,223,236),L"Način priključenja");
    }
    void drawButton(LPDRAWITEMSTRUCT it){
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);const bool down=(it->itemState&ODS_SELECTED)!=0;
        const COLORREF face=down?RGB(19,78,125):RGB(27,100,156);dc->FillSolidRect(r,face);
        CString t;GetDlgItem(IDOK)->GetWindowText(t);CFont* of=dc->SelectObject(&bold);
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(255,255,255));dc->DrawText(t,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);
        if(it->itemState&ODS_FOCUS){CRect fr(r);fr.DeflateRect(3,3);dc->DrawFocusRect(fr);}
    }
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_ERASEBKGND){paintBack(CDC::FromHandle(reinterpret_cast<HDC>(wp)));return TRUE;}
        if(msg==WM_DRAWITEM&&wp==IDOK){drawButton(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));return TRUE;}
        if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORBTN){HWND h=reinterpret_cast<HWND>(lp);wchar_t cls[16]=L"";::GetClassNameW(h,cls,16);
            if(!_wcsicmp(cls,L"Static")||!_wcsicmp(cls,L"Button")){HDC dc=reinterpret_cast<HDC>(wp);const int id=::GetDlgCtrlID(h);::SetBkMode(dc,TRANSPARENT);
                ::SetTextColor(dc,id==IdInfo?RGB(0,30,53):id==IdStatus?RGB(0,100,107):id==IdNote?RGB(120,72,0):RGB(49,90,128));
                if(id==IdInfo)::SelectObject(dc,bold.GetSafeHandle());
                return reinterpret_cast<LRESULT>((id==IdInfo?white:id==IdStatus?band:back).GetSafeHandle());}}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }
};

// ---- drawing side ----
inline gc::V v2(const AcGePoint3d& p){return gc::V(p.x,p.y);}
// One block reference of the current space: preselected (exactly one) or picked.
inline bool pickBlock(AcDbDatabase* db,AcDbObjectId& out){
    resbuf* filter=acutBuildList(RTDXF0,L"INSERT",RTNONE);ads_name ss;
    if(acedSSGet(L"_I",NULL,NULL,filter,ss)==RTNORM){
        Adesk::Int32 n=0;acedSSLength(ss,&n);ads_name e;AcDbObjectId id;
        if(n==1&&acedSSName(ss,0,e)==RTNORM&&acdbGetObjectId(id,e)==Acad::eOk)out=id;
        else if(n>1)acutPrintf(L"\nTVS_RESETKA: izabrano je više blokova – izaberite jedan blok rešetke.");
        acedSSFree(ss);acedSSSetFirst(NULL,NULL);
    }
    acutRelRb(filter);
    while(out.isNull()){
        ads_name e;ads_point pt;const int rc=acedEntSel(L"\nIzaberite blok rešetke: ",e,pt);
        if(rc==RTCAN)return false;
        if(rc!=RTNORM){resbuf rb={};if(acedGetVar(L"ERRNO",&rb)==RTNORM&&rb.restype==RTSHORT&&rb.resval.rint==52)return false;continue;}   // Enter: kraj
        AcDbObjectId id;if(acdbGetObjectId(id,e)!=Acad::eOk)continue;
        if(id.objectClass()!=AcDbBlockReference::desc()){acutPrintf(L"\nIzabrani objekat nije blok.");continue;}
        out=id;
    }
    if(out.objectClass()!=AcDbBlockReference::desc()){acutPrintf(L"\nTVS_RESETKA: izabrani objekat nije blok.");return false;}
    (void)db;return true;
}
// The duct: picked, or the one whose axis is nearest to the insertion point (Enter / Najblizi).
inline bool pickDuct(const AcDbObjectId& nearest,AcDbObjectId& out){
    for(;;){
        tvsMove::StretchHighlight mark;if(!nearest.isNull())mark.add(nearest);
        ads_name e;ads_point pt;int rc;
        if(!nearest.isNull()){acedInitGet(0,L"Najblizi");rc=acedEntSel(L"\nIzaberite kanal ili [Najblizi] <Najblizi>: ",e,pt);}
        else rc=acedEntSel(L"\nIzaberite kanal: ",e,pt);
        if(rc==RTCAN)return false;
        if(rc==RTKWORD||rc==RTNONE){if(nearest.isNull())return false;out=nearest;return true;}
        if(rc!=RTNORM){
            resbuf rb={};const bool enter=acedGetVar(L"ERRNO",&rb)==RTNORM&&rb.restype==RTSHORT&&rb.resval.rint==52;
            if(enter){if(nearest.isNull())return false;out=nearest;return true;}
            continue;}   // promašaj: ponovo
        AcDbObjectId id;if(acdbGetObjectId(id,e)!=Acad::eOk)continue;
        if(id.objectClass()!=TVS_Pipe::desc()){acutPrintf(L"\nIzaberite TVS kanal (pravi kanal).");continue;}
        out=id;return true;
    }
}
inline void execute(UINT idd,double collar){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db);
    AcDbObjectId blockId;if(!pickBlock(db,blockId))return;
    // ---- the block ----
    std::wstring name;AcGePoint3d ins;std::vector<tvsEquipmentLink::Pt> foot;tvsEquipment::Record old;bool wasLinked=false;AcGeVector3d localDown;
    {AcDbObjectPointer<AcDbBlockReference> ref(blockId,AcDb::kForRead);if(ref.openStatus()!=Acad::eOk){acutPrintf(L"\nTVS_RESETKA: blok se ne može otvoriti.");return;}
     if(ref->ownerId()!=db->currentSpaceId()){acutPrintf(L"\nTVS_RESETKA: blok nije u trenutnom prostoru crteža.");return;}
     {AcDbBlockTableRecordPointer btr(ref->blockTableRecord(),AcDb::kForRead);
      if(btr.openStatus()!=Acad::eOk||btr->isFromExternalReference()||btr->isLayout()){acutPrintf(L"\nTVS_RESETKA: xref / layout blok se ne priključuje.");return;}}
     if(std::fabs(std::fabs(ref->normal().z)-1)>1e-6){acutPrintf(L"\nTVS_RESETKA: blok nije u osnovi (3D rotacija).");return;}
     name=tvsEquipmentLinkCmd::blockName(blockId,ref->blockTableRecord());
     ins=ref->position();
     if(!tvsEquipmentLinkCmd::footprint(ref.object(),foot)||foot.size()!=4){acutPrintf(L"\nTVS_RESETKA: blok nema geometriju.");return;}
     wasLinked=tvsEquipmentStore::read(ref.object(),old);
     localDown=AcGeVector3d(0,-1,0);localDown.transformBy(ref->blockTransform());localDown.z=0;}
    if(localDown.length()<1e-9){acutPrintf(L"\nTVS_RESETKA: blok nema smjer u osnovi.");return;}
    tvsEquipment::Record def;const bool hasDefault=tvsEquipmentStore::readDefault(db,name.c_str(),def);
    tvsEquipment::Record rec=wasLinked?old:hasDefault?def:tvsEquipment::Record();
    if((wasLinked||hasDefault)&&!tvsEquipment::terminalKind(rec.kind)&&rec.kind!=tvsEquipment::Other){
        acutPrintf(L"\nTVS_RESETKA: blok „%ls“ je zapisan kao %ls (oprema u liniji), ne kao krajnji element. Promijenite vrstu (TVS_OPREMA_SVOJSTVA).",name.c_str(),tvsEquipment::kindName(rec.kind));return;}
    if(!(wasLinked||hasDefault))rec.kind=tvsEquipment::Grille;
    // ---- the duct ----
    tvsEquipmentLinkCmd::Scene scene;if(!tvsEquipmentLinkCmd::readScene(db,scene)){acutPrintf(L"\nTVS_RESETKA: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    AcDbObjectId nearest;double best=-1;const tvsEquipmentLink::Pt at(ins.x,ins.y);
    for(size_t k=0;k<scene.nodes.size();++k){const tvsMove::Node& n=scene.nodes[k];if(!tvsEquipmentLinkCmd::isDuct(n)||n.flex)continue;
        const double dist=tvsEquipmentLinkCmd::segmentDistance(at,tvsEquipmentLink::Pt(n.ports[0].x,n.ports[0].y),tvsEquipmentLink::Pt(n.ports[1].x,n.ports[1].y));
        if(best<0||dist<best){best=dist;nearest=n.id;}}
    AcDbObjectId ductId;if(!pickDuct(nearest,ductId))return;
    AcGePoint3d first,last;double A=0,B=0,elev=0;bool roundDuct=false;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()!=Acad::eOk){acutPrintf(L"\nTVS_RESETKA: kanal se ne može otvoriti.");return;}
     if(d->isA()!=TVS_Pipe::desc()||d->getDuctType()==DuctTypeFlex){acutPrintf(L"\nTVS_RESETKA: izaberite pravi TVS kanal (ne flex).");return;}
     if(d->ownerId()!=db->currentSpaceId()){acutPrintf(L"\nTVS_RESETKA: kanal nije u trenutnom prostoru crteža.");return;}
     first=d->FirstPoint;last=d->LastPoint;A=d->SizeA;B=d->SizeB;elev=d->Elev;roundDuct=d->ThisRound||d->SizeB<=0;}
    if(tvsEquipmentLinkCmd::inAssembly(ductId)){acutPrintf(L"\nTVS_RESETKA: kanal je dio etaža / obilaska.");return;}
    gc::Duct duct;duct.p0=v2(first);duct.p1=v2(last);duct.a=A;duct.b=roundDuct?0:B;duct.round=roundDuct;
    {   // free ends: no other element (TVS port, linked block port other than this block) on them
        const double tolEnd=1.0/f;const gc::V ends[2]={duct.p0,duct.p1};
        for(int e=0;e<2;++e){bool taken=false;
            for(size_t k=0;k<scene.nodes.size()&&!taken;++k){const tvsMove::Node& n=scene.nodes[k];if(n.id==ductId)continue;
                for(size_t q=0;q<n.ports.size();++q)if(std::hypot(n.ports[q].x-ends[e].x,n.ports[q].y-ends[e].y)<=tolEnd){taken=true;break;}}
            for(size_t q=0;q<scene.equipment.size()&&!taken;++q)
                if(scene.equipment[q].id!=blockId&&std::hypot(scene.equipment[q].at.x-ends[e].x,scene.equipment[q].at.y-ends[e].y)<=tolEnd)taken=true;
            duct.freeEnd[e]=!taken;}
    }
    if(!(duct.len()>0)){acutPrintf(L"\nTVS_RESETKA: kanal nema dužinu u osnovi.");return;}
    gc::V centre;for(size_t k=0;k<foot.size();++k)centre=gc::add(centre,gc::V(foot[k].x,foot[k].y));centre=gc::mul(centre,1.0/foot.size());
    const gc::Place pi=gc::place(duct,v2(ins)),pc=gc::place(duct,centre);
    gc::Options o;o.collar=collar>0?collar:50/f;o.tapMinimum=tvsTeeData::tapMinimumMm()/f;o.coneLength=tvsTee::coneLengthMm()/f;
    o.margin=tvsTee::tapMarginMm()/f;o.minPiece=50/f;
    // ---- dialog ----
    Dialog dg(idd);
    dg.end=gc::nearFreeEnd(duct,v2(ins),A+150/f);   // grille by a free end: Kraj kanala (user 2026-11-23)
    dg.side=dg.end>=0?gc::End:pc.side==gc::Bottom||pi.side==gc::Bottom?gc::Bottom:pi.side;
    dg.method=dg.side==gc::Bottom||dg.side==gc::End?gc::Direct:gc::Tee;dg.stubMm=storedStubMm();dg.endLengthMm=roundMm(duct.len()*f);
    dg.lengthMm=dg.side==gc::End?dg.endLengthMm:dg.stubMm;
    // size: neck of the record, else the default of the block name, else the block footprint
    const double footW=std::hypot(foot[1].x-foot[0].x,foot[1].y-foot[0].y)*f,footD=std::hypot(foot[3].x-foot[0].x,foot[3].y-foot[0].y)*f;
    if(wasLinked&&old.neckA>0){dg.aMm=old.neckA;dg.bMm=old.neckB;dg.round=old.neckRound;}
    else if(hasDefault&&def.neckA>0){dg.aMm=def.neckA;dg.bMm=def.neckB;dg.round=def.neckRound;}
    else if(hasDefault&&def.a>0){dg.aMm=def.a;dg.bMm=def.b;dg.round=def.round;}
    else{dg.aMm=roundMm(footW);dg.bMm=dg.side==gc::Bottom?roundMm(footD):roundMm((roundDuct?A:B)*f);dg.round=false;}
    if(dg.round||!(dg.bMm>0))dg.bMm=roundMm((roundDuct?A:B)*f);
    dg.duct=duct;dg.atInsert=pi;dg.atCentre=pc;dg.options=o;dg.f=f;dg.mainRound=roundDuct;
    CString size;if(roundDuct)size.Format(L"Ø%.0f",A*f);else size.Format(L"%.0f×%.0f",A*f,B*f);
    dg.banner.Format(L"Blok „%ls“  ·  kanal %ls mm",name.c_str(),size.GetString());
    dg.facts.Format(L"Kanal dužine %.0f mm. Tačka priključenja (projekcija tačke umetanja) %.0f mm od početka kanala; blok %.0f mm %ls ose.%ls",
        duct.len()*f,pi.t*f,std::fabs(pi.offset)*f,pi.offset>=0?L"lijevo od":L"desno od",wasLinked?L"\nBlok je već povezan – zapis se zamjenjuje.":L"");
    INT_PTR rc;
    {CAcModuleResourceOverride resources;rc=dg.DoModal();}
    if(rc!=IDOK)return;
    const bool bottom=dg.side==gc::Bottom,atEnd=dg.side==gc::End;
    if(!bottom&&!atEnd)storeStubMm(dg.lengthMm);
    gc::Choice c;c.side=dg.side;c.method=dg.method;c.round=dg.round;c.stub=dg.lengthMm/f;c.a=dg.aMm/f;c.b=dg.round?0:dg.bMm/f;c.end=dg.end;
    const gc::Place& p=bottom?pc:pi;
    const gc::Check k=gc::validate(duct,p,c,o);
    if(k.error!=gc::Ok){acutPrintf(L"\nTVS_RESETKA: %ls. Ništa nije promijenjeno.",gc::errorText(k.error));return;}
    // ---- preflight: block and duct can be written ----
    std::vector<AcDbObjectId> touched;touched.push_back(blockId);if(!bottom)touched.push_back(ductId);
    if(!tvsAction::writable(touched)){acutPrintf(L"\nTVS_RESETKA: kanal ili blok je na zaključanom sloju. Ništa nije promijenjeno.");return;}
    rec.version=1;rec.ports=1;rec.mmPerUnit=f;rec.lengthMm=0;rec.port[1]=tvsEquipment::Port();
    if(bottom){
        rec.mount=tvsEquipment::Bottom;rec.round=roundDuct;rec.a=A*f;rec.b=roundDuct?0:B*f;
        rec.neckA=dg.aMm;rec.neckB=dg.round?0:dg.bMm;rec.neckRound=dg.round;
        tvsEquipment::Port& q=rec.port[0];q.x=p.tap.x;q.y=p.tap.y;q.z=elev;q.dx=0;q.dy=0;
        tvsAction::Transaction tx;if(!tx.good()){acutPrintf(L"\nTVS_RESETKA: transakcija nije pokrenuta. Ništa nije promijenjeno.");return;}
        AcDbObject* b=tx.open(blockId,AcDb::kForWrite);const Acad::ErrorStatus es=b?tvsEquipmentStore::write(b,rec):tx.status;
        if(es!=Acad::eOk||!tx.commit()){acutPrintf(L"\nTVS_RESETKA: zapis nije upisan (%ls). Ništa nije promijenjeno.",acadErrorStatusText(es!=Acad::eOk?es:tx.status));return;}
        CString neck;if(dg.round)neck.Format(L"Ø%.0f",dg.aMm);else neck.Format(L"%.0f×%.0f",dg.aMm,dg.bMm);
        acutPrintf(L"\nTVS_RESETKA: rešetka „%ls“ povezana na dnu kanala %ls (priključak %ls mm, %.0f mm od početka kanala); ništa nije nacrtano.",
            name.c_str(),size.GetString(),neck.GetString(),p.t*f);
        return;
    }
    if(atEnd){   // ---- Kraj kanala: end duct to the given length, block on the end facing outward ----
        const double want=c.stub>0&&std::fabs(c.stub-duct.len())>0.5/f?c.stub:0;
        const gc::EndLayout e=gc::endLayout(duct,c.end,want);
        if(!e.ok){acutPrintf(L"\nTVS_RESETKA: kraj kanala se ne može izračunati. Ništa nije promijenjeno.");return;}
        rec.mount=tvsEquipment::AtEnd;rec.round=roundDuct;rec.a=A*f;rec.b=roundDuct?0:B*f;rec.neckA=rec.neckB=0;rec.neckRound=false;
        {tvsEquipment::Port& q=rec.port[0];q.x=e.at.x;q.y=e.at.y;q.z=elev;q.dx=-e.outward.x;q.dy=-e.outward.y;}
        tvsAction::Transaction tx;if(!tx.good()){acutPrintf(L"\nTVS_RESETKA: transakcija nije pokrenuta. Ništa nije promijenjeno.");return;}
        const wchar_t* failed=NULL;
        do{
            TVS_Pipe* d=TVS_Pipe::cast(tx.open(ductId,AcDb::kForWrite));if(!d){failed=L"kanal se ne može mijenjati";break;}
            if(want>0){Acad::ErrorStatus es=c.end==1?d->put_Lastpoint(AcGePoint3d(e.at.x,e.at.y,d->LastPoint.z)):d->put_FirstPoint(AcGePoint3d(e.at.x,e.at.y,d->FirstPoint.z));
                if(es!=Acad::eOk){failed=L"kanal se ne može skratiti / produžiti";break;}d->recordGraphicsModified();}
            AcDbBlockReference* ref=AcDbBlockReference::cast(tx.open(blockId,AcDb::kForWrite));if(!ref){failed=L"blok se ne može mijenjati";break;}
            const AcGePoint3d pos=ref->position();const AcGePoint3d target(e.at.x,e.at.y,pos.z);
            const double turn=gc::turnAngle(gc::V(localDown.x,localDown.y),e.outward);
            const AcGeMatrix3d m=AcGeMatrix3d::translation(target-pos)*AcGeMatrix3d::rotation(turn,AcGeVector3d::kZAxis,pos);
            if(ref->transformBy(m)!=Acad::eOk){failed=L"blok se ne može pomjeriti";break;}
            if(tvsEquipmentStore::write(ref,rec)!=Acad::eOk){failed=L"zapis opreme nije upisan";break;}
        }while(false);
        if(failed||!tx.commit()){acutPrintf(L"\nTVS_RESETKA: %ls. Ništa nije promijenjeno.",failed?failed:L"transakcija nije završena");return;}
        acutPrintf(L"\nTVS_RESETKA: rešetka „%ls“ priključena na kraj kanala %ls (U vraća sve).",name.c_str(),size.GetString());
        if(want>0)acutPrintf(L"\n  Kanal do rešetke: %.0f mm.",c.stub*f);
        return;
    }
    const gc::Layout y=gc::layout(duct,p,c,o);
    if(!y.ok){acutPrintf(L"\nTVS_RESETKA: priključak se ne može izračunati. Ništa nije promijenjeno.");return;}
    rec.mount=tvsEquipment::AtEnd;rec.round=dg.round;rec.a=dg.aMm;rec.b=dg.round?0:dg.bMm;rec.neckA=rec.neckB=0;rec.neckRound=false;
    {tvsEquipment::Port& q=rec.port[0];q.x=y.stubEnd.x;q.y=y.stubEnd.y;q.z=elev;q.dx=-y.outward.x;q.dy=-y.outward.y;}   // dir: out of the block, into the stub
    const double tol=0.5/f;
    tvsAction::Transaction tx;if(!tx.good()){acutPrintf(L"\nTVS_RESETKA: transakcija nije pokrenuta. Ništa nije promijenjeno.");return;}
    const wchar_t* failed=NULL;
    do{
        TVS_Pipe* d=TVS_Pipe::cast(tx.open(ductId,AcDb::kForWrite));if(!d){failed=L"kanal se ne može mijenjati";break;}
        AcDbBlockTableRecord* space=AcDbBlockTableRecord::cast(tx.open(d->ownerId(),AcDb::kForWrite));if(!space){failed=L"prostor crteža se ne može mijenjati";break;}
        const double z=d->LastPoint.z;
        const AcGePoint3d runStart(y.runStart.x,y.runStart.y,d->FirstPoint.z),runEnd(y.runEnd.x,y.runEnd.y,z),
                          branchPort(y.branchPort.x,y.branchPort.y,z),stubEnd(y.stubEnd.x,y.stubEnd.y,z);
        if(y.fit==gc::Split){
            // the duct after the T komad: a new piece with all properties of the duct (as TVS_OPREMA cuts)
            TVS_Pipe* rest=tvsEquipmentLinkCmd::newPiece(d,runEnd);AcDbObjectId restId;
            if(!tx.append(space,rest,restId)){failed=L"kanal iza T komada nije napravljen";break;}
            if(resbuf* x=d->xData(NULL)){const Acad::ErrorStatus es=rest->setXData(x);acutRelRb(x);if(es!=Acad::eOk){failed=L"svojstva kanala nisu prenesena";break;}}
            rest->recordGraphicsModified();
            if(d->put_Lastpoint(runStart)!=Acad::eOk){failed=L"kanal se ne može skratiti";break;}
        }
        // T komad at a free end: the duct ends at the T komad inlet, no new piece, the outlet stays free (čep)
        else if(y.fit==gc::AtLast){if(d->put_Lastpoint(runStart)!=Acad::eOk){failed=L"kanal se ne može skratiti / produžiti";break;}}
        else if(y.fit==gc::AtFirst){if(d->put_FirstPoint(AcGePoint3d(y.runEnd.x,y.runEnd.y,d->FirstPoint.z))!=Acad::eOk){failed=L"kanal se ne može skratiti / produžiti";break;}}
        else{failed=L"mjesto priključenja je preblizu kraju kanala";break;}
        d->recordGraphicsModified();
        // the T komad (TVS_WYE as TVS_WYE::add_new fills it, in the duct's space and transaction)
        TVS_WYE* w=new TVS_WYE();tvsOffsetProof::initialize(w);
        w->SizeApr=d->SizeA;w->SizeBpr=roundDuct?0:d->SizeB;w->SizeAotv=c.a;w->SizeBotv=c.round?0:c.b;w->LengthPl=y.lengthPl;
        w->Vectpr=AcGeVector3d(p.u.x,p.u.y,0);w->Vectotv=AcGeVector3d(y.outward.x,y.outward.y,0);w->Basepoint=AcGePoint3d(y.tap.x,y.tap.y,z);
        w->ThisRoundpr=roundDuct;w->ThisRoundotv=c.round;w->This1D=d->This1D;w->DuctType=d->DuctType;
        w->Flow=d->Flow;w->Elev=d->Elev;w->ElevUp=d->ElevUp;w->ElevDown=d->ElevDown;
        w->Wipeout=d->Wipeout;w->WipeoutLength=d->WipeoutLength;w->Grani=d->Grani;
        w->setDatabaseDefaults(d->database());w->setPropertiesFrom(d);w->setLineWeight(d->lineWeight());
        AcDbObjectId teeId;if(!tx.append(space,w,teeId)){failed=L"T komad nije napravljen";break;}
        if(y.teeType==tvsTee::Tap){
            tvsTeeData::Record r;r.type=tvsTee::Tap;r.l=y.lengthPl;r.second=y.cone;r.radius2=o.margin;
            if(tvsTeeData::write(w,r)!=Acad::eOk){failed=L"tip štucne nije upisan";break;}}
        w->recordGraphicsModified();
        if(y.teeType==tvsTee::Straight){const tvsTeeFit::Ports q=tvsTeeFit::ports(w);   // TK0: the TVS ports must be where the ducts go (stucna: tapRun, GrilleConnect.h)
         const AcGePoint3d inlet(y.runStart.x,y.runStart.y,z),outlet(y.runEnd.x,y.runEnd.y,z);
         if(!q.ok||tvsTeeFit::planDistance(q.p[0],inlet)>tol||tvsTeeFit::planDistance(q.p[1],outlet)>tol||tvsTeeFit::planDistance(q.p[2],branchPort)>tol){
            failed=L"priključci T komada ne odgovaraju kanalu";break;}}
        // the stub: straight duct of the branch size, same level as the duct
        TVS_Pipe* s=tvsEquipmentLinkCmd::newPiece(d,branchPort);
        s->LastPoint=stubEnd;s->SizeA=c.a;s->SizeB=c.round?0:c.b;s->ThisRound=c.round;s->Flow=0;
        AcDbObjectId stubId;if(!tx.append(space,s,stubId)){failed=L"ogranak nije napravljen";break;}
        s->recordGraphicsModified();
        // the block: base point on the stub end, local -y outward (attributes follow transformBy)
        AcDbBlockReference* ref=AcDbBlockReference::cast(tx.open(blockId,AcDb::kForWrite));if(!ref){failed=L"blok se ne može mijenjati";break;}
        const AcGePoint3d pos=ref->position();const AcGePoint3d target(stubEnd.x,stubEnd.y,pos.z);
        const double turn=gc::turnAngle(gc::V(localDown.x,localDown.y),y.outward);
        const AcGeMatrix3d m=AcGeMatrix3d::translation(target-pos)*AcGeMatrix3d::rotation(turn,AcGeVector3d::kZAxis,pos);
        if(ref->transformBy(m)!=Acad::eOk){failed=L"blok se ne može pomjeriti";break;}
        if(tvsEquipmentStore::write(ref,rec)!=Acad::eOk){failed=L"zapis opreme nije upisan";break;}
    }while(false);
    if(failed||!tx.commit()){acutPrintf(L"\nTVS_RESETKA: %ls. Ništa nije promijenjeno.",failed?failed:L"transakcija nije završena");return;}
    CString branch;if(dg.round)branch.Format(L"Ø%.0f",dg.aMm);else branch.Format(L"%.0f×%.0f",dg.aMm,dg.bMm);
    const wchar_t* how=c.method==gc::Tee?L"ravni T komad (TK0)":c.method==gc::Cone?L"konusna štucna SR5-12":dg.round?L"štucna SR5-11":L"štucna SR5-3";
    acutPrintf(L"\nTVS_RESETKA: rešetka „%ls“ priključena %ls na kanal %ls – %ls, ogranak %ls mm dužine %.0f mm; blok na kraju ogranka (U vraća sve).",
        name.c_str(),c.side==gc::Left?L"lijevo":L"desno",size.GetString(),how,branch.GetString(),dg.lengthMm);
    if(y.fit!=gc::Split)acutPrintf(L"\n  T komad je na slobodnom kraju kanala: kanal završava na ulazu T komada, prolazni izlaz ostaje slobodan (čep).");
}
}
