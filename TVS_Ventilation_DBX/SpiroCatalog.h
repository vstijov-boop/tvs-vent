#pragma once
// Spiro redukcije (okrugli prelaz Ø d1 -> Ø d2) po katalogu SOKO "Kanali za razvod vazduha v12",
// poglavlje 2.4 (R / RA bez zaptivke, RG / RAG sa gumenom zaptivkom):
//   konus      Lc = 2 (d1 - d2) + 100 mm (R, RA)   ili   + 50 mm (RG, RAG)
//   prikljucak a(d): 40 (80-224), 55 (250-355), 75 (400-630), 100 (710-900), 115 (1000-1400)
//   proizvodna duzina L = a(d1) + Lc + a(d2)
// U crtezu kanal ide na prikljucak do sicke; izmedju sicke i konusa je ravni dio c (25 mm,
// procjena sa katalogskog crteza). Element u crtezu je zato Lc + 2 c (od sicke do sicke), a
// dio prikljucka u kanalu (a - c) se crta isprekidano (ili nikako).
// Podesavanje po crtezu (TVS_SETTINGS): named-object dictionary TVS_SPIRO_V1 =
//   (70 katalog 0/1)(70 zaptivka RG 0/1)(40 c mm)(70 isprekidan prikljucak 0/1).
// Bez zapisa: katalog ukljucen, R, c = 25, isprekidano.
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "acutads.h"
#include "dbapserv.h"
#include "DrawingUnits.h"
#include <cmath>
#include <chrono>
#include <cwchar>
namespace tvsSpiro {
inline unsigned long long nowMs(){return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
struct Settings{bool catalog;bool rg;double cMm;bool dashed;Settings():catalog(true),rg(false),cMm(25),dashed(true){}};
inline const ACHAR* key(){return L"TVS_SPIRO_V1";}
// Read cache: every round prelaz asks on every draw; kept 1 s per database, cleared by write().
struct ReadCache {AcDbDatabase* db;unsigned long long at;Settings s;ReadCache():db(NULL),at(0){}};
inline ReadCache& readCache(){static ReadCache c;return c;}
inline Settings readNow(AcDbDatabase* db);
inline Settings read(AcDbDatabase* db){
    if(!db)db=acdbHostApplicationServices()->workingDatabase();
    ReadCache& c=readCache();const unsigned long long now=nowMs();
    if(db&&c.db==db&&now-c.at<1000)return c.s;
    const Settings s=readNow(db);c.db=db;c.at=now;c.s=s;return s;
}
inline Settings readNow(AcDbDatabase* db){
    Settings s;if(!db)return s;AcDbDictionary* nod=NULL;
    if(db->getNamedObjectsDictionary(nod,AcDb::kForRead)!=Acad::eOk||!nod)return s;
    AcDbObjectId id;const bool has=nod->getAt(key(),id)==Acad::eOk;nod->close();if(!has)return s;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return s;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return s;
    const resbuf* r=data;
    if(r&&r->restype==70){s.catalog=r->resval.rint!=0;r=r->rbnext;}
    if(r&&r->restype==70){s.rg=r->resval.rint!=0;r=r->rbnext;}
    if(r&&r->restype==40){if(r->resval.rreal>=0&&r->resval.rreal<=200)s.cMm=r->resval.rreal;r=r->rbnext;}
    if(r&&r->restype==70){s.dashed=r->resval.rint!=0;}
    acutRelRb(data);return s;
}
inline bool write(AcDbDatabase* db,const Settings& s){
    readCache().at=0;
    AcDbDictionary* nod=NULL;if(!db||db->getNamedObjectsDictionary(nod,AcDb::kForWrite)!=Acad::eOk||!nod)return false;
    AcDbObjectId id;AcDbXrecord* rec=NULL;bool ok=false;
    if(nod->getAt(key(),id)==Acad::eOk){if(acdbOpenObject(rec,id,AcDb::kForWrite)!=Acad::eOk)rec=NULL;}
    else{rec=new AcDbXrecord();if(nod->setAt(key(),rec,id)!=Acad::eOk){delete rec;rec=NULL;}}
    if(rec){resbuf* data=acutBuildList(70,s.catalog?1:0,70,s.rg?1:0,40,s.cMm,70,s.dashed?1:0,RTNONE);
        ok=rec->setFromRbChain(*data)==Acad::eOk;acutRelRb(data);rec->close();}
    nod->close();return ok;
}
// Prikljucak a (mm) za precnik d (mm).
inline double spigot(double d){
    const double r=std::floor(d+0.5);
    return r<=224?40:r<=355?55:r<=630?75:r<=900?100:115;
}
// Konus Lc (mm), d1 > d2.
inline double cone(double d1,double d2,bool rg){
    if(d1<d2){const double t=d1;d1=d2;d2=t;}
    return 2*(d1-d2)+(rg?50:100);
}
// Duzina elementa u crtezu (od sicke do sicke), mm.
inline double drawnMm(double d1,double d2,const Settings& s){return cone(d1,d2,s.rg)+2*s.cMm;}
// Proizvodna duzina L, mm.
inline double productionMm(double d1,double d2,bool rg){return spigot(d1)+cone(d1,d2,rg)+spigot(d2);}
// Duzina prelaza u jedinicama crteza: spiro (oba kraja okrugla, razliciti precnici, krut kanal,
// katalog ukljucen) po katalogu, inace `fallback`. a1 / a2: precnici u jedinicama crteza.
inline bool applies(AcDbDatabase* db,double a1,bool round1,double a2,bool round2,bool flex=false){
    if(flex||!round1||!round2||std::fabs(a1-a2)<1e-9)return false;
    return read(db).catalog;
}
inline double drawingLength(AcDbDatabase* db,double a1,bool round1,double a2,bool round2,double fallback,bool flex=false){
    if(!applies(db,a1,round1,a2,round2,flex))return fallback;
    if(!db)db=acdbHostApplicationServices()->workingDatabase();
    const double f=tvsUnits::factor(db);return drawnMm(a1*f,a2*f,read(db))/f;
}
// Kombinacija postoji u katalogu (tabele masa R / RG).
inline bool standard(double d1,double d2,bool rg){
    if(d1<d2){const double t=d1;d1=d2;d2=t;}
    static const int table[][11]={
        {140,125},{150,125,140},{160,125,140,150},{180,125,140,150,160},{200,125,140,150,160,180},
        {224,125,140,150,160,180,200},{250,125,140,150,160,180,200,224},{280,125,140,150,160,180,200,224,250},
        {300,125,140,150,160,180,200,224,250,280},{315,125,140,150,160,180,200,224,250,280,300},
        {355,150,160,180,200,224,250,280,300,315},{400,160,180,200,224,250,280,300,315,355},
        {450,200,224,250,280,300,315,355,400},{500,250,280,300,315,355,400,450},{560,300,315,355,400,450,500},
        {630,400,450,500,560},{710,450,500,560,630},{800,560,630,710},{900,710,800},{1000,800,900},
        {1120,900,1000},{1250,1000,1120},{1400,1250}};
    const int a=static_cast<int>(std::floor(d1+0.5)),b=static_cast<int>(std::floor(d2+0.5));
    if(std::fabs(d1-a)>0.01||std::fabs(d2-b)>0.01)return false;
    if(rg&&a==1400&&b==1250)return false;
    for(size_t i=0;i<sizeof(table)/sizeof(table[0]);++i){if(table[i][0]!=a)continue;
        for(int k=1;k<11&&table[i][k];++k)if(table[i][k]==b)return true;return false;}
    return false;
}
// Ekscentricna (RA): tip u tlocrtu 2 / 3 ili poravnanje po visini gore / dolje (TransAlignData.h).
inline bool eccentric(int transType,int verticalAlign){return transType==2||transType==3||verticalAlign==1||verticalAlign==2;}
// Sifra: R / RA / RG / RAG - Ød1 - Ød2.
inline void code(wchar_t* out,size_t n,double d1,double d2,bool eccentric,bool rg){
    if(d1<d2){const double t=d1;d1=d2;d2=t;}
    swprintf_s(out,n,L"%ls-%.0f-%.0f",eccentric?(rg?L"RAG":L"RA"):(rg?L"RG":L"R"),d1,d2);
}
}
