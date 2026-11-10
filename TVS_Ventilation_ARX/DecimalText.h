#pragma once
// Numbers typed in the dialogs with a decimal comma or point ("0,5" = "0.5"): the old dialogs read
// them with _wtof / DDX_Text, which stop at the comma (0,5 -> 0) or refuse it.
#include <cwchar>
namespace tvsDecimal {
inline CString normalized(CString s){s.Trim();s.Replace(L',',L'.');return s;}
inline bool parse(const CString& text,double& v){
    const CString t=normalized(text);if(t.IsEmpty())return false;
    wchar_t* end=NULL;v=std::wcstod(t,&end);return end&&end!=(LPCTSTR)t&&*end==0;
}
// _wtof with a comma (empty / text -> 0, as _wtof).
inline double value(const CString& text){double v=0;return parse(text,v)?v:0;}
// DDX_Text for a double, comma or point; a field that is not a number stops the OK with a message.
inline void DDX_Real(CDataExchange* dx,int id,double& v){
    CWnd* w=dx->m_pDlgWnd?dx->m_pDlgWnd->GetDlgItem(id):NULL;if(!w)return;
    if(dx->m_bSaveAndValidate){
        CString t;w->GetWindowText(t);
        if(!parse(t,v)){dx->PrepareEditCtrl(id);AfxMessageBox(L"Unesite broj (decimalni zarez ili tačka).",MB_ICONEXCLAMATION);dx->Fail();}
    }else{CString t;t.Format(L"%.15g",v);w->SetWindowText(t);}
}
// CString fields of the Svojstva dialog (empty = different values): the comma becomes a point.
inline void DDX_Number(CDataExchange* dx,int id,CString& s){DDX_Text(dx,id,s);if(dx->m_bSaveAndValidate)s=normalized(s);}
}
