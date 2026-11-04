#pragma once
// TVS_PRESJEK (faza P1): poprecni presjek of the TVS network along a section line in plan.
// Every TVS element the line cuts is drawn at its real elevation (rectangle with a cross, or
// a circle), with its size and bottom elevation; two lines give the floor level and the
// level of the floor above. The section is a block TVS_PRESJEK_<oznaka> whose parameters are
// stored with it, so "Osvjezi" rebuilds it after the network changed (no manual editing).
#include "../geometry/Section.h"
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/TVS_TRANS.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "TVSMoveCommand.h"
#include "TvsLinetypes.h"
#include <dbjig.h>
#include <acgi.h>
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbsymtb.h"
#include "dbobjptr.h"
#include <vector>
#include <string>
#include <cwchar>
namespace tvsSectionCmd {
// depth: zone behind the section plane (drawing units, 0 = cut elements only);
// hidden: 0 = parts covered by nearer elements are left out, 1 = drawn dashed.
struct Params {AcGePoint3d p1,p2;bool flip;double floor,ceiling,scale,depth;int hidden;std::wstring letter;   // floor/ceiling: drawing units
    Params():flip(false),floor(0),ceiling(0),scale(50),depth(0),hidden(0){}};
struct Remembered {double floorM,ceilingM,scale;int hidden;Remembered():floorM(0),ceilingM(3),scale(50),hidden(0){}};
inline Remembered& remembered(){static Remembered r;return r;}
inline const ACHAR* key(){return L"TVS_PRESJEK_V1";}
inline const ACHAR* layerName(){return L"TVS_PRESJEK";}
inline std::wstring blockName(const std::wstring& letter){return L"TVS_PRESJEK_"+letter;}

inline AcDbObjectId layer(AcDbDatabase* db){
    AcDbLayerTable* raw=NULL;if(db->getLayerTable(raw,AcDb::kForWrite)!=Acad::eOk)return db->clayer();
    AcDbObjectPointer<AcDbLayerTable> table;table.acquire(raw);AcDbObjectId id;
    if(table->getAt(layerName(),id)==Acad::eOk)return id;
    AcDbLayerTableRecord* rec=new AcDbLayerTableRecord();rec->setName(layerName());
    if(table->add(id,rec)!=Acad::eOk){delete rec;return db->clayer();}
    rec->close();return id;
}
inline AcDbObjectId textStyle(AcDbDatabase* db){
    AcDbTextStyleTable* raw=NULL;if(db->getTextStyleTable(raw,AcDb::kForRead)!=Acad::eOk)return db->textstyle();
    AcDbObjectPointer<AcDbTextStyleTable> table;table.acquire(raw);AcDbObjectId id;
    return table->getAt(L"TVS_TextStyle",id)==Acad::eOk?id:db->textstyle();
}
// TVS network as plan members (see geometry/Section.h).
inline void members(AcDbDatabase* db,std::vector<tvsSection::Member>& out){
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return;
    int tag=0;
    for(;!it->done();it->step()){
        AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
        tvsMove::Node n;
        if(TVS_Entity::cast(e)&&tvsMove::describe(e,n)&&n.ports.size()==n.sections.size()&&n.ports.size()>=2){
            ++tag;
            auto add=[&](const tvsMove::Point& a,const tvsMove::Point& b,const tvsMoveNetwork::Section& sa,const tvsMoveNetwork::Section& sb){
                tvsSection::Member m;m.a=tvsSection::P(a.x,a.y);m.b=tvsSection::P(b.x,b.y);
                m.A1=sa.a;m.B1=sa.b;m.A2=sb.a;m.B2=sb.b;m.round=sa.round;m.z=n.elevation;m.tag=tag;out.push_back(m);};
            if(TVS_Pipe::cast(e)||TVS_TRANS::cast(e))add(n.ports[0],n.ports[1],n.sections[0],n.sections[1]);
            else for(size_t j=0;j<n.ports.size();++j)add(n.middle,n.ports[j],n.sections[j],n.sections[j]);
        }
        e->close();
    }
    delete it;
}
inline AcDbObjectId append(AcDbBlockTableRecord* btr,AcDbEntity* e,const AcDbObjectId& layerId){
    e->setDatabaseDefaults(btr->database());e->setLayer(layerId);AcDbObjectId id;
    if(btr->appendAcDbEntity(id,e)!=Acad::eOk){delete e;return AcDbObjectId::kNull;}
    e->close();return id;
}
inline void text(AcDbBlockTableRecord* btr,const AcGePoint3d& at,const wchar_t* s,double h,const AcDbObjectId& style,const AcDbObjectId& layerId,bool left=false){
    AcDbText* t=new AcDbText(at,s,style,h,0);
    t->setDatabaseDefaults(btr->database());t->setTextStyle(style);t->setHeight(h);
    t->setHorizontalMode(left?AcDb::kTextRight:AcDb::kTextCenter);t->setVerticalMode(AcDb::kTextVertMid);t->setAlignmentPoint(at);
    append(btr,t,layerId);
}
inline void line(AcDbBlockTableRecord* btr,double x1,double y1,double x2,double y2,const AcDbObjectId& layerId){
    append(btr,new AcDbLine(AcGePoint3d(x1,y1,0),AcGePoint3d(x2,y2,0)),layerId);
}
inline std::wstring level(double drawing,double f){wchar_t s[40];swprintf_s(s,L"%+.3f",drawing*f/1000);return s;}
// Rebuilds the block contents from the stored parameters. Returns the number of ducts cut.
inline int build(AcDbBlockTableRecord* btr,const Params& p){
    AcDbDatabase* db=btr->database();const double f=tvsUnits::factor(db);
    {   // clear
        AcDbBlockTableRecordIterator* it=NULL;
        if(btr->newIterator(it)==Acad::eOk){for(;!it->done();it->step()){AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForWrite)==Acad::eOk){e->erase();e->close();}}delete it;}
    }
    const AcDbObjectId layerId=layer(db),style=textStyle(db);
    const double th=2.5*p.scale/f,gap=0.6*th;
    std::vector<tvsSection::Member> net;members(db,net);
    const tvsSection::P a(p.p1.x,p.p1.y),b(p.p2.x,p.p2.y);
    const std::vector<tvsSection::Cut> cuts=tvsSection::cuts(a,b,net,p.flip,2.0/f);
    const double L=p.p1.distanceTo(p.p2),margin=(std::max)(500/f,6*th),H=p.ceiling-p.floor;
    // floor and floor above
    line(btr,-margin,0,L+margin,0,layerId);
    line(btr,-margin,H,L+margin,H,layerId);
    text(btr,AcGePoint3d(-margin-gap,0,0),level(p.floor,f).c_str(),th,style,layerId,true);
    text(btr,AcGePoint3d(-margin-gap,H,0),level(p.ceiling,f).c_str(),th,style,layerId,true);
    for(size_t i=0;i<cuts.size();++i){
        const tvsSection::Cut& c=cuts[i];const double x=c.s,y=c.z-p.floor,hw=c.w/2,hh=c.h/2;
        if(c.round){
            if(c.sinA>0.98)append(btr,new AcDbCircle(AcGePoint3d(x,y,0),AcGeVector3d::kZAxis,c.A/2),layerId);
            else append(btr,new AcDbEllipse(AcGePoint3d(x,y,0),AcGeVector3d::kZAxis,AcGeVector3d(hw,0,0),hh/hw),layerId);
        }else{
            AcDbPolyline* r=new AcDbPolyline(4);
            r->addVertexAt(0,AcGePoint2d(x-hw,y-hh));r->addVertexAt(1,AcGePoint2d(x+hw,y-hh));
            r->addVertexAt(2,AcGePoint2d(x+hw,y+hh));r->addVertexAt(3,AcGePoint2d(x-hw,y+hh));r->setClosed(true);
            append(btr,r,layerId);
            line(btr,x-hw,y-hh,x+hw,y+hh,layerId);line(btr,x-hw,y+hh,x+hw,y-hh,layerId);
        }
        wchar_t size[64];
        if(c.round)swprintf_s(size,L"%%%%c%.0f",c.A*f);else swprintf_s(size,L"%.0fx%.0f",c.A*f,c.B*f);
        text(btr,AcGePoint3d(x,y+hh+gap+th/2,0),size,th,style,layerId);
        const std::wstring bottom=L"dno "+level(c.z-hh,f);
        text(btr,AcGePoint3d(x,y-hh-gap-th/2,0),bottom.c_str(),th,style,layerId);
        for(size_t j=0;j<i;++j)if(tvsSection::overlap(cuts[j],c)){
            acutPrintf(L"\nPresjek %ls: kanali se preklapaju kod %.0f mm od pocetka linije.",p.letter.c_str(),c.s*f);break;}
    }
    // elements behind the section plane (dubina), thin outline; covered parts left out or dashed
    const std::vector<tvsSection::View> views=tvsSection::views(a,b,net,p.flip,p.depth);
    const std::vector<tvsSection::Edge> edges=tvsSection::edges(cuts,views);
    AcDbObjectId dashed;
    if(p.hidden){tvsLinetypes::ensure(db);AcDbLinetypeTable* lt=NULL;
        if(db->getLinetypeTable(lt,AcDb::kForRead)==Acad::eOk){lt->getAt(L"tvs_hidden",dashed);lt->close();}}
    for(size_t i=0;i<edges.size();++i){const tvsSection::Edge& g=edges[i];
        if(g.hidden&&!p.hidden)continue;
        AcDbLine* l=new AcDbLine(AcGePoint3d(g.x0,g.y0-p.floor,0),AcGePoint3d(g.x1,g.y1-p.floor,0));
        const AcDbObjectId id=append(btr,l,layerId);
        if(g.hidden&&!dashed.isNull()){AcDbObjectPointer<AcDbLine> o(id,AcDb::kForWrite);if(o.openStatus()==Acad::eOk){o->setLinetype(dashed);o->setLinetypeScale(1/f);}}
    }
    const std::wstring title=L"PRESJEK "+p.letter+L"-"+p.letter;
    text(btr,AcGePoint3d(L/2,-4*th,0),title.c_str(),1.4*th,style,layerId);
    if(!views.empty())acutPrintf(L"\nPresjek %ls: vidljivih elemenata u dubini %d.",p.letter.c_str(),static_cast<int>(views.size()));
    return static_cast<int>(cuts.size());
}
inline void store(AcDbBlockTableRecord* btr,const Params& p){
    if(btr->createExtensionDictionary()!=Acad::eOk&&btr->extensionDictionary().isNull())return;
    AcDbObjectPointer<AcDbDictionary> dict(btr->extensionDictionary(),AcDb::kForWrite);if(dict.openStatus()!=Acad::eOk)return;
    resbuf* data=acutBuildList(1,L"TVS_PRESJEK/1",1,p.letter.c_str(),10,asDblArray(p.p1),10,asDblArray(p.p2),70,p.flip?1:0,
        40,p.floor,40,p.ceiling,40,p.scale,40,p.depth,70,p.hidden,RTNONE);
    AcDbObjectId id;AcDbXrecord* rec=NULL;
    if(dict->getAt(key(),id)==Acad::eOk){if(acdbOpenObject(rec,id,AcDb::kForWrite)!=Acad::eOk)rec=NULL;}
    else{rec=new AcDbXrecord();if(dict->setAt(key(),rec,id)!=Acad::eOk){delete rec;rec=NULL;}}
    if(rec){rec->setFromRbChain(*data);rec->close();}
    acutRelRb(data);
}
inline bool load(const AcDbBlockTableRecord* btr,Params& p){
    const AcDbObjectId dictId=btr->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    const short types[]={1,1,10,10,70,40,40,40};resbuf* q=data;bool ok=true;
    for(int j=0;j<8;++j){if(!q||q->restype!=types[j]){ok=false;break;}
        switch(j){case 1:p.letter=q->resval.rstring;break;case 2:p.p1=asPnt3d(q->resval.rpoint);break;case 3:p.p2=asPnt3d(q->resval.rpoint);break;
            case 4:p.flip=q->resval.rint!=0;break;case 5:p.floor=q->resval.rreal;break;case 6:p.ceiling=q->resval.rreal;break;case 7:p.scale=q->resval.rreal;break;}
        q=q->rbnext;}
    if(ok&&q&&q->restype==40){p.depth=q->resval.rreal;q=q->rbnext;if(q&&q->restype==70)p.hidden=q->resval.rint;}   // P1b; older sections: no depth
    acutRelRb(data);return ok;
}
// Section line in plan: line, view arrows and letters (plain entities).
inline void planMarks(AcDbDatabase* db,const Params& p){
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk)return;
    const double f=tvsUnits::factor(db),th=2.5*p.scale/f;const AcDbObjectId layerId=layer(db),style=textStyle(db);
    AcGeVector3d d=p.p2-p.p1;d.z=0;if(d.length()<1e-12)return;d.normalize();
    AcGeVector3d n(-d.y,d.x,0);if(p.flip)n=-n;   // looking direction
    AcDbLine* l=new AcDbLine(p.p1,p.p2);append(space.object(),l,layerId);
    if(p.depth>0){   // zone of the section depth: thin dashed, colour 9
        tvsLinetypes::ensure(db);AcDbObjectId dashed;AcDbLinetypeTable* lt=NULL;
        if(db->getLinetypeTable(lt,AcDb::kForRead)==Acad::eOk){lt->getAt(L"tvs_hidden",dashed);lt->close();}
        const AcGePoint3d c[4]={p.p1,p.p2,p.p2+n*p.depth,p.p1+n*p.depth};
        for(int k=1;k<4;++k){const AcDbObjectId id=append(space.object(),new AcDbLine(c[k],c[(k+1)%4]),layerId);
            AcDbObjectPointer<AcDbLine> o(id,AcDb::kForWrite);if(o.openStatus()==Acad::eOk){o->setColorIndex(9);if(!dashed.isNull()){o->setLinetype(dashed);o->setLinetypeScale(1/f);}}}
    }
    const AcGePoint3d ends[2]={p.p1,p.p2};
    for(int k=0;k<2;++k){
        append(space.object(),new AcDbLine(ends[k],ends[k]+n*(4*th)),layerId);
        append(space.object(),new AcDbLine(ends[k]+n*(4*th),ends[k]+n*(3*th)+d*(0.5*th)),layerId);
        append(space.object(),new AcDbLine(ends[k]+n*(4*th),ends[k]+n*(3*th)-d*(0.5*th)),layerId);
        const AcGePoint3d at=ends[k]+n*(5.5*th);
        AcDbText* t=new AcDbText(at,p.letter.c_str(),style,1.4*th,0);t->setDatabaseDefaults(db);t->setTextStyle(style);t->setHeight(1.4*th);
        t->setHorizontalMode(AcDb::kTextCenter);t->setVerticalMode(AcDb::kTextVertMid);t->setAlignmentPoint(at);append(space.object(),t,layerId);
    }
}
inline bool blockExists(AcDbDatabase* db,const std::wstring& name){
    AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForRead)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbBlockTable> t;t.acquire(raw);return t->has(name.c_str());
}
inline std::wstring nextLetter(AcDbDatabase* db){
    for(wchar_t c=L'A';c<=L'Z';++c){std::wstring s(1,c);if(!blockExists(db,blockName(s)))return s;}
    for(int i=1;i<1000;++i){wchar_t s[16];swprintf_s(s,L"P%d",i);if(!blockExists(db,blockName(s)))return s;}
    return L"X";
}
inline void refreshReferences(AcDbBlockTableRecord* btr){
    AcDbObjectIdArray refs;if(btr->getBlockReferenceIds(refs)!=Acad::eOk)return;
    for(int i=0;i<refs.length();++i){AcDbObjectPointer<AcDbBlockReference> r(refs[i],AcDb::kForWrite);if(r.openStatus()==Acad::eOk)r->recordGraphicsModified();}
}
inline void refresh(){
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite presjek za osvjezavanje: ",name,pick)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    AcDbObjectPointer<AcDbBlockReference> ref(id,AcDb::kForRead);if(ref.openStatus()!=Acad::eOk){acutPrintf(L"\nIzabrani objekat nije TVS presjek.");return;}
    const AcDbObjectId btrId=ref->blockTableRecord();ref.close();
    AcDbObjectPointer<AcDbBlockTableRecord> btr(btrId,AcDb::kForWrite);if(btr.openStatus()!=Acad::eOk)return;
    Params p;if(!load(btr.object(),p)){acutPrintf(L"\nIzabrani blok nije TVS presjek.");return;}
    const int n=build(btr.object(),p);refreshReferences(btr.object());
    acutPrintf(L"\nPresjek %ls-%ls osvjezen: presjecenih elemenata %d.",p.letter.c_str(),p.letter.c_str(),n);
}
// Third point of the section: side (looking direction) and depth, zone drawn while dragging.
class ZonePreview : public AcDbEntity {
public:
    AcGePoint3d p1,p2;bool flip;double depth;ZonePreview():flip(false),depth(0){}
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* w){
        AcGeVector3d d=p2-p1;d.z=0;d.normalize();AcGeVector3d n(-d.y,d.x,0);if(flip)n=-n;
        AcGePoint3d c[5]={p1,p2,p2+n*depth,p1+n*depth,p1};w->subEntityTraits().setColor(9);w->geometry().polyline(5,c);
        AcGePoint3d m=p1+(p2-p1)/2,arrow[2]={m,m+n*(p1.distanceTo(p2)*0.08)};w->subEntityTraits().setColor(3);w->geometry().polyline(2,arrow);
        return Adesk::kTrue;}
};
class DepthJig : public AcEdJig {
    ZonePreview z;AcGePoint3d last;
public:
    bool flip;double depth;
    DepthJig(const AcGePoint3d& a,const AcGePoint3d& b):flip(false),depth(0){z.p1=a;z.p2=b;last=b;
        setDispPrompt(L"\nStrana gledanja i dubina presjeka (povucite misem): ");
        setUserInputControls(static_cast<UserInputControls>(kAccept3dCoordinates|kGovernedByOrthoMode));}
    virtual AcDbEntity* entity()const{return const_cast<ZonePreview*>(&z);}
    virtual DragStatus sampler(){AcGePoint3d q;const DragStatus s=acquirePoint(q);if(s!=kNormal)return s;
        if(q.distanceTo(last)<1e-9)return kNoChange;last=q;return kNormal;}
    virtual Adesk::Boolean update(){
        AcGeVector3d d=z.p2-z.p1;d.z=0;d.normalize();const AcGeVector3d left(-d.y,d.x,0);
        AcGeVector3d s=last-z.p1;s.z=0;const double v=s.dotProduct(left);
        z.flip=flip=v<0;z.depth=depth=std::fabs(v);return Adesk::kTrue;}
};
inline bool askLevel(const wchar_t* prompt,double& valueM){
    wchar_t text[160];swprintf_s(text,L"%ls <%.2f>: ",prompt,valueM);acedInitGet(0,NULL);double v=0;
    const int rc=acedGetReal(text,&v);if(rc==RTNONE)return true;if(rc!=RTNORM)return false;valueM=v;return true;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db);
    ads_point a,b;
    acedInitGet(0,L"Osvjezi");
    int rc=acedGetPoint(NULL,L"\nPrva tacka linije presjeka ili [Osvjezi]: ",a);
    if(rc==RTKWORD){refresh();return;}if(rc!=RTNORM)return;
    if(acedGetPoint(a,L"\nDruga tacka linije presjeka: ",b)!=RTNORM)return;
    Params p;
    {   // UCS -> WCS
        resbuf from={};from.restype=RTSHORT;from.resval.rint=1;resbuf to={};to.restype=RTSHORT;to.resval.rint=0;
        ads_point wa,wb;acedTrans(a,&from,&to,0,wa);acedTrans(b,&from,&to,0,wb);
        p.p1=asPnt3d(wa);p.p2=asPnt3d(wb);p.p1.z=p.p2.z=0;
        if(AcGeVector3d(p.p2.x-p.p1.x,p.p2.y-p.p1.y,0).length()<1e-9){acutPrintf(L"\nLinija presjeka je prekratka.");return;}
    }
    {   // side and depth by dragging (as in Revit): the zone follows the cursor
        DepthJig jig(p.p1,p.p2);if(jig.drag()!=AcEdJig::kNormal)return;
        p.flip=jig.flip;p.depth=jig.depth<1.0/f?0:jig.depth;   // < 1 mm: cut elements only
    }
    {   Remembered& m0=remembered();
        if(p.depth>0){acedInitGet(0,L"Izostavi Isprekidano");ACHAR hk[32]=L"";
            wchar_t hp[120];swprintf_s(hp,L"\nZaklonjeni dijelovi [Izostavi/Isprekidano] <%ls>: ",m0.hidden?L"Isprekidano":L"Izostavi");
            const int hr=acedGetKword(hp,hk);if(hr!=RTNORM&&hr!=RTNONE)return;
            if(hr==RTNORM)m0.hidden=_wcsicmp(hk,L"Isprekidano")==0?1:0;}
        p.hidden=m0.hidden;
    }
    const std::wstring def=nextLetter(db);
    wchar_t prompt[120];swprintf_s(prompt,L"\nOznaka presjeka <%ls>: ",def.c_str());ACHAR input[64]=L"";
    rc=acedGetString(0,prompt,input);if(rc!=RTNORM&&rc!=RTNONE)return;
    p.letter=(rc==RTNORM&&input[0])?std::wstring(input):def;
    if(blockExists(db,blockName(p.letter))){acutPrintf(L"\nPresjek %ls vec postoji; za novi sadrzaj koristite Osvjezi ili drugu oznaku.",p.letter.c_str());return;}
    Remembered& m=remembered();
    if(!askLevel(L"\nKota poda etaze, m",m.floorM))return;
    if(!askLevel(L"\nKota gornje etaze, m",m.ceilingM))return;
    {wchar_t sp[80];swprintf_s(sp,L"\nRazmjera 1: <%.0f>: ",m.scale);acedInitGet(RSG_NOZERO|RSG_NONEG,NULL);double v=0;
     rc=acedGetReal(sp,&v);if(rc==RTNORM)m.scale=v;else if(rc!=RTNONE)return;}
    p.floor=m.floorM*1000/f;p.ceiling=m.ceilingM*1000/f;p.scale=m.scale;
    ads_point ins;if(acedGetPoint(NULL,L"\nTacka umetanja presjeka (lijevi kraj, kota poda): ",ins)!=RTNORM)return;
    AcGePoint3d position;{resbuf from={};from.restype=RTSHORT;from.resval.rint=1;resbuf to={};to.restype=RTSHORT;to.resval.rint=0;ads_point w;acedTrans(ins,&from,&to,0,w);position=asPnt3d(w);}
    AcDbObjectId btrId;
    {
        AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForWrite)!=Acad::eOk)return;
        AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
        AcDbBlockTableRecord* btr=new AcDbBlockTableRecord();btr->setName(blockName(p.letter).c_str());
        if(table->add(btrId,btr)!=Acad::eOk){delete btr;acutPrintf(L"\nBlok presjeka nije napravljen.");return;}
        const int n=build(btr,p);store(btr,p);btr->close();
        acutPrintf(L"\nPresjek %ls-%ls: presjecenih elemenata %d.",p.letter.c_str(),p.letter.c_str(),n);
    }
    {
        AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk)return;
        AcDbBlockReference* ref=new AcDbBlockReference(position,btrId);
        append(space.object(),ref,layer(db));
    }
    planMarks(db,p);
}
}
