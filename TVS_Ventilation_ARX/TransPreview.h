#pragma once
// Svojstva dialog: sketch of the chosen prelaz, plan (U tlocrtu) on the left and side view
// (Po visini) on the right, proportional to Dpr -> Dpr2 and Hpr -> Hpr2 (drawn left to right =
// direction of drawing; "desna ivica" is the lower edge of the plan sketch).
#include <algorithm>
namespace tvsTransPreview {
// plan: TransType 1 / 2 / 3 (-1 unknown -> 1); vert: 0 osa, 1 gore, 2 dolje (-1 -> 0)
inline void sketch(CDC* dc,const CRect& area,int plan,int vert,double a1,double b1,double a2,double b2){
    dc->FillSolidRect(area,::GetSysColor(COLOR_BTNFACE));
    if(!(a1>0))a1=1;if(!(a2>0))a2=a1;if(!(b1>0))b1=a1;if(!(b2>0))b2=b1;
    CPen pen(PS_SOLID,1,RGB(0,0,0)),thin(PS_DOT,1,RGB(120,120,120));CPen* old=dc->SelectObject(&pen);
    dc->SetBkMode(TRANSPARENT);
    const int pad=6,half=area.Width()/2,title=14;
    struct Panel {CRect r;const wchar_t* name;double s1,s2;int mode;};   // mode: 0 centre, 1 upper edge straight, 2 lower edge straight
    const int planMode=plan==2?2:plan==3?1:0,vertMode=vert==1?1:vert==2?2:0;
    Panel panels[2]={{CRect(area.left+pad,area.top+title,area.left+half-pad,area.bottom-pad),L"Tlocrt",a1,a2,planMode},
                     {CRect(area.left+half+pad,area.top+title,area.right-pad,area.bottom-pad),L"Presjek",b1,b2,vertMode}};
    for(int k=0;k<2;++k){const Panel& p=panels[k];
        dc->TextOut(p.r.left,area.top+1,p.name);
        const double big=(std::max)(p.s1,p.s2),scale=(p.r.Height()-10)/big;const int h1=int(p.s1*scale+.5),h2=int(p.s2*scale+.5);
        const int cy=p.r.top+p.r.Height()/2,w=p.r.Width(),x0=p.r.left,x1=x0+w/3,x2=x0+2*w/3,x3=p.r.right;
        auto top=[&](int h){return p.mode==1?cy-int(big*scale/2):p.mode==2?cy+int(big*scale/2)-h:cy-h/2;};
        const int t1=top(h1),t2=top(h2);
        // duct before, prelaz, duct after
        dc->MoveTo(x0,t1);dc->LineTo(x1,t1);dc->LineTo(x2,t2);dc->LineTo(x3,t2);
        dc->MoveTo(x0,t1+h1);dc->LineTo(x1,t1+h1);dc->LineTo(x2,t2+h2);dc->LineTo(x3,t2+h2);
        dc->MoveTo(x1,t1);dc->LineTo(x1,t1+h1);dc->MoveTo(x2,t2);dc->LineTo(x2,t2+h2);
        CPen* p0=dc->SelectObject(&thin);dc->MoveTo(x0,t1+h1/2);dc->LineTo(x1,t1+h1/2);dc->LineTo(x2,t2+h2/2);dc->LineTo(x3,t2+h2/2);dc->SelectObject(p0);
    }
    dc->SelectObject(old);
}
}
