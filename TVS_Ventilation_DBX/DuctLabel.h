#pragma once
// TVS_OZNAKE: printed labels on straight ducts - size/flow without units, e.g. 400x150/1150 or
// %%c250/500 (size only without flow); the legend of the drawing explains them. Drawn by the duct
// itself (always current: size, flow, position follow without any update) on layer AT-DIM_KANAL
// (colour / lineweight ByLayer, plotted; freeze the layer to hide), romans.shx, width 0.7, height
// 2.5 mm x drawing scale. Only where the text fits the duct length. Position above / on the axis /
// below the duct (readable from below / right): default per drawing, per duct xdata TVS_OZNAKA.
// Per drawing: named-object dictionary TVS_OZNAKE_V1 = (70 on)(70 position)(40 scale).
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbsymtb.h"
#include "dbobjptr.h"
#include "dbents.h"
#include "acgi.h"
#include "DrawingUnits.h"
#include <cmath>
#include <cwchar>
namespace tvsDuctLabel {
enum Position {Below=0,Middle=1,Above=2,Default=-1};
inline const ACHAR* key(){return L"TVS_OZNAKE_V1";}
inline const ACHAR* app(){return L"TVS_OZNAKA";}
inline const ACHAR* layerName(){return L"AT-DIM_KANAL";}
inline double paperMm(){return 2.5;}
struct Settings {bool on;int position;double scale;Settings():on(false),position(Above),scale(50){}};
inline Settings read(AcDbDatabase* db){
    Settings s;if(!db)return s;AcDbDictionary* nod=NULL;
    if(db->getNamedObjectsDictionary(nod,AcDb::kForRead)!=Acad::eOk||!nod)return s;
    AcDbObjectId id;const bool has=nod->getAt(key(),id)==Acad::eOk;nod->close();if(!has)return s;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return s;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return s;
    int shorts=0;for(resbuf* r=data;r;r=r->rbnext){
        if(r->restype==70){if(shorts==0)s.on=r->resval.rint!=0;else if(shorts==1&&r->resval.rint>=0&&r->resval.rint<=2)s.position=r->resval.rint;++shorts;}
        else if(r->restype==40&&r->resval.rreal>0)s.scale=r->resval.rreal;}
    acutRelRb(data);return s;
}
// The command (ARX) writes the settings; the caller regenerates.
inline bool write(AcDbDatabase* db,const Settings& s){
    AcDbDictionary* nod=NULL;if(!db||db->getNamedObjectsDictionary(nod,AcDb::kForWrite)!=Acad::eOk||!nod)return false;
    AcDbObjectId id;AcDbXrecord* rec=NULL;bool ok=false;
    if(nod->getAt(key(),id)==Acad::eOk){if(acdbOpenObject(rec,id,AcDb::kForWrite)!=Acad::eOk)rec=NULL;}
    else{rec=new AcDbXrecord();if(nod->setAt(key(),rec,id)!=Acad::eOk){delete rec;rec=NULL;}}
    if(rec){resbuf* data=acutBuildList(70,s.on?1:0,70,s.position,40,s.scale,RTNONE);ok=rec->setFromRbChain(*data)==Acad::eOk;acutRelRb(data);rec->close();}
    nod->close();return ok;
}
// Per duct position (xdata TVS_OZNAKA, 1070); Default: the drawing's.
inline int own(const AcDbEntity* e){
    if(!e)return Default;resbuf* x=e->xData(app());if(!x)return Default;int p=Default;
    for(resbuf* r=x;r;r=r->rbnext)if(r->restype==1070&&r->resval.rint>=0&&r->resval.rint<=2)p=r->resval.rint;
    acutRelRb(x);return p;
}
inline AcDbObjectId layer(AcDbDatabase* db){
    AcDbObjectId id;if(!db)return id;AcDbLayerTable* t=NULL;
    if(db->getLayerTable(t,AcDb::kForRead)!=Acad::eOk||!t)return id;
    if(t->getAt(layerName(),id)!=Acad::eOk)id=AcDbObjectId::kNull;t->close();return id;
}
inline void text(wchar_t* out,size_t n,double aMm,double bMm,bool round,double flow){
    wchar_t size[48];
    if(round)swprintf_s(size,L"%%%%c%.0f",aMm);else swprintf_s(size,L"%.0fx%.0f",aMm,bMm);
    if(flow>0)swprintf_s(out,n,L"%ls/%.0f",size,flow);else swprintf_s(out,n,L"%ls",size);
}
// Where the label goes (insertion point at the left end of the base line, direction), false: no label.
struct Place {AcGePoint3d at;AcGeVector3d dir;double h;wchar_t caption[96];};
inline bool place(AcDbDatabase* db,const Settings& s,int position,const AcGePoint3d& first,const AcGePoint3d& last,double a,double b,bool round,double flow,Place& out){
    if(!db||!s.on)return false;
    AcGeVector3d dir=last-first;dir.z=0;const double length=dir.length();if(length<1e-12)return false;dir.normalize();
    if(dir.x<-1e-9||(std::fabs(dir.x)<=1e-9&&dir.y<0))dir=-dir;   // readable from below / right
    const AcGeVector3d up(-dir.y,dir.x,0);
    const double f=tvsUnits::factor(db),h=paperMm()*s.scale/f;
    AcGiTextStyle style;style.setFileName(L"romans.shx");style.setTextSize(h);style.setXScale(0.7);style.loadStyleRec();
    text(out.caption,96,a*f,b*f,round,flow);
    const AcGePoint2d ext=style.extents(out.caption,Adesk::kFalse,-1,Adesk::kFalse);
    if(ext.x+h>length)return false;   // only where it fits the duct
    const AcGePoint3d mid=first+(last-first)/2;const double half=a/2;
    const int p=position==Default?s.position:position;
    const double off=p==Above?half+0.5*h:p==Below?-(half+1.5*h):-0.5*h;   // base line from the axis
    out.at=mid+up*off-dir*(ext.x/2);out.at.z=mid.z;out.dir=dir;out.h=h;
    return true;
}
inline void draw(AcGiWorldDraw* mode,const AcDbEntity* duct,const AcGePoint3d& first,const AcGePoint3d& last,double a,double b,bool round,double flow){
    if(!mode||!duct)return;AcDbDatabase* db=duct->database();if(!db)return;
    const Settings s=read(db);if(!s.on)return;
    Place pl;if(!place(db,s,own(duct),first,last,a,b,round,flow,pl))return;
    const AcDbObjectId lay=layer(db);
    AcGiTextStyle style;style.setFileName(L"romans.shx");style.setTextSize(pl.h);style.setXScale(0.7);style.loadStyleRec();
    AcGiSubEntityTraits& traits=mode->subEntityTraits();
    const Adesk::UInt16 oldColor=traits.color();const AcDb::LineWeight oldLw=traits.lineWeight();const AcDbObjectId oldLayer=traits.layerId();
    if(!lay.isNull())traits.setLayer(lay);traits.setColor(256);traits.setLineWeight(AcDb::kLnWtByLayer);
    mode->geometry().text(pl.at,AcGeVector3d::kZAxis,pl.dir,pl.caption,-1,Adesk::kFalse,style);
    traits.setLayer(oldLayer);traits.setLineWeight(oldLw);traits.setColor(oldColor);
}
// Explode / TVS_IZVOZ: the label as a TEXT on AT-DIM_KANAL (NULL: none).
inline AcDbText* entity(const AcDbEntity* duct,const AcGePoint3d& first,const AcGePoint3d& last,double a,double b,bool round,double flow){
    if(!duct)return NULL;AcDbDatabase* db=duct->database();if(!db)return NULL;
    const Settings s=read(db);Place pl;if(!place(db,s,own(duct),first,last,a,b,round,flow,pl))return NULL;
    AcDbText* t=new AcDbText(pl.at,pl.caption,AcDbObjectId::kNull,pl.h,std::atan2(pl.dir.y,pl.dir.x));
    t->setWidthFactor(0.7);t->setColorIndex(256);
    const AcDbObjectId lay=layer(db);if(!lay.isNull())t->setLayer(lay);
    {AcDbTextStyleTable* st=NULL;AcDbObjectId sid;   // the style the command made (romans, 0.7)
     if(db->getTextStyleTable(st,AcDb::kForRead)==Acad::eOk&&st){if(st->getAt(layerName(),sid)==Acad::eOk)t->setTextStyle(sid);st->close();}}
    return t;
}
}
