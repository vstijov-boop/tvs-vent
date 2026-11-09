#pragma once
// TVS_PRESJEK (faza P1): poprecni presjek of the TVS network along a section line in plan.
// Every TVS element the line cuts is drawn at its real elevation (rectangle with a cross, or
// a circle), with its size and bottom elevation; two lines give the floor level and the
// level of the floor above. The section is a block TVS_PRESJEK_<oznaka> whose parameters are
// stored with it, so "Osvjezi" rebuilds it after the network changed (no manual editing).
#include "../geometry/Section.h"
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/TVS_TRANS.h"
#include "../TVS_Ventilation_DBX/TVS_TAP.h"
#include "../TVS_Ventilation_DBX/TransAlignData.h"
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
// TVS_PRESJEK -> Uzduzni (faza P2): ProfileCommand.h, included at the end of this file.
namespace tvsProfileCmd {inline void run();inline bool rebuild(AcDbBlockTableRecord* btr);}
namespace tvsSectionCmd {
// depth: zone behind the section plane (drawing units, 0 = cut elements only);
// hidden: 0 = parts covered by nearer elements are left out, 1 = drawn dashed.
// sp: spusteni plafon, distance below the floor above (drawing units, 0 = none).
struct Params {AcGePoint3d p1,p2;bool flip;double floor,ceiling,scale,depth,sp;int hidden;std::wstring letter;   // floor/ceiling: drawing units
    Params():flip(false),floor(0),ceiling(0),scale(50),depth(0),sp(0),hidden(0){}};
struct Remembered {double floorM,ceilingM,scale,spM,depthM;bool spOn,table;int hidden;Remembered():floorM(0),ceilingM(3),scale(50),spM(0.5),depthM(0),spOn(false),table(true),hidden(0){}};
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
// TVS network as plan members (see geometry/Section.h). Sloped piece of a Z prelaz / obilazak:
// axis z at its ends from the assembly record (rise). Vertical elbows (Gore / Nadolje): one
// member between their ports, end elevations taken from the elements they join.
// ids (optional): object id of every element, ids[tag - 1].
inline void members(AcDbDatabase* db,std::vector<tvsSection::Member>& out,std::vector<AcDbObjectId>* ids=NULL){
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return;
    const double f=tvsUnits::factor(db),tol=2.0/f;
    int tag=0;std::vector<size_t> bends;std::vector<double> bendElev;
    for(;!it->done();it->step()){
        AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
        tvsMove::Node n;
        if(TVS_Entity::cast(e)&&tvsMove::describe(e,n)&&n.ports.size()==n.sections.size()&&n.ports.size()>=2){
            ++tag;if(ids)ids->push_back(e->objectId());
            auto add=[&](const tvsMove::Point& a,const tvsMove::Point& b,const tvsMoveNetwork::Section& sa,const tvsMoveNetwork::Section& sb){
                tvsSection::Member m;m.a=tvsSection::P(a.x,a.y);m.b=tvsSection::P(b.x,b.y);
                m.A1=sa.a;m.B1=sa.b;m.A2=sb.a;m.B2=sb.b;m.round=sa.round;m.z=m.za=m.zb=n.elevation;m.tag=tag;m.kind=n.kind;out.push_back(m);};
            TVS_TAP* tap=TVS_TAP::cast(e);
            if(TVS_TRANS* tr=TVS_TRANS::cast(e)){   // prelaz: axis and height change along it
                add(n.ports[0],n.ports[1],n.sections[0],n.sections[1]);tvsSection::Member& m=out.back();m.taper=true;
                m.dir=tvsSection::P(tr->VectTr.x,tr->VectTr.y);   // faces square to the duct, not to the skewed axis
                const double off=tvsTransAlign::endOffset(tvsTransAlign::read(tr),tr->SizeAp1,tr->SizeBp1,tr->SizeAp2,tr->SizeBp2);
                if(std::fabs(off)>1e-12){m.sloped=true;m.za=n.elevation;m.zb=n.elevation+off;}
            }
            else if(TVS_Pipe::cast(e)){
                add(n.ports[0],n.ports[1],n.sections[0],n.sections[1]);
#ifdef TVS_OFFSET_INTEGRATION
                tvsOffsetData::Record r;
                if(TVS_Pipe::cast(e)&&tvsOffsetData::read(e,r)==tvsOffsetData::Valid&&r.role==tvsOffsetData::SlopedStraight&&std::fabs(r.rise)>1e-9){
                    tvsSection::Member& m=out.back();m.sloped=true;m.za=n.elevation-r.rise/2/f;m.zb=n.elevation+r.rise/2/f;}
#endif
            }
            else if(tap&&tap->get_Form()!=Form_Direct){   // vertical elbow
                add(n.ports[0],n.ports[1],n.sections[0],n.sections[1]);out.back().sloped=true;out.back().bend=true;
                bends.push_back(out.size()-1);bendElev.push_back(n.elevation);
            }
            else for(size_t j=0;j<n.ports.size();++j)add(n.middle,n.ports[j],n.sections[j],n.sections[j]);
        }
        e->close();
    }
    delete it;
    // vertical elbows: each end at the elevation of the member it joins (nearest to the elbow's own)
    for(size_t k=0;k<bends.size();++k){
        tvsSection::Member& m=out[bends[k]];bool slopedA=false,slopedB=false;
        for(int end=0;end<2;++end){
            const tvsSection::P q=end?m.b:m.a;double best=bendElev[k],gap=-1;bool slopedNext=false;
            for(size_t i=0;i<out.size();++i){if(out[i].tag==m.tag)continue;
                bool isBend=false;for(size_t j=0;j<bends.size();++j)if(bends[j]==i)isBend=true;if(isBend)continue;
                const tvsSection::Member& o=out[i];
                for(int oe=0;oe<2;++oe){const tvsSection::P r=oe?o.b:o.a;if(std::hypot(r.x-q.x,r.y-q.y)>tol)continue;
                    const double z=o.zAt(oe?1.0:0.0),g=std::fabs(z-bendElev[k]);if(gap<0||g<gap){gap=g;best=z;slopedNext=o.sloped;}}}
            if(end)m.zb=best;else m.za=best;
            if(end)slopedB=slopedNext;else slopedA=slopedNext;
        }
        // level end: where the elbow joins a level duct (the other end joins the sloped piece)
        if(slopedA!=slopedB)m.levelA=!slopedA;
        else m.levelA=std::fabs(m.za-bendElev[k])<=std::fabs(m.zb-bendElev[k]);
        if(std::fabs(m.zb-m.za)<=1e-9){m.sloped=false;m.bend=false;}
    }
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
// Visinska kota on a duct edge at (x, edge): half triangle with its tip on the edge (above the
// top edge, below the bottom edge), vertical leader, shelf line, value above the shelf.
inline void levelMark(AcDbBlockTableRecord* btr,double x,double edge,bool top,const std::wstring& value,double th,const AcDbObjectId& style,const AcDbObjectId& layerId){
    const double t=1.2*th,s=top?1:-1,shelf=edge+s*t,w=0.6*t,len=0.65*th*value.size()+0.6*th;
    line(btr,x,edge,x-w,shelf,layerId);line(btr,x-w,shelf,x,shelf,layerId);           // triangle
    line(btr,x,edge,x,edge+s*2*t,layerId);                                             // leader
    line(btr,x-w,shelf,x+len,shelf,layerId);                                           // shelf
    AcDbText* tx=new AcDbText(AcGePoint3d(x+0.2*th,shelf+0.25*th,0),value.c_str(),style,th,0);
    tx->setDatabaseDefaults(btr->database());tx->setTextStyle(style);tx->setHeight(th);
    tx->setHorizontalMode(AcDb::kTextLeft);tx->setVerticalMode(AcDb::kTextBase);
    append(btr,tx,layerId);
}
inline std::wstring level(double drawing,double f){wchar_t s[40];swprintf_s(s,L"%+.3f",drawing*f/1000);return s;}
// Spusteni plafon: dashed line at (H - sp) above the floor, with its level (left, as the floor levels).
inline void ceilingLine(AcDbBlockTableRecord* btr,double x0,double x1,double floor,double ceiling,double sp,double th,double gap,const AcDbObjectId& style,const AcDbObjectId& layerId);
// Dimension style of the sections at this text height: values in mm (dimlfac = units factor).
inline AcDbObjectId dimStyle(AcDbDatabase* db,double th,double f,const AcDbObjectId& textStyle){
    wchar_t name[64];swprintf_s(name,L"TVS_PRESJEK_%.0f",th*f*100);
    AcDbDimStyleTable* raw=NULL;if(db->getDimStyleTable(raw,AcDb::kForWrite)!=Acad::eOk)return db->dimstyle();
    AcDbObjectPointer<AcDbDimStyleTable> table;table.acquire(raw);AcDbObjectId id;
    if(table->getAt(name,id)==Acad::eOk)return id;
    AcDbDimStyleTableRecord* st=new AcDbDimStyleTableRecord();st->setName(name);
    st->setDimtxsty(textStyle);st->setDimtxt(th);st->setDimasz(0.8*th);st->setDimscale(1);st->setDimlfac(f);
    st->setDimdec(0);st->setDimgap(0.4*th);st->setDimexo(0.4*th);st->setDimexe(0.6*th);st->setDimtad(0);
    if(table->add(id,st)!=Acad::eOk){delete st;return db->dimstyle();}
    st->close();return id;
}
inline void dimension(AcDbBlockTableRecord* btr,double x,double y0,double y1,double xline,const wchar_t* text,const AcDbObjectId& style,const AcDbObjectId& layerId){
    if(!(y1-y0>1e-9))return;
    AcDbRotatedDimension* d=new AcDbRotatedDimension(3.14159265358979323846/2,AcGePoint3d(x,y0,0),AcGePoint3d(x,y1,0),AcGePoint3d(xline,(y0+y1)/2,0),text,style);
    d->setDatabaseDefaults(btr->database());d->setDimensionStyle(style);
    const AcDbObjectId id=append(btr,d,layerId);
    AcDbObjectPointer<AcDbRotatedDimension> o(id,AcDb::kForWrite);if(o.openStatus()==Acad::eOk)o->recomputeDimBlock(true);
}
// Rebuilds the block contents from the stored parameters. Returns the number of ducts cut.
inline int build(AcDbBlockTableRecord* btr,const Params& p){
    AcDbDatabase* db=btr->database();const double f=tvsUnits::factor(db);
    {   // clear
        AcDbBlockTableRecordIterator* it=NULL;
        if(btr->newIterator(it)==Acad::eOk){for(;!it->done();it->step()){AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForWrite)==Acad::eOk){e->erase();e->close();}}delete it;}
    }
    const AcDbObjectId layerId=layer(db),style=textStyle(db);
    const double th=0.7*p.scale/f,gap=0.5*th;   // section labels: 0.7 mm x scale (1:50 -> 35 mm)
    std::vector<tvsSection::Member> net;members(db,net);
    const tvsSection::P a(p.p1.x,p.p1.y),b(p.p2.x,p.p2.y);
    const std::vector<tvsSection::Cut> cuts=tvsSection::cuts(a,b,net,p.flip,2.0/f);
    const double L=p.p1.distanceTo(p.p2),margin=(std::max)(500/f,6*th),H=p.ceiling-p.floor;
    // floor and floor above
    line(btr,-margin,0,L+margin,0,layerId);
    line(btr,-margin,H,L+margin,H,layerId);
    text(btr,AcGePoint3d(-margin-gap,0,0),level(p.floor,f).c_str(),th,style,layerId,true);
    text(btr,AcGePoint3d(-margin-gap,H,0),level(p.ceiling,f).c_str(),th,style,layerId,true);
    ceilingLine(btr,-margin,L+margin,p.floor,p.ceiling,p.sp,th,gap,style,layerId);
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
        text(btr,AcGePoint3d(x,y+hh+3.2*th,0),size,th,style,layerId);
        levelMark(btr,x,y+hh,true,level(c.z+hh,f),th,style,layerId);
        levelMark(btr,x,y-hh,false,level(c.z-hh,f),th,style,layerId);
        for(size_t j=0;j<i;++j)if(tvsSection::overlap(cuts[j],c)){
            acutPrintf(L"\nPresjek %ls: kanali se preklapaju kod %.0f mm od pocetka linije.",p.letter.c_str(),c.s*f);break;}
    }
    // elements behind the section plane (dubina), thin outline; covered parts left out or dashed
    const std::vector<tvsSection::View> views=tvsSection::views(a,b,net,p.flip,p.depth,2.0/f);
    const std::vector<tvsSection::Edge> edges=tvsSection::edges(cuts,views);
    AcDbObjectId dashed;
    if(p.hidden){tvsLinetypes::ensure(db);AcDbLinetypeTable* lt=NULL;
        if(db->getLinetypeTable(lt,AcDb::kForRead)==Acad::eOk){lt->getAt(L"tvs_hidden",dashed);lt->close();}}
    for(size_t i=0;i<edges.size();++i){const tvsSection::Edge& e=edges[i];const tvsSection::Prim& g=e.g;
        if(e.hidden&&!p.hidden)continue;
        AcDbEntity* ent=NULL;
        if(!g.arc)ent=new AcDbLine(AcGePoint3d(g.x0,g.y0-p.floor,0),AcGePoint3d(g.x1,g.y1-p.floor,0));
        else if(std::fabs(std::fabs(g.k)-1)<1e-3){   // true arc (elbow plane parallel to the section)
            double b0=g.k>0?g.a0:3.14159265358979323846-g.a0,b1=g.k>0?g.a1:3.14159265358979323846-g.a1;if(b1<b0)std::swap(b0,b1);
            if(b1-b0<1e-9||g.r<1e-9)continue;
            ent=new AcDbArc(AcGePoint3d(g.cx,g.cy-p.floor,0),g.r,b0,b1);
        }else{   // oblique: projected arc as a fine polyline
            AcDbPolyline* pl=new AcDbPolyline(33);
            for(int k=0;k<=32;++k){const tvsSection::P q=g.at(k/32.0);pl->addVertexAt(k,AcGePoint2d(q.x,q.y-p.floor));}
            ent=pl;
        }
        const AcDbObjectId id=append(btr,ent,layerId);
        if(e.hidden&&!dashed.isNull()){AcDbObjectPointer<AcDbEntity> o(id,AcDb::kForWrite);if(o.openStatus()==Acad::eOk){o->setLinetype(dashed);o->setLinetypeScale(1/f);}}
    }
    // edge elevations of the straight level pieces seen in depth (where visible)
    const std::vector<tvsSection::View> cover=tvsSection::cover(cuts,views);
    for(size_t i=0;i<views.size();++i){const tvsSection::View& v=views[i];
        if(!v.level()||v.x1-v.x0<8*th)continue;
        const double x=(v.x0+v.x1)/2,zt=v.zc0+v.h/2,zb=v.zc0-v.h/2;
        if(!tvsSection::covered(cover,v.tag,v.closest,x,zt-1e-6))levelMark(btr,x,zt-p.floor,true,level(zt,f),th,style,layerId);
        if(!tvsSection::covered(cover,v.tag,v.closest,x,zb+1e-6))levelMark(btr,x,zb-p.floor,false,level(zb,f),th,style,layerId);
    }
    // clear distances: between stacked ducts (razmak) and below the lower one (svijetla visina)
    {
        const std::vector<tvsSection::Gap> g=tvsSection::gaps(cuts,views,2*th);
        const AcDbObjectId dim=dimStyle(db,th,f,style);
        for(size_t i=0;i<g.size();++i){
            const double x=g[i].x;
            dimension(btr,x,g[i].low-p.floor,g[i].high-p.floor,x,L"razmak <>",dim,layerId);
            if(g[i].lowBottom>p.floor)dimension(btr,x+3*th,0,g[i].lowBottom-p.floor,x+3*th,L"h=<>",dim,layerId);
        }
    }
    const std::wstring title=L"PRESJEK "+p.letter+L"-"+p.letter;
    text(btr,AcGePoint3d(L/2,-6*th,0),title.c_str(),2.5*th,style,layerId);
    if(!views.empty())acutPrintf(L"\nPresjek %ls: vidljivih elemenata u dubini %d.",p.letter.c_str(),static_cast<int>(views.size()));
    return static_cast<int>(cuts.size());
}
inline void store(AcDbBlockTableRecord* btr,const Params& p){
    if(btr->createExtensionDictionary()!=Acad::eOk&&btr->extensionDictionary().isNull())return;
    AcDbObjectPointer<AcDbDictionary> dict(btr->extensionDictionary(),AcDb::kForWrite);if(dict.openStatus()!=Acad::eOk)return;
    resbuf* data=acutBuildList(1,L"TVS_PRESJEK/1",1,p.letter.c_str(),10,asDblArray(p.p1),10,asDblArray(p.p2),70,p.flip?1:0,
        40,p.floor,40,p.ceiling,40,p.scale,40,p.depth,70,p.hidden,40,p.sp,RTNONE);
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
    if(ok&&q&&q->restype==40){p.depth=q->resval.rreal;q=q->rbnext;if(q&&q->restype==70){p.hidden=q->resval.rint;q=q->rbnext;}   // P1b; older sections: no depth
        if(q&&q->restype==40)p.sp=q->resval.rreal;}                                                              // spusteni plafon
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
inline void ceilingLine(AcDbBlockTableRecord* btr,double x0,double x1,double floor,double ceiling,double sp,double th,double gap,const AcDbObjectId& style,const AcDbObjectId& layerId){
    const double H=ceiling-floor;if(!(sp>0)||!(sp<H))return;
    AcDbDatabase* db=btr->database();const double f=tvsUnits::factor(db),y=H-sp;
    tvsLinetypes::ensure(db);AcDbObjectId dashed;AcDbLinetypeTable* lt=NULL;
    if(db->getLinetypeTable(lt,AcDb::kForRead)==Acad::eOk){lt->getAt(L"tvs_hidden",dashed);lt->close();}
    const AcDbObjectId id=append(btr,new AcDbLine(AcGePoint3d(x0,y,0),AcGePoint3d(x1,y,0)),layerId);
    if(!dashed.isNull()){AcDbObjectPointer<AcDbLine> o(id,AcDb::kForWrite);if(o.openStatus()==Acad::eOk){o->setLinetype(dashed);o->setLinetypeScale(1/f);}}
    const std::wstring t=L"SP "+level(ceiling-sp,f);
    text(btr,AcGePoint3d(x0-gap,y,0),t.c_str(),th,style,layerId,true);
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
    Params p;if(!load(btr.object(),p)){if(!tvsProfileCmd::rebuild(btr.object()))acutPrintf(L"\nIzabrani blok nije TVS presjek.");return;}
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
    bool flip;double depth;double fixedDepth;   // fixedDepth >= 0: only the side is picked, the zone keeps this depth
    DepthJig(const AcGePoint3d& a,const AcGePoint3d& b,double fixed=-1):flip(false),depth(0),fixedDepth(fixed){z.p1=a;z.p2=b;last=b;
        setDispPrompt(L"\nStrana gledanja i dubina presjeka (povucite misem): ");
        setUserInputControls(static_cast<UserInputControls>(kAccept3dCoordinates|kGovernedByOrthoMode));}
    virtual AcDbEntity* entity()const{return const_cast<ZonePreview*>(&z);}
    virtual DragStatus sampler(){AcGePoint3d q;const DragStatus s=acquirePoint(q);if(s!=kNormal)return s;
        if(q.distanceTo(last)<1e-9)return kNoChange;last=q;return kNormal;}
    virtual Adesk::Boolean update(){
        AcGeVector3d d=z.p2-z.p1;d.z=0;d.normalize();const AcGeVector3d left(-d.y,d.x,0);
        AcGeVector3d s=last-z.p1;s.z=0;const double v=s.dotProduct(left);
        z.flip=flip=v<0;z.depth=depth=fixedDepth>=0?fixedDepth:std::fabs(v);return Adesk::kTrue;}
};
inline bool askLevel(const wchar_t* prompt,double& valueM){
    wchar_t text[160];swprintf_s(text,L"%ls <%.2f>: ",prompt,valueM);acedInitGet(0,NULL);double v=0;
    const int rc=acedGetReal(text,&v);if(rc==RTNONE)return true;if(rc!=RTNORM)return false;valueM=v;return true;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db);
    ads_point a,b;
    acedInitGet(0,L"Osvjezi Uzduzni");
    int rc=acedGetPoint(NULL,L"\nPrva tacka linije presjeka ili [Osvjezi/Uzduzni]: ",a);
    if(rc==RTKWORD){ACHAR kw[32]=L"";acedGetInput(kw);
        if(_wcsicmp(kw,L"Uzduzni")==0)tvsProfileCmd::run();else refresh();return;}
    if(rc!=RTNORM)return;
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
#include "ProfileCommand.h"
