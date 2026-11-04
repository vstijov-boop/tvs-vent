#pragma once
// TVS_KOTE: temporary elevation labels on TVS elements (gi / os / di = top / axis / bottom, mm),
// small grey text in the middle of the element, along its axis. Drawn by the element itself
// (no extra objects in the drawing), on screen only (not plotted). On/off per drawing:
// named-object dictionary entry TVS_KOTE_V1 = (70 1).
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "acgi.h"
#include "DrawingUnits.h"
#include <cmath>
#include <cwchar>
namespace tvsLevelLabels {
inline const ACHAR* key(){return L"TVS_KOTE_V1";}
inline double textMm(){return 25;}
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
// Draws the labels at `at` along `dir` for an element of width a (plan) and height b (0 = round, a = D).
inline void draw(AcGiWorldDraw* mode,AcDbDatabase* db,const AcGePoint3d& at,AcGeVector3d dir,double elev,double a,double b){
    if(!mode||!db||mode->context()->isPlotGeneration()||!enabled(db))return;
    dir.z=0;if(dir.length()<1e-12)return;dir.normalize();
    if(dir.x<-1e-9||(std::fabs(dir.x)<=1e-9&&dir.y<0))dir=-dir;   // readable from below / right
    const AcGeVector3d up(-dir.y,dir.x,0);
    const double f=tvsUnits::factor(db),h=textMm()/f,half=(b>0?b:a)/2;
    double step=a/2-h;if(step<1.4*h)step=1.4*h;
    AcGiTextStyle style;style.setFileName(L"txt.shx");style.setTextSize(h);style.loadStyleRec();
    wchar_t text[3][48];
    swprintf_s(text[0],L"gi:%.0f",(elev+half)*f);swprintf_s(text[1],L"os:%.0f",elev*f);swprintf_s(text[2],L"di:%.0f",(elev-half)*f);
    AcGiSubEntityTraits& traits=mode->subEntityTraits();const Adesk::UInt16 old=traits.color();traits.setColor(8);
    for(int k=0;k<3;++k){
        const AcGePoint2d ext=style.extents(text[k],Adesk::kFalse,-1,Adesk::kFalse);
        const AcGePoint3d c=at+up*(step*(1-k));
        AcGePoint3d p=c-dir*(ext.x/2)-up*(h/2);p.z=at.z;
        mode->geometry().text(p,AcGeVector3d::kZAxis,dir,text[k],-1,Adesk::kFalse,style);
    }
    traits.setColor(old);
}
}
