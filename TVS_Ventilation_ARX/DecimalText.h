#pragma once
// Numbers typed in the dialogs with a decimal comma or point ("0,5" = "0.5"): the old dialogs read
// them with _wtof / DDX_Text, which stop at the comma (0,5 -> 0) or refuse it.
// MFC side of the one numeric layer geometry/NumericField.h (review 2026-11b, proposal 4): parse,
// show (mm / degrees), and "changed" by number, never by comparing texts (N1).
#include <cwchar>
#include <string>
#include "../geometry/NumericField.h"
namespace tvsDecimal {
inline CString normalized(CString s){s.Trim();s.Replace(L',',L'.');return s;}
inline CString text(const std::wstring& s){return CString(s.c_str());}
inline bool parse(const CString& text,double& v){return tvsNumeric::parse(text.GetString(),v);}
// _wtof with a comma (empty / text -> 0, as _wtof).
inline double value(const CString& text){double v=0;return parse(text,v)?v:0;}
// Text that keeps a number exactly (values passed between a command and its dialog).
inline CString exact(double v){return text(tvsNumeric::exact(v));}
// Command side after the dialog: true = `after` is a number different from `before` -> write v.
inline bool differs(const CString& before,const CString& after,double& v){return tvsNumeric::differs(before.GetString(),after.GetString(),v);}
// DDX_Text for a double, comma or point; a field that is not a number stops the OK with a message.
inline void DDX_Real(CDataExchange* dx,int id,double& v){
    CWnd* w=dx->m_pDlgWnd?dx->m_pDlgWnd->GetDlgItem(id):NULL;if(!w)return;
    if(dx->m_bSaveAndValidate){
        CString t;w->GetWindowText(t);
        if(!parse(t,v)){dx->PrepareEditCtrl(id);AfxMessageBox(L"Unesite broj (decimalni zarez ili tačka).",MB_ICONEXCLAMATION);dx->Fail();}
    }else{w->SetWindowText(text(tvsNumeric::formatG(v,15)));}
}
// CString fields (empty = different values): the comma becomes a point.
inline void DDX_Number(CDataExchange* dx,int id,CString& s){DDX_Text(dx,id,s);if(dx->m_bSaveAndValidate)s=normalized(s);}
// A field of the Svojstva dialog: `stored` = value from the command (drawing units / radians, any
// text that is not a number - "Različito", empty - is shown as it is). Shown: stored * scale with
// `digits` significant digits. On OK `stored` changes ONLY when the user typed a different number
// (then stored = typed / scale, exact); untouched it stays exactly what it was. `outOfRange` (or
// NULL): message when a typed number is outside lo < v < hi (lo <= v with lowIncluded); the dialog
// stays open.
inline void DDX_Scaled(CDataExchange* dx,int id,CString& stored,double scale,int digits,
                       double lo,double hi,bool lowIncluded,const wchar_t* outOfRange){
    if(!dx->m_bSaveAndValidate){
        CString shown=text(tvsNumeric::show(stored.GetString(),scale,digits));
        DDX_Text(dx,id,shown);
        return;
    }
    CString typed;DDX_Text(dx,id,typed);
    double v=0,t=0;
    if(tvsNumeric::edited(stored.GetString(),typed.GetString(),scale,digits,v)){
        if(outOfRange&&parse(typed,t)&&!tvsNumeric::inside(t,lo,hi,lowIncluded)){
            dx->PrepareEditCtrl(id);AfxMessageBox(outOfRange,MB_ICONEXCLAMATION);dx->Fail();
        }
        stored=exact(v);
    }else if(!parse(typed,v))stored=typed;   // text (Različito, empty): passed on, means no change
}
// Length in drawing units shown in mm (factor = mm per drawing unit); positive: must be > 0.
inline void DDX_Millimetres(CDataExchange* dx,int id,CString& stored,double factor,bool positive){
    DDX_Scaled(dx,id,stored,factor,15,0,1e12,false,positive?L"Dimenzija mora biti veća od 0.":NULL);
}
// Elbow angle: stored in radians, shown in degrees; 0 < angle < 180 (S-2).
inline void DDX_Degrees(CDataExchange* dx,int id,CString& radians){
    DDX_Scaled(dx,id,radians,tvsNumeric::degPerRad(),10,0,180,false,L"Ugao koljena mora biti veći od 0° i manji od 180°.");
}
// Number without a unit (protok, k radijusa).
inline void DDX_Plain(CDataExchange* dx,int id,CString& stored){DDX_Scaled(dx,id,stored,1,15,0,0,false,NULL);}
}
