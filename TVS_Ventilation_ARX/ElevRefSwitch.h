#pragma once
// Visinska kota in the Svojstva / TVS_SETTINGS dialogs: switching Vrh / Centar / Dno converts
// the value shown (mm), so the same physical elevation stays: top = axis + H/2, bottom = axis - H/2
// (round: D). Controls: 1014 value, 1001 width / D, 1002 height (0 = round).
#include <cwchar>
namespace tvsElevRef {
inline bool number(CWnd* dlg,int id,double& v){CString s;dlg->GetDlgItemText(id,s);s.Trim();if(s.IsEmpty())return false;
    s.Replace(L',',L'.');wchar_t* end=NULL;v=wcstod(s,&end);return end&&end!=(LPCTSTR)s&&*end==0;}
inline double shift(int mode,double half){return mode==1?half:mode==2?-half:0;}
// mode: 0 Centar, 1 Vrh, 2 Dno
inline void convert(CWnd* dlg,int from,int to){
    if(from==to)return;double e=0,a=0,b=0;
    if(!number(dlg,1014,e)||!number(dlg,1001,a))return;if(!number(dlg,1002,b))b=0;
    const double half=(b>0?b:a)/2,axis=e-shift(from,half),value=axis+shift(to,half);
    CString s;s.Format(L"%.10g",value);dlg->SetDlgItemText(1014,s);
}
}
