#pragma once
// TVS_NATPIS: caption on straight ducts - size / flow m3/h / velocity m/s, e.g. 400x200/1500/5.21
// or %%c250/500/2.83 (size only without flow). Small grey text (romans.shx, width 0.7, 30 mm,
// colour 8) above the duct at its middle, along its axis, readable from below / right. Drawn by
// the duct itself (no extra objects), on screen only: never plotted. On/off per drawing:
// named-object dictionary entry TVS_NATPIS_V1 = (70 1). Same pattern as TVS_KOTE (LevelLabels.h).
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "acgi.h"
#include "DrawingUnits.h"
#include <cmath>
#include <cwchar>
namespace tvsDuctCaption {
inline const ACHAR* key(){return L"TVS_NATPIS_V1";}
inline double textMm(){return 30;}
inline bool enabled(AcDbDatabase* db){
    if(!db)return false;AcDbDictionary* nod=NULL;
    if(db->getNamedObjectsDictionary(nod,AcDb::kForRead)!=Acad::eOk||!nod)return false;
    AcDbObjectId id;const bool has=nod->getAt(key(),id)==Acad::eOk;nod->close();if(!has)return false;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    const bool on=data->restype==70&&data->resval.rint!=0;acutRelRb(data);return on;
}
// The command (ARX) writes the switch; the caller regenerates.
inline bool set(AcDbDatabase* db,bool on){
    AcDbDictionary* nod=NULL;if(!db||db->getNamedObjectsDictionary(nod,AcDb::kForWrite)!=Acad::eOk||!nod)return false;
    AcDbObjectId id;AcDbXrecord* rec=NULL;bool ok=false;
    if(nod->getAt(key(),id)==Acad::eOk){if(acdbOpenObject(rec,id,AcDb::kForWrite)!=Acad::eOk)rec=NULL;}
    else{rec=new AcDbXrecord();if(nod->setAt(key(),rec,id)!=Acad::eOk){delete rec;rec=NULL;}}
    if(rec){resbuf* data=acutBuildList(70,on?1:0,RTNONE);ok=rec->setFromRbChain(*data)==Acad::eOk;acutRelRb(data);rec->close();}
    nod->close();return ok;
}
// Mean velocity in m/s for flow in m3/h and the inner section in mm (round: a = D). 0 if unknown.
inline double velocity(double flow,double aMm,double bMm,bool round){
    const double area=round?3.14159265358979323846*aMm*aMm/4e6:aMm*bMm/1e6;
    return flow>0&&area>0?flow/3600/area:0;
}
inline void text(wchar_t* out,size_t n,double aMm,double bMm,bool round,double flow){
    wchar_t size[48];
    if(round)swprintf_s(size,L"%%%%c%.0f",aMm);else swprintf_s(size,L"%.0fx%.0f",aMm,bMm);
    if(flow>0)swprintf_s(out,n,L"%ls/%.0f/%.2f",size,flow,velocity(flow,aMm,bMm,round));
    else swprintf_s(out,n,L"%ls",size);
}
// Draws the caption for a duct from `first` to `last`, width a (plan; round: D), height b.
inline void draw(AcGiWorldDraw* mode,AcDbDatabase* db,const AcGePoint3d& first,const AcGePoint3d& last,double a,double b,bool round,double flow){
    if(!mode||!db||mode->context()->isPlotGeneration()||!enabled(db))return;
    AcGeVector3d dir=last-first;dir.z=0;if(dir.length()<1e-12)return;dir.normalize();
    if(dir.x<-1e-9||(std::fabs(dir.x)<=1e-9&&dir.y<0))dir=-dir;   // readable from below / right
    const AcGeVector3d up(-dir.y,dir.x,0);
    const double f=tvsUnits::factor(db),h=textMm()/f;
    AcGiTextStyle style;style.setFileName(L"romans.shx");style.setTextSize(h);style.setXScale(0.7);style.loadStyleRec();
    wchar_t caption[96];text(caption,96,a*f,b*f,round,flow);
    const AcGePoint2d ext=style.extents(caption,Adesk::kFalse,-1,Adesk::kFalse);
    const AcGePoint3d mid=first+(last-first)/2;
    AcGePoint3d p=mid+up*(a/2+0.5*h)-dir*(ext.x/2);p.z=mid.z;   // just outside the duct edge
    AcGiSubEntityTraits& traits=mode->subEntityTraits();const Adesk::UInt16 old=traits.color();const AcDb::LineWeight oldLw=traits.lineWeight();
    traits.setColor(8);traits.setLineWeight(AcDb::kLnWt009);   // always thin, not the duct layer's lineweight
    mode->geometry().text(p,AcGeVector3d::kZAxis,dir,caption,-1,Adesk::kFalse,style);
    traits.setLineWeight(oldLw);traits.setColor(old);
}
}
