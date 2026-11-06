#pragma once
// TVS_PRESJEK -> Uzduzni (faza P2): uzdužni presjek (razvijeni profil) of the TVS route between
// two picked elements. The route is developed into one plane (geometry/Profile.h): ducts, Z
// prelazi / obilasci with their sloped pieces and vertical elbows, plan elbows and T komadi at
// their level, prelazi as tapers; ducts of other routes crossed in plan as cut boxes. Floor lines
// as in P1; below the profile a table with stacionaža, kota dna and dimenzija. Same scale along
// and up (true size, text by the scale). The block TVS_UZDUZNI_<oznaka> stores its parameters,
// so TVS_PRESJEK -> Osvjezi rebuilds it from the current network.
#include "SectionCommand.h"
#include "../geometry/Profile.h"
namespace tvsProfileCmd {
struct Params {std::wstring letter;AcDbObjectId first,last;double floor,ceiling,scale;
    Params():floor(0),ceiling(0),scale(50){}};
inline const ACHAR* key(){return L"TVS_UZDUZNI_V1";}
inline std::wstring blockName(const std::wstring& letter){return L"TVS_UZDUZNI_"+letter;}
inline double pi(){return 3.14159265358979323846;}
inline std::wstring nextLetter(AcDbDatabase* db){
    for(wchar_t c=L'A';c<=L'Z';++c){std::wstring s(1,c);if(!tvsSectionCmd::blockExists(db,blockName(s)))return s;}
    for(int i=1;i<1000;++i){wchar_t s[16];swprintf_s(s,L"U%d",i);if(!tvsSectionCmd::blockExists(db,blockName(s)))return s;}
    return L"X";
}
// Network members and the route between the two elements (tags; ids[tag - 1] = object id).
inline bool collect(AcDbDatabase* db,const Params& p,std::vector<tvsSection::Member>& net,std::vector<int>& tags,
                    std::vector<AcDbObjectId>& ids,double tol){
    net.clear();ids.clear();tvsSectionCmd::members(db,net,&ids);
    int first=-1,last=-1;
    for(size_t i=0;i<ids.size();++i){if(ids[i]==p.first)first=static_cast<int>(i)+1;if(ids[i]==p.last)last=static_cast<int>(i)+1;}
    if(first<0||last<0){acutPrintf(L"\nUzdužni presjek: početni ili krajnji element trase nije TVS element u ovom prostoru (obrisan?).");return false;}
    if(!tvsProfile::route(net,first,last,tol,tags)){acutPrintf(L"\nUzdužni presjek: između izabranih elemenata nema povezane trase.");return false;}
    return true;
}
inline void htext(AcDbBlockTableRecord* btr,double x,double y,const wchar_t* s,double h,const AcDbObjectId& style,const AcDbObjectId& layerId,AcDb::TextHorzMode mode){
    AcDbText* t=new AcDbText(AcGePoint3d(x,y,0),s,style,h,0);
    t->setDatabaseDefaults(btr->database());t->setTextStyle(style);t->setHeight(h);
    t->setHorizontalMode(mode);t->setVerticalMode(AcDb::kTextVertMid);t->setAlignmentPoint(AcGePoint3d(x,y,0));
    tvsSectionCmd::append(btr,t,layerId);
}
// Text reading upwards (rotated 90 deg); centre or start at (x, y).
inline void vtext(AcDbBlockTableRecord* btr,double x,double y,const wchar_t* s,double h,const AcDbObjectId& style,const AcDbObjectId& layerId,bool center){
    AcDbText* t=new AcDbText(AcGePoint3d(x,y,0),s,style,h,pi()/2);
    t->setDatabaseDefaults(btr->database());t->setTextStyle(style);t->setHeight(h);t->setRotation(pi()/2);
    t->setHorizontalMode(center?AcDb::kTextCenter:AcDb::kTextLeft);t->setVerticalMode(AcDb::kTextVertMid);t->setAlignmentPoint(AcGePoint3d(x,y,0));
    tvsSectionCmd::append(btr,t,layerId);
}
inline void closed(AcDbBlockTableRecord* btr,const std::vector<tvsProfile::P>& q,double floor,const AcDbObjectId& layerId){
    if(q.size()<3)return;AcDbPolyline* pl=new AcDbPolyline(static_cast<unsigned int>(q.size()));
    for(size_t k=0;k<q.size();++k)pl->addVertexAt(static_cast<unsigned int>(k),AcGePoint2d(q[k].x,q[k].y-floor));
    pl->setClosed(true);tvsSectionCmd::append(btr,pl,layerId);
}
inline void prim(AcDbBlockTableRecord* btr,const tvsSection::Prim& g,double floor,const AcDbObjectId& layerId){
    if(!g.arc){tvsSectionCmd::line(btr,g.x0,g.y0-floor,g.x1,g.y1-floor,layerId);return;}
    double b0=g.k>0?g.a0:pi()-g.a0,b1=g.k>0?g.a1:pi()-g.a1;if(b1<b0)std::swap(b0,b1);
    if(b1-b0<1e-9||g.r<1e-9)return;
    tvsSectionCmd::append(btr,new AcDbArc(AcGePoint3d(g.cx,g.cy-floor,0),g.r,b0,b1),layerId);
}
inline std::wstring size(double A,double B,bool round,double f){
    wchar_t s[64];if(round)swprintf_s(s,L"%%%%c%.0f",A*f);else swprintf_s(s,L"%.0fx%.0f",A*f,B*f);return s;}
inline const wchar_t* fittingName(int kind){
    return kind==tvsProfile::Elbow?L"koljeno":kind==tvsProfile::Tee?L"T komad":kind==tvsProfile::Taper?L"prelaz":NULL;}
// Rebuilds the block contents. Returns the number of route elements, -1 = route not found
// (the block is left as it was).
inline int build(AcDbBlockTableRecord* btr,const Params& p){
    AcDbDatabase* db=btr->database();const double f=tvsUnits::factor(db),tol=2.0/f;
    std::vector<tvsSection::Member> net;std::vector<int> tags;std::vector<AcDbObjectId> ids;
    if(!collect(db,p,net,tags,ids,tol))return -1;
    const std::vector<tvsProfile::Piece> pieces=tvsProfile::develop(net,tags,tol);
    if(pieces.empty()){acutPrintf(L"\nUzdužni presjek: trasa nema dužinu.");return -1;}
    const std::vector<tvsProfile::Cross> crosses=tvsProfile::crossings(net,tags,pieces,tol);
    {   // clear
        AcDbBlockTableRecordIterator* it=NULL;
        if(btr->newIterator(it)==Acad::eOk){for(;!it->done();it->step()){AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForWrite)==Acad::eOk){e->erase();e->close();}}delete it;}
    }
    const AcDbObjectId layerId=tvsSectionCmd::layer(db),style=tvsSectionCmd::textStyle(db);
    const double th=0.7*p.scale/f,gap=0.5*th,L=pieces.back().s1,margin=(std::max)(500/f,6*th),H=p.ceiling-p.floor;
    // floor and floor above
    tvsSectionCmd::line(btr,-margin,0,L+margin,0,layerId);
    tvsSectionCmd::line(btr,-margin,H,L+margin,H,layerId);
    tvsSectionCmd::text(btr,AcGePoint3d(-margin-gap,0,0),tvsSectionCmd::level(p.floor,f).c_str(),th,style,layerId,true);
    tvsSectionCmd::text(btr,AcGePoint3d(-margin-gap,H,0),tvsSectionCmd::level(p.ceiling,f).c_str(),th,style,layerId,true);
    double low=0;   // lowest drawn point (relative to the floor), for the table
    for(size_t k=0;k<pieces.size();++k){
        const tvsProfile::Piece& c=pieces[k];
        if(c.kind==tvsProfile::Bend){
            tvsSection::View v;
            if(tvsSection::bendView(c.bend,c.s0,1,0,v)){for(size_t j=0;j<v.outline.size();++j)prim(btr,v.outline[j],p.floor,layerId);
                for(size_t j=0;j<v.poly.size();++j)low=(std::min)(low,v.poly[j].y-p.floor);continue;}
        }
        const std::vector<tvsProfile::P> q=tvsProfile::outline(c);closed(btr,q,p.floor,layerId);
        double top=-1e300;for(size_t j=0;j<q.size();++j){top=(std::max)(top,q[j].y-p.floor);low=(std::min)(low,q[j].y-p.floor);}
        if(const wchar_t* name=fittingName(c.kind))vtext(btr,(c.s0+c.s1)/2,top+gap,name,0.8*th,style,layerId,false);
    }
    // ducts of other routes crossed in plan
    for(size_t i=0;i<crosses.size();++i){
        const tvsSection::Cut& c=crosses[i].c;const double x=crosses[i].s,y=c.z-p.floor,hw=c.w/2,hh=c.h/2;
        if(c.round){
            if(c.sinA>0.98)tvsSectionCmd::append(btr,new AcDbCircle(AcGePoint3d(x,y,0),AcGeVector3d::kZAxis,c.A/2),layerId);
            else tvsSectionCmd::append(btr,new AcDbEllipse(AcGePoint3d(x,y,0),AcGeVector3d::kZAxis,AcGeVector3d(hw,0,0),hh/hw),layerId);
        }else{
            AcDbPolyline* r=new AcDbPolyline(4);
            r->addVertexAt(0,AcGePoint2d(x-hw,y-hh));r->addVertexAt(1,AcGePoint2d(x+hw,y-hh));
            r->addVertexAt(2,AcGePoint2d(x+hw,y+hh));r->addVertexAt(3,AcGePoint2d(x-hw,y+hh));r->setClosed(true);
            tvsSectionCmd::append(btr,r,layerId);
            tvsSectionCmd::line(btr,x-hw,y-hh,x+hw,y+hh,layerId);tvsSectionCmd::line(btr,x-hw,y+hh,x+hw,y-hh,layerId);
        }
        tvsSectionCmd::text(btr,AcGePoint3d(x,y+hh+1.5*th,0),size(c.A,c.B,c.round,f).c_str(),th,style,layerId);
        tvsSectionCmd::levelMark(btr,x,y-hh,false,tvsSectionCmd::level(c.z-hh,f),th,style,layerId);
        low=(std::min)(low,y-hh-2.4*th);
    }
    // table: stacionaza, kota dna, dimenzija
    const double lab=13*th,hv=7*th,hd=3*th,r0=low-3*th,r1=r0-hv,r2=r1-hv,r3=r2-hd;
    const double rows[4]={r0,r1,r2,r3};
    for(int k=0;k<4;++k)tvsSectionCmd::line(btr,-lab,rows[k],L,rows[k],layerId);
    tvsSectionCmd::line(btr,-lab,r0,-lab,r3,layerId);tvsSectionCmd::line(btr,0,r0,0,r3,layerId);tvsSectionCmd::line(btr,L,r0,L,r3,layerId);
    htext(btr,-lab+gap,(r0+r1)/2,L"Stacionaža [m]",th,style,layerId,AcDb::kTextLeft);
    htext(btr,-lab+gap,(r1+r2)/2,L"Kota dna [m]",th,style,layerId,AcDb::kTextLeft);
    htext(btr,-lab+gap,(r2+r3)/2,L"Dimenzija",th,style,layerId,AcDb::kTextLeft);
    const std::vector<double> st=tvsProfile::stations(pieces,1.8*th);
    for(size_t k=0;k<st.size();++k){const double s=st[k],x=s<=1.5*th?s+0.75*th:s-0.75*th;
        if(s>1e-9&&s<L-1e-9)tvsSectionCmd::line(btr,s,r0,s,r2,layerId);
        wchar_t v[40];swprintf_s(v,L"%.2f",s*f/1000);vtext(btr,x,(r0+r1)/2,v,th,style,layerId,true);
        double z=0;if(tvsProfile::bottomAt(pieces,s,tol,z))vtext(btr,x,(r1+r2)/2,tvsSectionCmd::level(z,f).c_str(),th,style,layerId,true);
    }
    {   // dimenzija: spans of equal size (a prelaz ends a span)
        size_t k=0;
        while(k<pieces.size()){
            const tvsProfile::Piece& c=pieces[k];
            const std::wstring label=c.kind==tvsProfile::Taper?std::wstring():size(c.A0,c.B0,c.round,f);
            size_t e=k+1;
            while(e<pieces.size()&&pieces[e].kind!=tvsProfile::Taper&&size(pieces[e].A0,pieces[e].B0,pieces[e].round,f)==label&&!label.empty())++e;
            const double s0=c.s0,s1=pieces[e-1].s1;
            if(s0>1e-9)tvsSectionCmd::line(btr,s0,r2,s0,r3,layerId);
            const double need=(label.size()>2?label.size()-2:label.size())*0.75*th+th;   // %%c is one character
            if(!label.empty()&&s1-s0>=need)htext(btr,(s0+s1)/2,(r2+r3)/2,label.c_str(),th,style,layerId,AcDb::kTextCenter);
            k=e;
        }
    }
    const std::wstring title=L"UZDUŽNI PRESJEK "+p.letter+L"-"+p.letter;
    htext(btr,L/2,r3-3.5*th,title.c_str(),2.5*th,style,layerId,AcDb::kTextCenter);
    if(!crosses.empty())acutPrintf(L"\nUzdužni presjek %ls: presječenih kanala drugih trasa %d.",p.letter.c_str(),static_cast<int>(crosses.size()));
    return static_cast<int>(pieces.size());
}
inline void store(AcDbBlockTableRecord* btr,const Params& p){
    if(btr->createExtensionDictionary()!=Acad::eOk&&btr->extensionDictionary().isNull())return;
    AcDbObjectPointer<AcDbDictionary> dict(btr->extensionDictionary(),AcDb::kForWrite);if(dict.openStatus()!=Acad::eOk)return;
    resbuf n[7]={};const short types[7]={1,1,330,330,40,40,40};
    for(int j=0;j<7;++j){n[j].restype=types[j];n[j].rbnext=j<6?&n[j+1]:NULL;}
    n[0].resval.rstring=const_cast<ACHAR*>(L"TVS_UZDUZNI/1");n[1].resval.rstring=const_cast<ACHAR*>(p.letter.c_str());
    if(acdbGetAdsName(n[2].resval.rlname,p.first)!=Acad::eOk||acdbGetAdsName(n[3].resval.rlname,p.last)!=Acad::eOk)return;
    n[4].resval.rreal=p.floor;n[5].resval.rreal=p.ceiling;n[6].resval.rreal=p.scale;
    AcDbObjectId id;AcDbXrecord* rec=NULL;
    if(dict->getAt(key(),id)==Acad::eOk){if(acdbOpenObject(rec,id,AcDb::kForWrite)!=Acad::eOk)rec=NULL;}
    else{rec=new AcDbXrecord();if(dict->setAt(key(),rec,id)!=Acad::eOk){delete rec;rec=NULL;}}
    if(rec){rec->setXlateReferences(true);rec->setFromRbChain(n[0]);rec->close();}
}
inline bool load(const AcDbBlockTableRecord* btr,Params& p){
    const AcDbObjectId dictId=btr->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    const short types[7]={1,1,330,330,40,40,40};resbuf* q=data;bool ok=true;
    for(int j=0;j<7;++j){if(!q||q->restype!=types[j]){ok=false;break;}
        switch(j){case 1:p.letter=q->resval.rstring;break;
            case 2:if(acdbGetObjectId(p.first,q->resval.rlname)!=Acad::eOk)ok=false;break;
            case 3:if(acdbGetObjectId(p.last,q->resval.rlname)!=Acad::eOk)ok=false;break;
            case 4:p.floor=q->resval.rreal;break;case 5:p.ceiling=q->resval.rreal;break;case 6:p.scale=q->resval.rreal;break;}
        if(!ok)break;q=q->rbnext;}
    acutRelRb(data);return ok;
}
// TVS_PRESJEK -> Osvjezi on a uzduzni presjek. false = not a uzduzni presjek block.
inline bool rebuild(AcDbBlockTableRecord* btr){
    Params p;if(!load(btr,p))return false;
    const int n=build(btr,p);
    if(n<0){acutPrintf(L"\nUzdužni presjek %ls nije osvježen; sadržaj je ostao kakav je bio.",p.letter.c_str());return true;}
    tvsSectionCmd::refreshReferences(btr);
    acutPrintf(L"\nUzdužni presjek %ls-%ls osvježen: elemenata trase %d.",p.letter.c_str(),p.letter.c_str(),n);
    return true;
}
// Route marks in plan: the letter at both ends of the route (layer TVS_PRESJEK).
inline void planMarks(AcDbDatabase* db,const Params& p,const std::vector<tvsProfile::Piece>& pieces){
    if(pieces.empty()||pieces.front().path.empty()||pieces.back().path.empty())return;
    AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk)return;
    const double f=tvsUnits::factor(db),th=2.5*p.scale/f;const AcDbObjectId layerId=tvsSectionCmd::layer(db),style=tvsSectionCmd::textStyle(db);
    const tvsProfile::Piece& a=pieces.front();const tvsProfile::Piece& b=pieces.back();
    const tvsProfile::P ends[2]={a.path.front(),b.path.back()},next[2]={a.path.size()>1?a.path[1]:a.path.front(),b.path.size()>1?b.path[b.path.size()-2]:b.path.back()};
    for(int k=0;k<2;++k){
        double dx=ends[k].x-next[k].x,dy=ends[k].y-next[k].y;const double l=std::hypot(dx,dy);if(l>1e-12){dx/=l;dy/=l;}else{dx=0;dy=1;}
        const AcGePoint3d at(ends[k].x+dx*3*th,ends[k].y+dy*3*th,0);
        AcDbText* t=new AcDbText(at,p.letter.c_str(),style,1.4*th,0);t->setDatabaseDefaults(db);t->setTextStyle(style);t->setHeight(1.4*th);
        t->setHorizontalMode(AcDb::kTextCenter);t->setVerticalMode(AcDb::kTextVertMid);t->setAlignmentPoint(at);tvsSectionCmd::append(space.object(),t,layerId);
        tvsSectionCmd::append(space.object(),new AcDbCircle(at,AcGeVector3d::kZAxis,1.2*th),layerId);
    }
}
inline bool pick(const wchar_t* prompt,AcDbObjectId& id){
    ads_name name;ads_point point;if(acedEntSel(prompt,name,point)!=RTNORM)return false;
    return acdbGetObjectId(id,name)==Acad::eOk;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=2.0/f;
    Params p;
    if(!pick(L"\nPrvi element trase (početak profila): ",p.first))return;
    if(!pick(L"\nZadnji element trase (kraj profila): ",p.last))return;
    std::vector<tvsSection::Member> net;std::vector<int> tags;std::vector<AcDbObjectId> ids;
    if(!collect(db,p,net,tags,ids,tol))return;
    const std::vector<tvsProfile::Piece> pieces=tvsProfile::develop(net,tags,tol);
    if(pieces.empty()){acutPrintf(L"\nUzdužni presjek: trasa nema dužinu.");return;}
    tvsMove::StretchHighlight route;for(size_t k=0;k<tags.size();++k)route.add(ids[tags[k]-1]);
    acutPrintf(L"\nTrasa: %d elemenata, razvijena dužina %.2f m (istaknuta).",static_cast<int>(tags.size()),pieces.back().s1*f/1000);
    const std::wstring def=nextLetter(db);
    wchar_t prompt[120];swprintf_s(prompt,L"\nOznaka uzdužnog presjeka <%ls>: ",def.c_str());ACHAR input[64]=L"";
    int rc=acedGetString(0,prompt,input);if(rc!=RTNORM&&rc!=RTNONE)return;
    p.letter=(rc==RTNORM&&input[0])?std::wstring(input):def;
    if(tvsSectionCmd::blockExists(db,blockName(p.letter))){acutPrintf(L"\nUzdužni presjek %ls već postoji; za novi sadržaj koristite Osvjezi ili drugu oznaku.",p.letter.c_str());return;}
    tvsSectionCmd::Remembered& m=tvsSectionCmd::remembered();
    if(!tvsSectionCmd::askLevel(L"\nKota poda etaže, m",m.floorM))return;
    if(!tvsSectionCmd::askLevel(L"\nKota gornje etaže, m",m.ceilingM))return;
    {wchar_t sp[80];swprintf_s(sp,L"\nRazmjera 1: <%.0f>: ",m.scale);acedInitGet(RSG_NOZERO|RSG_NONEG,NULL);double v=0;
     rc=acedGetReal(sp,&v);if(rc==RTNORM)m.scale=v;else if(rc!=RTNONE)return;}
    p.floor=m.floorM*1000/f;p.ceiling=m.ceilingM*1000/f;p.scale=m.scale;
    ads_point ins;if(acedGetPoint(NULL,L"\nTačka umetanja profila (početak trase, kota poda): ",ins)!=RTNORM)return;
    AcGePoint3d position;{resbuf from={};from.restype=RTSHORT;from.resval.rint=1;resbuf to={};to.restype=RTSHORT;to.resval.rint=0;ads_point w;acedTrans(ins,&from,&to,0,w);position=asPnt3d(w);}
    route.clear();
    AcDbObjectId btrId;
    {
        AcDbBlockTable* raw=NULL;if(db->getBlockTable(raw,AcDb::kForWrite)!=Acad::eOk)return;
        AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
        AcDbBlockTableRecord* btr=new AcDbBlockTableRecord();btr->setName(blockName(p.letter).c_str());
        if(table->add(btrId,btr)!=Acad::eOk){delete btr;acutPrintf(L"\nBlok uzdužnog presjeka nije napravljen.");return;}
        const int n=build(btr,p);store(btr,p);btr->close();
        acutPrintf(L"\nUzdužni presjek %ls-%ls: elemenata trase %d.",p.letter.c_str(),p.letter.c_str(),n);
    }
    {
        AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForWrite);if(space.openStatus()!=Acad::eOk)return;
        tvsSectionCmd::append(space.object(),new AcDbBlockReference(position,btrId),tvsSectionCmd::layer(db));
    }
    planMarks(db,p,pieces);
}
}
