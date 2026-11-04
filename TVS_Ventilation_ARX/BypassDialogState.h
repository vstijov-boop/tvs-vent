#pragma once
// Pure (Win32/ObjectARX-free) state rules of the bypass dialog, so they can be
// regression-tested. The dialog in BypassPreviewCommand.cpp only calls these.
#include <string>
namespace tvsBypassUx {
static const int FieldCount=26;
// Field indices that matter for the rules below (same order as the dialog).
enum Field {Ri=0,Insulation=1,ObstacleInsulation=2,WindowLeft=8,WindowRight=9,Floor=10,Ceiling=11};

// Session memory of user design settings for ONE document (lives in CDocData).
// Deliberately holds no object ids, snapshot, geometry, units or candidate.
struct Memory {
    bool valid;std::wstring values[FieldCount];int angle,side;
    Memory():valid(false),angle(1),side(0){}
};
// Ri comes from the TVS radius mode; window s and absolute Z limits always come
// from the newly selected route/obstacle. Everything else is a user design setting.
inline bool remembered(int field){
    return field!=Ri&&field!=WindowLeft&&field!=WindowRight&&field!=Floor&&field!=Ceiling;
}
inline void store(Memory& m,const std::wstring values[FieldCount],int angle,int side){
    for(int j=0;j<FieldCount;++j)m.values[j]=remembered(j)?values[j]:std::wstring();
    m.angle=angle;m.side=side;m.valid=true;
}
// Returns true when remembered insulation was offered (shown as a proposal).
inline bool apply(const Memory& m,std::wstring values[FieldCount],int& angle,int& side){
    if(!m.valid)return false;
    for(int j=0;j<FieldCount;++j)if(remembered(j))values[j]=m.values[j];
    angle=m.angle;side=m.side;
    return !m.values[Insulation].empty()||!m.values[ObstacleInsulation].empty();
}

// TVS rectangular elbow radius modes (values of RadiusTypeRect_* in TVS_TAP.h).
enum RadiusMode {RadiusConstant=1,RadiusVariable=2,RadiusSpecific=3};
struct RadiusSetting {int mode;double k,constantDrawing;RadiusSetting():mode(0),k(0),constantDrawing(0){}};
struct RadiusProposal {bool ok;double ri;const wchar_t* source;RadiusProposal():ok(false),ri(0),source(L""){}};
// Inner radius Ri in mm for a vertical (Gore/Dolje) rectangular elbow, whose bend
// plane dimension is B. Mirrors TVS_TAP::get_radius; otherwise Rc = 1.0 x B.
inline RadiusProposal radius(const RadiusSetting& s,double heightMm,double factor){
    RadiusProposal p;
    if(!(heightMm>0)||!(factor>0))return p;
    p.ok=true;
    if(s.mode==RadiusConstant&&s.constantDrawing>0){p.ri=s.constantDrawing*factor;p.source=L"TVS konstantni radijus";}
    else if(s.mode==RadiusVariable&&s.k>0){p.ri=s.k*heightMm;p.source=L"TVS radijus k*B";}
    else if(s.mode==RadiusSpecific){p.ri=heightMm<355.0?heightMm:heightMm/2;p.source=L"TVS specificni radijus";}
    else {p.ri=heightMm/2;p.source=L"prijedlog Rc = 1,0 x B";}
    return p;
}

// Which control changes invalidate the computed candidate. Numeric inputs,
// pass side, angle, units and the confirmation do; display controls do not.
static const int kBnClicked=0,kCbnSelChange=1,kEnChange=0x0300;
enum Control {FirstEdit=100,Side=500,Angle=501,Units=502,Confirm=503,Advanced=504,Dimensions=505,Axes=506,EdgeMode=507};
inline bool invalidatesResult(int id,int code){
    if(id>=FirstEdit&&id<FirstEdit+FieldCount)return code==kEnChange;
    if(id==Side||id==Angle||id==Units)return code==kCbnSelChange;
    if(id==Confirm)return code==kBnClicked;
    return false;
}

// Command-line answer after the in-drawing preview.
enum PreviewChoice {Accept,BackToDialog,Cancel};
inline PreviewChoice previewChoice(bool normal,bool none,const wchar_t* answer,bool canAccept){
    if(!normal&&!none)return Cancel;                       // Esc / error
    if(none||!answer||!*answer)return BackToDialog;        // Enter = default <Dijalog>
    std::wstring a(answer);for(auto& c:a)if(c>=L'a'&&c<=L'z')c=static_cast<wchar_t>(c-32);
    if(canAccept&&a==L"PRIHVATI")return Accept;
    if(a==L"ODUSTANI")return Cancel;
    return BackToDialog;
}
}
