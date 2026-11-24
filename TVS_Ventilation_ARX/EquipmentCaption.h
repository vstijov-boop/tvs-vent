#pragma once
// TVS_NATPIS for oprema na mrezi (user 2026-11-23): with the duct captions on, every block that
// is equipment shows a small caption above it (screen only, never plotted):
//   "+ DAS-1 / 550 m3/h / 30 Pa"  linked into the network (TVS_OPREMA record, geometry/Equipment.h)
//   "- Dif_1"                     a block whose NAME is known as equipment (TVS_OPREMA_TIPOVI
//                                 default) but this one is not linked
// Drawn by an AcGiDrawableOverrule on AcDbBlockReference: the block draws itself, the caption is
// added; nothing is stored in the drawing. Text and switch as tvsDuctCaption (DuctCaption.h).
#include "rxoverrule.h"
#include "drawable.h"
#include "dbents.h"
#include "dbobjptr.h"
#include "acgi.h"
#include "EquipmentStore.h"
#include "../TVS_Ventilation_DBX/DuctCaption.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include <cwchar>
#include <string>
namespace tvsEquipmentCaption {
// Caption text of a linked record (pure; ASCII only so romans.shx can draw it).
inline std::wstring linkedText(const tvsEquipment::Record& r){
    std::wstring s=L"+ ";
    s+=!r.tag.empty()?r.tag:std::wstring(r.terminal()?L"terminal":L"oprema");
    wchar_t b[64];
    if(r.terminal()&&r.qTerminal>0){swprintf_s(b,L" / %.0f m3/h",r.qTerminal);s+=b;}
    switch(r.model){
    case tvsEquipment::FixedDp:swprintf_s(b,L" / %.0f Pa",r.dp);s+=b;break;
    case tvsEquipment::Zeta:swprintf_s(b,L" / zeta %.2f",r.zeta);s+=b;break;
    case tvsEquipment::Nominal:
        if(r.terminal()&&r.qTerminal>0&&r.qNominal>0){swprintf_s(b,L" / %.0f Pa",tvsEquipment::loss(r,r.qTerminal,0));s+=b;}
        else{swprintf_s(b,L" / %.0f Pa pri %.0f m3/h",r.dpNominal,r.qNominal);s+=b;}
        break;
    case tvsEquipment::Curve:
        if(r.curve.empty())s+=L" / kriva ?";
        else if(r.terminal()&&r.qTerminal>0){swprintf_s(b,L" / %.0f Pa (kriva)",tvsEquipment::loss(r,r.qTerminal,0));s+=b;}
        else{swprintf_s(b,L" / kriva %d tac.",static_cast<int>(r.curve.size()));s+=b;}
        break;
    default:s+=L" / dp ?";
    }
    return s;
}
inline std::wstring nameOf(const AcDbBlockReference* ref){
    AcDbObjectPointer<AcDbBlockTableRecord> btr(ref->blockTableRecord(),AcDb::kForRead);if(btr.openStatus()!=Acad::eOk)return std::wstring();
    const ACHAR* n=NULL;if(btr->getName(n)!=Acad::eOk||!n)return std::wstring();return std::wstring(n);
}
inline bool& busy(){static bool b=false;return b;}   // no caption while the caption itself asks for extents
class Overrule : public AcGiDrawableOverrule {
public:
    virtual bool isApplicable(const AcRxObject* subject) const {return AcDbBlockReference::cast(subject)!=NULL;}
    virtual Adesk::Boolean worldDraw(AcGiDrawable* subject,AcGiWorldDraw* wd){
        const Adesk::Boolean done=AcGiDrawableOverrule::worldDraw(subject,wd);
        AcDbBlockReference* ref=AcDbBlockReference::cast(subject);
        if(!ref||!wd||busy()||wd->context()->isPlotGeneration())return done;
        if(wd->regenType()==kAcGiForExplode||wd->regenType()==kAcGiSaveWorldDrawForProxy)return done;   // never into EXPLODE / proxy graphics
        // only blocks of the drawing being edited: never xref inserts / blocks inside xrefs (their
        // database is not the working one) - big xref'd plans redraw a lot (user crash report 24.11.)
        AcDbDatabase* db=ref->database();
        if(!db||db!=acdbHostApplicationServices()->workingDatabase())return done;
        if(!tvsDuctCaption::enabled(db))return done;
        tvsEquipment::Record r;std::wstring text;int colour=3;
        if(tvsEquipmentStore::read(ref,r)){
            if(tvsEquipmentStore::detached(ref,r)){text=L"- "+(r.tag.empty()?nameOf(ref):r.tag)+L" odvojen (TVS_OPREMA)";colour=1;}   // moved / copied after linking
            else text=linkedText(r);}
        else{const std::wstring n=nameOf(ref);tvsEquipment::Record d;
            if(n.empty()||n[0]==L'*'||!tvsEquipmentStore::readDefault(db,n.c_str(),d))return done;
            text=L"- "+n;colour=1;}
        AcDbExtents ext;Acad::ErrorStatus es;{busy()=true;es=ref->getGeomExtents(ext);busy()=false;}
        if(es!=Acad::eOk)return done;
        const double f=tvsUnits::factor(db),h=tvsDuctCaption::textMm()/f;
        AcGiTextStyle style;style.setFileName(L"romans.shx");style.setTextSize(h);style.setXScale(0.7);style.loadStyleRec();
        const AcGePoint3d p(ext.minPoint().x,ext.maxPoint().y+0.5*h,ref->position().z);
        AcGiSubEntityTraits& traits=wd->subEntityTraits();const Adesk::UInt16 old=traits.color();const AcDb::LineWeight oldLw=traits.lineWeight();
        traits.setColor(static_cast<Adesk::UInt16>(colour));traits.setLineWeight(AcDb::kLnWt009);
        wd->geometry().text(p,AcGeVector3d::kZAxis,AcGeVector3d::kXAxis,text.c_str(),-1,Adesk::kFalse,style);
        traits.setLineWeight(oldLw);traits.setColor(old);
        return done;
    }
};
inline Overrule& instance(){static Overrule o;return o;}
inline bool& running(){static bool on=false;return on;}
inline void start(){
    if(running())return;
    if(AcRxOverrule::addOverrule(AcDbBlockReference::desc(),&instance(),false)==Acad::eOk){running()=true;AcRxOverrule::setIsOverruling(true);}
}
inline void stop(){
    if(!running())return;
    AcRxOverrule::removeOverrule(AcDbBlockReference::desc(),&instance());running()=false;
}
}
