#pragma once
// Ventilator u mreži (user 2026-11-27): a linked inline block of the kind Fan (geometry/Equipment.h)
// has a suction side (usis) and a discharge side (potis). No attributes in the block (colleagues'
// blocks stay plain): the side is chosen by hand - temporary labels USIS / POTIS and an arrow of
// the air are drawn at the two ports (transient graphics, never in the drawing), Prihvati / Zamijeni.
// Asked by TVS_OPREMA after linking a fan without a side, by TVS_VENTILATOR at any time; TVS_PROTOK
// and TVS_PAD started on the fan show the labels while they work.
#include "EquipmentStore.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "acgiTransient.h"
#include "dbents.h"
#include <memory>
#include <vector>
namespace tvsFan {
// the record of a linked inline block (2 ports, not a linijski difuzor)
inline bool inlineRecord(const AcDbObjectId& id,tvsEquipment::Record& r){
    AcDbObjectPointer<AcDbBlockReference> b(id,AcDb::kForRead);if(b.openStatus()!=Acad::eOk)return false;
    return tvsEquipmentStore::read(b.object(),r)&&!tvsEquipmentStore::detached(b.object(),r)&&r.ports==2&&!r.twin;
}
inline bool isFan(const tvsEquipment::Record& r){return r.kind==tvsEquipment::Fan&&r.ports==2&&!r.twin;}
// USIS / POTIS at the ports and an arrow from the suction port to the discharge port
class Labels {
    AcGiTransientManager* manager;AcDbIntArray viewports;std::vector<AcDbEntity*> shown;
    void add(AcDbEntity* e){if(!manager){delete e;return;}
        if(manager->addTransient(e,kAcGiDirectShortTerm,128,viewports))shown.push_back(e);else delete e;}
public:
    Labels():manager(acgiGetTransientManager()){}
    ~Labels(){clear();}
    void clear(){for(size_t k=0;k<shown.size();++k){if(manager)manager->eraseTransient(shown[k],viewports);delete shown[k];}shown.clear();}
    void show(const tvsEquipment::Record& r,int suction,double f){
        clear();if(r.ports!=2||suction<0||suction>1)return;
        const double s=r.mmPerUnit>0?r.mmPerUnit/f:1;   // record units -> drawing units
        AcGePoint3d p[2];AcGeVector3d d[2];
        for(int k=0;k<2;++k){p[k]=AcGePoint3d(r.port[k].x*s,r.port[k].y*s,r.port[k].z*s);d[k]=AcGeVector3d(r.port[k].dx,r.port[k].dy,0);
            if(d[k].length()<1e-9)d[k]=AcGeVector3d(1,0,0);d[k].normalize();}
        const double size=(r.a>0?r.a:400)/f,h=(size*0.3>60/f?size*0.3:60/f),off=size*0.6+h;
        const wchar_t* text[2]={L"USIS",L"POTIS"};const Adesk::UInt16 colour[2]={4,1};
        for(int k=0;k<2;++k){const int port=k==0?suction:1-suction;
            AcDbText* t=new AcDbText();t->setTextString(text[k]);t->setHeight(h);t->setColorIndex(colour[k]);
            t->setHorizontalMode(AcDb::kTextCenter);t->setVerticalMode(AcDb::kTextVertMid);
            const AcGePoint3d at=p[port]+d[port]*off;t->setPosition(at);t->setAlignmentPoint(at);add(t);}
        // the air: from beyond the suction port through the block to beyond the discharge port
        const AcGePoint3d a=p[suction]+d[suction]*(size*0.4),b=p[1-suction]+d[1-suction]*(size*0.4);
        AcDbLine* l=new AcDbLine(a,b);l->setColorIndex(2);add(l);
        AcGeVector3d u=b-a;if(u.length()>1e-9){u.normalize();const AcGeVector3d n(-u.y,u.x,0);const double w=size*0.25;
            for(int e=0;e<2;++e){AcDbLine* x=new AcDbLine(b,b-u*w+n*(e==0?w*0.5:-w*0.5));x->setColorIndex(2);add(x);}}
        acedUpdateDisplay();
    }
};
// Shows the labels and asks; writes kind Fan and the suction port. false: cancelled / not written.
inline bool chooseSide(const AcDbObjectId& id){
    tvsEquipment::Record r;if(!inlineRecord(id,r)){acutPrintf(L"\nVentilator: blok nije povezan u liniju kanala (TVS_OPREMA).");return false;}
    AcDbDatabase* db=id.database();const double f=tvsUnits::factor(db);
    int suction=r.suction>=0?r.suction:0;Labels labels;
    for(;;){
        labels.show(r,suction,f);
        acedInitGet(0,L"Prihvati Zamijeni");ACHAR kw[16]=L"";wchar_t prompt[200];
        swprintf_s(prompt,L"\nVentilator %ls: strana usisa [Prihvati/Zamijeni] <Prihvati>: ",r.tag.empty()?L"":r.tag.c_str());
        const int rc=acedGetKword(prompt,kw,16);
        if(rc==RTNORM&&_wcsicmp(kw,L"Zamijeni")==0){suction=1-suction;continue;}
        if(rc!=RTNORM&&rc!=RTNONE){acutPrintf(L"\nVentilator: strana usisa nije upisana.");return false;}
        break;}
    labels.clear();
    tvsEquipment::Record w=r;w.kind=tvsEquipment::Fan;w.suction=suction;
    AcDbObjectPointer<AcDbBlockReference> b(id,AcDb::kForWrite);
    if(b.openStatus()!=Acad::eOk||tvsEquipmentStore::write(b.object(),w)!=Acad::eOk){acutPrintf(L"\nVentilator: zapis nije upisan (zaključan sloj?).");return false;}
    b->recordGraphicsModified();
    acutPrintf(L"\nVentilator: usis i potis upisani.");return true;
}
// TVS_VENTILATOR: pick linked inline blocks; each becomes a fan and its suction side is chosen.
inline void execute(){
    for(;;){ads_name e;ads_point p;
        const int rc=acedEntSel(L"\nTVS_VENTILATOR: izaberite blok ventilatora (povezan sa TVS_OPREMA): ",e,p);
        if(rc==RTCAN||rc==RTNONE)return;if(rc!=RTNORM)continue;
        AcDbObjectId id;if(acdbGetObjectId(id,e)!=Acad::eOk)continue;
        if(id.objectClass()!=AcDbBlockReference::desc()){acutPrintf(L"\nIzaberite blok.");continue;}
        chooseSide(id);return;}
}
}
