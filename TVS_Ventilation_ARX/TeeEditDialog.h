#pragma once
// Svojstva dialog (dgAllEdit) for T komadi TK-O / TK-G: the extra width of the TVS_TEE_V1
// record (TK-O: outlet of the run C; TK-G: left branch, may differ from the right one, as
// in the dynamic block). When the first branch width changes and L was not edited, L is
// recomputed so the branch centre-line radius stays = branch width.
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "dgAllEdit.h"
#include <cwchar>
namespace tvsTeeEdit {
struct Gather {
    bool any,mixedType,mixedSecond;int type;double second;
    Gather():any(false),mixedType(false),mixedSecond(false),type(0),second(0){}
    void add(const TVS_WYE* w){
        tvsTeeData::Record r;const int t=tvsTeeData::read(w,r)?r.type:0;
        const double s=t==tvsTee::Branch?(r.second>0?r.second:w->SizeApr):t==tvsTee::Pants?(r.second>0?r.second:w->SizeApr):0;
        if(!any){any=true;type=t;second=s;return;}
        if(t!=type)mixedType=true;
        if(std::fabs(s-second)>1e-9)mixedSecond=true;
    }
    int commonType()const{return any&&!mixedType?type:0;}
    // Drawing units; dgAllEdit converts to mm for display.
    CString text()const{CString s;if(commonType()&&!mixedSecond)s.Format(L"%.15g",second);return s;}
};
inline void prepare(dgAllEdit& dg,const Gather& g){dg.TeeType=g.commonType();dg.TeeSecond=g.text();}
// Called with the element open for write, after the generic fields were applied.
inline void apply(TVS_WYE* w,const Gather& g,const dgAllEdit& dg,bool widthAChanged,bool widthA2Changed,bool lengthChanged){
    if(!g.commonType())return;
    tvsTeeData::Record r;if(!tvsTeeData::read(w,r))return;
    bool changed=false;
    if(dg.TeeSecond!=g.text()){
        wchar_t* end=NULL;const double v=wcstod(dg.TeeSecond,&end);
        if(end!=(LPCTSTR)dg.TeeSecond&&end&&*end==0&&v>0){
            r.second=v;if(r.type==tvsTee::Pants)r.radius2=tvsTee::defaultRadius(v);changed=true;}
    }
    const bool branchWidth=r.type==tvsTee::Branch?widthA2Changed:widthAChanged;
    if(changed){w->assertWriteEnabled();if(tvsTeeData::write(w,r)!=Acad::eOk)return;}
    if(branchWidth&&!lengthChanged){
        const double w1=r.type==tvsTee::Branch?w->SizeAotv:w->SizeApr;
        w->put_Length(tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1));
    }
    if(changed||branchWidth)w->recordGraphicsModified();
}
}
