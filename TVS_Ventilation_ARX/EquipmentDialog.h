#pragma once
// TVS_OPREMA_SVOJSTVA: svojstva povezanih blokova opreme (IDD_EQUIPMENT). One or more blocks
// linked by TVS_OPREMA (record TVS_OPREMA_V1, EquipmentStore.h): vrsta, oznaka (Nova = next free
// tag of the kind's prefix in the drawing), opis / tip, model pada pritiska with its values,
// protok (terminals only), read-only priključci / dimenzija / dužina. Multiple selection: a field
// whose blocks differ shows "Različito" and stays as it is in every block unless edited (pure
// rules: geometry/EquipmentEdit.h). "Primijeni na sve blokove ovog naziva": the record without
// ports / tag / flow becomes the default of the block name (writeDefault) and kind, note, model
// and values go to every linked block of that name. All block writes in one transaction after a
// writable() preflight (ActionTransaction.h).
#include "../geometry/EquipmentEdit.h"
#include "EquipmentStore.h"
#include "ActionTransaction.h"
#include "DecimalText.h"
#include "adui.h"
#include "dbents.h"
#include "dbsymtb.h"
#include <map>
#include <set>
#include <string>
#include <vector>
namespace tvsEquipmentDialog {
class Dialog : public CAdUiBaseDialog {
public:
    enum {IdInfo=1701,IdKind=1702,IdTag=1703,IdNewTag=1704,IdNote=1705,IdNone=1706,IdFixed=1707,IdZeta=1708,IdNominal=1709,
          IdDp=1710,IdZetaValue=1711,IdDpn=1712,IdQn=1713,IdFlow=1714,IdFacts=1715,IdAll=1716,IdStatus=1717,
          IdDpLabel=1718,IdZetaLabel=1719,IdDpnLabel=1720,IdQnLabel=1721,IdFlowLabel=1722};
    std::vector<tvsEquipment::Record> records;   // in: as read; after OK: as edited
    std::vector<char> changed;                   // after OK: record k changed
    std::vector<std::wstring> tags;              // every tag in the drawing (Nova)
    tvsEquipmentEdit::Values shown;              // what the dialog showed
    CString dp,zeta,dpn,qn,q;                    // numeric fields (exact numbers / Različito), DecimalText.h
    CString banner,allText;bool applyAll;bool loading;CFont bold,title;CBrush back,band,white;
    Dialog(const std::vector<tvsEquipment::Record>& rs,UINT idd):CAdUiBaseDialog(idd,NULL,NULL),records(rs),applyAll(false),loading(false){
        shown=tvsEquipmentEdit::merge(records);
        dp=shown.dp.c_str();zeta=shown.zeta.c_str();dpn=shown.dpn.c_str();qn=shown.qn.c_str();q=shown.q.c_str();
    }
    void enable(int id,bool on){if(CWnd* w=GetDlgItem(id))w->EnableWindow(on?TRUE:FALSE);}
    int kindSel(){CComboBox* b=static_cast<CComboBox*>(GetDlgItem(IdKind));const int k=b?b->GetCurSel():-1;
        return k>=0&&k<tvsEquipment::kindCount()?k:-1;}
    int modelSel(){
        for(int m=tvsEquipment::NoLoss;m<=tvsEquipment::Nominal;++m)if(IsDlgButtonChecked(IdNone+m)==BST_CHECKED)return m;
        return -1;   // different models, none chosen
    }
    void refresh(){
        const int m=modelSel();const bool any=m<0;   // different models: every value can be edited
        const bool fixed=any||m==tvsEquipment::FixedDp,zt=any||m==tvsEquipment::Zeta,nom=any||m==tvsEquipment::Nominal;
        enable(IdDp,fixed);enable(IdDpLabel,fixed);enable(IdZetaValue,zt);enable(IdZetaLabel,zt);
        enable(IdDpn,nom);enable(IdDpnLabel,nom);enable(IdQn,nom);enable(IdQnLabel,nom);
        enable(IdNewTag,kindSel()>=0);
    }
    // A numeric field through the shared layer: a typed text that is not a number stops the OK.
    void number(CDataExchange* dx,int id,CString& stored){
        if(dx->m_bSaveAndValidate){
            CString t;GetDlgItemText(id,t);
            const std::wstring was=tvsNumeric::show(stored.GetString(),1,15);
            if(!tvsEquipmentEdit::typedOk(was,std::wstring(t.GetString()))){
                dx->PrepareEditCtrl(id);AfxMessageBox(L"Unesite broj (decimalni zarez ili tačka).",MB_ICONEXCLAMATION);dx->Fail();}
        }
        tvsDecimal::DDX_Plain(dx,id,stored);
    }
    virtual void DoDataExchange(CDataExchange* dx){
        CAdUiBaseDialog::DoDataExchange(dx);
        number(dx,IdDp,dp);number(dx,IdZetaValue,zeta);number(dx,IdDpn,dpn);number(dx,IdQn,qn);
        if(shown.terminals())number(dx,IdFlow,q);
    }
    virtual BOOL OnInitDialog(){
        CAdUiBaseDialog::OnInitDialog();
        LOGFONT lf={};if(CFont* f=GetFont())f->GetLogFont(&lf);
        lf.lfWeight=FW_SEMIBOLD;bold.CreateFontIndirect(&lf);lf.lfHeight=lf.lfHeight*13/10;title.CreateFontIndirect(&lf);
        back.CreateSolidBrush(RGB(244,245,247));band.CreateSolidBrush(RGB(231,236,242));white.CreateSolidBrush(RGB(255,255,255));
        SetWindowText(L"TVS_VENT – svojstva opreme");
        loading=true;
        if(CComboBox* b=static_cast<CComboBox*>(GetDlgItem(IdKind))){
            for(int k=0;k<tvsEquipment::kindCount();++k)b->AddString(tvsEquipment::kindName(k));
            if(shown.kind<0)b->AddString(tvsEquipmentEdit::differentText());
            b->SetCurSel(shown.kind<0?tvsEquipment::kindCount():shown.kind);}
        SetDlgItemText(IdTag,shown.tag.c_str());SetDlgItemText(IdNote,shown.note.c_str());
        if(shown.model>=0)CheckRadioButton(IdNone,IdNominal,IdNone+shown.model);
        enable(IdFlow,shown.terminals());enable(IdFlowLabel,shown.terminals());
        if(!shown.terminals())SetDlgItemText(IdFlow,L"");
        CString facts;
        if(shown.ports==1)facts=L"Priključak: 1 (krajnji element)";
        else if(shown.ports==2)facts=L"Priključci: 2 (u liniji kanala)";
        else facts=L"Priključci: različito";
        facts+=L"\nDimenzija: ";facts+=shown.size.empty()?CString(L"–"):CString(shown.size.c_str())+(shown.size==tvsEquipmentEdit::differentText()?L"":L" mm");
        if(shown.ports!=1){facts+=L"  ·  dužina: ";facts+=CString(shown.length.c_str())+(shown.length==tvsEquipmentEdit::differentText()?L"":L" mm");}
        SetDlgItemText(IdFacts,facts);
        SetDlgItemText(IdInfo,banner);SetDlgItemText(IdAll,allText);
        SetDlgItemText(IdStatus,records.size()>1?L"Različito: ostaje kako je u svakom bloku dok se ne promijeni":L"Upisuje se u blok (jedan UNDO korak)");
        loading=false;refresh();
        GetDlgItem(IdKind)->SetFocus();return FALSE;
    }
    virtual void OnOK(){
        if(!UpdateData(TRUE))return;
        tvsEquipmentEdit::Values a=shown;CString t;
        a.kind=kindSel();
        GetDlgItemText(IdTag,t);a.tag=t.GetString();GetDlgItemText(IdNote,t);a.note=t.GetString();
        a.model=modelSel();
        a.dp=dp.GetString();a.zeta=zeta.GetString();a.dpn=dpn.GetString();a.qn=qn.GetString();
        if(shown.terminals())a.q=q.GetString();
        std::vector<tvsEquipment::Record> out=records;std::vector<char> flags(out.size(),0);
        for(size_t k=0;k<out.size();++k){
            flags[k]=tvsEquipmentEdit::apply(shown,a,out[k])?1:0;
            const std::wstring m=tvsEquipmentEdit::validate(out[k]);
            if(!m.empty()){AfxMessageBox(m.c_str(),MB_ICONEXCLAMATION);return;}
        }
        records=out;changed=flags;applyAll=IsDlgButtonChecked(IdAll)==BST_CHECKED;
        EndDialog(IDOK);
    }
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp){
        const int id=LOWORD(wp),code=HIWORD(wp);
        if(!loading&&id>=IdNone&&id<=IdNominal&&code==BN_CLICKED){refresh();}
        if(!loading&&id==IdKind&&code==CBN_SELCHANGE){refresh();return TRUE;}
        if(id==IdNewTag&&code==BN_CLICKED){const int k=kindSel();
            if(k>=0)SetDlgItemText(IdTag,tvsEquipmentEdit::nextTag(tvsEquipment::tagPrefix(k),tags).c_str());
            return TRUE;}
        return CAdUiBaseDialog::OnCommand(wp,lp);
    }
    CRect du(int x,int y,int w,int h){CRect r(x,y,x+w,y+h);MapDialogRect(&r);return r;}
    void frame(CDC* dc,const CRect& r,COLORREF col){CPen p(PS_SOLID,1,col);CPen* op=dc->SelectObject(&p);CBrush* ob=static_cast<CBrush*>(dc->SelectStockObject(NULL_BRUSH));dc->Rectangle(r);dc->SelectObject(op);dc->SelectObject(ob);}
    void paintBack(CDC* dc){
        CRect all;GetClientRect(&all);dc->FillSolidRect(all,RGB(244,245,247));
        const CRect head=du(0,0,320,22),foot=du(0,226,320,30);
        dc->FillSolidRect(CRect(all.left,all.top,all.right,head.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,head.bottom,all.Width(),1,RGB(173,186,203));
        dc->FillSolidRect(CRect(all.left,foot.top,all.right,all.bottom),RGB(231,236,242));dc->FillSolidRect(all.left,foot.top,all.Width(),1,RGB(173,186,203));
        dc->SetBkMode(TRANSPARENT);
        {CFont* of=dc->SelectObject(&title);dc->SetTextColor(RGB(0,26,45));CRect t=du(10,0,160,22);dc->DrawText(L"Svojstva opreme",t,DT_LEFT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CFont* of=dc->SelectObject(GetFont());dc->SetTextColor(RGB(7,53,90));CRect t=du(170,0,140,22);dc->DrawText(L"Oprema na mreži  ·  TVS_OPREMA_SVOJSTVA",t,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);}
        {CRect r=du(10,28,300,15);dc->FillSolidRect(r,RGB(255,255,255));dc->FillSolidRect(r.left,r.top,3,r.Height(),RGB(38,117,174));}   // banner
        {CRect r=du(10,106,300,58);frame(dc,r,RGB(213,223,236));CFont* of=dc->SelectObject(&bold);const wchar_t* t=L"Model pada pritiska";CSize sz=dc->GetTextExtent(t);
         dc->FillSolidRect(CRect(r.left+8,r.top-sz.cy/2,r.left+14+sz.cx,r.top+sz.cy/2+1),RGB(244,245,247));dc->SetTextColor(RGB(0,52,89));dc->TextOut(r.left+11,r.top-sz.cy/2,t);dc->SelectObject(of);}
    }
    void drawButton(LPDRAWITEMSTRUCT it){
        CDC* dc=CDC::FromHandle(it->hDC);CRect r(it->rcItem);const bool down=(it->itemState&ODS_SELECTED)!=0;
        const COLORREF face=down?RGB(19,78,125):RGB(27,100,156);dc->FillSolidRect(r,face);
        CString t;GetDlgItem(IDOK)->GetWindowText(t);CFont* of=dc->SelectObject(&bold);
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(RGB(255,255,255));dc->DrawText(t,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc->SelectObject(of);
        if(it->itemState&ODS_FOCUS){CRect f(r);f.DeflateRect(3,3);dc->DrawFocusRect(f);}
    }
    virtual LRESULT WindowProc(UINT msg,WPARAM wp,LPARAM lp){
        if(msg==WM_ACAD_KEEPFOCUS)return TRUE;
        if(msg==WM_ERASEBKGND){paintBack(CDC::FromHandle(reinterpret_cast<HDC>(wp)));return TRUE;}
        if(msg==WM_DRAWITEM&&wp==IDOK){drawButton(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));return TRUE;}
        if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLORBTN){HWND h=reinterpret_cast<HWND>(lp);wchar_t cls[16]=L"";::GetClassNameW(h,cls,16);
            if(!_wcsicmp(cls,L"Static")||!_wcsicmp(cls,L"Button")){HDC dc=reinterpret_cast<HDC>(wp);const int id=::GetDlgCtrlID(h);::SetBkMode(dc,TRANSPARENT);
                ::SetTextColor(dc,id==IdInfo?RGB(0,30,53):id==IdStatus?RGB(0,100,107):RGB(49,90,128));
                if(id==IdInfo)::SelectObject(dc,bold.GetSafeHandle());
                return reinterpret_cast<LRESULT>((id==IdInfo?white:id==IdStatus?band:back).GetSafeHandle());}}
        return CAdUiBaseDialog::WindowProc(msg,wp,lp);
    }
};

// ---- drawing side ----
struct Item {AcDbObjectId id;std::wstring block;tvsEquipment::Record record;};
inline std::wstring blockName(const AcDbBlockReference* ref){
    if(!ref)return std::wstring();
    AcDbObjectPointer<AcDbBlockTableRecord> btr(ref->blockTableRecord(),AcDb::kForRead);if(btr.openStatus()!=Acad::eOk)return std::wstring();
    const ACHAR* n=NULL;if(btr->getName(n)!=Acad::eOk||!n)return std::wstring();
    return std::wstring(n);
}
// Tags of every linked block in the drawing (all spaces and block definitions).
inline void allTags(AcDbDatabase* db,std::vector<std::wstring>& tags){
    AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForRead)!=Acad::eOk)return;
    AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
    AcDbBlockTableIterator* bi=NULL;if(table->newIterator(bi)!=Acad::eOk||!bi)return;
    for(;!bi->done();bi->step()){AcDbObjectId btrId;if(bi->getRecordId(btrId)!=Acad::eOk)continue;
        AcDbObjectPointer<AcDbBlockTableRecord> btr(btrId,AcDb::kForRead);if(btr.openStatus()!=Acad::eOk)continue;
        AcDbBlockTableRecordIterator* ei=NULL;if(btr->newIterator(ei)!=Acad::eOk||!ei)continue;
        for(;!ei->done();ei->step()){AcDbObjectId id;if(ei->getEntityId(id)!=Acad::eOk||!id.objectClass()||!id.objectClass()->isDerivedFrom(AcDbBlockReference::desc()))continue;
            tvsEquipment::Record r;if(tvsEquipmentStore::read(id,r)&&!r.tag.empty())tags.push_back(r.tag);}
        delete ei;}
    delete bi;
}
// Picked (or preselected) block references.
inline bool pick(std::vector<AcDbObjectId>& ids){
    resbuf* filter=acutBuildList(RTDXF0,L"INSERT",RTNONE);ads_name ss;
    int rc=acedSSGet(L"_I",NULL,NULL,filter,ss);
    if(rc==RTNORM)acedSSSetFirst(NULL,NULL);
    else{acutPrintf(L"\nIzaberite blokove opreme: ");rc=acedSSGet(NULL,NULL,NULL,filter,ss);}
    acutRelRb(filter);
    if(rc!=RTNORM)return false;
    Adesk::Int32 n=0;acedSSLength(ss,&n);
    for(Adesk::Int32 i=0;i<n;++i){ads_name e;AcDbObjectId id;if(acedSSName(ss,i,e)==RTNORM&&acdbGetObjectId(id,e)==Acad::eOk)ids.push_back(id);}
    acedSSFree(ss);return !ids.empty();
}
// The given blocks (TVS_OPREMA: linked blocks whose name has no default kind yet).
inline void run(UINT idd,const std::vector<AcDbObjectId>& ids){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db||ids.empty())return;
    std::vector<Item> items;int unlinked=0;
    for(size_t k=0;k<ids.size();++k){
        AcDbObjectPointer<AcDbBlockReference> ref(ids[k],AcDb::kForRead);if(ref.openStatus()!=Acad::eOk)continue;
        Item it;it.id=ids[k];
        if(!tvsEquipmentStore::read(ref.object(),it.record)){++unlinked;continue;}
        it.block=blockName(ref.object());items.push_back(it);
    }
    if(items.empty()){acutPrintf(L"\nBlok nije povezan u mrežu (TVS_OPREMA).");return;}
    if(unlinked)acutPrintf(L"\nBlok nije povezan u mrežu (TVS_OPREMA): preskočeno %d.",unlinked);
    std::vector<tvsEquipment::Record> records;std::set<std::wstring> names;
    for(size_t k=0;k<items.size();++k){records.push_back(items[k].record);if(!items[k].block.empty())names.insert(items[k].block);}
    Dialog dg(records,idd);
    allTags(db,dg.tags);
    if(items.size()==1)dg.banner.Format(L"Blok „%ls“",items[0].block.c_str());
    else dg.banner.Format(L"Izabrano blokova: %d  ·  naziva: %d",static_cast<int>(items.size()),static_cast<int>(names.size()));
    if(names.size()==1)dg.allText.Format(L"Primijeni na sve blokove ovog naziva (%ls)",names.begin()->c_str());
    else dg.allText=L"Primijeni na sve blokove ovog naziva";
    INT_PTR rc;
    {CAcModuleResourceOverride resources;rc=dg.DoModal();}
    if(rc!=IDOK||dg.records.size()!=items.size())return;
    // what is written: the edited blocks, and with "sve blokove ovog naziva" every linked block of the names
    std::map<AcDbObjectId,tvsEquipment::Record> writes;std::map<std::wstring,tvsEquipment::Record> defaults;
    int sameName=0,sameNameUnlinked=0;   // other blocks of the names (report)
    for(size_t k=0;k<items.size();++k)if(k<dg.changed.size()&&dg.changed[k])writes[items[k].id]=dg.records[k];
    if(dg.applyAll){
        for(size_t k=0;k<items.size();++k)if(!items[k].block.empty()&&!defaults.count(items[k].block))
            defaults[items[k].block]=tvsEquipmentEdit::typeDefault(dg.records[k]);   // first selected block of the name
        for(size_t k=0;k<items.size();++k){std::map<std::wstring,tvsEquipment::Record>::const_iterator d=defaults.find(items[k].block);
            if(d==defaults.end())continue;tvsEquipment::Record r=dg.records[k];
            if(tvsEquipmentEdit::applyType(d->second,r)||(k<dg.changed.size()&&dg.changed[k]))writes[items[k].id]=r;}
        for(std::map<std::wstring,tvsEquipment::Record>::const_iterator d=defaults.begin();d!=defaults.end();++d){
            AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForRead)!=Acad::eOk)continue;
            AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
            AcDbObjectId btrId;if(table->getAt(d->first.c_str(),btrId)!=Acad::eOk)continue;
            AcDbObjectPointer<AcDbBlockTableRecord> btr(btrId,AcDb::kForRead);if(btr.openStatus()!=Acad::eOk)continue;
            AcDbObjectIdArray refs;if(btr->getBlockReferenceIds(refs)!=Acad::eOk)continue;
            for(int i=0;i<refs.length();++i){
                bool selected=false;for(size_t k=0;k<items.size()&&!selected;++k)selected=items[k].id==refs[i];
                if(selected)continue;
                {AcDbObjectPointer<AcDbBlockReference> o(refs[i],AcDb::kForRead);if(o.openStatus()!=Acad::eOk||o->isErased())continue;}
                ++sameName;
                tvsEquipment::Record r;
                if(!tvsEquipmentStore::read(refs[i],r)){++sameNameUnlinked;continue;}   // not linked: gets the default when TVS_OPREMA links it
                if(writes.count(refs[i]))r=writes[refs[i]];
                if(tvsEquipmentEdit::applyType(d->second,r))writes[refs[i]]=r;}
        }
    }
    if(writes.empty()&&defaults.empty()){acutPrintf(L"\nNišta nije promijenjeno.");return;}
    if(dg.applyAll){
        acutPrintf(L"\nOstalih blokova istog naziva: %d (povezanih %d).",sameName,sameName-sameNameUnlinked);
        if(sameNameUnlinked)acutPrintf(L"\n  %d nije povezano u mrežu – svojstva dobijaju kad ih poveže TVS_OPREMA (TVS_OPREMA, opcija Sve).",sameNameUnlinked);}
    std::vector<AcDbObjectId> targets;
    for(std::map<AcDbObjectId,tvsEquipment::Record>::const_iterator w=writes.begin();w!=writes.end();++w)targets.push_back(w->first);
    if(!tvsAction::writable(targets)){acutPrintf(L"\nBlok opreme je na zaključanom sloju ili nije dostupan. Ništa nije promijenjeno.");return;}
    if(!writes.empty()){
        tvsAction::Transaction tx;
        if(!tx.good()){acutPrintf(L"\nTransakcija nije pokrenuta. Ništa nije promijenjeno.");return;}
        for(std::map<AcDbObjectId,tvsEquipment::Record>::const_iterator w=writes.begin();w!=writes.end();++w){
            AcDbObject* o=tx.open(w->first,AcDb::kForWrite);
            const Acad::ErrorStatus es=o?tvsEquipmentStore::write(o,w->second):tx.status;
            if(es!=Acad::eOk){acutPrintf(L"\nZapis opreme nije upisan (%ls). Ništa nije promijenjeno.",acadErrorStatusText(es));return;}
        }
        if(!tx.commit()){acutPrintf(L"\nZapis opreme nije upisan (%ls). Ništa nije promijenjeno.",acadErrorStatusText(tx.status));return;}
    }
    // per block name: after the blocks (open / close on the named objects dictionary)
    int saved=0;
    for(std::map<std::wstring,tvsEquipment::Record>::const_iterator d=defaults.begin();d!=defaults.end();++d){
        const Acad::ErrorStatus es=tvsEquipmentStore::writeDefault(db,d->first.c_str(),d->second);
        if(es==Acad::eOk)++saved;
        else acutPrintf(L"\nPodrazumijevana svojstva bloka „%ls“ nisu sačuvana (%ls).",d->first.c_str(),acadErrorStatusText(es));
    }
    acutPrintf(L"\nPromijenjeno blokova opreme: %d.",static_cast<int>(writes.size()));
    if(saved)acutPrintf(L"\nSvojstva sačuvana po nazivu bloka: %d (novi blokovi tog naziva ih dobijaju pri povezivanju).",saved);
}
inline void run(UINT idd){
    std::vector<AcDbObjectId> ids;if(!pick(ids))return;
    run(idd,ids);
}
}
